#ifndef _ENGINE_DEBUG_MENU_
#define _ENGINE_DEBUG_MENU_
#pragma once

#include "../core/Colour.h"
#include "../core/Vector.h"
#include "../core/LinkedList.h"

#include "Singleton.h"
#include "Gui.h"

//\brief The Debug Menu handles all in-game editing functionality. The current version will
//		 create and configure gui elements, game objects and control engine settings.

class DebugMenu : public Singleton<DebugMenu>
{
public:

	DebugMenu() 
		: m_numMenuItems(0)
	{
		Startup();
	}

	//\brief Startup registers input callbacks for mouse clicks
	void Startup();

	//\brief Update will draw all debug menu items
	//\param a_dt float of time since the last frame in seconds
	void Update(float a_dt);

	//\brief Callback handler for when a debug menu item is clicked
	//\param A pointer to the widget that was interacted with
	void OnMenuItemMouseUp(/*Gui::Widget * a_widget*/);

	//\brief Callback handler for when a debug menu item is rolled over
	//\param A pointer to the widget that was interacted with
	void OnMenuItemMouseOver(/*Gui::Widget * a_widget*/);

private:

	//\brief A menu item is a button that can be hovered over to display child buttons
	struct MenuItem
	{
		MenuItem()					// Ctor nulls out linked list pointers to children
			: m_firstChild(NULL)
			, m_next(NULL)
		{}

		Gui::Widget m_widget;		// The widget itself for drawing and interaction
		Gui::Widget * m_firstChild;	// The first child gui element
		Gui::Widget * m_next;		// The next sibling in the linked list of children
	};

	unsigned int m_numMenuItems;	// How many items in total the debug menu displays

	MenuItem m_creationMenu;		// There are four root debug menu items: creating gameobjects
	MenuItem m_settingsMenu;		// Engine settings such as rendering and sound
	MenuItem m_game;				// And a user defined game debug meny configured through script
};

#endif //_ENGINE_DEBUG_MENU_