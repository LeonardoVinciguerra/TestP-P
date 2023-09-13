/***************************************************************************
 *   Copyright (C) 2011 by Gabriel Ferri                                   *
 *   gabrielferri81@gmail.it                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "v4l2.h"

#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <iostream>
#include <dirent.h>


#ifdef __ARM__
	#include <string.h>
	void convertYUYVtoRGB24( void* inbuf, void* outbuf, size_t length );
#endif


#ifdef __LOG_ERROR
#include "q_inifile.h"
#include "q_logger.h"
extern CLogger QuadraLogger;
#endif

#include <mss.h>


struct sVideoModes
{
	unsigned int modeId;
	const char* name;
};

const struct sVideoModes videoModes[] =
{
	{ V4L2_PIX_FMT_GREY    , "V4L2_PIX_FMT_GREY" },

#ifdef __ARM__
	{ V4L2_PIX_FMT_YUYV    , "V4L2_PIX_FMT_YUYV" },
#endif

#ifdef __USE_PIXFC_SSE__
	{ V4L2_PIX_FMT_YUYV    , "V4L2_PIX_FMT_YUYV" },
#endif

#ifdef __USE_JPEGUTILS__
	{ V4L2_PIX_FMT_MJPEG   , "V4L2_PIX_FMT_MJPEG" },
#endif

	{ 0, NULL }
};


VideoForLinux2::VideoForLinux2()
{
	fd = -1;

	mapped = false;
	streaming = false;

	buffers = NULL;
	n_buffers = 0;
	pixelFormat = -1;

	#ifdef __USE_PIXFC_SSE__
	pixfc = 0;
	#endif

	#ifdef __USE_JPEGUTILS__
	jpeg_trans_buf = 0;

	set_transform_default( tran_option );
	#endif

	frameW = 0;
	frameH = 0;
}

VideoForLinux2::~VideoForLinux2()
{
	umapDevice();
}

bool VideoForLinux2::checkDevice( enum v4l2_buf_type type, unsigned int format )
{
	devType = type;

	if( !checkCapabilities() )
		return false;

	if( !checkPixelFormat( format ) )
		return false;

	enumerateControls();

	if( devType == V4L2_BUF_TYPE_VIDEO_CAPTURE )
		enumerateInputs();

	return true;
}

bool VideoForLinux2::streamOn()
{
	if( streaming )
	{
		return true;
	}

	printf( "stream on\n" );

	for( unsigned int i = 0; i < n_buffers; ++i )
	{
		struct v4l2_buffer buf;
		memset( &buf, 0, sizeof(struct v4l2_buffer) );
	
		buf.type = devType;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
	
		if( ioctl( fd, VIDIOC_QBUF, &buf ) == -1 )
		{
			#ifdef __LOG_ERROR
			if( Get_WriteErrorLog() )
			{
				QuadraLogger.Log( "V4L2::streamOn [port: %s]  VIDIOC_QBUF: %03d", devName.c_str(), errno );
			}
			#endif
			//perror("VIDIOC_QBUF");
			return false;
		}
	}

	enum v4l2_buf_type type = devType;

	if( ioctl(fd, VIDIOC_STREAMON, &type) == -1 )
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::streamOn [port: %s]  VIDIOC_STREAMON: %03d", devName.c_str(), errno );
		}
		#endif
		//perror("VIDIOC_STREAMON");
		return false;
	}
	
	streaming = true;
	return true;
}

bool VideoForLinux2::streamOff()
{
	if( !streaming )
	{
		return true;
	}

	printf( "stream off\n" );

	enum v4l2_buf_type type = devType;
	
	if( ioctl( fd, VIDIOC_STREAMOFF, &type ) == -1 )
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::streamOff [port: %s]  VIDIOC_STREAMOFF: %03d", devName.c_str(), errno );
		}
		#endif
		//perror( "VIDIOC_STREAMOFF" );
		return false;
	}

	streaming = false;
	return true;
}


//-----------------------------------------------------------------------------
// Read frame from video device
// Return: 1 frame OK
//         0 frame not ready
//        -1 error
//-----------------------------------------------------------------------------
int VideoForLinux2::getFrame( char** buffer, int* w, int* h )
{
	struct v4l2_buffer buf;
	memset( &buf, 0, sizeof(struct v4l2_buffer) );

	buf.type = devType;
	buf.memory = V4L2_MEMORY_MMAP;

	if( ioctl(fd, VIDIOC_DQBUF, &buf) == -1 )
	{
		if( errno == EAGAIN )
		{
			// frame not ready
			return 0;
		}
		else
		{
			// error
			#ifdef __LOG_ERROR
			if( Get_WriteErrorLog() )
			{
				QuadraLogger.Log( "V4L2::getFrame [port: %s]  VIDIOC_DQBUF: %03d", devName.c_str(), errno );
			}
			#endif
			//perror("VIDIOC_DQBUF");
			return -1;
		}
	}

	assert(buf.index < n_buffers);

	if( *buffer )
	{
		*w = frameW;
		*h = frameH;

		switch( pixelFormat )
		{
			case V4L2_PIX_FMT_GREY:
				memcpy( *buffer, buffers[buf.index].start, buffers[buf.index].length );
				break;

			#ifdef __ARM__
			case V4L2_PIX_FMT_YUYV:
				convertYUYVtoRGB24( buffers[buf.index].start, *buffer, buffers[buf.index].length );
				break;
			#endif

			#ifdef __USE_PIXFC_SSE__
			case V4L2_PIX_FMT_YUYV:
				pixfc->convert( pixfc, buffers[buf.index].start, *buffer );
				break;
			#endif

			#ifdef __USE_JPEGUTILS__
			case V4L2_PIX_FMT_MJPEG:
				//TODO
				#if 1
					if( tran_option.crop != FALSE || tran_option.transform != JXFORM_NONE )
					{
						unsigned long len_jpeg_trans_buf;
						if( transform_jpeg_raw( (unsigned char *)buffers[buf.index].start, buffers[buf.index].length, tran_option, &jpeg_trans_buf, len_jpeg_trans_buf ) )
						{
							decode_jpeg_raw( jpeg_trans_buf, len_jpeg_trans_buf, (unsigned char **)buffer, w, h );
						}
					}
					else
					{
						decode_jpeg_raw( (unsigned char *)buffers[buf.index].start, buffers[buf.index].length, (unsigned char **)buffer, w, h );
					}
				#else
					decode_jpeg_raw( (unsigned char *)buffers[buf.index].start, buffers[buf.index].length, (unsigned char **)buffer, w, h );
				#endif
					break;
			#endif

			default:
				std::cout << __FUNCTION__ << " : pixelformat is unknown" << std::endl;
				break;
		}
	}

	if( ioctl(fd, VIDIOC_QBUF, &buf) == -1 )
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::getFrame [port: %s]  VIDIOC_QBUF: %03d", devName.c_str(), errno );
		}
		#endif
		//perror("VIDIOC_QBUF");
		return -1;
	}

	return 1;
}

bool VideoForLinux2::setResolution( unsigned int& width, unsigned int& height )
{
	printf( "Setting resolution (%dx%d) ...\n", width, height );
	
	struct v4l2_format fmt;
	memset( &fmt, 0, sizeof(struct v4l2_format) );
	fmt.type = devType;

	if( ioctl( fd, VIDIOC_G_FMT, &fmt ) == -1 )
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::setResolution [port: %s]  VIDIOC_G_FMT: %03d", devName.c_str(), errno );
		}
		#endif
		//perror( "VIDIOC_G_FMT" );
		return false;
	}

	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = pixelFormat;

	if( ioctl( fd, VIDIOC_S_FMT, &fmt ) == -1 )
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::setResolution [port: %s]  VIDIOC_S_FMT: %03d", devName.c_str(), errno );
		}
		#endif
		//perror( "VIDIOC_S_FMT" );
		return false;
	}

	width = fmt.fmt.pix.width;
	height = fmt.fmt.pix.height;

	allocateConversionStructs( width, height );

	printf( "Resolution setted (%dx%d)\n", width, height );
	return true;
}

bool VideoForLinux2::getResolution( unsigned int &width, unsigned int &height )
{
	struct v4l2_format fmt;
	memset( &fmt, 0, sizeof(struct v4l2_format) );
	fmt.type = devType;
	
	if( ioctl( fd, VIDIOC_G_FMT, &fmt ) == -1 )
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::getResolution [port: %s]  VIDIOC_G_FMT: %03d", devName.c_str(), errno );
		}
		#endif
		//perror( "VIDIOC_G_FMT" );
		return false;
	}
	
	width = fmt.fmt.pix.width;
	height = fmt.fmt.pix.height;
	return true;
}

bool VideoForLinux2::setFPS( unsigned int& fps )
{
	printf( "Setting FPS (%d) ...\n", fps );

	struct v4l2_streamparm capture_dev;
	memset( &capture_dev, 0, sizeof(struct v4l2_streamparm) );
	capture_dev.type = devType;

	if( !ioctl(fd, VIDIOC_G_PARM, &capture_dev) )
	{
		if( !(capture_dev.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) )
		{
			printf( "V4L2_CAP_TIMEPERFRAME not supported\n" );
			return false;
		}
	}
	else
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::setFPS [port: %s]  VIDIOC_G_PARM: %03d", devName.c_str(), errno );
		}
		#endif
		//perror( "VIDIOC_G_PARM" );
		return false;
	}

	capture_dev.parm.capture.timeperframe.denominator = fps;
	if( ioctl(fd, VIDIOC_S_PARM, &capture_dev) )
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::setFPS [port: %s]  VIDIOC_S_PARM: %03d", devName.c_str(), errno );
		}
		#endif
		//perror( "VIDIOC_S_PARM" );
		return false;
	}

	fps = capture_dev.parm.capture.timeperframe.denominator;

	printf( "FPS setted (%d)\n", fps );
	return true;
}

bool VideoForLinux2::getFPS( unsigned int& fps )
{
	struct v4l2_streamparm capture_dev;
	memset( &capture_dev, 0, sizeof(struct v4l2_streamparm) );
	capture_dev.type = devType;

	if( !ioctl(fd, VIDIOC_G_PARM, &capture_dev) )
	{
		if( !(capture_dev.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) )
		{
			printf( "V4L2_CAP_TIMEPERFRAME not supported\n" );
			return false;
		}
	}
	else
	{
		#ifdef __LOG_ERROR
		if( Get_WriteErrorLog() )
		{
			QuadraLogger.Log( "V4L2::getFPS [port: %s]  VIDIOC_G_PARM: %03d", devName.c_str(), errno );
		}
		#endif
		//perror( "VIDIOC_G_PARM" );
		return false;
	}

	fps = capture_dev.parm.capture.timeperframe.denominator;
	return true;
}

#ifdef __USE_JPEGUTILS__
bool VideoForLinux2::setCropArea( int x, int y, int w, int h )
{
	char spec[128];
	snprintf( spec, sizeof(spec), "%dx%d+%d+%d", w, h, x, y );
	return set_crop( tran_option, spec );
}

void VideoForLinux2::resetCropArea()
{
	set_crop( tran_option, "" );
}
#endif

bool VideoForLinux2::setControlValue( unsigned int id, int value )
{
	for( unsigned int i = 0; i < controls.size(); i++ )
	{
		if( id == controls[i].ctrl.id )
		{
			struct v4l2_control control;
			control.id = controls[i].ctrl.id;

			if( controls[i].ctrl.type == V4L2_CTRL_TYPE_MENU )
			{
				if( value >= controls[i].choices_list.size() )
				{
					return false;
				}

				control.value = controls[i].choices_list[value].index;
			}
			else
			{
				control.value = value;
			}

			if( ioctl( fd, VIDIOC_S_CTRL, &control ) == -1 )
			{
				#ifdef __LOG_ERROR
				if( Get_WriteErrorLog() )
				{
					QuadraLogger.Log( "V4L2::setControlValue [port: %s]  VIDIOC_S_CTRL: %03d", devName.c_str(), errno );
				}
				#endif
				//perror( "VIDIOC_S_CTRL" );
				return false;
			}

			return true;
		}
	}

	printf( "Control \"%d\" not found\n", id );
	return false;
}

bool VideoForLinux2::getControlValue( unsigned int id, int& value )
{
	for( unsigned int i = 0; i < controls.size(); i++ )
	{
		if( id == controls[i].ctrl.id )
		{
			struct v4l2_control control;
			memset( &control, 0, sizeof (control) );
			control.id = controls[i].ctrl.id;

			if( ioctl( fd, VIDIOC_G_CTRL, &control ) == -1 )
			{
				#ifdef __LOG_ERROR
				if( Get_WriteErrorLog() )
				{
					QuadraLogger.Log( "V4L2::getControlValue [port: %s]  VIDIOC_G_CTRL: %03d", devName.c_str(), errno );
				}
				#endif
				//perror( "VIDIOC_G_CTRL" );
				return false;
			}

			if( controls[i].ctrl.type == V4L2_CTRL_TYPE_MENU )
			{
				for( int c = 0; c < controls[i].choices_list.size(); c++ )
				{
					if( controls[i].choices_list[c].index == control.value )
					{
						value = c;
						return true;
					}
				}
			}
			else
			{
				value = control.value;
				return true;
			}
		}
	}

	printf( "Control \"%d\" not found\n", id );
	return false;
}

//--------------------------------------------------------------------------
// Return a pointer to control
//--------------------------------------------------------------------------
bool VideoForLinux2::getControl( unsigned int id, V4L2Control** ctrl )
{
	for( unsigned int i = 0; i < controls.size(); i++ )
	{
		if( id == controls[i].ctrl.id )
		{
			*ctrl = &controls[i];
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------
// Return a pointer to control at given index
//--------------------------------------------------------------------------
bool VideoForLinux2::getControlAt( unsigned int index, V4L2Control** ctrl )
{
	if( controls.size() == 0 || index >= controls.size() )
	{
		*ctrl = 0;
		return false;
	}

	*ctrl = &controls[index];
	return true;
}


bool VideoForLinux2::selectInput( const char* name )
{
	for( unsigned int i = 0; i < inputs.size(); i++ )
	{
		if( !strcmp( (const char*)inputs[i].name, name ) )
		{
			if( ioctl( fd, VIDIOC_S_INPUT, &inputs[i].index ) == -1 )
			{
				#ifdef __LOG_ERROR
				if( Get_WriteErrorLog() )
				{
					QuadraLogger.Log( "V4L2::selectInput [port: %s]  VIDIOC_S_INPUT: %03d", devName.c_str(), errno );
				}
				#endif
				//perror( "VIDIOC_S_INPUT" );
				return false;
			}
			return true;
		}
	}

	printf( "Input \"%s\" not found\n", name );
	return false;
}

bool VideoForLinux2::mapDevice( int numBuffers )
{
	if( mapped )
		return true;

	printf( "mapDevice...\n" );

	struct v4l2_requestbuffers req;
	memset( &req, 0, sizeof(struct v4l2_requestbuffers) );

	req.type   = devType;
	req.count  = numBuffers;
	req.memory = V4L2_MEMORY_MMAP;

	if( ioctl( fd, VIDIOC_REQBUFS, &req ) == -1 )
	{
		if( EINVAL == errno )
		{
			printf( "Memory mapping not supported\n" );
			return false;
		}
		else
		{
			#ifdef __LOG_ERROR
			if( Get_WriteErrorLog() )
			{
				QuadraLogger.Log( "V4L2::mapDevice [port: %s]  VIDIOC_REQBUFS: %03d", devName.c_str(), errno );
			}
			#endif
			//perror( "VIDIOC_REQBUFS" );
			return false;
		}
	}

	if( req.count < numBuffers )
	{
		printf( "insufficient buffer memory\n" );
		return false;
	}

	buffers = (v4l_buffer*)calloc(req.count, sizeof (*buffers));
	if( !buffers )
	{
		printf( "Out of memory\n" );
		return false;
	}

	for( n_buffers = 0; n_buffers < req.count; ++n_buffers )
	{
		struct v4l2_buffer buf;
		memset( &buf, 0, sizeof(struct v4l2_buffer) );

		buf.type   = devType;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = n_buffers;

		if( ioctl( fd, VIDIOC_QUERYBUF, &buf ) == -1 )
		{
			#ifdef __LOG_ERROR
			if( Get_WriteErrorLog() )
			{
				QuadraLogger.Log( "V4L2::mapDevice [port: %s]  VIDIOC_QUERYBUF: %03d", devName.c_str(), errno );
			}
			#endif
			//perror("VIDIOC_QUERYBUF");
			return false;
		}

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
				mmap( NULL /* start anywhere */,
					  buf.length,
					  PROT_READ | PROT_WRITE /* required */,
					  MAP_SHARED /* recommended */,
					  fd, buf.m.offset );

		if( MAP_FAILED == buffers[n_buffers].start )
		{
			#ifdef __LOG_ERROR
			if( Get_WriteErrorLog() )
			{
				QuadraLogger.Log( "V4L2::mapDevice [port: %s]  MMAP: %03d", devName.c_str(), errno );
			}
			#endif
			//perror( "mmap" );
			return false;
		}
	}

	mapped = true;

	printf( "mapDevice - OK\n" );
	return true;
}

void VideoForLinux2::umapDevice()
{
	if( !mapped )
		return;

	printf( "umapDevice...\n" );

	for( unsigned int i = 0; i < n_buffers; ++i )
	{
		if( munmap( buffers[i].start, buffers[i].length ) == -1 )
		{
			#ifdef __LOG_ERROR
			if( Get_WriteErrorLog() )
			{
				QuadraLogger.Log( "V4L2::umapDevice [port: %s]  MUNMAP: %03d", devName.c_str(), errno );
			}
			#endif
			//perror( "munmap" );
		}
	}

	free(buffers);

	mapped = false;

	printf( "umapDevice - OK\n" );
}



	//-----------------------//
	//   Private functions   //
	//-----------------------//

bool VideoForLinux2::checkCapabilities()
{
	struct v4l2_capability cap;
	if( ioctl( fd, VIDIOC_QUERYCAP, &cap ) == -1 )
	{
		//perror( "VIDIOC_QUERYCAP" );
		return false;
	}

	if( devType == V4L2_BUF_TYPE_VIDEO_CAPTURE )
	{
		//INPUT DEVICE
		if( !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) || !(cap.capabilities & V4L2_CAP_STREAMING) )
		{
			printf( "Video capture not supported\n" );
			return false;
		}
	}
	else if( devType == V4L2_BUF_TYPE_VIDEO_OUTPUT )
	{
		//OUTPUT DEVICE
		if( !(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) )
		{
			printf( "Video output not supported\n" );
			return false;
		}
	}
	else
	{
		printf( "Device type not supported\n" );
		return false;
	}
	
	return true;
}

bool VideoForLinux2::checkPixelFormat( unsigned int format )
{
	pixelFormat = -1;
	
	int mode_idx = 0;
	while( videoModes[mode_idx].modeId )
	{
		if( format == videoModes[mode_idx].modeId )
		{
			break;
		}
		mode_idx++;
	}
	if( videoModes[mode_idx].modeId == 0 )
	{
		printf( "Pixel format not supported by library\n" );
		return false;
	}
	
	struct v4l2_fmtdesc desc;
	memset( &desc, 0, sizeof(struct v4l2_fmtdesc) );
	desc.type = devType;
	
	int index = 0;
	while( 1 )
	{
		desc.index = index;
		if( ioctl( fd, VIDIOC_ENUM_FMT, &desc ) == -1 )
		{
			if( errno == EINVAL )
			{
				break;
			}

			//perror( "VIDIOC_ENUM_FMT" );
			return false;
		}
		
		if( desc.pixelformat == videoModes[mode_idx].modeId )
		{
			pixelFormat = videoModes[mode_idx].modeId;
			break;
		}

		index++;
	}
	
	if( pixelFormat == -1 )
	{
		printf( "%s: pixel format not supported by device %s\n", videoModes[mode_idx].name, devName.c_str() );
		return false;
	}
	
	return true;
}

void VideoForLinux2::enumerateMenu( V4L2Control& control )
{
	struct v4l2_querymenu querymenu;
	memset (&querymenu, 0, sizeof (querymenu));
	querymenu.id = control.ctrl.id;

	for( querymenu.index = control.ctrl.minimum; querymenu.index <= control.ctrl.maximum; querymenu.index++ )
	{
		if( 0 == ioctl (fd, VIDIOC_QUERYMENU, &querymenu) )
		{
			control.choices_list.push_back( querymenu );
		}
		else
		{
			//perror( "VIDIOC_QUERYMENU" );
		}
	}
}



#define CONTROL_IO_ERROR_RETRIES                     2

void VideoForLinux2::enumerateControls()
{
	struct v4l2_queryctrl v4l2_ctrl;
	V4L2Control control;

	memset( &v4l2_ctrl, 0, sizeof(v4l2_ctrl) );

	controls.clear();

	// Test if the driver supports the V4L2_CTRL_FLAG_NEXT_CTRL flag
	v4l2_ctrl.id = 0 | V4L2_CTRL_FLAG_NEXT_CTRL;
	if( ioctl( fd, VIDIOC_QUERYCTRL, &v4l2_ctrl) == 0 )
	{
		// The driver supports the V4L2_CTRL_FLAG_NEXT_CTRL flag, so go ahead with
		// the advanced enumeration way.

		int r;
		v4l2_ctrl.id = 0;
		int current_ctrl = v4l2_ctrl.id;
		v4l2_ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
		// Loop as int as ioctl does not return EINVAL
		while( (r = ioctl( fd, VIDIOC_QUERYCTRL, &v4l2_ctrl)), r ? errno != EINVAL : 1 )
		{
			if(r && (errno == EIO || errno == EPIPE || errno == ETIMEDOUT))
			{
				// An I/O error occurred, so retry the query a few times.
				// This part is a little tricky. On the one hand, we want to retrieve the ID
				// of the next control in case the query succeeds or we give up on retrying.
				// On the other hand we want to retry the erroneous control instead of just
				// skipping to the next one, wo we needed to backup the ID of the failing
				// control first (above in current_ctrl).
				// Keep in mind that with the NEXT_CTRL flag VIDIO_QUERYCTRL returns the
				// first control with a *higher* ID than the specified one.
				int tries = CONTROL_IO_ERROR_RETRIES;
				v4l2_ctrl.id = current_ctrl | V4L2_CTRL_FLAG_NEXT_CTRL;
				while( tries-- && (r = ioctl( fd, VIDIOC_QUERYCTRL, &v4l2_ctrl)) && (errno == EIO || errno == EPIPE || errno == ETIMEDOUT) )
				{
					v4l2_ctrl.id = current_ctrl | V4L2_CTRL_FLAG_NEXT_CTRL;
				}
			}

			// Prevent infinite loops for buggy NEXT_CTRL implementations
			if( r && v4l2_ctrl.id <= current_ctrl )
			{
				// If there was an error but the driver failed to provide us with the ID
				// of the next control, we have to manually increase the control ID,
				// otherwise we risk getting stuck querying the erroneous control.
				current_ctrl++;
				printf(
				"Warning: The driver behind device %s has a slightly buggy implementation\n"
				"  of the V4L2_CTRL_FLAG_NEXT_CTRL flag. It does not return the next higher\n"
				"  control ID if a control query fails. A workaround has been enabled.",
				devName.c_str() );
				goto next_control;
			}
			else if( !r && v4l2_ctrl.id == current_ctrl )
			{
				// If there was no error but the driver did not increase the control ID
				// we simply cancel the enumeration.
				printf(
				"Error: The driver behind device %s has a buggy\n"
				"  implementation of the V4L2_CTRL_FLAG_NEXT_CTRL flag. It does not raise an\n"
				"  error or return the next control. Canceling control enumeration.",
				devName.c_str() );
				break;
			}

			current_ctrl = v4l2_ctrl.id;

			// Skip failed and disabled controls
			if(r || v4l2_ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				goto next_control;


			control.ctrl = v4l2_ctrl;
			control.choices_list.clear();

			if( v4l2_ctrl.type == V4L2_CTRL_TYPE_MENU )
				enumerateMenu( control );

			controls.push_back( control );

next_control:
			v4l2_ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
		}
	}
	else
	{
		// The driver does not support the V4L2_CTRL_FLAG_NEXT_CTRL flag, so we need
		// to fall back to the old way of enumerating controls, i.e. enumerating the
		// standard V4L2 controls first, followed by the driver's private controls.
		// It won't be possible to enumerate controls with non-contiguous IDs but in
		// this case the driver probably doesn't implement any.

		// Enumerate default V4L2 controls.
		// We use a separate variable instead of v4l2_ctrl.id for the loop counter because
		// some drivers (bttv) simply return a fake control with ID 0 when the device
		// doesn't support a control in the [V4L2_CID_BASE, V4L2_CID_LASTP1) interval,
		// thereby overwriting our loop variable and causing us to restart from 0.
		int current_ctrl;
		for(current_ctrl = V4L2_CID_BASE; current_ctrl < V4L2_CID_LASTP1; current_ctrl++) {
			v4l2_ctrl.id = current_ctrl;

			int r = 0, tries = 1 + CONTROL_IO_ERROR_RETRIES;
			while(tries-- &&
				  (r =  ioctl( fd, VIDIOC_QUERYCTRL, &v4l2_ctrl)) &&
				  (errno == EIO || errno == EPIPE || errno == ETIMEDOUT));
			if(r || v4l2_ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;


			control.ctrl = v4l2_ctrl;
			control.choices_list.clear();

			if( v4l2_ctrl.type == V4L2_CTRL_TYPE_MENU )
				enumerateMenu( control );

			controls.push_back( control );
		}

		// Enumerate custom controls
		for(v4l2_ctrl.id = V4L2_CID_PRIVATE_BASE;; v4l2_ctrl.id++)
		{
			int r = 0, tries = 1 + CONTROL_IO_ERROR_RETRIES;
			while(tries-- &&
				  (r =  ioctl( fd, VIDIOC_QUERYCTRL, &v4l2_ctrl)) &&
				  (errno == EIO || errno == EPIPE || errno == ETIMEDOUT));
			if(r)
				break;

			if(v4l2_ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;


			control.ctrl = v4l2_ctrl;
			control.choices_list.clear();

			if( v4l2_ctrl.type == V4L2_CTRL_TYPE_MENU )
				enumerateMenu( control );

			controls.push_back( control );
		}
	}
}

void VideoForLinux2::enumerateInputs()
{
	struct v4l2_input inp;
	
	inputs.clear();
	
	for( inp.index = 0; ; inp.index++ )
	{
		if( ioctl( fd, VIDIOC_ENUMINPUT, &inp ) == -1 )
		{
			if( errno == EINVAL )
			{
				break;
			}
			else
			{
				//perror( "VIDIOC_ENUMINPUT" );
				return;
			}
		}
		else
		{
			inputs.push_back( inp );
		}
	}
}

void VideoForLinux2::allocateConversionStructs( unsigned int w, unsigned int h )
{
	clearConversionStructs();

	frameW = w;
	frameH = h;

	#ifdef __USE_PIXFC_SSE__
	// Create struct pixfc
	create_pixfc( &pixfc, PixFcYUYV, PixFcRGB24, w, h, PixFcFlag_Default );
	#endif

	#ifdef __USE_JPEGUTILS__
	// allocate maximum image memory
	//TODO: si potrebbe allocare molta meno memoria spostando l'allocazione in getFrame()
	jpeg_trans_buf = new unsigned char[w * h * 3];
	#endif
}

void VideoForLinux2::clearConversionStructs()
{
	#ifdef __USE_PIXFC_SSE__
	if( pixfc )
	{
		destroy_pixfc( pixfc );
		pixfc = 0;
	}
	#endif

	#ifdef __USE_JPEGUTILS__
	if( jpeg_trans_buf )
	{
		delete [] jpeg_trans_buf;
		jpeg_trans_buf = 0;
	}
	#endif
}





	//-----------------------//
	//                       //
	//-----------------------//


// Reads the USB information for the given device into the given structure.
bool get_device_usb_info( char* device_name, CUSBInfo* usbinfo )
{
	if(device_name == NULL || usbinfo == NULL)
		return false;

	// File names in the /sys/class/video4linux/video?/device directory and
	// corresponding pointers in the CUSBInfo structure.
	char *files[] = {
		"idVendor",
		"idProduct",
		"bcdDevice"
	};
	unsigned short *fields[] = {
		&usbinfo->vendor,
		&usbinfo->product,
		&usbinfo->release
	};

	// Read USB information
	int i;
	for(i = 0; i < 3; i++) {
		char* filename = NULL;
		if(asprintf(&filename, "/sys/class/video4linux/%s/device/../%s", device_name, files[i]) < 0)
		{
			return false;
		}

		FILE *input = fopen(filename, "r");
		if(input)
		{
			if(fscanf(input, "%hx", fields[i]) != 1)
				*fields[i] = 0;
			fclose(input);
		}

		free(filename);
	}

	return true;
}

// Reads the information for the given device into the given structure.
bool get_device_details( V4L2Device* device )
{
	bool ret = true;

	// Open the corresponding V4L2 device
	int v4l2_dev = open( device->longName.c_str(), 0 );
	if( !v4l2_dev )
	{
		return false;
	}

	// Query the device
	struct v4l2_capability v4l2_cap;
	if( !ioctl( v4l2_dev, VIDIOC_QUERYCAP, &v4l2_cap) )
	{
		if( v4l2_cap.card[0] )
			device->name = (char *)v4l2_cap.card;
		else
			device->name = device->shortName;

		device->driver = (char *)v4l2_cap.driver;

		if(v4l2_cap.bus_info[0])
			device->location = (char *)v4l2_cap.bus_info;
		else
			device->location = device->shortName;
	}
	else
	{
		ret = false;
	}

	close( v4l2_dev );

	return ret;
}

// Synchronizes the device list with the information available in sysfs.
bool EnumerateV4L2Device( std::vector<V4L2Device>& devices )
{
	struct dirent* dir_entry;
	devices.clear();

	// Go through all devices in sysfs and validate the list entries that have correspondences in sysfs.
	DIR* v4l_dir = opendir("/sys/class/video4linux");
	if( v4l_dir )
	{
		while( (dir_entry = readdir(v4l_dir)) )
		{
			// Ignore non-video devices
			if( strstr(dir_entry->d_name, "video") != dir_entry->d_name &&
				strstr(dir_entry->d_name, "subdev") != dir_entry->d_name )
				continue;

			V4L2Device dev;
			dev.longName = "/dev/";
			dev.longName.append( dir_entry->d_name );

			dev.shortName = dir_entry->d_name;

			// Read detail information about the device
			if( !get_device_details( &dev ) )
			{
				continue;
			}
			if( !get_device_usb_info( dir_entry->d_name, &dev.usb ) )
			{
				continue;
			}

			devices.push_back( dev );
		}

		closedir( v4l_dir );
	}

	return true;
}
