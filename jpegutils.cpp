/*
 *  jpegutils.c: Some Utility programs for dealing with
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

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <jerror.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "jpegutils.h"



/*******************************************************************
 *                                                                 *
 *    The following routines define a JPEG Source manager which    *
 *    just reads from a given buffer (instead of a file as in      *
 *    the jpeg library)                                            *
 *                                                                 *
 *******************************************************************/


/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */

static void init_source( j_decompress_ptr cinfo )
{
   /* no work necessary here */
}


/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * Should never be called since all data should be allready provided.
 * Is nevertheless sometimes called - sets the input buffer to data
 * which is the JPEG EOI marker;
 *
 */

static uint8_t EOI_data[2] = { 0xFF, 0xD9 };

static boolean fill_input_buffer( j_decompress_ptr cinfo )
{
   cinfo->src->next_input_byte = EOI_data;
   cinfo->src->bytes_in_buffer = 2;
   return TRUE;
}


/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 */

static void skip_input_data( j_decompress_ptr cinfo, long num_bytes )
{
   if (num_bytes > 0) {
      if (num_bytes > (int) cinfo->src->bytes_in_buffer)
         num_bytes = (int) cinfo->src->bytes_in_buffer;
      cinfo->src->next_input_byte += (size_t) num_bytes;
      cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
   }
}


/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 */

static void term_source( j_decompress_ptr cinfo )
{
   /* no work necessary here */
}


/*
 * Prepare for input from a data buffer.
 */

static void
jpeg_buffer_src( j_decompress_ptr cinfo, unsigned char* buffer, int num )
{
	/* The source object and input buffer are made permanent so that a series
	* of JPEG images can be read from the same buffer by calling jpeg_buffer_src
	* only before the first one.  (If we discarded the buffer at the end of
	* one image, we'd likely lose the start of the next one.)
	* This makes it unsafe to use this manager and a different source
	* manager serially with the same JPEG object.  Caveat programmer.
	*/
	if (cinfo->src == NULL) /* first time for this JPEG object? */
	{
		cinfo->src = (struct jpeg_source_mgr *)	(*cinfo->mem->alloc_small)( (j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr) );
	}

	cinfo->src->init_source = init_source;
	cinfo->src->fill_input_buffer = fill_input_buffer;
	cinfo->src->skip_input_data = skip_input_data;
	cinfo->src->resync_to_restart = jpeg_resync_to_restart;	/* use default method */
	cinfo->src->term_source = term_source;
	cinfo->src->bytes_in_buffer = num;
	cinfo->src->next_input_byte = (JOCTET *) buffer;
}


/*******************************************************************
 *                                                                 *
 *    decode_jpeg_data: Decode a (possibly interlaced) JPEG frame  *
 *                                                                 *
 *******************************************************************/

/*
 * ERROR HANDLING:
 *
 *    We want in all cases to return to the user.
 *    The following kind of error handling is from the
 *    example.c file in the Independent JPEG Group's JPEG software
 */

struct my_error_mgr
{
	struct jpeg_error_mgr pub;   /* "public" fields */
	jmp_buf setjmp_buffer;       /* for return to caller */

	/* original emit_message method */
	JMETHOD(void, original_emit_message, (j_common_ptr cinfo, int msg_level));
	int warning_seen;		/* was a corrupt-data warning seen */
};

static void my_error_exit( j_common_ptr cinfo )
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	struct my_error_mgr *myerr = (struct my_error_mgr *) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp (myerr->setjmp_buffer, 1);
}

static void my_emit_message( j_common_ptr cinfo, int msg_level )
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	struct my_error_mgr *myerr = (struct my_error_mgr *) cinfo->err;

	if(msg_level < 0)
		myerr->warning_seen = 0;

	/* call original emit_message() */
	//GF: tolto per evitare la stampa di numerosi warnings in stderr
	//(myerr->original_emit_message)(cinfo, msg_level);
}


#if 1  /* generation of 'std' Huffman tables... */

static void add_huff_table( j_decompress_ptr dinfo, JHUFF_TBL** htblptr, const UINT8* bits, const UINT8* val )
/* Define a Huffman table */
{
	int nsymbols, len;

	if( *htblptr == NULL )
	{
		*htblptr = jpeg_alloc_huff_table((j_common_ptr) dinfo);
	}

	/* Copy the number-of-symbols-of-each-code-length counts */
	memcpy((*htblptr)->bits, bits, sizeof((*htblptr)->bits));

	/* Validate the counts.  We do this here mainly so we can copy the right
	* number of symbols from the val[] array, without risking marching off
	* the end of memory.  jchuff.c will do a more thorough test later.
	*/
	nsymbols = 0;
	for( len = 1; len <= 16; len++ )
	{
		nsymbols += bits[len];
	}

	if (nsymbols < 1 || nsymbols > 256)
	{
		fprintf( stderr, "jpegutils.cpp: can only do one image transformation at a time\n" );
		exit( 1 );
	}

	memcpy((*htblptr)->huffval, val, nsymbols * sizeof(UINT8));
}



static void std_huff_tables (j_decompress_ptr dinfo)
/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
{
  static const UINT8 bits_dc_luminance[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_luminance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits_dc_chrominance[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_chrominance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 bits_ac_luminance[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8 val_ac_luminance[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  static const UINT8 bits_ac_chrominance[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8 val_ac_chrominance[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  add_huff_table(dinfo, &dinfo->dc_huff_tbl_ptrs[0],
		 bits_dc_luminance, val_dc_luminance);
  add_huff_table(dinfo, &dinfo->ac_huff_tbl_ptrs[0],
		 bits_ac_luminance, val_ac_luminance);
  add_huff_table(dinfo, &dinfo->dc_huff_tbl_ptrs[1],
		 bits_dc_chrominance, val_dc_chrominance);
  add_huff_table(dinfo, &dinfo->ac_huff_tbl_ptrs[1],
		 bits_ac_chrominance, val_ac_chrominance);
}



static void guarantee_huff_tables( j_decompress_ptr dinfo )
{
	if( (dinfo->dc_huff_tbl_ptrs[0] == NULL) &&	(dinfo->dc_huff_tbl_ptrs[1] == NULL) &&	(dinfo->ac_huff_tbl_ptrs[0] == NULL) &&	(dinfo->ac_huff_tbl_ptrs[1] == NULL) )
	{
		std_huff_tables( dinfo );
	}
}


#endif /* ...'std' Huffman table generation */



/*
 * jpeg_data:       Buffer with jpeg data to decode
 * len:             Length of buffer
 * returns:
 *	-1 on fatal error
 *	0 on success
 *	1 if jpeg lib threw a "corrupt jpeg data" warning.  
 *		in this case, "a damaged output image is likely."
 *	
 */

int decode_jpeg_raw( unsigned char* jpeg_data, int len, unsigned char** rgb_data, int* w, int* h )
{
	struct jpeg_decompress_struct dinfo;
	struct my_error_mgr jerr;

	/* We set up the normal JPEG error routines, then override error_exit. */
	dinfo.err = jpeg_std_error( &jerr.pub );
	jerr.pub.error_exit = my_error_exit;
	/* also hook the emit_message routine to note corrupt-data warnings */
	jerr.original_emit_message = jerr.pub.emit_message;
	jerr.pub.emit_message = my_emit_message;
	jerr.warning_seen = 0;

	/* Establish the setjmp return context for my_error_exit to use. */
	if( setjmp(jerr.setjmp_buffer) )
	{
		/* If we get here, the JPEG code has signaled an error. */
		jpeg_destroy_decompress( &dinfo );
		return -1;
	}

	jpeg_create_decompress( &dinfo );

	//jpeg_mem_src( &dinfo, jpeg_data, len );
	jpeg_buffer_src( &dinfo, jpeg_data, len );

	/* Read header, make some checks and try to figure out what the user really wants */
	jpeg_read_header( &dinfo, TRUE );
	dinfo.raw_data_out = FALSE;
	dinfo.do_fancy_upsampling = TRUE;
	dinfo.out_color_space = JCS_RGB;
	dinfo.dct_method = JDCT_IFAST;
	guarantee_huff_tables( &dinfo );

	jpeg_start_decompress( &dinfo );

	if( dinfo.output_components != 3 )
	{
		fprintf( stderr, "jpegutils.cpp: output components of JPEG image = %d, must be 3\n", dinfo.output_components );
		jpeg_destroy_decompress( &dinfo );
		return -1;
	}

	int row_stride = dinfo.output_width * dinfo.output_components;
	JSAMPARRAY scanarray = (*dinfo.mem->alloc_sarray)((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, 1);
	unsigned char* p_rgb_data = *rgb_data;

	while( dinfo.output_scanline < dinfo.output_height )
	{
		(void) jpeg_read_scanlines( &dinfo, scanarray, 1 );

		memcpy( p_rgb_data, scanarray[0], row_stride );
		p_rgb_data += row_stride;
	}

	(void) jpeg_finish_decompress( &dinfo );

	*w = dinfo.image_width;
	*h = dinfo.image_height;

	jpeg_destroy_decompress( &dinfo );
	return jerr.warning_seen ? 1 : 0;
}




/*******************************************************************
 *                                                                 *
 *    transform_jpeg_raw: Transform a JPEG frame                   *
 *                                                                 *
 *******************************************************************/


//-----------------------------------------------------------------------------
// Silly little routine to detect multiple transform options,
// which we can't handle.
//-----------------------------------------------------------------------------
bool _select_transform( JXFORM_CODE transform, jpeg_transform_info& transformoption )
{
	if( transformoption.transform == JXFORM_NONE || transformoption.transform == transform )
	{
		transformoption.transform = transform;
		return true;
	}

	fprintf( stderr, "jpegutils.cpp: can only do one image transformation at a time\n" );
	return false;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void set_transform_default( jpeg_transform_info& transformoption )
{
	// Set up default JPEG parameters.
	transformoption.transform = JXFORM_NONE;
	transformoption.perfect = FALSE;
	transformoption.trim = FALSE;
	transformoption.force_grayscale = FALSE;
	transformoption.crop = FALSE;
}


//-----------------------------------------------------------------------------
// Crop to a rectangular subarea
//    spec = "WxH+X+Y"
//    spec = "" reset crop
//-----------------------------------------------------------------------------
bool set_crop( jpeg_transform_info& transformoption, const char* spec )
{
	if( !jtransform_parse_crop_spec(&transformoption, spec) )
	{
		fprintf( stderr, "jpegutils.cpp: bogus crop argument '%s'\n", spec);
		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool set_flip( jpeg_transform_info& transformoption, int mode )
{
	if( mode == JPEG_TRAN_FLIP_H )
	{
		return _select_transform( JXFORM_FLIP_H, transformoption );
	}
	else if( mode == JPEG_TRAN_FLIP_V )
	{
		return _select_transform( JXFORM_FLIP_V, transformoption );
	}
	return false;
}


//-----------------------------------------------------------------------------
// NB: Fail if there is any partial edge MCUs that the transform can't handle
//-----------------------------------------------------------------------------
bool set_perfect( jpeg_transform_info& transformoption )
{
	transformoption.perfect = TRUE;
	return true;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool set_rotate( jpeg_transform_info& transformoption, int mode )
{
	if( mode == JPEG_TRAN_ROT_90 )
	{
		return _select_transform( JXFORM_ROT_90, transformoption );
	}
	else if( mode == JPEG_TRAN_ROT_180 )
	{
		return _select_transform( JXFORM_ROT_180, transformoption );
	}
	else if( mode == JPEG_TRAN_ROT_270 )
	{
		return _select_transform( JXFORM_ROT_270, transformoption );
	}

	return false;
}


//-----------------------------------------------------------------------------
// Trim off any partial edge MCUs that the transform can't handle
//-----------------------------------------------------------------------------
bool set_trim( jpeg_transform_info& transformoption )
{
	transformoption.trim = TRUE;
	return true;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
typedef enum {
	JPEG_OPT_ARITHMETIC,
	JPEG_OPT_VERBOSE
} JPEG_OPT_CODE;

bool set_option( j_compress_ptr cinfo, int option )
{
	cinfo->arith_code = FALSE;
	cinfo->err->trace_level = 0;

	if( option & JPEG_OPT_ARITHMETIC )
	{
		/* Use arithmetic coding. */
		cinfo->arith_code = TRUE;
	}
	if( option & JPEG_OPT_VERBOSE )
	{
		/* Enable debug printouts. */
		cinfo->err->trace_level++;
	}

	return true;
}




int transform_jpeg_raw( unsigned char* jpeg_data, int len, jpeg_transform_info tran_option, unsigned char** out_data, unsigned long& out_len )
{
	struct jpeg_decompress_struct srcinfo;
	struct jpeg_compress_struct dstinfo;
	jvirt_barray_ptr* src_coef_arrays;
	jvirt_barray_ptr* dst_coef_arrays;


	/* Initialize the JPEG decompression object with custom error handling. */
	struct my_error_mgr jsrcerr;

	/* We set up the normal JPEG error routines, then override error_exit. */
	srcinfo.err = jpeg_std_error( &jsrcerr.pub );

	jsrcerr.pub.error_exit = my_error_exit;
	/* also hook the emit_message routine to note corrupt-data warnings */
	jsrcerr.original_emit_message = jsrcerr.pub.emit_message;
	jsrcerr.pub.emit_message = my_emit_message;
	jsrcerr.warning_seen = 0;

	/* Establish the setjmp return context for my_error_exit to use. */
	if( setjmp(jsrcerr.setjmp_buffer) )
	{
		/* If we get here, the JPEG code has signaled an error. */
		jpeg_destroy_decompress( &srcinfo );
		jpeg_destroy_decompress( &srcinfo );
		return -1;
	}

	/* Initialize the JPEG compression object with default error handling. */
	struct jpeg_error_mgr jdsterr;
	dstinfo.err = jpeg_std_error(&jdsterr);


	jpeg_create_decompress( &srcinfo );
	jpeg_create_compress( &dstinfo );


	/* Note that most of the transform option affect the destination JPEG object,
	*  so we parse into that and then copy over what needs to affects the source too.
	*/
	srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;


	/* Specify data source for decompression */
	jpeg_mem_src( &srcinfo, jpeg_data, len );

	/* Enable saving of extra markers that we want to copy */
	jcopy_markers_setup( &srcinfo, JCOPYOPT_DEFAULT );

	/* Read header, make some checks and try to figure out what the user really wants */
	jpeg_read_header( &srcinfo, TRUE );
	//srcinfo.raw_data_out = FALSE;
	//srcinfo.do_fancy_upsampling = TRUE;
	//srcinfo.out_color_space = JCS_RGB;
	//srcinfo.dct_method = JDCT_IFAST;
	guarantee_huff_tables( &srcinfo );


	/* Any space needed by a transform option must be requested before
	* jpeg_read_coefficients so that memory allocation will be done right.
	* Fail right away if perfect flag is given and transformation is not perfect.
	*/
	if( !jtransform_request_workspace( &srcinfo, &tran_option ) )
	{
		fprintf( stderr, "jpegutils.cpp: transformation is not perfect\n");
		jpeg_destroy_compress(&dstinfo);
		jpeg_destroy_decompress(&srcinfo);
		return 0;
	}

	/* Read source file as DCT coefficients */
	src_coef_arrays = jpeg_read_coefficients(&srcinfo);

	/* Initialize destination compression parameters from source values */
	jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

	/* Adjust destination parameters if required by transform options;
	* also find out which set of coefficient arrays will hold the output.
	*/
	dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo, src_coef_arrays, &tran_option);


	/* Specify data destination for compression */
	jpeg_mem_dest( &dstinfo, out_data, &out_len );


	/* Start compressor (note no image data is actually written here) */
	jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

	/* Copy to the output file any extra markers that we want to preserve */
	jcopy_markers_execute( &srcinfo, &dstinfo, JCOPYOPT_DEFAULT );

	/* Execute image transformation, if any */
	jtransform_execute_transformation(&srcinfo, &dstinfo, src_coef_arrays, &tran_option);

	/* Finish compression and release memory */
	jpeg_finish_compress(&dstinfo);
	jpeg_destroy_compress(&dstinfo);
	(void) jpeg_finish_decompress(&srcinfo);
	jpeg_destroy_decompress(&srcinfo);

	/* All done. */
	//exit( jsrcerr.num_warnings + jdsterr.num_warnings ? EXIT_WARNING: EXIT_SUCCESS );
	return 1;			/* suppress no-return-value warnings */
}
