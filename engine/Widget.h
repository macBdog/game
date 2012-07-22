#ifndef _ENGINE_WIDGET_
#define _ENGINE_WIDGET_
#pragma once

#include "../core/Colour.h"
#include "../core/Delegate.h"
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

	//\brief Two float ctor for convenience
	inline WidgetVector() { x = 0.0f; y = 0.0f; }
	inline WidgetVector(float a_x, float a_y) { x = a_x; y = a_y; }

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
	  eAlignNone = 0,	// Placed wherever it's position dictates
	  eAlignLeft,
	  eAlignRight,
	  eAlignTop,
	  eAlignBtm,

	  eAlignCount
	};

	//\brief Represents states of element selection
	enum eSelectionFlags
	{
	  eSelectionNone			= 0,	// Drawn without tint
	  eSelectionRollover		= 2,	// Indicating possible selection
	  eSelectionSelected		= 4,	// Showing element selection
	  eSelectionEditRollover	= 8,	// Showing the element will be selected for editing
	  eSelectionEditSelected	= 16,	// Indicating edits will affect this element

	  eSelectionCount = 5,
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
		: m_fontNameHash(0)
		, m_selectFlags(0)
		, m_active(true)
		, m_texture(NULL)
		, m_nextWidget(NULL)
		, m_childWidget(NULL)
	{}

	//\brief Used for passing around a definition to create a widget without
	//		 having a method with a gigantic parameter list
	struct WidgetDef
	{
		WidgetVector m_size;
		Colour m_colour;
		const char * m_name;
		unsigned int m_fontNameHash;
		eSelectionFlags m_selectFlags;
	};
		
	//\brief Base implementation will tint for selection
	virtual void Draw();

	//\brief Update selection flags for the widget based on a position
	//\param a_pos is the position to update from, usually the mouse pos
	bool UpdateSelection(WidgetVector a_pos);

	//\brief Find out if the widget is currently selected
	//\param a_selectMode is the kind of selection to check for
	//\return true if the element is selected
	bool IsSelected(eSelectionFlags a_selectMode = eSelectionRollover);
	inline bool IsActive() { return m_active; }

	//\brief Append a child widget pointer onto this widget creating a hierachy
	//\param a_child is a pointer to the allocated widget to append
	void AddChild(Widget * a_child);
	void AddSibling(Widget * a_sibling);

	//\brief Property accessors to return the head of the sibling or child widgets
	//\return The sibling or child widget or NULL if not set
	inline Widget * GetNext() { return m_nextWidget; }
	inline Widget * GetChild() { return m_childWidget; }

	//\brief Basic property accessors should remain unchanged for all instances of this class
	inline void SetTexture(Texture * a_tex) { m_texture = a_tex; }
	inline void SetPos(Vector2 a_pixelPos, WidgetVector::eCoordType a_type = WidgetVector::CoordTypeAbsolute) { m_pos = a_pixelPos; m_pos.SetCoordType(a_type); }
	inline void SetSize(Vector2 a_relPos, WidgetVector::eCoordType a_type = WidgetVector::CoordTypeAbsolute) { m_size = a_relPos, m_size.SetCoordType(a_type); }
	inline void SetColour(Colour a_colour) { m_colour = a_colour; }
	inline void SetActive(bool a_active = true) { m_active = a_active; }
	inline void SetFontName(unsigned int a_fontNameHash) { m_fontNameHash = a_fontNameHash; }
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetSelectFlags(eSelectionFlags a_flags) { m_selectFlags = a_flags; }

	inline WidgetVector GetPos() { return m_pos; }
	inline WidgetVector GetSize() { return m_size; }

	//\brief Execute the callback if defined
	inline void Activate() { m_action.Execute(this); }

	// Templated function so any part of the engine can be a widget action listener
	template <class TObj, typename TMethod>
	void SetAction(TObj * a_listenerObject, TMethod a_listenerFunc)
	{
		m_action.SetCallback(a_listenerObject, a_listenerFunc);
	}

private:

	// Colours used for rolling over elements
	static const Colour sc_rolloverColour;
	static const Colour sc_selectedColour;
	static const Colour sc_editRolloverColour;
	static const Colour sc_editSelectedColour;

	WidgetVector m_size;			// How much of the parent container the element takes up
	WidgetVector m_pos;				// Where in the parent container the element resides
	Colour m_colour;				// What the base colour of the widget is, will tint texture
	bool m_active;					// If the widget should be drawn and reactive
	unsigned int m_fontNameHash;	// Hash of the name of the font to render with
	Texture * m_texture;			// What to draw
	Widget * m_nextWidget;			// Conitiguous widgets are stored as a linked list
	Widget * m_childWidget;			// And each widget can have multiple children
	unsigned int m_selectFlags;		// Bit mask of kind of selection this widget supports
	eSelectionFlags m_selection;	// The current type of selection that that is current applied to the widget
	char m_name[StringUtils::s_maxCharsPerName];	// Display name or label
	Delegate<bool, Widget *> m_action;  // What to call when the widget is activated
};


#endif // _ENGINE_WIDGET_
