//---------------------------------------------------------------------------
// Name:        c_win_table.h
// Author:      Gabriel Ferri
// Created:     18/11/2011
// Description: CWindowTable class definition
//---------------------------------------------------------------------------

#ifndef __C_WIN_TABLE_H
#define __C_WIN_TABLE_H

#include <string>
#include "c_window.h"
#include "c_table.h"
#include "gui_submenu.h"


class CMenuWindowTable;

class CWindowTable : public CWindow
{
friend class CMenuWindowTable;

public:
	CWindowTable( CWindow* parent );
	~CWindowTable();

	void Show( bool select = true, bool focused = true );
	void Hide();
	void Select();
	void SetFocus();

protected:
	CMenuWindowTable* m_menu;

	CTable* m_table;

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


class CMenuWindowTable : public GUI_SubMenu
{
public:
	CMenuWindowTable( CWindowTable* parent ) { m_parent = parent; }

protected:
	CWindowTable* m_parent;

	bool onSelect();
};


#endif
