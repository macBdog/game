#ifndef _ENGINE_DEBUG_MENU_COMMANDS_
#define _ENGINE_DEBUG_MENU_COMMANDS_
#pragma once

#include "../core/Colour.h"
#include "../core/Delegate.h"

class GameObject;
class Widget;
struct Alignment;
	
//\brief Space saving macros for command functions that hide or show different options
#define DEBUG_COMMANDS_LOOP_BEGIN									\
	CommandNode * curNode = m_commands.GetHead();					\
	while (curNode != NULL)											\
	{																\
		DebugMenuCommand * debugCommand = curNode->GetData();		\
		Widget * debugWidget = debugCommand->GetWidget();	

#define DEBUG_COMMANDS_LOOP_END										\
		curNode = curNode->GetNext();								\
	}

//\brief Shorthand names for alignment settings for command butons
namespace DebugMenuCommandAlign
{
	enum Enum
	{
		Right,	///< Top left anchor to top right of aligned to
		Below,	///< Top left anchor to btm left of aligned to
		Above,	///< Bottom left anchor to top left of aligned to 
		Left,	///< Top right anchor to top left of aligned to
	};
}

//\brief Debug menu commands are functions that the debug menu executes from a button press
class DebugMenuCommand
{
public:

	//\brief Commands have a name, widget and delegate function to call on click
	DebugMenuCommand(const char * a_name, Widget * a_parent, Colour a_colour);
	~DebugMenuCommand() { }

	//\brief Accessors
	inline Widget * GetWidget() const { return m_widget; }
	inline DebugMenuCommandAlign::Enum GetAlignment() const { return m_alignment; }

	//\brief Functions for setting the callbacks that debug commands provide
	template <typename TObj, typename TMethod>
	inline void SetWidgetFunction(TObj * a_callerObject, TMethod a_callback) { m_widgetFunction.SetCallback(a_callerObject, a_callback); }
	template <typename TObj, typename TMethod>
	inline void SetGameObjectFunction(TObj * a_callerObject, TMethod a_callback) { m_gameObjectFunction.SetCallback(a_callerObject, a_callback); }

	//\brief Set the visual hierachy of the command buttons
	void SetAlignment(Widget * a_alignedTo, DebugMenuCommandAlign::Enum a_alignment);
	void SetWidgetAlignment(DebugMenuCommandAlign::Enum a_alignment);
	inline bool Execute(Widget * a_selectedWidget, GameObject * a_selectedGameObject)
	{
		if (m_widgetFunction.IsSet())		{ return m_widgetFunction.Execute(a_selectedWidget); }
		if (m_gameObjectFunction.IsSet())	{ return m_gameObjectFunction.Execute(a_selectedGameObject); }
		return false;
	}

	//\ingroup Event handling
	bool OnMouseUp();
	bool OnKeyDown();

	//\brief Helper function for creating a widget in a predifined style for debug menu commands
	static Widget * CreateButton(const char * a_name, Widget * a_parent, Colour a_colour);

private:

	Delegate<bool, GameObject *> m_gameObjectFunction;		///< Function if the command is registered on a game object
	Delegate<bool, Widget *> m_widgetFunction;				///< Function if the command is registered on a widget
	DebugMenuCommandAlign::Enum m_alignment;				///< How the command is aligned to it's parent command NOTE: this is separate to widget alignment
	Widget * m_widget;										///< Visual representation of the command button
};

class DebugMenuCommandRegistry
{
public:

	DebugMenuCommandRegistry() : m_rootCommand(NULL) { }
	
	// Linked list of commands that the debug menu can do
	typedef LinkedListNode<DebugMenuCommand> CommandNode;
	typedef LinkedList<DebugMenuCommand> CommandList;

	//\brief Add debug menu commands to a list
	void Startup(Widget * a_parent);
	void Shutdown();

	//\brief State accessors
	bool IsActive() const;

	//\brief Handlers for various things that can happen from the debug menu
	bool HandleLeftClick(Widget * a_clickedWidget, Widget * a_selectedWidget, GameObject * a_selectedGameObject);
	bool HandleRightClick(Widget * a_clickedWidget, Widget * a_selectedWidget, GameObject * a_selectedGameObject);

	//\brief Menu button visibility functions
	void ShowRootCommands();
	void ShowWidgetCommands();
	void ShowGameObjectCommands();
	void HideRootCommands();
	void HideWidgetCommands();
	void HideGameObjectCommands();
	void Hide();
	
private:

	//\brief Helper function for creating and setting up a new debug menu widget
	DebugMenuCommand * Create(const char * a_name, Widget * a_parent, Widget * a_alignTo, DebugMenuCommandAlign::Enum a_align, Colour a_colour);

	//\brief Set the alignment of the widgets for each command according to rules for where the user clicked
	void SetMenuAlignment(Alignment * a_screenAlign);

	//\ brief Debug menu command functions
	bool CreateWidget(Widget * a_widget);

	Widget * m_rootCommand;
	CommandList m_commands;
};

#endif //_ENGINE_DEBUG_MENU_COMMANDS_