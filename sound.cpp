//---------------------------------------------------------------------------
//
// Name:        sound.cpp
// Author:      Gabriel Ferri
// Created:     17/10/2008 11.42.20
// Description: sound functions implementation
//
//---------------------------------------------------------------------------
#include "sound.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <stdio.h>

#include <mss.h>


bool sound( int freq, int len )
{
	if( freq == 0 || len == 0 )
	{
		return false;
	}

	int fd = open( "/dev/console", O_WRONLY );
	if( fd == -1 )
	{
		return false;
	}

	ioctl( fd, KIOCSOUND, (int)(1193180/freq) );
	usleep( 1000*len );
	ioctl( fd, KIOCSOUND, 0 );
	close( fd );

	return true;
}
