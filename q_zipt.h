#ifndef __Q_ZIPT_H
#define __Q_ZIPT_H

#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filefn.h"

#define SIGNATURE_SIZE                  4

#define SIGN_LOCAL_HEADER               0x04034b50
#define SIGN_CENTRAL_DIRECTORY          0x02014b50
#define SIGN_END_CENTRAL_DIRECTORY      0x06054b50
#define SIGN_SPANNING_MARKER            0x08074b50
#define SIGN_TEMPSPANNING_MARKER        0x30304b50

#define ZIP_MADEBY_MSDOS				0
#define ZIP_MADEBY_UNIX					3

#define VERSIONMADEBY	ZIP_MADEBY_UNIX
#define VERSION_NEEDED  20   	//2.0


#ifndef DEF_MEM_LEVEL
#  if MAX_MEM_LEVEL >= 8
#    define DEF_MEM_LEVEL 8
#  else
#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#  endif
#endif

#ifndef Z_BUFSIZE
#define Z_BUFSIZE (32*1024)
#endif

#define DEFAULT_SPANSIZE int(1.4*1024*1024)

#endif
