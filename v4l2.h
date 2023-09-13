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

#ifndef __V4L2_H
#define __V4L2_H

#include <string>
#include <linux/videodev2.h>
#include <vector>

#define __USE_PIXFC_SSE__
#define __USE_JPEGUTILS__


#ifdef __USE_PIXFC_SSE__
	#include <pixfc-sse.h>
#endif

#ifdef __USE_JPEGUTILS__
	#include "jpegutils.h"
#endif


typedef struct _CUSBInfo
{
	/// The vendor ID.
	unsigned short	vendor;
	/// The product ID.
	unsigned short	product;
	/// The product revision number.
	unsigned short	release;

} CUSBInfo;


typedef struct _V4L2Device
{
	/// A unique short name.
	/// For V4L2 devices this is the name of the device file. For example, if
	/// a device appears as '/dev/video0', the short name is 'video0'.
	std::string longName;
	std::string shortName;

	/// The human-readable name of the device.
	std::string name;

	/// The name of the driver providing the camera interface.
	std::string driver;

	/// The location of the camera.
	/// This string is dependent on the implementation that provides the device.
	/// It could either be a string provided by the driver or a device name.
	std::string location;

	/// USB related information about the camera.
	CUSBInfo usb;

} V4L2Device;


typedef struct _V4L2Control
{
	v4l2_queryctrl ctrl;
	std::vector<v4l2_querymenu> choices_list;

} V4L2Control;


unsigned int get_control_id_from_v4l2( int v4l2_id );
bool EnumerateV4L2Device( std::vector<V4L2Device>& devices );



class VideoForLinux2
{
public:
	VideoForLinux2();
	~VideoForLinux2();

	void setFd( int val, const char* deviceName ) { fd = val; devName = deviceName; }
	bool checkDevice( enum v4l2_buf_type type, unsigned int format );

	bool mapDevice( int numBuffers );
	void umapDevice();
	bool isMapped() { return mapped; }

	bool streamOn();
	bool streamOff();

	int getFrame( char** buffer, int* w, int* h );

	bool setResolution( unsigned int& width, unsigned int& height );
	bool getResolution( unsigned int& width, unsigned int& height );

	bool setFPS( unsigned int& fps );
	bool getFPS( unsigned int& fps );

	#ifdef __USE_JPEGUTILS__
	bool setCropArea( int x, int y, int w, int h );
	void resetCropArea();
	#endif

	bool setControlValue( unsigned int id, int value );
	bool getControlValue( unsigned int id, int& value );
	bool getControl( unsigned int id, V4L2Control** ctrl );

	int getControlsCount() { return controls.size(); }
	bool getControlAt( unsigned int index, V4L2Control** ctrl );

	bool selectInput( const char* name );

private:
	void enumerateMenu( V4L2Control& control );
	void enumerateControls();
	void enumerateInputs();

	bool checkCapabilities();
	bool checkPixelFormat( unsigned int format );

	void allocateConversionStructs( unsigned int w, unsigned int h );
	void clearConversionStructs();

	int fd;
	enum v4l2_buf_type devType;
	std::string devName;

	bool mapped;
	bool streaming;

	struct v4l_buffer
	{
		void* start;
		size_t length;
	};

	struct v4l_buffer* buffers;
	unsigned int n_buffers;

	int pixelFormat;

	#ifdef __USE_PIXFC_SSE__
	struct PixFcSSE* pixfc;
	#endif

	#ifdef __USE_JPEGUTILS__
	unsigned char* jpeg_trans_buf;
	jpeg_transform_info tran_option;
	#endif

	std::vector<v4l2_input> inputs;
	std::vector<V4L2Control> controls;

	int frameW;
	int frameH;
};


#endif
