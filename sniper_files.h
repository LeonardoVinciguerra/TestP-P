
#ifndef __SNIPER_FILES_H
#define __SNIPER_FILES_H

#ifdef __UBUNTU14
	#ifdef __DEBUG
		#define SOFT_VER		"200.14"
	#else
		#define SOFT_VER		"2.14"
	#endif
#elif __UBUNTU16
	#ifdef __DEBUG
		#define SOFT_VER		"400.14"
	#else
		#define SOFT_VER		"4.14b"
	#endif
#elif __UBUNTU18
	#ifdef __DEBUG
		#define SOFT_VER		"300.14"
	#else
		#define SOFT_VER		"3.14"
	#endif
#else
	#ifdef __DEBUG
		#define SOFT_VER		"100.14"
	#else
		#define SOFT_VER		"1.14"
	#endif
#endif

	#define STDFILENAME		"qsniper"
	#define EXE_FILE		"qdvcevo-exe"
	#define STD_CUSTOMER	"tws"
	#define STD_PROGRAM		"test01"
	#define STD_FEEDCONF	"test01"
	#define MAPFILENAME		"map"
	#define MOTORHEADFILENAME		"head"

	#define FPACKDOSEXT		".sdk"
	#define FPACKDOSEXT1	".sd1"

	#define HOME_DIR					"/home/quadradvc"
	#define INSTALL_STORAGE_DIR			"install"
	#define FONT_DIR                    "font"

	#define ZIP_DATA_FILE				"datadir.zip"
	#define ZIP_EXE_FILE				"qdvc-exe.zip"
	#define ZIP_HOME_FILE				"home.zip"

	#define START_SCRIPT_FILE			"qdvc"
	#define INSTALL_SCRIPT_FILE			"qinstall"
	#define UPDATE_SCRIPT_FILE			"qupdate"
	#define BACKUP_SCRIPT_FILE			"qbackup"
	#define RESTORE_SCRIPT_FILE			"qrestore"
	#define OFF_SCRIPT_FILE				"off"
	#define RESET_SCRIPT_FILE			"reset"
	#define SET_LANGUAGE_SCRIPT_FILE	"setlang"

	#ifdef __DISP2_REMOVABLE
		#define START_DISP1_SCRIPT_FILE		"disp1"
		#define START_DISP2_SCRIPT_FILE		"disp2"
		#define COPY_DISP1_DATA_SCRIPT_FILE	"copy1"
		#define COPY_DISP2_DATA_SCRIPT_FILE	"copy2"
		#define DISP1_DATA_DIR				"disp1"
		#define DISP2_DATA_DIR				"disp2"
	#endif

	#define INSTALL_HELPER_SCRIPT_FILE	"_install"
	#define UPDATE_HELPER_SCRIPT_FILE	"_update"
	#define RESTORE_INFO_FILE			"restore_info"

	#define INSTALL_HELPER_FILES_DIR	"install/"

	#define VERSION_DEF_FILE			STDFILENAME ".ver"

	#define REMOTEBKP_DIR				"qsniper-bkp"

#endif
