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

	//\brief Alignment types are used as every widget is relative to another
	enum eAlignmentX
	{
		eAlignLeft = 0,
		eAlignMiddle,
		eAlignRight,

		eAlignXCount,
	};
	enum eAlignmentY
	{
		eAlignTop = 0,
		eAlignCentre,
		eAlignBottom,

		eAlignYCount,
	};
	struct Alignment
	{
		Alignment() : m_x(eAlignLeft), m_y(eAlignTop) {}
		
		//\brief String equivalents for  alignment constants written in gui files
		inline void GetStringX(char * a_string_OUT) { sprintf(a_string_OUT, "%s", s_alignXNames[m_x]); }
		inline void GetStringY(char * a_string_OUT) { sprintf(a_string_OUT, "%s", s_alignYNames[m_y]); }
		inline void SetFromStringX(const char * a_alignString) { for (int i = 0; i < eAlignXCount; ++i) { if (strcmp(a_alignString, s_alignXNames[m_x]) == 0) { m_x = (eAlignmentX)i; break; } } }
		inline void SetFromStringY(const char * a_alignString) { for (int i = 0; i < eAlignYCount; ++i) { if (strcmp(a_alignString, s_alignYNames[m_y]) == 0) { m_y = (eAlignmentY)i; break; } } }
		
		eAlignmentX m_x;		///< Is widget position relative to left, middle or right
		eAlignmentY m_y;		///< Is widget position relative to top, centre or bottom
	};

	//\brief Two float ctor for convenience, default to top left
	inline WidgetVector() : m_align() { x = 0.0f; y = 0.0f; }
	inline WidgetVector(float a_x, float a_y) : m_align() { x = a_x; y = a_y; }

	//\brief Mutators
	inline Vector2 GetVector() { return Vector2(x, y); }
	inline Alignment GetAlignment() const { return m_align; }
	inline void SetAlignment(const Alignment & a_align) { m_align.m_x = a_align.m_x; m_align.m_y = a_align.m_y; }
	inline void SetAlignment(eAlignmentX a_x, eAlignmentY a_y) { m_align.m_x = a_x; m_align.m_y = a_y; }

	// Operator overloads are not inherited by default
	void operator = (const Vector2 & a_val) { x = a_val.GetX(); y = a_val.GetY(); }
	const WidgetVector operator * (const WidgetVector & a_val) { return WidgetVector(a_val.x * x, a_val.y * y); }
	const WidgetVector operator * (const float & a_val) { return WidgetVector(a_val * x, a_val * y); }
	const WidgetVector operator + (const WidgetVector & a_val) { return WidgetVector(a_val.x + x, a_val.y + y); }
	const WidgetVector operator - (const WidgetVector & a_val) { return WidgetVector(a_val.x - x, a_val.y - y); }
	using Vector2::operator +;
	using Vector2::operator -;
	using Vector2::operator *;

	//\brief Literal names for alignment types
	static const char * s_alignXNames[eAlignXCount];
	static const char * s_alignYNames[eAlignYCount];

private:
	Alignment m_alignAnchor;				///< Where the widget is relative to it's parent
	Alignment m_align;						///< How the widget is aligned
};

//\brief Widgets are the base 2D elements that make up the GUI system
class Widget
{
public:

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
		: m_pos(0.0f, 0.0f)
		, m_fontNameHash(0)
		, m_fontSize(1.0f)
		, m_selectFlags(0)
		, m_selection(eSelectionNone)
		, m_colour(sc_colourWhite)
		, m_showTextCursor(false)
		, m_active(true)
		, m_texture(NULL)
		, m_parent(NULL)
		, m_next(NULL)
		, m_firstChild(NULL)
		, m_debugRender(false)
		, m_alwaysRender(false)
		, m_selectedListItemId(eSelectionNone)
	{
		m_name[0] = '\0';
		m_script[0] = '\0';
		m_text[0] = '\0';
		m_filePath[0] = '\0';
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
			, m_fontSize(0.0f)
			, m_selectFlags(eSelectionRollover) {}

		WidgetVector m_size;
		WidgetVector m_pos;
		Colour m_colour;
		const char * m_name;
		unsigned int m_fontNameHash;
		float m_fontSize;
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
	inline void ClearSelection() { m_selection = eSelectionNone; }

	//\brief Active means rendering, updating selection and responding to events
	inline bool IsActive() { return m_active; }

	//\brief Debug widgets are rendered in a different batch to regular widgets
	inline bool IsDebugWidget() { return m_debugRender; }

	//\brief Append a child widget pointer onto this widget creating a hierachy
	//\param a_child is a pointer to the allocated widget to append
	void AddChild(Widget * a_child);
	void AddSibling(Widget * a_sibling);
	void Orphan();

	//\brief Property accessors to return the head of the sibling or child widgets
	//\return The sibling or child widget or NULL if not set
	inline Widget * GetNext() { return m_next; }
	inline Widget * GetChild() { return m_firstChild; }
	inline Widget * GetParent() { return m_parent; }
	Widget * Find(const char * a_name);

	//\brief Basic property accessors should remain unchanged for all instances of this class
	inline void SetTexture(Texture * a_tex) { m_texture = a_tex; }
	inline void SetOffset(Vector2 a_pctOffset) { m_pos = a_pctOffset; }
	inline void SetAlignment(const WidgetVector::Alignment & a_align) { m_pos.SetAlignment(a_align); } 
	inline void SetAlignment(WidgetVector::eAlignmentX a_alignX, WidgetVector::eAlignmentY a_alignY) { m_pos.SetAlignment(a_alignX, a_alignY); } 
	inline void SetSize(Vector2 a_pctSize) { m_size = a_pctSize; }
	inline void SetColour(Colour a_colour) { m_colour = a_colour; }
	inline void SetActive(bool a_active = true) { m_active = a_active; }
	inline void SetFontName(unsigned int a_fontNameHash) { m_fontNameHash = a_fontNameHash; }
	inline void SetFontSize(float a_newSize) { m_fontSize = a_newSize; }
	inline void SetName(const char * a_name) { sprintf(m_name, "%s", a_name); }
	inline void SetScript(const char * a_script) { sprintf(m_script, "%s", a_script); }
	inline void SetText(const char * a_text) { sprintf(m_text, "%s", a_text); }
	inline void SetFilePath(const char * a_path) { sprintf(m_filePath, "%s", a_path); }
	inline void SetSelectFlags(eSelectionFlags a_flags) { m_selectFlags = a_flags; }
	inline void SetDebugWidget() { m_debugRender = true; }
	inline void SetAlwaysDraw() { m_alwaysRender = true; }
	inline void SetShowTextCursor(bool a_show) { m_showTextCursor = a_show; }
	
	inline WidgetVector GetPos() { return m_pos; }
	inline WidgetVector GetSize() { return m_size; }
	inline const char * GetName() { return m_name; }
	inline const char * GetScript() { return m_script; }
	inline const char * GetText() { return m_text; }
	inline const char * GetFilePath() { return m_filePath; }

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
	inline void ClearAction()
	{
		m_action.ClearCallback();
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

	WidgetVector m_size;				///< How much of the parent container the element takes up
	WidgetVector m_pos;					///< Where in the parent container the element resides
	Colour m_colour;					///< What the base colour of the widget is, will tint texture
	bool m_active;						///< If the widget should be drawn and reactive
	bool m_showTextCursor;				///< If the cursor should be shown on a text field
	unsigned int m_fontNameHash;		///< Hash of the name of the font to render with
	float m_fontSize;					///< How large to draw text on this widget 
	Texture * m_texture;				///< What to draw
	Widget * m_parent;					///< Who's ya daddy
	Widget * m_next;					///< Conitiguous widgets are stored as a linked list
	Widget * m_firstChild;				///< And each widget can have multiple children
	unsigned int m_selectFlags;			///< Bit mask of kind of selection this widget supports
	eSelectionFlags m_selection;		///< The current type of selection that that is current applied to the widget
	Delegate<bool, Widget *> m_action;  ///< What to call when the widget is activated
	bool m_debugRender;					///< If the widget should be rendered using the debug batch
	bool m_alwaysRender;				///< If the widget should be rendered when the debug menu is off
	char m_name[StringUtils::s_maxCharsPerName];			///< Display name or label
	char m_script[StringUtils::s_maxCharsPerName];			///< Script filename
	char m_text[StringUtils::s_maxCharsPerLine];			///< Text for drawing labels and buttons
	char m_filePath[StringUtils::s_maxCharsPerLine];		///< Path for loading and saving, only menus should have this property

	LinkedList<StringHash> m_listItems;						///< Any string items that belong to this widget for lists and combo boxes
	unsigned int m_selectedListItemId;						///< Which item is currently selected
};


#endif // _ENGINE_WIDGET_
