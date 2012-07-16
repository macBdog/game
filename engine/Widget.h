#ifndef _ENGINE_WIDGET_
#define _ENGINE_WIDGET_
#pragma once

#include "../core/Colour.h"
#include "../core/Vector.h"

#include "StringHash.h"
#include "Texture.h"

//\brief A widget vector is a 2D vector with the additional properties
//       of different coordinate types to deal with widescreen resolutions
//       and alignment types to create layouts that work in all screen sizes
class WidgetVector : public Vector2
{
public: 

	//brief Corordinate types allow element positioning relative to other elements
	enum eCoordType
	{
	  CoordTypeRelative = 0,	// -1.0 to 1.0 means the edge of the container 
	  CoordTypeAbsolute, 		// 1.0 means 1 pixel from the edge of the container

	  CoordTypeCount
	};

	void SetCoordType(eCoordType a_type) { m_type = a_type; }
	inline Vector2 GetVector() { return Vector2(x, y); }

	// Operator overloads are not inherited by default
	using Vector2::operator=;
	using Vector2::operator*;
	using Vector2::operator+;
	using Vector2::operator-;

private:

	eCoordType m_type;
};

//\brief Widgets are the base 2D elements that make up the GUI system
class Widget
{

public:

	//\brief Alignment types are used when coords are relative to anothe widget
	enum eAlignment
	{
	  AlignNone = 0,	// Placed wherever it's position dictates
	  AlignLeft,
	  AlignRight,
	  AlignTop,
	  AlignBtm,

	  AlignCount
	};

	//\brief Represents states of element selection
	enum eSelection
	{
	  Unselected = 0,	// Drawn without tint
	  Rollover,			// Indicating possible selection
	  Selected,			// Showing element selection
	  EditRollover,		// Showing the element will be selected for editing
	  EditSelected,		// Indicating edits will affect this element

	  SelectionCount,
	};

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

	//\brief Ctor nulls all pointers out for safety
	Widget()
		: m_fontName(NULL)
		, m_selectFlags(0)
		, m_active(true)
		, m_texture(NULL)
		, m_nextWidget(NULL)
		, m_childWidget(NULL)
	{}
		
	//\brief Base implementation will tint for selection
	virtual void Draw();

	//\brief Basic property accessors should remain unchanged for all instances of this class
	inline void SetTexture(Texture * a_tex) { m_texture = a_tex; }
	inline void SetPos(Vector2 a_pixelPos, WidgetVector::eCoordType a_type = WidgetVector::CoordTypeAbsolute) { m_pos = a_pixelPos; m_pos.SetCoordType(a_type); }
	inline void SetSize(Vector2 a_relPos, WidgetVector::eCoordType a_type = WidgetVector::CoordTypeAbsolute) { m_size = a_relPos, m_size.SetCoordType(a_type); }
	inline void SetColour(Colour a_colour) { m_colour = a_colour; }
	inline void SetActive(bool a_active = true) { m_active = a_active; }
	inline void SetFontName(StringHash * a_fontName) { m_fontName = a_fontName; }
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }

private:

	WidgetVector m_size;			// How much of the parent container the element takes up
	WidgetVector m_pos;				// Where in the parent container the element resides
	Colour m_colour;				// What the base colour of the widget is, will tint texture
	eSelection m_selection;			// If the widget is being rolled over etc
	bool m_active;					// If the widget should be drawn and reactive
	StringHash * m_fontName;		// Pointer to a stringhash containing the name of the font to render with
	Texture * m_texture;			// What to draw
	Widget * m_nextWidget;			// Conitiguous widgets are stored as a linked list
	Widget * m_childWidget;			// And each widget can have multiple children
	unsigned int m_selectFlags;		// What kind of selection this widget supports
	char m_name[StringUtils::s_maxCharsPerName];	// Display name or label
};


#endif // _ENGINE_WIDGET_
