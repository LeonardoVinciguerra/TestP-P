//---------------------------------------------------------------------------
//
// Name:        c_win_par.h
// Author:      Gabriel Ferri
// Created:     01/10/2011
// Description: CWindowParams class definition
//
//---------------------------------------------------------------------------

#ifndef __C_WIN_PAR_H
#define __C_WIN_PAR_H

#include <string>
#include <map>
#include "c_window.h"
#include "c_combolist.h"
#include "gui_submenu.h"


class CMenuWindowParams;

class CWindowParams : public CWindow
{
friend class CMenuWindowParams;

public:
	CWindowParams( CWindow* parent );
	~CWindowParams();

	void Show( bool select = true, bool focused = true );
	void Hide();
	void Select();
	void SetFocus();

	void SelectFirstCell();
	void DeselectCells();

protected:
	CMenuWindowParams* m_menu;

	std::map<int,C_Combo*> m_combos;
	CComboList* m_comboList;

	virtual void onInit() {}
	virtual void onShow() {}
	virtual void onRefresh() {}
	virtual void onEdit() {}
	virtual void onClose() {}
	virtual void onShowMenu() {}
	virtual bool onKeyPress( int key ) { return false; }
	virtual void onIdle() {}

	void forceExit() { m_forceExit = true; }

private:
	void ShowMenu();
	void WorkingCycle();

	bool m_forceExit;
};


class CMenuWindowParams : public GUI_SubMenu
{
public:
	CMenuWindowParams( CWindowParams* parent ) { m_parent = parent; }

protected:
	CWindowParams* m_parent;

	bool onSelect();
};


#endif
