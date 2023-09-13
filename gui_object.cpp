//---------------------------------------------------------------------------
// Name:        gui_object.cpp
// Author:      Gabriel Ferri
// Created:     15/11/2011
// Description: GUIObject class implementation
//---------------------------------------------------------------------------
#include "gui_object.h"

#include "gui_functions.h"


GUIObject::~GUIObject()
{
	for( unsigned int i = 0; i < buffers.size(); i++ )
	{
		if( buffers[i] )
		{
			// free buffer
			GUI_FreeSurface( &buffers[i] );
		}
	}
	buffers.clear();
}

//--------------------------------------------------------------------------
// Salva porzione di schermo
//--------------------------------------------------------------------------
unsigned int GUIObject::SaveScreen()
{
	void* buffer = GUI_SaveScreen( graph_area );

	buffers.push_back( buffer );
	return buffers.size() - 1;
}

bool GUIObject::RestoreScreen( unsigned int index )
{
	if( index < 0 || index >= buffers.size() )
		return false;

	if( buffers[index] == 0 )
		return false;

	// restore screen
	GUI_DrawSurface( PointI( graph_area.X, graph_area.Y ), buffers[index] );
	// free buffer
	GUI_FreeSurface( &buffers[index] );

	buffers[index] = 0;

	return true;
}
