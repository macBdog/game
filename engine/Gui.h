#ifndef _ENGINE_GUI_
#define _ENGINE_GUI_
#pragma once

#include "GameFile.h"
#include "Singleton.h"
#include "Widget.h"

//\brief Gui handles creation and drawing of 2D interactive elements to form
//		 a menu system for the game. It's rudimentary compared to some examples
//		 out there but it's still possible to create a very charming UI easily
//		 and quickly without leaving the game interface.

namespace Gui
{
	class GuiManager : public Singleton<GuiManager>
	{
	public:

		//\brief Load up resources from the gui config file
		bool Startup(const char * a_guiPath);
		bool Shutdown();

		//\brief Draw all visible widgets
		//\param a_dt is the time since the last frame was drawn
		bool Update(float a_dt);

		//\brief TODO stubbed out but will load widgets from some file
		bool CreateWidget(Widget::eWidgetType a_type);
		bool LoadWidgets(GameFile *a_inputFile);
		bool SaveWidgets(GameFile *a_outputFile);

	private:

		GameFile m_configFile;	// Base gui config file
		Widget m_screen;		// The parent of all widget that are created
		Widget m_cursor;		// A special widget for the mouse position
	};
}

#endif // _ENGINE_GUI_
