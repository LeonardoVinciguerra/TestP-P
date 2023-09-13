//---------------------------------------------------------------------------
//
// Name:        q_slide.cpp
// Author:      Gabriel Ferri
// Created:     13/04/2012
// Description: Salvataggio immagine schermo
//
//---------------------------------------------------------------------------
#include "q_slide.h"

#include <sys/stat.h>
#include <stdio.h>
#include "img_savepng.h"
#include "fileutils.h"
#include "mathlib.h"
#include "tv.h"

#include <mss.h>


#define SLIDES_DIR "slides"


//---------------------------------------------------------------------------
// Salva lo schermo in file slideXXX.png progressivi
//---------------------------------------------------------------------------
void make_slide()
{
	int cam = pauseLiveVideo();

	mkdir( SLIDES_DIR, DIR_CREATION_FLAG );

	// look for last slide number
	int file_n = 0;
	std::vector<std::string> fileList;

	FindFiles( SLIDES_DIR, "*.png", fileList );

	for( unsigned int i = 0; i < fileList.size(); i++ )
	{
		char nbuf[5];
		snprintf( nbuf, 5, "%s", strchr( fileList[i].c_str(), 'e' ) + 1 );
		int num = atoi( nbuf );

		if( num > file_n )
			file_n = num;
	}

	file_n = MIN( file_n + 1, 999 );

	// save slide
	char filename[MAXNPATH];
	snprintf( filename, MAXNPATH, "%s/slide%03d.png", SLIDES_DIR, file_n );

	IMG_SavePNG( filename, SDL_GetVideoSurface(), 9 );

	playLiveVideo( cam );
}
