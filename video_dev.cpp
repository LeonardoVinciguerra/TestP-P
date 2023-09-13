//---------------------------------------------------------------------------
//
// Name:        video_dev.cpp
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: VideoDevice class implementation
//
//---------------------------------------------------------------------------
#include "video_dev.h"

#include <stdio.h>
#include <fcntl.h>
#include "lnxdefs.h"
#include "Timer.h"

#include <mss.h>


//---------------------------------------------------------------------------------
// VideoDevice
// ATTENZIONE! quando e' attiva la visualizzazione live nessun altro processo puo'
// disegnare pixel sullo schermo
//---------------------------------------------------------------------------------


#define MAX_CONTROL_VALUE     60000
#define MIN_CONTROL_VALUE     1000
#define DEF_CONTROL_VALUE     32768

#define CONTROL_VALUE_RANGE   65536

#define NUM_DEVICE_BUFFERS    2
#define NUM_FRAMES_TO_SKIP    1

#define THREAD_SLEEP_MS       20


//--------------------------------------------------------------------------
// Costruttore
//--------------------------------------------------------------------------
VideoDevice::VideoDevice()
{
	devName = "";
	devKey = "fake_key";
	fd = -1;

	m_fnDisplayFrameCallback = 0;

	is_streaming = true;
	show_video = false;
	frame_ready = false;

	live_stream_w = 0;
	live_stream_h = 0;
	live_stream = 0;
	
	video_fmt = -1;
	video_fps = 25;
	video_bpp = 1;

	crop_w = 0;
	crop_h = 0;
}

//--------------------------------------------------------------------------
// Distruttore
//--------------------------------------------------------------------------
VideoDevice::~VideoDevice()
{
	StopThread();

	v4lDevice.streamOff();
	v4lDevice.umapDevice();

	if( fd >= 0 )
	{
		close( fd );
	}

	clearStruct();
}

//--------------------------------------------------------------------------
// Open video device
//--------------------------------------------------------------------------
bool VideoDevice::OpenDevice( const char* deviceName, int width, int height, int format, int fps )
{
	devName = deviceName;
	video_def_w = width;
	video_def_h = height;
	video_fmt = format;
	video_fps = fps;

	// open device
	fd = open( devName.c_str(), O_RDWR | O_NONBLOCK );
	if( fd < 0 )
	{
		printf( "Unable to open device %s\n", devName.c_str() );
		return false;
	}

	v4lDevice.setFd( fd, devName.c_str() );

	if( !v4lDevice.checkDevice( V4L2_BUF_TYPE_VIDEO_CAPTURE, format ) )
	{
		return false;
	}

	// set resolution
	video_w = width;
	video_h = height;
	if( !v4lDevice.setResolution( video_w, video_h ) )
	{
		return false;
	}
	// set fps
	unsigned int tFps = video_fps;
	v4lDevice.setFPS( tFps );

	// map device
	if( !v4lDevice.mapDevice( NUM_DEVICE_BUFFERS ) )
	{
		return false;
	}

	// set video byte per pixel
	if( video_fmt == V4L2_PIX_FMT_GREY )
		video_bpp = 1;
	else if( video_fmt == V4L2_PIX_FMT_YUYV )
		video_bpp = 3;
	else if( video_fmt == V4L2_PIX_FMT_MJPEG )
		video_bpp = 3;

	// allocate stream buffer
	live_stream_w = video_w;
	live_stream_h = video_h;
	allocateStruct( live_stream_w, live_stream_h );
	return true;
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void VideoDevice::SetResolution( unsigned int width, unsigned int height )
{
	video_def_w = width;
	video_def_h = height;

	while( !stop_video_mutex.try_lock() )
	{
		delay( 1 );
	}

	bool was_mapped = v4lDevice.isMapped();
	if( was_mapped )
	{
		v4lDevice.streamOff();
		v4lDevice.umapDevice();

		// reopen device file descriptor
		close( fd );
		fd = open( devName.c_str(), O_RDWR | O_NONBLOCK );
		if( fd < 0 )
		{
			printf( "Unable to open device %s\n", devName.c_str() );
			return;
		}
		v4lDevice.setFd( fd, devName.c_str() );
	}

	// set resolution
	if( v4lDevice.setResolution( width, height ) )
	{
		video_w = width;
		video_h = height;

		if( video_w > live_stream_w || video_h > live_stream_h )
		{
			live_stream_w = video_w;
			live_stream_h = video_h;

			allocateStruct( live_stream_w, live_stream_h );
		}
	}

	// set fps
	unsigned int tFps = video_fps;
	v4lDevice.setFPS( tFps );

	// set/reset image cropping
	setOutputImageCrop();

	if( was_mapped )
	{
		v4lDevice.mapDevice( NUM_DEVICE_BUFFERS );
	}

	stop_video_mutex.unlock();
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
bool VideoDevice::SetCrop_Centered( unsigned int width, unsigned int height )
{
	crop_w = width;
	crop_h = height;

	return setOutputImageCrop();
}

//--------------------------------------------------------------------------
// Set/reset image cropping
//--------------------------------------------------------------------------
bool VideoDevice::setOutputImageCrop()
{
	#ifdef __USE_JPEGUTILS__
	if( crop_w == 0 || crop_h == 0 )
	{
		v4lDevice.resetCropArea();
		return true;
	}

	if( video_w <= crop_w || video_h <= crop_h )
	{
		v4lDevice.resetCropArea();
		return true;
	}

	unsigned int crop_x = (video_w - crop_w) / 2;
	unsigned int crop_y = (video_h - crop_h) / 2;

	if( !v4lDevice.setCropArea( crop_x, crop_y, crop_w, crop_h ) )
	{
		printf( "set crop are ERROR !\n" );
		return false;
	}
	#endif
	return true;
}


//--------------------------------------------------------------------------
// Setta luminosita'
//--------------------------------------------------------------------------
bool VideoDevice::SetBrightness( unsigned int value )
{
	V4L2Control* ctrl;
	v4lDevice.getControl( V4L2_CID_BRIGHTNESS, &ctrl );

	// serve per poter usare diversi dispositivi
	int _value = (value * (ctrl->ctrl.maximum - ctrl->ctrl.minimum)) / CONTROL_VALUE_RANGE + ctrl->ctrl.minimum;
	return v4lDevice.setControlValue( V4L2_CID_BRIGHTNESS, _value );
}

//--------------------------------------------------------------------------
// Setta contrasto
//--------------------------------------------------------------------------
bool VideoDevice::SetContrast( unsigned int value )
{
	V4L2Control* ctrl;
	v4lDevice.getControl( V4L2_CID_CONTRAST, &ctrl );

	// serve per poter usare diversi dispositivi
	int _value = (value * (ctrl->ctrl.maximum - ctrl->ctrl.minimum)) / CONTROL_VALUE_RANGE + ctrl->ctrl.minimum;
	return v4lDevice.setControlValue( V4L2_CID_CONTRAST, _value );
}

//--------------------------------------------------------------------------
// Start video thread
//--------------------------------------------------------------------------
void VideoDevice::StartThread()
{
	if( !live_stream )
	{
		printf( "start_thread ERROR: open device first !\n" );
		return;
	}

	m_Thread = boost::thread( &VideoDevice::showLive, this );
}

//--------------------------------------------------------------------------
// Stop video thread
//--------------------------------------------------------------------------
void VideoDevice::StopThread()
{
	m_Thread.interrupt();
	m_Thread.join();
}

//--------------------------------------------------------------------------
// Play video on screen
//--------------------------------------------------------------------------
void VideoDevice::PlayVideo()
{
	while( !stop_video_mutex.try_lock() )
	{
		delay( 1 );
	}

	show_video = true;

	stop_video_mutex.unlock();
}

//--------------------------------------------------------------------------
// Stop video on screen
//--------------------------------------------------------------------------
void VideoDevice::StopVideo()
{
	while( !stop_video_mutex.try_lock() )
	{
		delay( 1 );
	}

	show_video = false;

	stop_video_mutex.unlock();
}

//--------------------------------------------------------------------------
// Get current frame
//--------------------------------------------------------------------------
void VideoDevice::GetFrame( void* frame )
{
	if( !live_stream )
	{
		printf( "GetFrame ERROR: open device first !\n" );
		return;
	}

	// wait frame ready
	Timer timeoutTimer;
	timeoutTimer.start();

	while( 1 )
	{
		if( frame_ready )
		{
			break;
		}

		if( timeoutTimer.getElapsedTimeInMilliSec() > 1000 )
		{
			printf( "Error - VideoDevice::GetFrame timeout !\n");
			break;
		}

		delay( 1 );
	}

	// get frame
	while( !stop_video_mutex.try_lock() )
	{
		delay( 1 );
	}

	memcpy( frame, live_stream, GetFrame_SizeInByte() );

	stop_video_mutex.unlock();
}

//--------------------------------------------------------------------------
// Thread main loop
//--------------------------------------------------------------------------
void VideoDevice::showLive()
{
	bool need_restart = false;
	char skip_frame = 0;

	for(;;)
	{
		// punto di interruzione: serve per poter terminare il thread
		boost::this_thread::interruption_point();

		// questo ritardo serve per permettere l'esecuzione del thread principale
		boost::posix_time::milliseconds _sleepTime( THREAD_SLEEP_MS );
		boost::this_thread::sleep( _sleepTime );

		if( need_restart )
		{
			boost::mutex::scoped_lock lock( stop_video_mutex );

			// close device file descriptor
			if( fd >= 0 )
			{
				close( fd );
				fd = -1;
			}


			// look for device key
			std::vector<V4L2Device> devices;
			EnumerateV4L2Device( devices );
			int dev_index = -1;

			for( int i = 0; i < devices.size(); i++ )
			{
				V4L2Device* device = &devices[i];

				if( strstr( device->name.c_str(), devKey.c_str() ) != NULL )
				{
					dev_index = i;
					devName = device->longName;
					break;
				}
			}

			if( dev_index == -1 )
			{
				continue;
			}

			// device found ...


			// open device file descriptor
			fd = open( devName.c_str(), O_RDWR | O_NONBLOCK );
			if( fd < 0 )
			{
				continue;
			}

			v4lDevice.setFd( fd, devName.c_str() );
			// set normal resolution
			video_w = video_def_w;
			video_h = video_def_h;
			if( !v4lDevice.setResolution( video_w, video_h ) )
				continue;
			// set fps
			unsigned int tFps = video_fps;
			if( !v4lDevice.setFPS( tFps ) )
				continue;

			if( !v4lDevice.mapDevice( NUM_DEVICE_BUFFERS ) )
				continue;

			//TODO: set the camera params

			need_restart = false;
		}



		if( !is_streaming )
		{
			v4lDevice.streamOff();
			skip_frame = 0;
			frame_ready = false;
			continue;
		}
		v4lDevice.streamOn();


		char* pFrame = (char*)live_stream;
		int w, h;
		int ret = v4lDevice.getFrame( &pFrame, &w, &h );
		if( ret == 1 )
		{
			// frame ready
			boost::mutex::scoped_lock lock( stop_video_mutex );

			if( skip_frame < NUM_FRAMES_TO_SKIP )
			{
				skip_frame++;
				continue;
			}

			frame_ready = true;

			if( !show_video )
			{
				continue;
			}

			if( m_fnDisplayFrameCallback )
			{
				// execute callback function
				m_fnDisplayFrameCallback( (unsigned char*)live_stream, w, h, video_bpp );
			}
		}
		else if( ret == -1 )
		{
			printf( "Get frame error on device: %s\n", devName.c_str() );

			boost::mutex::scoped_lock lock( stop_video_mutex );

			v4lDevice.streamOff();
			v4lDevice.umapDevice();

			need_restart = true;
		}
	}
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void VideoDevice::allocateStruct( unsigned int w, unsigned int h )
{
	clearStruct();
	
	// Allocate buffers (must be 16-byte aligned)
	posix_memalign( &live_stream, 16, w * h * 3 );
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void VideoDevice::clearStruct()
{
	if( live_stream )
	{
		free( live_stream );
		live_stream = 0;
	}
}
