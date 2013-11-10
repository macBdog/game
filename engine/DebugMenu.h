#ifndef _ENGINE_DEBUG_MENU_
#define _ENGINE_DEBUG_MENU_
#pragma once

#include "../core/BitSet.h"
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
	~DebugMenu() { Shutdown(); }

	//\brief Startup registers input callbacks for mouse clicks
	bool Startup();
	void Shutdown() {};

	//\brief Update will draw all debug menu items
	//\param a_dt float of time since the last frame in seconds
	void Update(float a_dt);

	//\brief Perform any tasks after the scene is drawn, in this case, clear script debug widgets
	void PostRender();

	//\brief Write any changed resources to disk
	//\return bool true if anything was written, will not write if dirty
	bool SaveChanges();

	//\brief Callback handler for when a debug menu item is clicked
	//\param A pointer to the widget that was interacted with
	bool OnMenuItemMouseUp(Widget * a_widget);

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
	bool OnAlphaKeyDown(bool a_unused);
	bool OnAlphaKeyUp(bool a_unused);

	//\brief Debug menu enabled means are we in edit mode, has TAB been toggled
	bool IsDebugMenuEnabled() const { return m_enabled; }

	//\brief Debug menu active means there is a menu or dialog of the debug menu visible
	bool IsDebugMenuActive() const 
	{ 
		return	(	m_btnCreateRoot			!= NULL && 
					m_btnChangeGUIRoot		!= NULL && 
					m_btnChangeObjectRoot	!= NULL &&
					m_resourceSelect		!= NULL && 
					m_textInput				!= NULL) 
					&& 
				(	m_btnCreateRoot->IsActive()			|| 
					m_btnChangeGUIRoot->IsActive()		|| 
					m_btnChangeObjectRoot->IsActive()	||
					m_resourceSelect->IsActive()		||
					m_textInput->IsActive()); }

	//\brief Show the resource selection dialog to enable a file to be chose
	//\param a_startingPath A pointer to a c string with files that should listed in the dialog
	//\param a_fileExtensionFilter A pointer to a c string which will limit the files displayed
	void ShowResourceSelect(const char * a_startingPath, const char * a_fileExtensionFilter = NULL);

	//\brief Show the text input dialog to enable something to be named
	//\param a_startingText An optional pointer to a c string with the text that should be displayed
	void ShowTextInput(const char * a_startingText = NULL);

	//\brief Show a debug menu widget for a frame, called from script
	bool ShowScriptDebugText(const char * a_text, float a_posX = -0.5f, float a_posY = 0.5f);

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
		eEditModeText,			///< Cursor keys bound to text value
		eEditModeModel,			///< Setting the model for an object
		eEditModeTemplate,		///< Create an object from a template
		eEditModeSaveTemplate,	///< Set the template name for an object

		eEditModeCount,
	};

	//\brief Dirty flags are stored in a bitset to keep track of changes that need writing to disk
	enum eDirtyFlag
	{
		eDirtyFlagGUI = 0,		///< GUI files need writing
		eDirtyFlagScene,		///< Scene needs writing

		eDirtyFlagCount,
	};

	//\brief Helper function to handle widget visibility and position as a result of actions
	//\param a_widget is a pointer to the widget that was activated on
	//\return a bool indicating that the action was handled correctly
	bool HandleMenuAction(Widget * a_widget);

	static const float sc_cursorSize;				// Size of debug mouse cursor
	static Vector2 sc_vectorCursor[4];				// Debug menu does not have textures so it draws mouse cursors by vectors
	static const int sc_numScriptDebugWidgets = 8;	// Number of debug widgets the script system owns
	Widget * m_scriptDebugWidgets[sc_numScriptDebugWidgets];

	//\brief All debug menu visuals are drawn here, called from Update()
	void Draw();

	//\brief Helper function to create a debug menu button in one line
	Widget * CreateButton(const char * a_name, Colour a_colour, Widget * a_parent);

	//\brief Helper function to close debug menus
	inline void ShowCreateMenu(bool a_show);
	inline void ShowChangeGUIMenu(bool a_show);
	inline void ShowChangeObjectMenu(bool a_show);

	void HideResoureSelect();
	void HideTextInput();

	bool m_enabled;									///< Is the menu being shown
	bool m_handledCommand;							///< In the case that we are responding both to a global and a gui command
	BitSet m_dirtyFlags;							///< Bitset of types of resources that need writing
	Vector2 m_lastMousePosRelative;					///< Cache off the last mouse pos to diff between frames
	eEditType m_editType;							///< What type of object we are editing 
	eEditMode m_editMode;							///< If we are in a modal editing mode, which mode are we in
	Widget * m_widgetToEdit;						///< If we have selected a widget to edit, this will be set
	GameObject * m_gameObjectToEdit;				///< If we have selected a game object to edit, this will be set

	//\ingroup Debug menu buttons organised in hierachy using tabs

	Widget * m_btnCreateRoot;						///< Pointer to a widget that we create on startup with all other buttons as children
		Widget * m_btnCreateWidget;
		Widget * m_btnCreateGameObject;
			Widget * m_btnCreateGameObjectFromTemplate;
			Widget * m_btnCreateGameObjectNew;

	Widget * m_btnChangeGUIRoot;
		Widget * m_btnChangeGUIPos;
		Widget * m_btnChangeGUIShape;
		Widget * m_btnChangeGUIName;
		Widget * m_btnChangeGUIText;
		Widget * m_btnChangeGUITexture;
		Widget * m_btnDeleteGUI;

	Widget * m_btnChangeObjectRoot;
		Widget * m_btnChangeObjectModel;
		Widget * m_btnChangeObjectName;
		Widget * m_btnSaveObjectTemplate;
		Widget * m_btnDeleteObject;

	Widget * m_resourceSelect;
		Widget * m_resourceSelectList;
		Widget * m_btnResourceSelectOk;
		Widget * m_btnResourceSelectCancel;

	Widget * m_textInput;
		Widget * m_textInputField;
		Widget * m_btnTextInputOk;
		Widget * m_btnTextInputCancel;
};

#endif //_ENGINE_DEBUG_MENU_