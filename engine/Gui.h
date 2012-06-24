#ifndef _ENGINE_GUI_
#define _ENGINE_GUI_
#pragma once

#include "../core/Colour.h"
#include "../core/Vector.h"

#include "GameFile.h"
#include "Singleton.h"

//\brief Gui handles creation and drawing of 2D interactive elements to form
//		 a menu system for the game. It's rudimentary compared to some examples
//		 out there but it's still possible to create a very charming UI easily
//		 and quickly without leaving the game interface.

namespace Gui
{
	enum Alignment
	{
	  AlignNone = 0,	// Placed wherever it's position dictates
	  AlignLeft,
	  AlignRight,
	  AlignTop,
	  AlignBtm,

	  AlignCount
	};
 
	enum CoordType
	{
	  CoordTypeRelative = 0,	// -1.0 to 1.0 means the edge of the container 
	  CoordTypeAbsolute, 		// 1.0 means 1 pixel from the edge of the container

	  CoordTypeCount
	};

	//\brief Represents states of element selection
	enum Selection
	{
	  Unselected = 0,	// Drawn without tint
	  Rollover,			// Indicating possible selection
	  Selected,			// Showing element selection
	  EditRollover,		// Showing the element will be selected for editing
	  EditSelected,		// Indicating edits will affect this element

	  SelectionCount,
	};

	//\brief A widget vector is a 2D vector with the additional properties
	//       of different coordinate types to deal with widescreen resolutions
	//       and alignment types to create layouts that work in all screen sizes
	class WidgetVector : Vector2
	{
	public: 

	  void SetCoordType(CoordType a_type) { m_type = a_type; }

	  	// Operator overloads are not inherited by default
		using Vector2::operator=;
		using Vector2::operator*;
		using Vector2::operator+;
		using Vector2::operator-;

	private:

	  CoordType m_type;
	};

	//\brief A widget is the most basic 2D GUI element.
	class Widget
	{

	public:

	  //\brief Base implementation will tint for selection
	  virtual void Draw();

	  //\brief Basic property accessors should remain unchanged for all instances of this class
	  void SetTexture(Texture * a_tex) { m_texture = a_tex; }
	  void SetPos(Vector2 a_pixelPos, CoordType a_type = CoordTypeAbsolute) { m_pos = a_pixelPos; m_pos.SetCoordType(a_type); }
	  void SetSize(Vector2 a_relPos, CoordType a_type = CoordTypeAbsolute) { m_size = a_relPos, m_size.SetCoordType(a_type); }

	private:

	  WidgetVector m_size;		// How much of the parent container the element takes up
	  WidgetVector m_pos;		// Where in the parent container the element resides
	  Colour m_colour;			// What the base colour of the widget is
	  Selection m_selection;	// If the widget is being rolled over etc
	  Texture * m_texture;		// What to draw
	  Widget * m_nextWidget;	// Conitguous widgets are stored as a linked list
	  Widget * m_childWidget;	// And each widget can have multiple children
	};

	class GuiManager : public Singleton<GuiManager>
	{
	public:

		//\brief Each widget type has a distinct behavior 
		enum eWidgetType
		{
			eWidgetTypeCursor = 0,
			eWidgetTypeImage,
			eWidgetTypeButton,
			eWidgetTypeCheckbox,
			eWidgetTypeList,
			
			eWidgetTypeCount,
		};

		bool Startup(const char * a_guiPath);
		bool Shutdown();

		//\brief Draw all visible widgets
		//\param a_dt is the time since the last frame was drawn
		bool Update(float a_dt);

		void SetMousePos(Vector2 a_pos);

		bool CreateWidget(eWidgetType a_type);
		bool LoadWidgets(GameFile *a_inputFile);
		bool SaveWidgets(GameFile *a_outputFile);

	private:

		GameFile m_configFile;	// Base gui config file
		Widget m_screen;		// The parent of all widget that are created
		Widget m_cursor;		// A special widget for the mouse position
	};
}

#endif // _ENGINE_GUI_
