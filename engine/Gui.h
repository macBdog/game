#ifndef _ENGINE_GUI_
#define _ENGINE_GUI_
#pragma once

#include "GameFile.h"
#include "Singleton.h"
#include "Widget.h"

//\brief Gui handles creation and drawing of 2D interactive elements to form
//		 a menu system for the game. It's rudimentary compared to some examples
//		 out there but it's still possible to create a very charming UI easily
//		 and quickly without leaving the game interface.
class Gui : public Singleton<Gui>
{
public:

	// Destructor cleans up all allocations
	~Gui() { Shutdown(); }

	//\brief Load up resources from the gui config file
	bool Startup(const char * a_guiPath);
	bool Shutdown();

	//\brief Draw all visible widgets
	//\param a_dt is the time since the last frame was drawn
	bool Update(float a_dt);

	//\ingroup Widget lifecycle functions
	//\brief Allocate memory for a widget and return a pointer to it
	//return A pointer to the newly created widget if succesfull otherwise NULL
	Widget * CreateWidget(const Widget::WidgetDef & a_def, Widget * a_parent, bool a_startActive = true);
	void DestroyWidget(Widget * a_widget);
	bool LoadWidgets(GameFile *a_inputFile);
	bool SaveWidgets(GameFile *a_outputFile);

	//brief Called when an input device is used
	bool MouseInputHandler(bool active = false);
	bool KeyInputHandler();

	//\brief Utility function to get the base container for all objects
	inline Widget * GetScreenWidget() { return &m_screen; }

private:

	//\brief Helper function to update selection status of all widgets based on mouse pos
	void UpdateSelection();

	GameFile m_configFile;	// Base gui config file
	Widget m_screen;		// The parent of all widget that are created
	Widget m_cursor;		// A special widget for the mouse position
};


#endif // _ENGINE_GUI_
