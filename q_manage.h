//---------------------------------------------------------------------------
//
// Name:        q_manage.h
// Author:      Gabriel Ferri
// Created:     31/01/2012
// Description: Quadra customer, programs,etc... manager
//
//---------------------------------------------------------------------------

#ifndef __Q_MANAGE_H
#define __Q_MANAGE_H

#include <map>
#include "c_win_select.h"
#include "c_combolist.h"


//---------------------------------------------------------------------------
// finestra: Customers manager
//---------------------------------------------------------------------------
class CustomersManagerUI : public CWindowSelect
{
public:
	CustomersManagerUI( CWindow* parent );
	~CustomersManagerUI();

protected:
	virtual void onShowSomething();
	virtual void onEnter();
	virtual void onShowMenu();
	virtual bool onHotKey( int key );

private:
	GUI_SubMenu* SM_SaveOn; // sub menu salva in ...
	GUI_SubMenu* SM_LoadFrom; // sub menu carica da ...

	std::vector<std::string> nameList;

	int newCustomer();
	int duplicateCustomer();
	int renameCustomer();
	int deleteCustomer();
	int toSharedFolder();
	int fromSharedFolder();
	int toUSBDevice();
	int fromUSBDevice();

	void loadCustomer();
	bool zipFilesAndCopy( const std::string& cust, const std::string& dest );
	bool unzipFiles( const std::string& cust, const std::string& src );
};

int fn_CustomersManager();



//---------------------------------------------------------------------------
// finestra: Programs manager
//---------------------------------------------------------------------------
class ProgramsManagerUI : public CWindowSelect
{
public:
	ProgramsManagerUI( CWindow* parent );
	~ProgramsManagerUI();

	typedef enum
	{
		PRG_MODDATE,
		PRG_NOTE1,
		PRG_NOTE2
	} combo_labels;

protected:
	virtual void onInitSomething();
	virtual void onShowSomething();
	virtual void onEnter();
	virtual void onShowMenu();
	virtual bool onHotKey( int key );

private:
	GUI_SubMenu* SM_SaveOn;     // sub menu salva in ...
	GUI_SubMenu* SM_LoadFrom;   // sub menu carica da ...
	GUI_SubMenu* SM_ImportFrom; // sub menu importa da ...
	GUI_SubMenu* SM_ExportTo;   // sub menu esporta in ...

	std::map<int,C_Combo*> combos;
	CComboList* comboList;

	std::vector<std::string> nameList;

	int newProgram();
	int duplicateProgram();
	int renameProgram();
	int deleteProgram();
	int toSharedFolder();
	int fromSharedFolder();
	int toUSBDevice();
	int fromUSBDevice();
	int insertProgramNotes();
	// asciiQ
	int asciiQFromSharedFolder();
	int asciiQToSharedFolder();
	int asciiQFromUSBDevice();
	int asciiQToUSBDevice();

	void loadPrograms();
	int onSelectionChange( unsigned int row, unsigned int col );
};

int fn_ProgramsManager();



//---------------------------------------------------------------------------
// finestra: Feeder configuration manager
//---------------------------------------------------------------------------
class FeederConfigManagerUI : public CWindowSelect
{
public:
	FeederConfigManagerUI( CWindow* parent );
	~FeederConfigManagerUI();

protected:
	virtual void onShowSomething();
	virtual void onEnter();
	virtual void onShowMenu();
	virtual bool onHotKey( int key );

private:
	GUI_SubMenu* SM_SaveOn; // sub menu salva in ...
	GUI_SubMenu* SM_LoadFrom; // sub menu carica da ...
	GUI_SubMenu* SM_ExportTo;

	std::vector<std::string> nameList;

	int newFeederConfig();
	int duplicateFeederConfig();
	int renameFeederConfig();
	int deleteFeederConfig();
	int toSharedFolder();
	int fromSharedFolder();
	int toUSBDevice();
	int fromUSBDevice();
	int importFromCustomer();
	int csvToSharedFolder();
	int csvToUSBDevice();

	void loadFeederConfigs();
};

int fn_FeederConfigManager();



//---------------------------------------------------------------------------
// finestra: Packages library manager
//---------------------------------------------------------------------------
class PackagesLibManagerUI : public CWindowSelect
{
public:
	PackagesLibManagerUI( CWindow* parent );
	~PackagesLibManagerUI();

protected:
	virtual void onShowSomething();
	virtual void onEnter();
	virtual void onShowMenu();
	virtual bool onHotKey( int key );

private:
	GUI_SubMenu* SM_SaveOn; // sub menu salva in ...
	GUI_SubMenu* SM_LoadFrom; // sub menu carica da ...

	std::vector<std::string> nameList;

	int newPackagesLib();
	int duplicatePackagesLib();
	int renamePackagesLib();
	int deletePackagesLib();
	int toSharedFolder();
	int fromSharedFolder();
	int toUSBDevice();
	int fromUSBDevice();

	void loadPackagesLibs();
};

int fn_PackagesLibManager();



//---------------------------------------------------------------------------
// finestra: Quick load programs list
//---------------------------------------------------------------------------
int fn_QuickLoadPrograms();
int fn_LoadProgramFromQuickList( unsigned int row );

#endif
