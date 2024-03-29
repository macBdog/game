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

//\brief Alignment types are used as every widget is relative to another
enum class AlignX : unsigned char
{
	Left = 0,
	Middle,
	Right,
	Count,
};

enum class AlignY : unsigned char
{
	Top = 0,
	Centre,
	Bottom,
	Count,
};

//		 ___________________________________________________________________________________________
//		|																							|\
//		| Left Top								Middle Top								  Right Top | \
//		|																							|  \
//		|																							|   | 
//		|																							|   |
//		|																							|   |
//		| Left Centre							Middle Centre						Right Centre	|   |
//		|																							|   |
//		|																							|   |
//		|																							|	|
//		|																							|	|
//		| Left Bottom							Middle Bottom						Right Bottom	|	|
//		|___________________________________________________________________________________________|	|
//		\																							 \  |
//		 \																							  \ |
//		  \____________________________________________________________________________________________\|

//\brief Alignment structs contain data and utility functions for delcaring a position in 2D space relative to some other position in 2D space
struct Alignment
{
	Alignment() : m_x(AlignX::Middle), m_y(AlignY::Centre) { }
	Alignment(AlignX a_x, AlignY a_y) : m_x(a_x), m_y(a_y) { }
		
	//\brief String equivalents for  alignment constants written in gui files
	inline void GetStringX(char * a_string_OUT) { sprintf(a_string_OUT, "%s", s_alignXNames[static_cast<int>(m_x)]); }
	inline void GetStringY(char * a_string_OUT) { sprintf(a_string_OUT, "%s", s_alignYNames[static_cast<int>(m_y)]); }
	inline void SetXFromString(const char * a_alignString) { for (int i = 0; i < static_cast<int>(AlignX::Count); ++i) { if (strcmp(a_alignString, s_alignXNames[i]) == 0) { m_x = (AlignX)i; break; } } }
	inline void SetYFromString(const char * a_alignString) { for (int i = 0; i < static_cast<int>(AlignY::Count); ++i) { if (strcmp(a_alignString, s_alignYNames[i]) == 0) { m_y = (AlignY)i; break; } } }
		
	//\brief Literal names for alignment types
	static const char * s_alignXNames[static_cast<int>(AlignX::Count)];
	static const char * s_alignYNames[static_cast<int>(AlignY::Count)];

	AlignX m_x{};		///< Is widget position relative to left, middle or right
	AlignY m_y{};		///< Is widget position relative to top, centre or bottom
};

//\brief A widget vector is a 2D vector with the additional properties
//       of different coordinate types to deal with widescreen resolutions
//       and alignment types to create layouts that work in all screen sizes
class WidgetVector : public Vector2
{
public: 

	//\brief Two float ctor for convenience, default to top left aligning to bottom right
	inline WidgetVector() 
		: m_alignAnchor(AlignX::Middle, AlignY::Centre) 
		, m_align(AlignX::Middle, AlignY::Centre)
		{ 
			x = 0.0f; y = 0.0f; 
		}
	inline WidgetVector(float a_x, float a_y) : m_align() { x = a_x; y = a_y; }

	//\brief Mutators
	inline Vector2 GetVector() { return Vector2(x, y); }
	inline Alignment GetAlignment() const { return m_align; }
	inline Alignment GetAlignmentAnchor() const { return m_alignAnchor; }
	inline void SetAlignment(const Alignment & a_align) { m_align.m_x = a_align.m_x; m_align.m_y = a_align.m_y; }
	inline void SetAlignment(AlignX a_x, AlignY a_y) { m_align.m_x = a_x; m_align.m_y = a_y; }
	inline void SetAlignmentAnchor(const Alignment & a_align) { m_alignAnchor.m_x = a_align.m_x; m_alignAnchor.m_y = a_align.m_y; }
	inline void SetAlignmentAnchor(AlignX a_x, AlignY a_y) { m_alignAnchor.m_x = a_x; m_alignAnchor.m_y = a_y; }

	// Operator overloads are not inherited by default
	void operator = (const Vector2 & a_val) { x = a_val.GetX(); y = a_val.GetY(); }
	const WidgetVector operator * (const WidgetVector & a_val) { return WidgetVector(a_val.x * x, a_val.y * y); }
	const WidgetVector operator * (const float & a_val) { return WidgetVector(a_val * x, a_val * y); }
	const WidgetVector operator + (const WidgetVector & a_val) { return WidgetVector(a_val.x + x, a_val.y + y); }
	const WidgetVector operator - (const WidgetVector & a_val) { return WidgetVector(a_val.x - x, a_val.y - y); }
	using Vector2::operator +;
	using Vector2::operator -;
	using Vector2::operator *;

	Alignment m_alignAnchor;				///< Which part of the widget is aligned
	Alignment m_align;						///< How the widget is aligned relative to another widget
};

//\brief Represents states of element selection
enum class SelectionFlags : unsigned char
{
	None			= 0,	// Drawn without tint
	Rollover		= 2,	// Indicating possible selection
	Selected		= 4,	// Showing element selection
	EditRollover	= 8,	// Showing the element will be selected for editing
	EditSelected	= 16,	// Indicating edits will affect this element
	Count = 5,
};


//\brief The idea is that any widget has the capabilities to behave as
// any kind of widget but the types exist to short cut behaviour and appearance
// traits together.
namespace WidgetType
{
	enum Enum
	{
		Cursor = 0,
		Image,
		Button,
		Checkbox,
		List,
		Count,
	};
}

//\brief Specifies how a widget with items in it's list is displayed
namespace WidgetListType
{
	enum Enum
	{
		List = 0,
		DropDown,
		Checkbox,
		Radio,
		Count,
	};
}

//\brief Widgets are the base 2D elements that make up the GUI system
class Widget
{
public:

	//\brief Ctor nulls all pointers out for safety
	Widget()
		: m_pos(0.0f, 0.0f)
		, m_size(0.0f, 0.0f)
		, m_drawPos(0.0f, 0.0f)
		, m_scissorRect(0.0f, 0.0f)
		, m_fontNameHash(0)
		, m_fontSize(1.0f)
		, m_selectFlags(0)
		, m_selection(SelectionFlags::None)
		, m_colour(sc_colourWhite)
		, m_showTextCursor(false)
		, m_active(true)
		, m_texture(nullptr)
		, m_alignTo(nullptr)
		, m_children()
		, m_debugRender(false)
		, m_alwaysRender(false)
		, m_selectedListItemId(-1)
		, m_rolloverListItemId(-1)
		, m_textWidth(-1.0f)
		, m_textHeight(-1.0f)
	{
		m_name[0] = '\0';
		m_alignToName[0] = '\0';
		m_scriptFuncName[0] = '\0';
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
			, m_name(nullptr)
			, m_fontNameHash(0)
			, m_fontSize(0.0f)
			, m_selectFlags(SelectionFlags::Rollover) {}

		WidgetVector m_size;
		WidgetVector m_pos;
		Colour m_colour;
		const char * m_name;
		unsigned int m_fontNameHash;
		float m_fontSize;
		SelectionFlags m_selectFlags;
	};
		
	typedef LinkedList<Widget> WidgetList;
	typedef LinkedListNode<Widget> WidgetNode;

	//\brief Base implementation will tint for selection
	void Draw();

	//\brief Debug widgets are rendered in a different renderLayer to regular widgets
	inline bool IsDebugWidget() { return m_debugRender; }

	//\brief Update selection flags for the widget and family based on a position
	//\param a_pos is the position to update from, usually the mouse pos
	void UpdateSelection(WidgetVector a_pos);

	//\brief Activate callback for widget and family based on selection flags
	bool DoActivation();

	//\brief Find out if the widget is currently selected
	//\param a_selectMode is the kind of selection to check for
	//\return true if the element is selected
	bool IsSelected(SelectionFlags a_selectMode = SelectionFlags::Rollover);
	inline void SetSelection(SelectionFlags a_newFlags) { m_selection = a_newFlags; }
	inline void ClearSelection() { m_selection = SelectionFlags::None; }

	//\brief Active means rendering, updating selection and responding to events
	inline bool IsActive() { return m_active; }

	//\brief Append a child widget pointer onto this widget creating a hierachy
	//\param a_child is a pointer to the allocated widget to append
	void AddChild(Widget * a_child);
	bool RemoveChild(Widget * a_child);
	bool RemoveChildren();

	//\brief Property accessors to return the head of the sibling or child widgets
	//\return The sibling or child widget or nullptr if not set
	inline WidgetNode * GetChildren() { return m_children.GetHead(); }
	inline bool HasChildren() { return m_children.GetLength() > 0; }
	Widget * Find(const char * a_name);
	Widget * FindSelected();
	bool RemoveAlignmentTo(Widget * a_alignedTo);
	bool RemoveFromChildren(Widget * a_child);

	//\brief Basic property accessors should remain unchanged for all instances of this class
	inline void SetTexture(Texture * a_tex) { m_texture = a_tex; }
	inline void SetOffset(const Vector2 & a_pctOffset) { m_pos = a_pctOffset; }
	inline void SetDrawPos(const Vector2 & a_pos) { m_drawPos = a_pos; }
	inline void SetScissorRect(const Vector2 & a_rect) { m_scissorRect = a_rect; }
	inline void SetPos(const WidgetVector & a_pos) { m_pos = a_pos; }
	inline void SetAlignment(const Alignment & a_align) { m_pos.SetAlignment(a_align); } 
	inline void SetAlignment(AlignX a_alignX, AlignY a_alignY) { m_pos.SetAlignment(a_alignX, a_alignY); }
	inline void SetAlignmentAnchor(const Alignment & a_align) { m_pos.SetAlignmentAnchor(a_align); } 
	inline void SetAlignmentAnchor(AlignX a_alignX, AlignY a_alignY) { m_pos.SetAlignmentAnchor(a_alignX, a_alignY); }
	inline void SetSize(Vector2 a_size) { m_size = a_size; }
	inline void SetColour(Colour a_colour) { m_colour = a_colour; }
	inline void SetActive(bool a_active = true) { m_active = a_active; }
	inline void SetFontName(unsigned int a_fontNameHash) { m_fontNameHash = a_fontNameHash; m_textWidth = -1.0f; m_textHeight = -1.0f; }
	inline void SetFontSize(float a_newSize) { m_fontSize = a_newSize; m_textWidth = -1.0f; m_textHeight = -1.0f; }
	inline void SetName(const char * a_name) 
	{ 
		if (a_name) 
		{ 
			strncpy(m_name, a_name, StringUtils::s_maxCharsPerName); 
		} 
	}
	inline void SetText(const char * a_text)
	{
		if (a_text)
		{
			strncpy(m_text, a_text, StringUtils::s_maxCharsPerLine);
			m_textWidth = -1.0f;
			m_textHeight = -1.0f;
		}
	}
	inline void SetFilePath(const char * a_path)
	{
		if (a_path)
		{
			strncpy(m_filePath, a_path, StringUtils::s_maxCharsPerLine);
		}
	}
	inline void SetScriptFuncName(const char * a_scriptName)
	{
		if (a_scriptName)
		{
			strncpy(m_scriptFuncName, a_scriptName, StringUtils::s_maxCharsPerName);
		}
	}
	inline void SetSelectFlags(const SelectionFlags & a_flags) { m_selectFlags = static_cast<unsigned int>(a_flags); }
	inline void SetDebugWidget() { m_debugRender = true; }
	inline void SetAlwaysDraw() { m_alwaysRender = true; }
	inline void SetShowTextCursor(bool a_show) { m_showTextCursor = a_show; }
	void SetAlignTo(Widget * a_alignWidget);
	void SetAlignTo(const char * a_alignWidgetName);
	void ClearAlignTo();
	void UpdateAlignTo();
	
	inline WidgetVector GetPos() const { return m_pos; }
	inline Vector2 GetDrawPos() const { return m_drawPos; }
	inline WidgetVector GetSize() const { return m_size; }
	inline const char * GetName() const { return m_name; }
	inline const char * GetText() const { return m_text; }
	inline float GetTextWidth() { if (m_textWidth < 0.0) { MeasureText(); } return m_textWidth; }
	inline float GetTextHeight() { if (m_textHeight < 0.0) { MeasureText(); } return m_textHeight; }
	inline const char * GetFilePath() const { return m_filePath; }
	inline float GetFontSize() const { return m_fontSize; }
	inline Colour GetColour() const { return m_colour; }
	inline Widget * GetAlignTo() const { return m_alignTo; }
	inline bool HasAlignTo() const { return m_alignTo != nullptr || m_alignToName[0] != '\0'; }
	inline const char * GetScriptFuncName() { return m_scriptFuncName; }

	//\brief Execute the callback if defined
	void Activate();

	//\brief Write the widget and all properties to a file stream
	//\param a_outputStream is a pointer to an output stream to write to, will create one if nullptr
	//\param a_indentCount is an optional number of tab character to prefix each line with
	void Serialise(std::ofstream * a_outputStream = nullptr, unsigned int a_indentCount = 0);

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
	void SetRolloverListItem(unsigned int a_itemId);
	void SetSelectedListItem(const char * a_itemName);
	void SetRolloverListItem(const char * a_itemName);
	const char * GetListItem(unsigned int a_itemId);
	inline const char * GetSelectedListItem() { return GetListItem(m_selectedListItemId); } 

#ifndef _RELEASE
	//\brief Show alignment handles and parent alignment
	void DrawDebugAlignment();
	void DrawDebugAlignmentHandleSelectionBox(const Vector2 & a_topLeft, const Vector2 & a_handleSize);
	bool GetAlignmentSelection(const Vector2 & a_selectionPos, const Vector2 & a_handleSize, Vector2 & a_handlePos_OUT, AlignX & a_xSelected_OUT, AlignY & a_ySelected_OUT);
	void DrawDebugAlignmentHandles(const Vector2 & a_boxSize, const Colour & a_drawColour, const Colour & a_selectedColour);
#endif

	static float s_selectedColourValue;				///< Value to attenuate a selected widget's colour with, only one selected widget ever
	static const float sc_alignmentHandleSize;		///< How big to draw the alignment handles when selecting

private:

	// Colours used for rolling over elements in normal and game modes
	static const Colour sc_rolloverColour;
	static const Colour sc_selectedColour;
	static const Colour sc_editRolloverColour;
	static const Colour sc_editSelectedColour;

	//\brief Work out the top left coordinate of widget according to it's align anchor and parent
	//\param a_alignParent the widget to align the position to
	//\return A vector2 with the coordinate in relative sapce (-1 to +1)
	Vector2 GetPositionRelative(Widget * a_alignParent);
	
	///\brief Refresh the internal text width and height values, called only when text properties change
	void MeasureText();

	WidgetVector m_size;				///< How much of the parent container the element takes up
	WidgetVector m_pos;					///< Where in the parent container the element resides
	Vector2 m_drawPos;					///< The calculated screen position of this widget based on it's hierachy
	Vector2 m_scissorRect;				///< If the widget drawing should be clipped by some amount, this vec will be non zero
	Colour m_colour;					///< What the base colour of the widget is, will tint texture
	bool m_active;						///< If the widget should be drawn and reactive
	bool m_showTextCursor;				///< If the cursor should be shown on a text field
	unsigned int m_fontNameHash;		///< Hash of the name of the font to render with
	float m_fontSize;					///< How large to draw text on this widget 
	Texture* m_texture{ nullptr };		///< What to draw
	Widget * m_alignTo{ nullptr };		///< If this widget's position depends on another
	WidgetList m_children;				///< Each widget can have multiple children
	unsigned int m_selectFlags;			///< Bit mask of kind of selection this widget supports
	SelectionFlags m_selection;			///< The current type of selection that that is current applied to the widget
	Delegate<bool, Widget *> m_action;  ///< What to call when the widget is activated
	bool m_debugRender;					///< If the widget should be rendered using the debug renderLayer
	bool m_alwaysRender;				///< If the widget should be rendered when the debug menu is off
	char m_name[StringUtils::s_maxCharsPerName];			///< Display name or label
	char m_alignToName[StringUtils::s_maxCharsPerName];		///< Name of alignment relative widget as widgets may be loaded out of order
	char m_scriptFuncName[StringUtils::s_maxCharsPerName];	///< The name of the global LUA script to call on activation
	char m_text[StringUtils::s_maxCharsPerLine];			///< Text for drawing labels and buttons
	float m_textWidth;										///< How wide the text in the current font is
	float m_textHeight;										///< How high the text in the current font is
	char m_filePath[StringUtils::s_maxCharsPerLine];		///< Path for loading and saving, only menus should have this property
	LinkedList<StringHash> m_listItems;						///< Any string items that belong to this widget for lists and combo boxes
	int m_rolloverListItemId;								///< Which item is currently being rolled over, negative for none
	int m_selectedListItemId;								///< Which item is currently selected, negative for none

};


#endif // _ENGINE_WIDGET_
