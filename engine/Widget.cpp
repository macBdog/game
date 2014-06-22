#include <assert.h>

#include "DebugMenu.h"
#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Widget.h"

using namespace std;	//< For fstream operations

const char * Alignment::s_alignXNames[AlignX::Count] = 
{
	"left",
	"middle",
	"right"
};
const char * Alignment::s_alignYNames[AlignY::Count] =
{
	"top",
	"centre",
	"bottom"
};

const Colour Widget::sc_rolloverColour = Colour(0.25f, 0.25f, 0.25f, 0.5f);
const Colour Widget::sc_selectedColour = Colour(0.35f, 0.35f, 0.35f, 0.5f);
const Colour Widget::sc_editRolloverColour = Colour(0.05f, 0.2f, 0.2f, 0.2f);
const Colour Widget::sc_editSelectedColour = Colour(0.05f, 0.85f, 0.85f, 0.0f);

float Widget::s_selectedColourValue = 0.0f;	///< Value to attenuate a selected widget's colour with, only one selected widget ever

Widget::~Widget()
{
	// Delete list items owned by this widget
	LinkedListNode<StringHash> * nextItem = m_listItems.GetHead();
	while(nextItem != NULL)
	{
		// Cache off working node
		LinkedListNode<StringHash> * curItem = nextItem;
		nextItem = nextItem->GetNext();

		m_listItems.Remove(curItem);
		delete curItem->GetData();
		delete curItem;	
	}	
}

void Widget::Draw()
{
	bool shouldDraw =  !(IsDebugWidget() && !DebugMenu::Get().IsDebugMenuEnabled());
	if (m_active && (shouldDraw || m_alwaysRender))	// Is a debug widget and the debug menu is on or should we always draw it
	{
		// Derive position based on alignment to parent
		Vector2 drawPos = m_pos.GetVector();
		UpdateAlignTo();
		if (m_alignTo != NULL)
		{
			drawPos = GetPositionRelative(m_alignTo);
		}
		m_drawPos = drawPos;

		// Determine draw style from selection
		Colour selectColour = m_colour;
		RenderManager & rMan = RenderManager::Get();
		RenderLayer::Enum renderLayer = m_debugRender ? RenderLayer::Debug2D : RenderLayer::Gui;
		switch (m_selection)
		{
			case SelectionFlags::Rollover:		selectColour -= sc_rolloverColour * s_selectedColourValue;	break;
			case SelectionFlags::Selected:		selectColour -= sc_selectedColour * s_selectedColourValue;	break;
			
			// Draw a selection box around a selected widget
			case SelectionFlags::EditRollover:	
			case SelectionFlags::EditSelected:
			{
				selectColour -= sc_editSelectedColour;
				const float extraSelectionSize = 0.05f;
				const Vector2 selectSize = m_size + (m_size * extraSelectionSize);
				Vector2 startVec = drawPos;
				startVec.SetX(startVec.GetX() - (selectSize.GetX() - m_size.GetX()) * 0.5f);
				startVec.SetY(startVec.GetY() + (selectSize.GetY() - m_size.GetY()) * 0.5f);
				Vector2 endVec = Vector2(startVec.GetX() + selectSize.GetX(), startVec.GetY());  rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, selectColour);
				startVec = endVec;	endVec -= Vector2(0.0f, selectSize.GetY());	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, selectColour);
				startVec = endVec;	endVec -= Vector2(selectSize.GetX(), 0.0f);	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, selectColour);
				startVec = endVec;	endVec += Vector2(0.0f, selectSize.GetY());	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, selectColour);	
				break;
			}
			default: break;
		}

		// Draw the quad in various states of activation
		if (m_texture != NULL)
		{
			rMan.AddQuad2D(renderLayer, drawPos, m_size.GetVector(), m_texture, TextureOrientation::Normal, selectColour);
		}

		// Draw fill colour for debug widgets
		if (IsDebugWidget())
		{
			Colour fillColour = selectColour * 0.3f;
			fillColour.SetA(0.95f);
			rMan.AddQuad2D(renderLayer, drawPos, m_size.GetVector(), NULL, TextureOrientation::Normal, fillColour);
		}

		// Draw widget bounds for editing mode or for debug widgets
		if (DebugMenu::Get().IsDebugMenuEnabled() || IsDebugWidget())
		{
			Colour boxColour = selectColour;
			boxColour.SetA(0.1f);
			Vector2 startVec = drawPos;
			Vector2 endVec = Vector2(startVec.GetX() + m_size.GetX(), startVec.GetY());
			rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, boxColour);
			
			startVec = endVec;	endVec -= Vector2(0.0f, m_size.GetY());	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, boxColour);
			startVec = endVec;	endVec -= Vector2(m_size.GetX(), 0.0f);	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, boxColour);
			startVec = endVec;	endVec += Vector2(0.0f, m_size.GetY());	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, boxColour);
		}

		// Draw gui label on top of the widget in editing mode
		const float fontDisplaySize = 0.07f;
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			if (m_fontNameHash > 0)
			{
				FontManager::Get().DrawDebugString2D(m_name, drawPos, m_colour, renderLayer);

				// Some widgets also text like labels and buttons
				if (m_text[0] != '\0')
				{
					if (m_showTextCursor)
					{
						// Draw the string with a cursor
						char cursorText[StringUtils::s_maxCharsPerLine];
						if (Gui::GetTextCursorBlink())
						{
							sprintf(cursorText, "%s|", m_text);
						}
						else
						{
							sprintf(cursorText, "%s", m_text);
						}
						FontManager::Get().DrawDebugString2D(cursorText, Vector2(drawPos.GetX() + fontDisplaySize, drawPos.GetY() - fontDisplaySize), m_colour, renderLayer);
					}
				}
			}	
		} 

		// Always display text for a widget
		if (m_text[0] != '\0') 
		{
			FontManager::Get().DrawString(m_text, m_fontNameHash, m_fontSize, drawPos, selectColour, renderLayer);
		}

		// Draw any list items
		LinkedListNode<StringHash> * nextItem = m_listItems.GetHead();
		Vector2 listDisplayPos = Vector2(drawPos.GetX() + fontDisplaySize, drawPos.GetY() - fontDisplaySize);
		Vector2 itemDisplayPos(listDisplayPos);
		unsigned int numItems = 0;
		while(nextItem != NULL)
		{
			// If this is the selected item then draw the highlight bar behind the item
			if (numItems == m_selectedListItemId)
			{
				Vector2 hLightPos(listDisplayPos.GetX(), listDisplayPos.GetY() - (fontDisplaySize * numItems));
				Vector2 hLightSize(m_size.GetVector().GetX() - fontDisplaySize, fontDisplaySize);
				rMan.AddQuad2D(renderLayer, hLightPos, hLightSize, NULL, TextureOrientation::Normal, selectColour - sc_rolloverColour);
			}
			else if (numItems == m_rolloverListItemId)
			{
				Vector2 hLightPos(listDisplayPos.GetX(), listDisplayPos.GetY() - (fontDisplaySize * numItems));
				Vector2 hLightSize(m_size.GetVector().GetX() - fontDisplaySize, fontDisplaySize);
				rMan.AddQuad2D(renderLayer, hLightPos, hLightSize, NULL, TextureOrientation::Normal, selectColour - sc_selectedColour);
			}

			// Now draw the item text
			FontManager::Get().DrawString(nextItem->GetData()->GetCString(), m_fontNameHash, 1.0f, itemDisplayPos, m_colour, renderLayer);
			itemDisplayPos.SetY(itemDisplayPos.GetY() - fontDisplaySize);

			nextItem = nextItem->GetNext();
			++numItems;
		}
	}

	// Draw any child widgets
	WidgetNode * curWidget = m_children.GetHead();
	while (curWidget != NULL)
	{
		curWidget->GetData()->Draw();
		curWidget = curWidget->GetNext();
	}
}

void Widget::DrawAlignment()
{
	const float boxSize = 0.05f;
	const float halfBox = boxSize * 0.5f;
	RenderManager & rMan = RenderManager::Get();
		
	// Show crosshair of alignment anchor
	Vector2 pos = m_drawPos;
	Colour drawColour = sc_colourPurple;
	Vector2 alignPos = Vector2(pos.GetX() + halfBox, pos.GetY() - halfBox);
	rMan.AddDebugLine2D(alignPos - Vector2(halfBox, 0.0f), alignPos + Vector2(halfBox, 0.0f), drawColour);
	rMan.AddDebugLine2D(alignPos - Vector2(0.0f, halfBox), alignPos + Vector2(0.0f, halfBox), drawColour);

	// Show nine alignment anchors
	drawColour.SetA(0.25);
	Vector2 boxSizeVec(boxSize);
	Vector2 anchorPos(pos.GetX(), pos.GetY());																	rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos.SetX(anchorPos.GetX() + (m_size.GetX() * 0.5f) - (boxSizeVec.GetX() * 0.5f));						rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos.SetX(anchorPos.GetX() + (m_size.GetX() * 0.5f) - (boxSizeVec.GetX() * 0.5f));						rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos = Vector2(pos.GetX(), anchorPos.GetY() - (m_size.GetY() * 0.5f) + (boxSizeVec.GetY() * 0.5f));	rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos.SetX(anchorPos.GetX() + (m_size.GetX() * 0.5f) - (boxSizeVec.GetX() * 0.5f));						rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos.SetX(anchorPos.GetX() + (m_size.GetX() * 0.5f) - (boxSizeVec.GetX() * 0.5f));						rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos = Vector2(pos.GetX(), anchorPos.GetY() - (m_size.GetY() * 0.5f) + (boxSizeVec.GetY() * 0.5f));	rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos.SetX(anchorPos.GetX() + (m_size.GetX() * 0.5f) - (boxSizeVec.GetX() * 0.5f));						rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
	anchorPos.SetX(anchorPos.GetX() + (m_size.GetX() * 0.5f) - (boxSizeVec.GetX() * 0.5f));						rMan.AddDebugQuad2D(anchorPos, boxSizeVec, drawColour);
}

void Widget::UpdateSelection(WidgetVector a_pos)
{
	// Don't select debug menu elements if we aren't in debug mode
	if (IsDebugWidget() && !DebugMenu::Get().IsDebugMenuEnabled())
	{
		m_selection = SelectionFlags::None;
		return;
	}

	// Derive position based on alignment to parent
	Vector2 drawPos = m_pos.GetVector();
	if (m_alignTo != NULL)
	{
		drawPos = GetPositionRelative(m_alignTo);
	}

	// Check if the mouse position is inside the widget
	if (a_pos.GetX() >= drawPos.GetX() && a_pos.GetX() <= drawPos.GetX() + m_size.GetX() && 
		a_pos.GetY() <= drawPos.GetY() && a_pos.GetY() >= drawPos.GetY() - m_size.GetY())
	{
		// Check selection flags
		if ((m_selectFlags & SelectionFlags::Rollover) > 0)
		{
			if (DebugMenu::Get().IsDebugMenuEnabled())
			{
				if (m_selection <= SelectionFlags::EditRollover)
				{
					m_selection = SelectionFlags::EditRollover;
				}
			}
			else
			{
				m_selection = SelectionFlags::Rollover;
			}
		}
	}
	else // Selection position not inside bounds
	{
		// Don't unselect a selection by mouseover
		if (m_selection != SelectionFlags::Selected && 
			m_selection != SelectionFlags::EditSelected)
		{
			m_selection = SelectionFlags::None;
		}
	}

	// Update selection of list items
	if (m_selection > SelectionFlags::None)
	{
		const float fontDisplaySize = 0.07f;
		LinkedListNode<StringHash> * nextItem = m_listItems.GetHead();
		Vector2 listDisplayPos = Vector2(drawPos.GetX() + fontDisplaySize, drawPos.GetY() - fontDisplaySize);
		unsigned int numItems = 0;
		while(nextItem != NULL)
		{
			Vector2 testPos(listDisplayPos.GetX(), listDisplayPos.GetY() - (fontDisplaySize * numItems));
			Vector2 testSize(m_size.GetVector().GetX() - fontDisplaySize, fontDisplaySize);
			if (a_pos.GetX() >= testPos.GetX() && a_pos.GetX() <= testPos.GetX() + testSize.GetX() && 
				a_pos.GetY() <= testPos.GetY() && a_pos.GetY() >= testPos.GetY() - testSize.GetY())
			{
				m_rolloverListItemId = numItems;
				break;
			}

			nextItem = nextItem->GetNext();
			numItems++;
		}
	}

	// Update selection on any child widgets
	WidgetNode * curWidget = m_children.GetHead();
	while (curWidget != NULL)
	{
		curWidget->GetData()->UpdateSelection(a_pos);
		curWidget = curWidget->GetNext();
	}
}

bool Widget::DoActivation()
{
	// Don't activate debug menu elements if we aren't in debug mode
	if (IsDebugWidget() && !DebugMenu::Get().IsDebugMenuEnabled())
	{
		return false;
	}

	// Check if the position is inside the widget
	DebugMenu & debug = DebugMenu::Get();
	bool activated = false;
	SelectionFlags::Enum flags = debug.IsDebugMenuEnabled() ? SelectionFlags::EditRollover : SelectionFlags::Rollover;
	if (IsSelected(flags))
	{
		if (!IsDebugWidget() && debug.IsDebugMenuEnabled())
		{
			// Do nothing, potentially test the action but not execute 
		} 
		else if (IsActive())
		{
			// Perform whatever the widget's function is
			Activate();
			activated = true;
		}
	}
	
	// Activate any child widgets
	WidgetNode * curWidget = m_children.GetHead();
	while (curWidget != NULL)
	{
		activated &= curWidget->GetData()->DoActivation();
		curWidget = curWidget->GetNext();
	}

	return activated;
}

bool Widget::IsSelected(SelectionFlags::Enum a_selectMode)
{ 
	// TODO Should be a bittest with every selection flag
	return m_selection == a_selectMode;
}

void Widget::AddChild(Widget * a_child)
{
	m_children.InsertNew(a_child);
}

bool Widget::RemoveChild(Widget * a_child)
{
	// This will not handle align references or parent lists, just child relationships!
	WidgetNode * cur = m_children.GetHead();
	while (cur != NULL)
	{
		WidgetNode * next = cur->GetNext();
		if (cur->GetData() == a_child)
		{
			m_children.RemoveDelete(cur);
			return true;
		}
		cur = next;
	}
	return false;
}

bool Widget::RemoveChildren()
{
	// This will not handle align references or parent lists, just child relationships!
	WidgetNode * cur = m_children.GetHead();
	while (cur != NULL)
	{
		WidgetNode * next = cur->GetNext();
		if (cur->GetData()->m_children.GetLength() > 0)
		{
			cur->GetData()->RemoveChildren();
		}
		m_children.Remove(cur);
		delete cur;
		cur = next;
	}
	return false;
}

Widget * Widget::Find(const char * a_name)
{
	// Non search cases
	if (strcmp(m_name, a_name) == 0)
	{
		return this;
	}

	if (m_children.IsEmpty())
	{
		return NULL;
	}

	// Exhaustive search through child and siblings to find widget with name
	Widget * foundWidget = NULL;
	WidgetNode * curWidget = m_children.GetHead();
	while (curWidget != NULL && foundWidget == NULL)
	{
		if (strcmp(curWidget->GetData()->GetName(), a_name) == 0)
		{
			return curWidget->GetData();
		}
		// Recurse into children's family to find the widget
		if (!curWidget->GetData()->m_children.IsEmpty())
		{
			foundWidget = curWidget->GetData()->Find(a_name);
		}
		// Keep searching the widget list
		curWidget = curWidget->GetNext();
	}
	return foundWidget;
}

bool Widget::RemoveAlignmentTo(Widget * a_alignedTo)
{
	bool cleared = false;
	if (a_alignedTo == NULL)
	{
		return false;
	}

	if (m_children.IsEmpty())
	{
		return false;
	}

	// Clear self
	if (m_alignTo == a_alignedTo)
	{
		ClearAlignTo();
		cleared = true;
	}

	// Exhaustive search through children to clear alignment to a widget
	WidgetNode * cur = m_children.GetHead();
	while (cur != NULL)
	{
		Widget * curWidget = cur->GetData();
		if (curWidget->m_alignTo == a_alignedTo)
		{
			curWidget->ClearAlignTo();
			cleared = true;
		}
		// Recurse into children's family to find the widget
		if (!curWidget->m_children.IsEmpty())
		{
			cleared = curWidget->RemoveAlignmentTo(a_alignedTo);
		}
		cur = cur->GetNext();
	}
	return cleared;
}

bool Widget::RemoveFromChildren(Widget * a_child)
{
	if (a_child == NULL)
	{
		return false;
	}

	if (m_children.IsEmpty())
	{
		return false;
	}

	// Early out after the first removal but better to be exhaustive
	bool removed = RemoveChild(a_child);
	WidgetNode * cur = m_children.GetHead();
	while (cur != NULL && !removed)
	{
		Widget * curWidget = cur->GetData();
		if (curWidget->RemoveChild(a_child))
		{
			removed = true;
		}
		if (!curWidget->m_children.IsEmpty())
		{
			if (curWidget->RemoveFromChildren(a_child))
			{
				removed = true;
			}
		}
		cur = cur->GetNext();
	}
	
	return removed;
}

void Widget::SetAlignTo(Widget * a_alignWidget)
{
	m_alignTo = a_alignWidget;
	strcpy(m_alignToName, a_alignWidget->GetName());
}

void Widget::SetAlignTo(const char * a_alignWidgetName)
{
	strcpy(m_alignToName, a_alignWidgetName);
	UpdateAlignTo();
}

void Widget::ClearAlignTo()
{
	m_alignTo = NULL;
	m_alignToName[0] = '\0';
}

void Widget::UpdateAlignTo()
{
	// Lookup the align to widget if the name is set but the pointer isn't OR
	// if the name and the pointer mismatch
	if ( (m_alignTo == NULL && m_alignToName[0] != '\0') || 
		 (m_alignTo != NULL && m_alignToName[0] != '\0' && strcmp(m_alignTo->GetName(), m_alignToName) != 0))
	{
		m_alignTo = Gui::Get().FindWidget(m_alignToName);
	}
}

void Widget::Activate() 
{ 
	// Check then call the callback
	if (m_action.IsSet())
	{
		m_action.Execute(this);
	}
	else if (!m_debugRender)
	{
		Log::Get().Write(LogLevel::Warning, LogCategory::Engine, "Widget named %s does not have an action set.", GetName());
	}

	// Update selection if a list has been clicked on
	if (m_selection > SelectionFlags::None && m_listItems.GetLength() > 0 && m_rolloverListItemId >= 0)
	{
		m_selectedListItemId = m_rolloverListItemId;
	}
}

void Widget::Serialise(std::ofstream * a_outputStream, unsigned int a_indentCount)
{
	char outBuf[StringUtils::s_maxCharsPerName];

	// If this is a recursive call the output stream will be already set up
	if (a_outputStream != NULL)
	{
		// Generate the correct tab amount
		char tabs[StringUtils::s_maxCharsPerName];
		memset(&tabs[0], '\t', a_indentCount);
		tabs[a_indentCount] = '\0';

		std::ofstream & menuStream = *a_outputStream;
		menuStream << tabs << "widget" << StringUtils::s_charLineEnd;
		menuStream << tabs << "{" << StringUtils::s_charLineEnd;
		menuStream << tabs << StringUtils::s_charTab << "name: "	<< m_name				<< StringUtils::s_charLineEnd;

		// Write the begin loaded property out
		if (Gui::Get().GetStartupMenu() == this)
		{
			menuStream << tabs << StringUtils::s_charTab << "beginLoaded: true"	<< StringUtils::s_charLineEnd;
		}

		strncpy(outBuf, m_text, strlen(m_text) + 1);
		menuStream << tabs << StringUtils::s_charTab << "text: " << outBuf << StringUtils::s_charLineEnd;
		
		m_colour.GetString(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "colour: "	<< outBuf	<< StringUtils::s_charLineEnd;

		sprintf(outBuf, "%s", FontManager::Get().GetLoadedFontName(m_fontNameHash));
		menuStream << tabs << StringUtils::s_charTab << "font: " << outBuf	 << StringUtils::s_charLineEnd;

		sprintf(outBuf, "%f", m_fontSize);
		menuStream << tabs << StringUtils::s_charTab << "fontSize: " << outBuf << StringUtils::s_charLineEnd;

		m_pos.GetString(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "offset: "	<< outBuf	<< StringUtils::s_charLineEnd;

		m_size.GetString(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "size: "	<< outBuf	<< StringUtils::s_charLineEnd;

		if (m_alignToName[0] != '\0')
		{
			menuStream << tabs << StringUtils::s_charTab << "alignTo: "	<< m_alignToName	<< StringUtils::s_charLineEnd;
		}

		m_pos.GetAlignment().GetStringX(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "alignX: "	<< outBuf	<< StringUtils::s_charLineEnd;

		m_pos.GetAlignment().GetStringY(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "alignY: "	<< outBuf	<< StringUtils::s_charLineEnd;

		m_pos.GetAlignmentAnchor().GetStringX(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "alignAnchorX: "	<< outBuf	<< StringUtils::s_charLineEnd;

		m_pos.GetAlignmentAnchor().GetStringY(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "alignAnchorY: "	<< outBuf	<< StringUtils::s_charLineEnd;

		if (m_texture != NULL)
		{
			menuStream << tabs << StringUtils::s_charTab << "texture: " << m_texture->GetFileName() << StringUtils::s_charLineEnd;
		}

		menuStream << tabs << "}" << StringUtils::s_charLineEnd;

		// Serialise any children of this child
		WidgetNode * curChild = m_children.GetHead();
		while (curChild != NULL)
		{
			curChild->GetData()->Serialise(a_outputStream, ++a_indentCount);
			curChild = curChild->GetNext();
		}
	}
	else if (strlen(m_filePath) > 0)
	{
		// Create an output stream
		ofstream menuOutput;
		menuOutput.open(m_filePath);

		// Write menu header
		if (menuOutput.is_open())
		{
			menuOutput << "menu"	<< StringUtils::s_charLineEnd;
			menuOutput << "{"		<< StringUtils::s_charLineEnd;
			menuOutput << StringUtils::s_charTab		<< "name: "		<< m_name << StringUtils::s_charLineEnd;
					
			// Write the begin loaded property out
			if (Gui::Get().GetStartupMenu() == this)
			{
				menuOutput << StringUtils::s_charTab << "beginLoaded: true"	<< StringUtils::s_charLineEnd;
			}

			// Write out any children of this child
			WidgetNode * curChild = m_children.GetHead();
			while (curChild != NULL)
			{
				curChild->GetData()->Serialise(&menuOutput, 1);
				curChild = curChild->GetNext();
			}

			menuOutput << "}" << StringUtils::s_charLineEnd;
		}

		menuOutput.close();
	}
}

void Widget::AddListItem(const char * a_newItemName)
{
	// Allocate a new node and set data
	if (LinkedListNode<StringHash> * newListItem = new LinkedListNode<StringHash>())
	{
		newListItem->SetData(new StringHash(a_newItemName));
		m_listItems.Insert(newListItem);
	}
}

void Widget::RemoveListItem(const char * a_existingItemName)
{
	LinkedListNode<StringHash> * cur = m_listItems.GetHead();
	while(cur != NULL)
	{
		// Remove item and quit out of the loop if found
		if (cur->GetData()->GetHash() == StringHash::GenerateCRC(a_existingItemName))
		{
			delete cur->GetData();
			m_listItems.Remove(cur);
			delete cur;
			break;
		}
	}
}

void Widget::ClearListItems()
{
	LinkedListNode<StringHash> * nextItem = m_listItems.GetHead();
	while(nextItem != NULL)
	{
		// Remove all items by caching off the current pointer
		LinkedListNode<StringHash> * curItem = nextItem;
		nextItem = nextItem->GetNext();

		m_listItems.Remove(curItem);
		delete curItem->GetData();
		delete curItem;	
	}
}

const char * Widget::GetListItem(unsigned int a_itemId)
{
	if (a_itemId >= m_listItems.GetLength())
	{
		Log::Get().Write(LogLevel::Warning, LogCategory::Engine, "Trying to get an invalid list item %d from widget called %s.", a_itemId, m_name);
		return NULL;
	}

	unsigned int numItems = 0;
	LinkedListNode<StringHash> * cur = m_listItems.GetHead();
	while(cur != NULL)
	{
		// Remove item and quit out of the loop if found
		if (numItems == a_itemId)
		{
			return cur->GetData()->GetCString();
		}

		cur = cur->GetNext();
		++numItems;
	}

	return NULL;
}

Vector2 Widget::GetPositionRelative(Widget * a_alignParent)
{
	Vector2 pos = a_alignParent->m_drawPos + m_pos.GetVector();
	const float sizeX = m_size.GetX();
	const float sizeY = m_size.GetY();
	const float parentSizeX = a_alignParent->GetSize().GetX();
	const float parentSizeY = a_alignParent->GetSize().GetY();

	// Offset from parent X
	switch (m_pos.GetAlignment().m_x)
	{
		case AlignX::Middle:	pos.SetX(pos.GetX() + parentSizeX * 0.5f);	break;
		case AlignX::Right:		pos.SetX(pos.GetX() + parentSizeX);			break;
		default: break;
	}
	// Offset from parent Y
	switch (m_pos.GetAlignment().m_y)
	{
		case AlignY::Centre:	pos.SetY(pos.GetY() - parentSizeY * 0.5f);	break;
		case AlignY::Bottom:	pos.SetY(pos.GetY() - parentSizeY);			break;
		default: break;
	}
	// Offset from anchor X
	switch (m_pos.GetAlignmentAnchor().m_x)
	{
		case AlignX::Middle:	pos.SetX(pos.GetX() - sizeX * 0.5f);	break;
		case AlignX::Right:		pos.SetX(pos.GetX() - sizeX);			break;
		default: break;
	}
	// Offset from anchor Y
	switch (m_pos.GetAlignmentAnchor().m_y)
	{
		case AlignY::Bottom:	pos.SetY(pos.GetY() + sizeY);			break;
		case AlignY::Centre:	pos.SetY(pos.GetY() + sizeY * 0.5f);	break;
		default: break;
	}
	
	return pos;
}