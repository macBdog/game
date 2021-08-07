#ifndef _ENGINE_GUI_
#define _ENGINE_GUI_
#pragma once

#include "../core/LinkedList.h"

#include "GameFile.h"
#include "Singleton.h"
#include "Widget.h"

class DataPack;

//\brief Gui handles creation and drawing of 2D interactive elements to form
//		 a menu system for the game. It's rudimentary compared to some examples
//		 out there but it's still possible to create a very charming UI easily
//		 and quickly without leaving the game interface.
class Gui : public Singleton<Gui>
{
public:

	// Constructor just for initialisation list
	Gui() 
		: m_activeMenu(nullptr)
		, m_startupMenu(nullptr)
	{ m_guiPath[0] = '\0'; }

	// Destructor cleans up all allocations
	~Gui() { Shutdown(); }

	//\brief Load up resources from the gui config file
	bool Startup(const char * a_guiPath, const DataPack * a_dataPack);
	bool Shutdown();

	//\brief Draw all visible widgets
	//\param a_dt is the time since the last frame was drawn
	bool Update(float a_dt);

	//\brief Called when the script is reloaded
	void ReloadMenus();

	//\ingroup Widget lifecycle functions
	//\brief Allocate memory for a widget and return a pointer to it
	//return A pointer to the newly created widget if succesfull otherwise nullptr
	Widget * CreateWidget(const Widget::WidgetDef & a_def, Widget * a_parent, bool a_startActive = true);
	Widget * CreateWidget(GameFile::Object * a_widgetFile, Widget * a_parent, bool a_startActive = true);
	Widget * FindWidget(const char * a_widgetName);
	Widget * FindSelectedWidget();
	void DestroyWidget(Widget * a_widget);
	bool LoadWidgets(GameFile *a_inputFile);
	bool SaveWidgets(GameFile *a_outputFile);

	//brief Called when an input device is used
	bool MouseInputHandler(bool active = false);
	bool KeyInputHandler();

	//\brief Utility function to get the base containers
	inline Widget * GetDebugRoot() { return &m_debugRoot; }
	inline Widget * GetActiveMenu() { return m_activeMenu; }
	inline Widget * GetStartupMenu() { return m_startupMenu; }
	inline void SetActiveMenu(Widget * a_newMenu) { m_activeMenu = a_newMenu; }

	//\brief Mouse display toggle
	inline void EnableMouseCursor() { m_cursor.SetActive(true); }
	inline void DisableMouseCursor() { m_cursor.SetActive(false); }

	//brief Return the top widget with different selection flags
	Widget * GetActiveWidget();

	//brief Get the visible status of the global blinking cursor
	static bool GetTextCursorBlink() { return s_cursorBlink; }

private:

	//\brief Shortcut to access lists of menu items
	typedef LinkedListNode<Widget> MenuListNode;
	typedef LinkedList<Widget> MenuList; 

	//\brief Load and unload gui menus and child widgets from permanent storage
	//\param a_menuFile is a pointer to the c string of the path of the menu file to load
	//\return bool true if the load or load operation completed without failure
	bool LoadMenu(const GameFile & a_menuFile, const DataPack * a_dataPack);
	bool LoadMenus(const char * a_guiPath, const DataPack * a_dataPack);
	bool UnloadMenus();

	//\brief Helper function to update selection status of all widgets based on mouse pos
	void UpdateSelection();

	// Cursor blink is global across the application
	static bool s_cursorBlink;
	static float s_cursorBlinkTimer;
	static const float s_cursorBlinkTime;
	static float s_widgetPulseTimer;
	
	char m_guiPath[StringUtils::s_maxCharsPerLine];
	MenuList m_menus{};					///< All menus loaded from data or created on the fly
	GameFile m_configFile{};			///< Base gui config file
	Widget m_debugRoot{};				///< All debug menu elements are children of this
	Widget m_cursor{};					///< A special widget for the mouse position
	Widget* m_activeMenu{ nullptr };	///< The current menu that's active of editing and display
	Widget * m_startupMenu{ nullptr };	///< The widget that has been marked as the menu that should be loaded when the game starts
};


#endif // _ENGINE_GUI_
