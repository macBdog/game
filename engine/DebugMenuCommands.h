#ifndef _ENGINE_DEBUG_MENU_COMMANDS_
#define _ENGINE_DEBUG_MENU_COMMANDS_
#pragma once

class Widget;

//\brief Debug menu commands are functions that the debug menu executes from a button press
class DebugMenuCommand
{
public:

	//\brief Ctor calls startup
	DebugMenuCommand(const char * a_name, Widget * a_parentMenu, Widget * a_alignedTo) { }
	~DebugMenuCommand() { }

	void Execute();

private:

	Widget * m_widget;
};

#endif //_ENGINE_DEBUG_MENU_COMMANDS_