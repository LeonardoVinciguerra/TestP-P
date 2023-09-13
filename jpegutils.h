/*
 *  jpegutils.h: Some Utility programs for dealing with
 *               JPEG encoded images
 *
 *  Copyright (C) 1999 Rainer Johanni <Rainer@Johanni.de>
 *  Copyright (C) 2001 pHilipp Zabel  <pzabel@gmx.de>
 *
 *  based on jdatasrc.c and jdatadst.c from the Independent
 *  JPEG Group's software by Thomas G. Lane
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef __JPEGUTILS_H__
#define __JPEGUTILS_H__

#include <jpeglib.h>
extern "C" {
#include "transupp.h"
}

 /*
 * jpeg_data:       buffer with input / output jpeg
 * len:             Length of jpeg buffer
 * width            width of Y channel (width of U/V is width/2)
 * height           height of Y channel (height of U/V is height/2)
 */

int decode_jpeg_raw( unsigned char* jpeg_data, int len, unsigned char** rgb_data, int* w, int* h );



typedef enum {
	JPEG_TRAN_NONE,			/* no transformation */
	JPEG_TRAN_FLIP_H,		/* horizontal flip */
	JPEG_TRAN_FLIP_V,		/* vertical flip */
	JPEG_TRAN_ROT_90,		/* 90-degree clockwise rotation */
	JPEG_TRAN_ROT_180,		/* 180-degree rotation */
	JPEG_TRAN_ROT_270		/* 270-degree clockwise (or 90 ccw) */
} JPEG_TRAN_CODE;


void set_transform_default( jpeg_transform_info& transformoption );
bool set_crop( jpeg_transform_info& transformoption, const char* spec );
bool set_flip( jpeg_transform_info& transformoption, int mode );
bool set_perfect( jpeg_transform_info& transformoption );
bool set_rotate( jpeg_transform_info& transformoption, int mode );
bool set_trim( jpeg_transform_info& transformoption );

int transform_jpeg_raw( unsigned char* jpeg_data, int len, jpeg_transform_info tran_option, unsigned char** out_data, unsigned long& out_len );

#endif
