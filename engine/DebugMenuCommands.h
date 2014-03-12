#ifndef _ENGINE_DEBUG_MENU_COMMANDS_
#define _ENGINE_DEBUG_MENU_COMMANDS_
#pragma once

#include "../core/Colour.h"
#include "../core/Delegate.h"

#include "StringUtils.h"

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

//\brief Dirty flags are stored in a bitset to keep track of changes that need writing to disk
namespace DirtyFlag
{
	enum Enum
	{
		None = -1,		///< Nothing needs writing
		GUI,			///< GUI files need writing
		Scene,			///< Scene needs writing
		Count,
	};
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

//\brief When editing we can change properties of GUI widgets and also game objects
namespace EditType
{
	enum Enum
	{
		None = -1,		///< Not changing anything
		Widget,			///< Changing a gui widget
		GameObject,		///< Changing a game object
		Count,
	};
}

//\brief What type of editing mode is being performed 
namespace EditMode
{
	enum Enum
	{
		None = -1,
		Pos,			///< Widget top left stuck to mouse pos
		Shape,			///< Widget bottom right stuck to mouse pos
		Texture,		///< File selection dialog active
		Name,			///< Cursor keys bound to display name
		Text,			///< Cursor keys bound to text value
		Model,			///< Setting the model for an object
		Template,		///< Create an object from a template
		SaveTemplate,	///< Set the template name for an object

		eEditModeCount,
	};
}

//\brief Contains some data that a command can return, passed around by value so remember to keep it small
struct DebugCommandReturnData
{
	DebugCommandReturnData() 
		: m_editMode(EditMode::None)
		, m_editType(EditType::None)
		, m_dirtyFlag(DirtyFlag::None)
		, m_textEditString(NULL)
		, m_resourceSelectPath(NULL)
		, m_resourceSelectExtension(NULL)
		, m_success(false)
		{ }
	
	char * m_textEditString;
	char * m_resourceSelectPath;
	char * m_resourceSelectExtension;
	DirtyFlag::Enum m_dirtyFlag;	///< If the command touched a widget or game object it should set a flag
	EditMode::Enum m_editMode;		///< If the command set a new mode for the debug menu
	EditType::Enum m_editType;		///< If the command set a new type of editing for the debug menu
	bool m_success;					///< Was the command successful

};

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
	inline DebugCommandReturnData Execute(Widget * a_selectedWidget, GameObject * a_selectedGameObject)
	{
		if (m_widgetFunction.IsSet())		{ return m_widgetFunction.Execute(a_selectedWidget); }
		if (m_gameObjectFunction.IsSet())	{ return m_gameObjectFunction.Execute(a_selectedGameObject); }
		return DebugCommandReturnData();
	}

	//\ingroup Event handling
	bool OnMouseUp();
	bool OnKeyDown();

	//\brief Helper function for creating a widget in a predifined style for debug menu commands
	static Widget * CreateButton(const char * a_name, Widget * a_parent, Colour a_colour);

private:

	Delegate<DebugCommandReturnData, GameObject *> m_gameObjectFunction;		///< Function if the command is registered on a game object
	Delegate<DebugCommandReturnData, Widget *> m_widgetFunction;				///< Function if the command is registered on a widget
	DebugMenuCommandAlign::Enum m_alignment;									///< How the command is aligned to it's parent command NOTE: this is separate to widget alignment
	Widget * m_widget;															///< Visual representation of the command button
};

class DebugMenuCommandRegistry
{
public:

	DebugMenuCommandRegistry()
		: m_btnCreateRoot(NULL)
		, m_btnWidgetRoot(NULL)
		, m_btnGameObjectRoot(NULL)	
	{ 
		m_textEditString[0] = '\0';
		m_resourceSelectPath[0] = '\0';
		m_resourceSelectExtension[0] = '\0';
	}
	
	// Linked list of commands that the debug menu can do
	typedef LinkedListNode<DebugMenuCommand> CommandNode;
	typedef LinkedList<DebugMenuCommand> CommandList;

	//\brief Add debug menu commands to a list
	void Startup();
	void Shutdown();

	//\brief State accessors
	bool IsActive() const;

	//\brief Handlers for various things that can happen from the debug menu
	DebugCommandReturnData HandleLeftClick(Widget * a_selectedWidget, GameObject * a_selectedGameObject);
	DebugCommandReturnData HandleRightClick(Widget * a_selectedWidget, GameObject * a_selectedGameObject);

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
	DebugCommandReturnData CreateWidget(Widget * a_widget);
	DebugCommandReturnData ChangeWidgetAlignment(Widget * a_widget);
	DebugCommandReturnData ChangeWidgetOffset(Widget * a_widget);
	DebugCommandReturnData ChangeWidgetShape(Widget * a_widget);
	DebugCommandReturnData ChangeWidgetName(Widget * a_widget);
	DebugCommandReturnData ChangeWidgetText(Widget * a_widget);
	DebugCommandReturnData ChangeWidgetTexture(Widget * a_widget);
	DebugCommandReturnData DeleteWidget(Widget * a_widget);

	DebugCommandReturnData CreateGameObject(GameObject * a_gameObj);
	DebugCommandReturnData CreateGameObjectFromTemplate(GameObject * a_gameObj);
	DebugCommandReturnData ChangeGameObjectModel(GameObject * a_gameObj);
	DebugCommandReturnData ChangeGameObjectName(GameObject * a_gameObj);
	DebugCommandReturnData ChangeGameObjectClipType(GameObject * a_gameObj);
	DebugCommandReturnData ChangeGameObjectClipSize(GameObject * a_gameObj);
	DebugCommandReturnData ChangeGameObjectClipPosition(GameObject * a_gameObj);
	DebugCommandReturnData SaveGameObjectTemplate(GameObject * a_gameObj);
	DebugCommandReturnData DeleteGameObject(GameObject * a_gameObj);

	char m_textEditString[StringUtils::s_maxCharsPerName];
	char m_resourceSelectPath[StringUtils::s_maxCharsPerLine];
	char m_resourceSelectExtension[4];

	Widget * m_btnCreateRoot;						///< Pointer to a widget that is created on startup for the first sub menu
	Widget * m_btnWidgetRoot;						///< Pointer to a widget that is created on startup for functions related to widgets
	Widget * m_btnGameObjectRoot;					///< Pointer to a widget that is created on startup for functions related to game objects
	CommandList m_commands;							///< List of commands that can be executed
};

#endif //_ENGINE_DEBUG_MENU_COMMANDS_