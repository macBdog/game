#ifndef _ENGINE_GUI_
#define _ENGINE_GUI_
#pragma once

#include "../core/LinkedList.h"

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

	// Constructor just for initialisation list
	Gui() : m_activeMenu(NULL) {}

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

	//\brief Utility function to get the base containers
	inline Widget * GetDebugRoot() { return &m_debugRoot; }
	inline Widget * GetActiveMenu() { return m_activeMenu; }

	//brief Return the top widget with different selection flags
	Widget * GetActiveWidget();

private:

	//\brief Shortcut to access lists of menu items
	typedef LinkedListNode<Widget> MenuListNode;
	typedef LinkedList<Widget> MenuList; 

	//\brief Load and unload gui menus and child widgets from permanent storage
	//\param a_menuFile is a pointer to the c string of the path of the menu file to load
	//\return bool true if the load or load operation completed without failure
	bool LoadMenu(const char * a_menuFile);
	bool LoadMenus(const char * a_guiPath);
	bool UnloadMenus();

	//\brief Helper function to update selection status of all widgets based on mouse pos
	void UpdateSelection();

	MenuList m_menus;		// All menus loaded from data or created on the fly
	GameFile m_configFile;	// Base gui config file
	Widget m_debugRoot;		// All debug menu elements are children of this
	Widget m_cursor;		// A special widget for the mouse position
	Widget * m_activeMenu;	// The current menu that's active of editing and display
};


#endif // _ENGINE_GUI_
