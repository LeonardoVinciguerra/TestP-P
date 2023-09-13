//---------------------------------------------------------------------------
//
// Name:        q_packages.h
// Author:      Gabriel Ferri
// Created:     27/02/2012
// Description: Quadra packages manager
//
//---------------------------------------------------------------------------

#ifndef __Q_PACKAGES_H
#define __Q_PACKAGES_H

#include <list>
#include "c_win_par.h"
#include "c_win_table.h"

#define PACKAGESLIB_EXT           ".pak"
#define PACKAGESOFFLIB_EXT        ".pof"

#define PACK_DIMOK     1
#define PACK_ERROR     0
#define PACK_TOOBIG   -1
#define PACK_TOOHIGH  -2
#define PACK_NOTOOLS  -3


#define PKDISPLACEMENT_MIN      0.f
#define PKDISPLACEMENT_MAX      20.f


namespace CenteringMode
{
	enum eCentering
	{
		SNIPER = 0,
		EXTCAM,
		NONE
	};
}

//************************************************************************
// file: libreria packages
//************************************************************************
#pragma pack(1)
struct SPackageData
{
	unsigned short code;

	char name[22];
	char notes[24];

	float x;
	float y;
	float z;

	unsigned char speedXY;
	unsigned char speedRot;
	unsigned char speedPick;
	unsigned char speedPlace;

	char tools[8];
	char centeringMode;
	float orientation;

	// sniper
	float snpX;            // dimensione misurata sniper
	float snpY;
	float snpZOffset;      // offset di scansione dal centro del componente
	float snpTolerance;    // tolleranza
	unsigned char snpMode; // algoritmo di centraggio

	// external camera
	float extAngle;        // angolo di centraggio

	// advanced params
	unsigned char centeringPID;
	unsigned char placementPID;
	char checkPick;
	unsigned char checkVacuumThr;
	unsigned char steadyCentering;
	char placementMode;

	// ledth placement
	float ledth_insidedim;
	float ledth_securitygap;
	float ledth_interf;
	int ledth_place_vmin;
	int ledth_place_vel;
	int ledth_place_acc;
	int ledth_rising_vmin;
	int ledth_rising_vel;
	int ledth_rising_acc;

	unsigned char check_post_place;

	char spare[99];
};

struct SPackageOffsetData
{
	float angle;
	float offX[4];
	float offY[4];
	float offRot[4];
};
#pragma pack()



//---------------------------------------------------------------------------
// finestra: Package parameters
//---------------------------------------------------------------------------
class PackageParamsUI : public CWindowParams
{
public:
	PackageParamsUI( CWindow* parent, int curRecord );
	~PackageParamsUI();

	typedef enum
	{
		NAME,
		NOTES,
		DIM_X,
		DIM_Y,
		DIM_Z,
		SPEED_XY,
		SPEED_ROT,
		SPEED_PICK,
		SPEED_PLACE,
		TOOLS,
		CENTERING,
		ORIENTATION,
		SNIPER_X,
		SNIPER_Y,
		SNIPER_Z,
		SNIPER_TOL,
		SNIPER_MODE,
		EXT_ANGLE
	} combo_labels;

protected:
	void onInit();
	void onShow();
	void onRefresh();
	void onEdit();
	void onShowMenu();
	bool onKeyPress( int key );
	void onClose();

private:
	int onPackageZOffset();
	int onPackageAngleTeach();
	int onAdvancedParameters();
	int onImageParams();
	int onDimensionTeach1();
	int onDimensionTeach2();
	int onImageTeach1();
	int onImageTeach2();
	int onPackageOffset();

	#ifndef __DISP2
	int onDispensingData();
	#else
	int onDispensingData1();
	int onDispensingData2();
	#endif

	bool adjustPackageData();
	bool checkPackageData();

	int index;

	GUI_SubMenu* SM_DimTeach; // sub menu apprendimento dimensioni
	GUI_SubMenu* SM_VisionTeach; // sub menu apprendimento visione
	GUI_SubMenu* SM_VisionParams; // sub menu parametri visione
	#ifdef __DISP2
	GUI_SubMenu* SM_DispParams; // sub menu parametri dispensers
	#endif
};


//---------------------------------------------------------------------------
// finestra: Package advanced parameters
//---------------------------------------------------------------------------
class PackageAdvancedParamsUI : public CWindowParams
{
public:
	PackageAdvancedParamsUI( CWindow* parent, int curRecord );

	typedef enum
	{
		CHECK_PICK,
		CHECK_VTHR,
		PID_CENT,
		PID_PLACE,
		STEADY_CENT,
		PLACE_MODE,
		INSIDE_DIM,
		SECURITY_GAP,
		INTERFERENCE,
		VELSTART_DOWN,
		VEL_DOWN,
		ACC_DOWN,
		VELSTART_UP,
		VEL_UP,
		ACC_UP,
		CHECK_PLACE
	} combo_labels;

protected:
	void onInit();
	void onShow();
	void onRefresh();
	void onEdit();
	void onClose();

private:
	int index;
};



//---------------------------------------------------------------------------
// finestra: Package corrections
//---------------------------------------------------------------------------
class PackageCorrectionsUI : public CWindowTable
{
public:
	PackageCorrectionsUI( CWindow* parent, int curRecord );
	~PackageCorrectionsUI();


	typedef enum
	{
		GLOBAL_ANGLE
	} combo_labels;

protected:
	void onInit();
	void onShow();
	void onRefresh();
	void onEdit();
	void onShowMenu();
	bool onKeyPress( int key );

	int onSelectionChange( unsigned int row, unsigned int col );
	int onOffsetTeaching();

private:
	std::map<int,C_Combo*> combos;
	CComboList* comboList;

	int index;
	int selectedObject;
	int prevTableCol;
	bool firstTime;
};



//---------------------------------------------------------------------------
// finestra: Package dispensing
//---------------------------------------------------------------------------
class PackageDispensingUI : public CWindowTable
{
public:
	PackageDispensingUI( CWindow* parent, int curRecord, int dispenser );
	~PackageDispensingUI();

	typedef enum
	{
		QUANT,
		DISPL
	} combo_labels;

protected:
	void onInit();
	void onShow();
	void onRefresh();
	void onEdit();
	void onShowMenu();
	bool onKeyPress( int key );
	void onClose();

	int onSelectionChange( unsigned int row, unsigned int col );
	int onPackPreview();

private:
	std::map<int,C_Combo*> combos;
	CComboList* comboList;

	int index;
	int dispNumber;
	int selectedObject;
	int prevTableCol;
	bool firstTime;

	void printPackHints();
};



//---------------------------------------------------------------------------
// finestra: Packages selection
//---------------------------------------------------------------------------
class PackagesSelectUI : public CWindowTable
{
public:
	PackagesSelectUI( CWindow* parent, const std::string& name, int selectEnable = false );
	~PackagesSelectUI();

	void ShowItems();
	std::string GetPackageName();
	int GetPackageCode();

	int GetExitCode() { return m_exitCode; }

protected:
	virtual void onEnter() {}
	virtual bool onHotKey( int key ) { return false; }

	class CPan* tips;

private:
	struct SRow
	{
		int index;
		std::string name;
		std::string notes;
	};

	virtual void onInit();
	virtual void onShow();
	virtual void onShowMenu();
	virtual void onRefresh() {}
	virtual void onEdit() {}
	virtual void onClose();
	virtual bool onKeyPress( int key );
	virtual void onIdle() {}

	void loadPackages();
	void showItems( int start_item );
	bool vSelect( int key );
	void searchItem();
	void showSearch( bool error = false );
	bool compareNoCase( SRow row1, SRow row2 );
	int getSelectedRow();

	int searchFirstFreeSlot();
	int searchName( const std::string& name );

	int showPackage();
	int newPackage();
	int duplicatePackage();
	int renamePackage();
	int deletePackage();

	int m_exitCode;
	int m_x, m_y;
	unsigned int m_numRows;
	unsigned int m_width;

	std::list<SRow> m_items;
	unsigned int m_start_item;

	std::string m_search;
	bool m_selectEnable;
};



bool PackagesLib_Load( const std::string& libname );
bool PackagesLib_Save( const std::string& libname );
bool PackagesLib_Create( const std::string& libname );


int CheckPackageNozzle( SPackageData* pack, int nozzle );
int CheckPackageVisionData( SPackageData& pack, char* libname, bool showError );
void GetPackageOffsetCorrection( float angle, int packIndex, float& x, float& y, int& theta );


int fn_PackagesTable( CWindow* parent, const std::string& name = "" );
int fn_PackagesTableSelect( CWindow* parent, const std::string& name, int& packCode, std::string& packName );

#endif
