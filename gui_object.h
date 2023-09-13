//---------------------------------------------------------------------------
// Name:        gui_object.h
// Author:      Gabriel Ferri
// Created:     15/11/2011
// Description: GUIObject class definition
//---------------------------------------------------------------------------

#ifndef __GUI_OBJECT_H
#define __GUI_OBJECT_H

#include <vector>
#include "mathlib.h"


class GUIObject
{
public:
	GUIObject() {}
	~GUIObject();

	void SetClientArea( const RectI& rect ) { client_area = rect; }
	void SetGraphArea( const RectI& rect ) { graph_area = rect; }

	unsigned int SaveScreen();
	bool RestoreScreen( unsigned int index );

protected:
	RectI client_area;
	RectI graph_area;
	std::vector<void*> buffers;
};

#endif
