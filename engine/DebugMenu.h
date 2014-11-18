#ifndef _ENGINE_DEBUG_MENU_
#define _ENGINE_DEBUG_MENU_
#pragma once

#include "../core/BitSet.h"
#include "../core/Colour.h"
#include "../core/Vector.h"
#include "../core/LinkedList.h"

#include "DebugMenuCommands.h"
#include "GameObject.h"
#include "Gui.h"
#include "Singleton.h"

struct Light;
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
	inline void Shutdown() { m_commands.Shutdown(); };

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

	//\brief Listener function to reload scripts in designer and debug builds
	bool OnReload(bool a_active);

	//\brief Listener functions to manually manipulate game dt
	bool OnTimePause(bool a_active);

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
	inline bool IsDebugMenuEnabled() const { return m_enabled; }

	//\brief Debug menu active means there is a menu or dialog of the debug menu visible
	inline bool IsDebugMenuActive() const 
	{ 
		return	m_commands.IsActive() ||
				(m_resourceSelect != NULL && m_resourceSelect->IsActive()) ||
				(m_textInput != NULL && m_textInput->IsActive());
	}

	//\brief If sim dt and game dt should advance
	inline bool IsTimePaused() const { return m_timePaused; }

	//\brief Show the resource selection dialog to enable a file to be chose
	//\param a_startingPath A pointer to a c string with files that should listed in the dialog
	//\param a_fileExtensionFilter A pointer to a c string which will limit the files displayed
	void ShowResourceSelect(const char * a_startingPath, const char * a_fileExtensionFilter = NULL);

	//\brief Show the resource selection dialog to enable a font to be chosen
	void ShowFontSelect();

	//\brief Show the text input dialog to enable something to be named
	//\param a_startingText An optional pointer to a c string with the text that should be displayed
	void ShowTextInput(const char * a_startingText = NULL);

	//\brief Show a debug menu widget for a frame, called from script
	bool ShowScriptDebugText(const char * a_text, float a_posX = -0.5f, float a_posY = 0.5f);

	//\brief Game time scale modification and access
	inline float GetGameTimeScale() { return m_gameTimeScale; }
	inline void ResetGameTimeScale() { m_gameTimeScale = 1.0f; }

private:

	static const float sc_gameTimeScaleFast;		// Value of game time scale in fast mode
	static const float sc_gameTimeScaleSlow;		// Value of game time scale in slow mode
	static const float sc_cursorSize;				// Size of debug mouse cursor
	static Vector2 sc_vectorCursor[4];				// Debug menu does not have textures so it draws mouse cursors by vectors
	static const int sc_numScriptDebugWidgets = 8;	// Number of debug widgets the script system owns
	Widget * m_scriptDebugWidgets[sc_numScriptDebugWidgets];

	//\brief All debug menu visuals are drawn here, called from Update()
	void Draw();

	//\brief Helper function to create a debug menu button in one line
	Widget * CreateButton(const char * a_name, Colour a_colour, Widget * a_parent);

	//\brief Helper function to close debug menus
	void HideResoureSelect();
	void HideTextInput();

	bool m_enabled;									///< Is the menu being shown
	bool m_timePaused;								///< Is the game dt running forward
	float m_gameTimeScale;							///< How fast the game is running, 1.0 means real time
	BitSet m_dirtyFlags;							///< Bitset of types of resources that need writing
	Vector2 m_lastMousePosRelative;					///< Cache off the last mouse pos to diff between frames
	EditType::Enum m_editType;						///< What type of object we are editing 
	EditMode::Enum m_editMode;						///< If we are in a modal editing mode, which mode are we in
	Widget * m_widgetToEdit;						///< If we have selected a widget to edit, this will be set
	GameObject * m_gameObjectToEdit;				///< If we have selected a game object to edit, this will be set
	Light * m_lightToEdit;							///< If a light has been selected to edit, this will be set

	DebugMenuCommandRegistry m_commands;			///< List of commands that can be performed by the debug menu

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