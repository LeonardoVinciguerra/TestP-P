//---------------------------------------------------------------------------
// Name:        c_win_select.h
// Author:      Gabriel Ferri
// Created:     18/11/2011
// Description: CWindowSelect class definition
//---------------------------------------------------------------------------

#ifndef __C_WIN_SELECT_H
#define __C_WIN_SELECT_H

#include <list>
#include <string>
#include "c_win_table.h"
#include "c_pan.h"


class CWindowSelect : public CWindowTable
{
public:
	CWindowSelect( CWindow* parent, int tx, int ty, unsigned int rows, unsigned int width );
	~CWindowSelect();

	void AddItem( const char* item );
	void ClearItems();
	void Sort();
	void ShowItems();
	std::string GetSelectedItem() { return m_table->GetText( m_table->GetCurRow(), m_table->GetCurCol() ); }
	int GetSelectedItemIndex();

	int GetExitCode() { return m_exitCode; }

protected:
	virtual void onInitSomething() {}
	virtual void onShowSomething() {}
	virtual void onEnter() {}
	virtual void onShowMenu() {}
	virtual bool onHotKey( int key ) { return false; }

	class CPan* tips;

private:
	virtual void onInit();
	virtual void onShow();
	virtual void onRefresh() {}
	virtual void onEdit() {}
	virtual void onClose();
	virtual bool onKeyPress( int key );
	virtual void onIdle() {}

	int m_exitCode;
	int m_x, m_y;
	unsigned int m_numRows;
	unsigned int m_width;

	std::list<std::string> m_items;
	unsigned int m_start_item;

	std::string m_search;

	void showItems( int start_item );
	bool vSelect( int key );
	void searchItem();
	void showSearch( bool error = false );

	bool compareNoCase( std::string str1, std::string str2 );
};

#endif
