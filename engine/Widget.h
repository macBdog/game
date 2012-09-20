#ifndef _ENGINE_WIDGET_
#define _ENGINE_WIDGET_
#pragma once

#include <iostream>
#include <fstream>

#include "../core/Colour.h"
#include "../core/Delegate.h"
#include "../core/LinkedList.h"
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
	  eCoordTypeRelative = 0,	// -1.0 to 1.0 means the edge of the container 
	  eCoordTypeAbsolute, 		// 1.0 means 1 pixel from the edge of the container

	  eCoordTypeCount
	};

	//\brief Two float ctor for convenience, default to absolute positioning
	inline WidgetVector() { x = 0.0f; y = 0.0f; m_type = eCoordTypeAbsolute; }
	inline WidgetVector(float a_x, float a_y) { x = a_x; y = a_y; m_type = eCoordTypeAbsolute; }

	void SetCoordType(eCoordType a_type) { m_type = a_type; }
	inline Vector2 GetVector() { return Vector2(x, y); }

	// Operator overloads are not inherited by default
	void operator = (const Vector2 & a_val) { x = a_val.GetX(); y = a_val.GetY(); }
	const WidgetVector operator * (const WidgetVector & a_val) { return WidgetVector(a_val.x * x, a_val.y * y); }
	const WidgetVector operator * (const float & a_val) { return WidgetVector(a_val * x, a_val * y); }
	const WidgetVector operator + (const WidgetVector & a_val) { return WidgetVector(a_val.x + x, a_val.y + y); }
	const WidgetVector operator - (const WidgetVector & a_val) { return WidgetVector(a_val.x - x, a_val.y - y); }
	using Vector2::operator +;
	using Vector2::operator -;
	using Vector2::operator *;

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

	//\brief The idea is that any widget has the capabilities to behave as
	// any kind of widget but the types exist to short cut behaviour and appearance
	// traits together.
	enum eWidgetType
	{
		eWidgetTypeCursor = 0,
		eWidgetTypeImage,
		eWidgetTypeButton,
		eWidgetTypeCheckbox,
		eWidgetTypeList,
			
		eWidgetTypeCount,
	};

	//\brief Specifies how a widget with items in it's list is displayed
	enum WidgetListType
	{
		eWidgetListType_List = 0,
		eWidgetListType_DropDown,
		eWidgetListType_Checkbox,
		eWidgetListType_Radio,

		eWidgetListType_Count,
	};

	//\brief Ctor nulls all pointers out for safety
	Widget()
		: m_fontNameHash(0)
		, m_selectFlags(0)
		, m_colour(sc_colourWhite)
		, m_active(true)
		, m_texture(NULL)
		, m_nextWidget(NULL)
		, m_childWidget(NULL)
		, m_debugRender(false)
		, m_selectedListItemId(0)
	{
		memset(&m_name, 0, sizeof(char) * StringUtils::s_maxCharsPerName);
		memset(&m_filePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	}

	//\brief Removes and deallocates all list items
	~Widget();

	//\brief Used for passing around a definition to create a widget without
	//		 having a method with a gigantic parameter list
	struct WidgetDef
	{
		WidgetDef()
			: m_size()
			, m_pos()
			, m_colour(sc_colourWhite)
			, m_name(NULL)
			, m_fontNameHash(0)
			, m_selectFlags(eSelectionRollover) {}

		WidgetVector m_size;
		WidgetVector m_pos;
		Colour m_colour;
		const char * m_name;
		unsigned int m_fontNameHash;
		eSelectionFlags m_selectFlags;
	};
		
	//\brief Base implementation will tint for selection
	virtual void Draw();

	//\brief Update selection flags for the widget and family based on a position
	//\param a_pos is the position to update from, usually the mouse pos
	void UpdateSelection(WidgetVector a_pos);

	//\brief Activate callback for widget and family based on selection flags
	bool DoActivation();

	//\brief Find out if the widget is currently selected
	//\param a_selectMode is the kind of selection to check for
	//\return true if the element is selected
	bool IsSelected(eSelectionFlags a_selectMode = eSelectionRollover);
	inline void SetSelection(eSelectionFlags a_newFlags) { m_selection = a_newFlags; }

	//\brief Active means rendering, updating selection and responding to events
	inline bool IsActive() { return m_active; }

	//\brief Debug widgets are rendered in a different batch to regular widgets
	inline bool IsDebugWidget() { return m_debugRender; }

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
	inline void SetPos(Vector2 a_pixelPos, WidgetVector::eCoordType a_type = WidgetVector::eCoordTypeAbsolute) { m_pos = a_pixelPos; m_pos.SetCoordType(a_type); }
	inline void SetSize(Vector2 a_relPos, WidgetVector::eCoordType a_type = WidgetVector::eCoordTypeAbsolute) { m_size = a_relPos, m_size.SetCoordType(a_type); }
	inline void SetColour(Colour a_colour) { m_colour = a_colour; }
	inline void SetActive(bool a_active = true) { m_active = a_active; }
	inline void SetFontName(unsigned int a_fontNameHash) { m_fontNameHash = a_fontNameHash; }
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetFilePath(const char * a_path) { sprintf(m_filePath, "%s", a_path); }
	inline void SetSelectFlags(eSelectionFlags a_flags) { m_selectFlags = a_flags; }
	inline void SetDebugWidget() { m_debugRender = true; }
	
	inline WidgetVector GetPos() { return m_pos; }
	inline WidgetVector GetSize() { return m_size; }
	inline const char * GetName() { return m_name; }

	//\brief Execute the callback if defined
	void Activate();

	//\brief Write the widget and all properties to a file stream
	//\param a_outputStream is a pointer to an output stream to write to, will create one if NULL
	//\param a_indentCount is an optional number of tab character to prefix each line with
	void Serialise(std::ofstream * a_outputStream = NULL, unsigned int a_indentCount = 0);

	// Templated function so any part of the engine or game can be a widget action listener
	template <class TObj, typename TMethod>
	inline void SetAction(TObj * a_listenerObject, TMethod a_listenerFunc)
	{
		m_action.SetCallback(a_listenerObject, a_listenerFunc);
	}

	//\brief List accessors for combo box and list type widgets
	void AddListItem(const char * a_newItemName);
	void RemoveListItem(const char * a_existingItemName);
	void ClearListItems();
	void SetSelectedListItem(unsigned int a_itemId);
	void SetSelectedListItem(const char * a_itemName);
	const char * GetListItem(unsigned int a_itemId);
	inline const char * GetSelectedListItem() { return GetListItem(m_selectedListItemId); } 

private:

	// Colours used for rolling over elements
	static const Colour sc_rolloverColour;
	static const Colour sc_selectedColour;
	static const Colour sc_editRolloverColour;
	static const Colour sc_editSelectedColour;

	WidgetVector m_size;				// How much of the parent container the element takes up
	WidgetVector m_pos;					// Where in the parent container the element resides
	Colour m_colour;					// What the base colour of the widget is, will tint texture
	bool m_active;						// If the widget should be drawn and reactive
	unsigned int m_fontNameHash;		// Hash of the name of the font to render with
	Texture * m_texture;				// What to draw
	Widget * m_nextWidget;				// Conitiguous widgets are stored as a linked list
	Widget * m_childWidget;				// And each widget can have multiple children
	unsigned int m_selectFlags;			// Bit mask of kind of selection this widget supports
	eSelectionFlags m_selection;		// The current type of selection that that is current applied to the widget
	Delegate<bool, Widget *> m_action;  // What to call when the widget is activated
	bool m_debugRender;					// If the widget should be rendered using the debug batch
	char m_name[StringUtils::s_maxCharsPerName];		// Display name or label
	char m_filePath[StringUtils::s_maxCharsPerLine];	// Path for loading and saving, only menus should have this property

	LinkedList<StringHash> m_listItems;						// Any string items that belong to this widget for lists and combo boxes
	unsigned int m_selectedListItemId;						// Which item is currently selected
};


#endif // _ENGINE_WIDGET_
