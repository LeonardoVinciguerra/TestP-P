#ifndef __Q_ZIP_H
#define __Q_ZIP_H

#include <zlib.h>
#include <list>

#define MSDOS_ATTR_RDONLY	   0x01
#define MSDOS_ATTR_DIR		   0x10


#define QZIP_OK                0
#define QZIP_NOTOPEN          -1
#define QZIP_SEEK_ERR         -2

#define QZIP_NOLOCALHDR       -3
#define QZIP_NOCDIRHDR        -4
#define QZIP_NOCDIREND        -5

#define QZIP_READ_ERR         -6
#define QZIP_FORMAT_ERR       -7
#define QZIP_ISOPEN           -8
#define QZIP_NOTFOUND         -9
#define QZIP_OPEN_ERR        -10
#define QZIP_NOINIT_ZIP      -11
#define QZIP_INTERNAL_ERR    -12
#define QZIP_WRITE_ERR       -13


#define COMPRESSION_NORMAL       0 //this one is the only supported
#define COMPRESSION_MAXIMUM      2
#define COMPRESSION_FAST         4
#define COMPRESSION_SUPERFAST    6

#define ZIP_OPEN_READ     1
#define ZIP_OPEN_CREATE   2
#define ZIP_OPEN_SPAN     4

#pragma pack(1)

using namespace std;

typedef struct
{
  unsigned char *data;
  unsigned int len;
} sDataBlock;

typedef struct
{
  struct LocHdr
  {  
    unsigned short ver_to_extract;
    unsigned short gp_flag;
    unsigned short method;
    unsigned short last_mod_time;
    unsigned short last_mod_date;
    unsigned int   crc32;
    unsigned int   compressed_size;
    unsigned int   uncompressed_size;
    unsigned short filename_len;
    unsigned short extrafield_len;
  } hdr;

  struct LocHdrVar
  {
    char *filename;
    char *extrafield;
  } hdr_var;

  struct LocData
  {
    unsigned int len;
    unsigned char *buf;
  } data;

  unsigned int pos;

} sLocalData;

typedef struct
{
  struct CDirHdr
  {
    unsigned short version_madeby;
    unsigned short version_needed;
    unsigned short gp_flag;
    unsigned short method;
    unsigned short last_mod_time;
    unsigned short last_mod_date;
    unsigned int   crc32;
    unsigned int   compressed_size;
    unsigned int   uncompressed_size;
    unsigned short filename_len;
    unsigned short extrafield_len;
    unsigned short filecomment_len;
    unsigned short diskstart;
    unsigned short internal_attrib;
    unsigned int   external_attrib;
    unsigned int   localhdr_offset;
  } hdr;

  struct CDirHdrVar
  {
    char *filename;
    char *extrafield;
    char *filecomment;
  } hdr_var;

  unsigned int pos;
  
  
} sCDirData;

typedef struct
{
  struct CDirEndHdr
  {
    unsigned short disk;
    unsigned short disk_startcd;
    unsigned short nentry_cdir;
    unsigned short nentry_cdir_tot;
    unsigned int   cdir_size;
    unsigned int   cdir_offset;
    unsigned short zipcomment_len;
  } hdr;

  struct CDirEndVar
  {
    char *zipcomment;
  } hdr_var;

  unsigned int pos;
  
} sCDirEnd;

#define EXTRAFIELDTAG_EXTTS 					"UT"
#define EXTRAFIELDTAG_UNIX 						"Ux"

#define LOC_EXTRAFIELDSIZE_EXTTS 				9		
#define LOC_EXTRAFIELDSIZE_UNIX  				4

#define EXTRAFIELD_EXTTS_FLAG_MTIME				(1 << 0)
#define EXTRAFIELD_EXTTS_FLAG_ATIME				(1 << 1)
#define EXTRAFIELD_EXTTS_FLAG_CTIME				(1 << 2)
#define EXTRAFIELD_EXTTS_FLAGS					(EXTRAFIELD_EXTTS_FLAG_ATIME | EXTRAFIELD_EXTTS_FLAG_MTIME)

#define CDATA_EXTRAFIELDSIZE_EXTTS 				5		
#define CDATA_EXTRAFIELDSIZE_UNIX  				0

#define MSDOS_ATTRIB_DIR						0x10

typedef struct
{
	char tag[2];
	unsigned short size;
	unsigned char flags;	
	time_t modtime;
	time_t lastacc;		
} sLocalExtraField_extendedTS;

typedef struct
{
	char tag[2];
	unsigned short size;
	unsigned short uid;
	unsigned short gid;
} sLocalExtraField_unix;

typedef struct
{
	char tag[2];
	unsigned short size;
	unsigned char flags;	
	time_t modtime;
} sCDataExtraField_extendedTS;

typedef struct
{
	char tag[2];
	unsigned short size;
} sCDataExtraField_unix;

#pragma pack()

#define LOCHDR_SIZE sizeof(sLocalData::LocHdr)
#define CDIRHDR_SIZE sizeof(sCDirData::CDirHdr)
#define CDIREND_SIZE sizeof(sCDirEnd::CDirEndHdr)

#define QZIP_DECOMP   0
#define QZIP_DIRCOMP  1
#define QZIP_COMPRESS 2

#define MAX_PROGFUNC_TYPE 3

class ZipClass
{
  private:
    int hFile;

    z_stream zs;
    unsigned char *zs_buffer;
    int zs_init;
    int zs_FlushDest;
    
    std::list<sDataBlock>   zTempList;
	std::list<sLocalData *> zLocalDataList;
	std::list<sCDirData *>  zCDirDataList;
    sCDirEnd *zCDirEnd;

    unsigned int zCrc32;

    unsigned int zOpenMode;
    unsigned int zSpan;          //in create mode: maximum file size for spanning
                                 //(0 for no spanning)
                                 //in read mode 1 for multidisk spanning 0 otherwise
    unsigned int zWriting;

    int zDisk;
    int zSpanAvail;

    char *zFileName;

    int (*zPromptDisk)(int,char *);
    int (*fProgress[MAX_PROGFUNC_TYPE])(void *);

    //Read & Write segments

    int GetLocalFileData(sLocalData *locdata);
    int ReadDataBlock(sLocalData *locdata);

    int GetCentralDirData(sCDirData *cdir_data);
    int GetAllCentralDirData(void);
    
    int GetCentralDirEnd(sCDirEnd *cdir_end);

    int WriteLocalFileData(sLocalData *locdata,unsigned int *locoffset=NULL);
    int WriteCDirData(sCDirData *cdir_data);
    int WriteCentralDirEnd(sCDirEnd *cdir_end);

    int WriteToZipEnd(void);

    //Memory and list function

    sLocalData *AllocLocalFileData(void);
    void FreeLocalFileData(sLocalData *d);
    void ClearLocalDataList(void);
    int GetLocalDataSize(sLocalData *d);

    sCDirData *AllocCDirData(void);
    void FreeCDirData(sCDirData *d);
    void ClearCDirDataList(void);
    int GetCDirDataSize(sCDirData *d);

    sCDirEnd *AllocCDirEnd(void);
    void FreeCDirEnd(sCDirEnd *d);

    void AddToTempBlocks(unsigned char *data,int len);
    void ClearTempBlocks(void);
    unsigned char *FlushTempBlocks(unsigned int &len);
    
    //Compression functions

    int CompressInit(int level,int method);
    int CompressBuffer(unsigned char *inbuf,int len);
    int CompressEnd(void);

    //DeCompression functions

    int  DeCompressInit(void);
    int  DeCompressBuffer(unsigned char *inbuf,int len);
    void DeCompressEnd(void);

    int  ZipStreamFlush(void);


  public:

    ZipClass(void);
    ~ZipClass(void);

    int  Open(const char *fname,int mode);
    void Close(void);

    int  SetZipNotes(const char *notes);
    int  GetZipNotes(char *notes);

    void SetSpanSize(unsigned int size);

    int  IsOpen(void);
    int  IsSpan(void);

    int  GetTotalDiskNumber(void);
    int  GetTotalFilesNumber(void);

	int  CompressFile(const char *file,int level,bool remove_pathname=false);
    int  CompressDir(const char *dir,int level);
    int  WriteToZip(void);

    int  DeCompressAll(const char *dest);

    int  GetNumberOfDisks(void);
    int  GetNumberOfFiles(void);

    int  PromptForDisk(int disk);

    void SetProgressFunction(int(*f)(void *),int mode);
    void UnSetProgressFunction(int(*f)(void *),int mode);

    void SetPromptForDiskFunction(int(*f)(int,char *));

};

#endif
