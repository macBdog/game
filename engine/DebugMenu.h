#ifndef _ENGINE_DEBUG_MENU_
#define _ENGINE_DEBUG_MENU_
#pragma once

#include "../core/Colour.h"
#include "../core/Vector.h"
#include "../core/LinkedList.h"

#include "Singleton.h"
#include "Gui.h"

class Widget;

//\brief The Debug Menu handles all in-game editing functionality. The current version will
//		 create and configure gui elements, game objects and control engine settings.
class DebugMenu : public Singleton<DebugMenu>
{
public:

	//\brief Ctor calls startup
	DebugMenu();

	//\brief Startup registers input callbacks for mouse clicks
	bool Startup();

	//\brief Update will draw all debug menu items
	//\param a_dt float of time since the last frame in seconds
	void Update(float a_dt);

	//\brief Callback handler for when a debug menu item is clicked
	//\param A pointer to the widget that was interacted with
	bool OnMenuItemMouseUp(Widget * a_widget);

	//\brief Callback handler for when a debug menu item is rolled over
	//\param A pointer to the widget that was interacted with
	bool OnMenuItemMouseOver(Widget * a_widget);

	//\param Listener function to activate the debug menu
	//\param a_active if the menu should be shown or hidden
	bool OnActivate(bool a_active);

private:

	Widget * m_debugMenuRoot;		// Pointer to a widget that we create on startup
	Widget * m_debugMenuCancel;
	Widget * m_debugMenuCreateWidget;
	Widget * m_debugMenuCreateGameObject;
};

#endif //_ENGINE_DEBUG_MENU_