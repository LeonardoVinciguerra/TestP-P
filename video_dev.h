//---------------------------------------------------------------------------
//
// Name:        video_dev.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: VideoDevice class definition
//
//---------------------------------------------------------------------------

#ifndef __VIDEO_DEV_H
#define __VIDEO_DEV_H


//---------------------------------------------------------------------------------
// VideoDevice
// ATTENZIONE! quando e' attiva la visualizzazione live nessun altro processo puo'
// disegnare pixel sullo schermo
//---------------------------------------------------------------------------------


#include <string>
#include <boost/thread.hpp>
#include "v4l2.h"

#define __USE_LIBWEBCAM


class VideoDevice
{
public:
	VideoDevice();
	~VideoDevice();

	bool OpenDevice( const char* deviceName, int width, int height, int format, int fps );
	void SetDeviceKey( const char* key ) { devKey = key; }

	VideoForLinux2* GetV4LDevice() { return &v4lDevice; }

	//------------//
	//  Controls  //
	//------------//

	void SetResolution( unsigned int width, unsigned int height );
	bool SetCrop_Centered( unsigned int width, unsigned int height );
	bool SetBrightness( unsigned int value );
	bool SetContrast( unsigned int value );


	//----------//
	//  Thread  //
	//----------//

	void StartThread();
	void StopThread();


	//--------------//
	//  Live video  //
	//--------------//

	void SetVideoCallback( void (*f)( unsigned char* img, int img_w, int img_h, int bpp ) ) { m_fnDisplayFrameCallback = f; }
	bool IsVideoLive() { return show_video; }
	void PlayVideo();
	void StopVideo();


	//---------//
	//  Frame  //
	//---------//

	int GetFrame_Width() { return video_w; }
	int GetFrame_Height() { return video_h; }
	int GetFrame_Bpp() { return video_bpp; }
	int GetFrame_SizeInByte() { return video_w * video_h * video_bpp; }
	void GetFrame( void* frame );


private:
	void showLive();
	void allocateStruct( unsigned int w, unsigned int h );
	void clearStruct();
	bool setOutputImageCrop();

	VideoForLinux2 v4lDevice;

	std::string devName;
	std::string devKey;
	int fd;

	boost::thread m_Thread;
	boost::mutex stop_video_mutex;

	int video_fmt; // formato video
	int video_fps; // frames per second
	unsigned int video_def_w, video_def_h; // dimensione frame richiesta
	unsigned int video_w, video_h; // dimensione frame reale
	char video_bpp; // byte per pixel

	bool is_streaming;
	bool show_video;
	bool frame_ready;

	unsigned int live_stream_w, live_stream_h; // dimensione buffer
	void* live_stream;

	unsigned int crop_w;
	unsigned int crop_h;

	void (*m_fnDisplayFrameCallback)( unsigned char* img, int img_w, int img_h, int bpp ); // puntatore a funzione di callback
};

#endif
