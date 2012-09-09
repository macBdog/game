#ifndef _ENGINE_DEBUG_MENU_
#define _ENGINE_DEBUG_MENU_
#pragma once

#include "../core/Colour.h"
#include "../core/Vector.h"
#include "../core/LinkedList.h"

#include "GameObject.h"
#include "Gui.h"
#include "Singleton.h"

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

	//\brief Listener function to show the debug menu
	//\param a_active if the menu should be shown or hidden
	bool OnActivate(bool a_active);

	//\brief Listener function to select a widget
	//\param a_active if the menu should be shown or hidden
	bool OnSelect(bool a_active);

	//\brief Listener function to enable debug menu functions
	//\param a_toggle if the debug menu should stay enabled
	bool OnEnable(bool a_toggle);

	//\brief Listener function for typing alpha keys
	//\param a_unused
	bool OnAlphaKey(bool a_unused);

	//\brief Debug menu enabled means are we in edit mode, has TAB been toggled
	bool IsDebugMenuEnabled() const { return m_enabled; }

	//\brief Debug menu active means there is a menu or dialog of the debug menu visible
	bool IsDebugMenuActive() const { return (m_btnCreateRoot != NULL && m_btnChangeRoot != NULL) && 
											(m_btnCreateRoot->IsActive() || m_btnChangeRoot->IsActive()); }

	//\brief Show the resource selection dialog to enable a file to be chose
	//\param a_startingPath A pointer to a c string with files that should listed in the dialog
	//\param a_fileExtensionFilter A pointer to a c string which will limit the files displayed
	void ShowResourceSelect(const char * a_startingPath, const char * a_fileExtensionFilter = NULL);

private:

	//\brief When editing we can change properties of GUI widgets and also game objects
	enum eEditType
	{
		eEditTypeNone = -1,		///< Not changing anything
		eEditTypeWidget,		///< Changing a gui widget
		eEditTypeGameObject,	///< Changing a game object
		
		eEditTypeCount,
	};

	//\brief What type of editing mode is being performed 
	enum eEditMode
	{
		eEditModeNone = -1,
		eEditModePos,			///< Widget top left stuck to mouse pos
		eEditModeShape,			///< Widget bottom right stuck to mouse pos
		eEditModeTexture,		///< File selection dialog active
		eEditModeName,			///< Cursor keys bound to display name

		eEditModeCount,
	};

	//\brief Helper function to handle widget visibility and position as a result of actions
	//\param a_widget is a pointer to the widget that was activated on
	//\return a bool indicating that the action was handled correctly
	bool HandleMenuAction(Widget * a_widget);

	static const float sc_cursorSize;				// Size of debug mouse cursor
	static Vector2 sc_vectorCursor[4];				// Debug menu does not have textures so it draws mouse cursors by vectors

	//\brief All debug menu visuals are drawn here, called from Update()
	void Draw();

	//\brief Helper function to create a debug menu button in one line
	Widget * CreateButton(const char * a_name, Colour a_colour, Widget * a_parent);

	//\brief Helper function to close debug menus
	inline void ShowCreateMenu(bool a_show);
	inline void ShowChangeMenu(bool a_show);
	inline void HideResoureMenu();

	bool m_enabled;									///< Is the menu being shown
	bool m_handledCommand;							///< In the case that we are responding both to a global and a gui command
	eEditType m_editType;							///< What type of object we are editing 
	eEditMode m_editMode;							///< If we are in a modal editing mode, which mode are we in
	Widget * m_widgetToEdit;						///< If we have selected a widget to edit, this will be set
	GameObject * m_gameObjectToEdit;				///< If we have selected a game object to edit, this will be set

	//\ingroup Debug menu buttons organised in hierachy using tabs

	Widget * m_btnCreateRoot;						///< Pointer to a widget that we create on startup with all other buttons as children
		Widget * m_btnCreateWidget;
		Widget * m_btnCreateGameObject2D;
		Widget * m_btnCreateGameObject3D;
			Widget * m_btnCreateGameObjectFromTemplate;
			Widget * m_btnCreateGameObjectNew;

	Widget * m_btnChangeRoot;
		Widget * m_btnChangePos;
		Widget * m_btnChangeShape;
		Widget * m_btnChangeName;
		Widget * m_btnChangeTexture;
		Widget * m_btnCancel;

	Widget * m_resourceSelect;
		Widget * m_resourceSelectList;
		Widget * m_btnResourceSelectOk;
		Widget * m_btnResourceSelectCancel;
};

#endif //_ENGINE_DEBUG_MENU_