#include <assert.h>

#include "DebugMenu.h"
#include "FontManager.h"
#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Widget.h"

using namespace std;	//< For fstream operations

const char * Alignment::s_alignXNames[static_cast<int>(AlignX::Count)] = 
{
	"left",
	"middle",
	"right"
};
const char * Alignment::s_alignYNames[static_cast<int>(AlignY::Count)] =
{
	"top",
	"centre",
	"bottom"
};

const float Widget::sc_alignmentHandleSize = 0.05f;									///< How big to draw the alignment handles when selecting
const Colour Widget::sc_rolloverColour = Colour(0.25f, 0.25f, 0.25f, 0.5f);
const Colour Widget::sc_selectedColour = Colour(0.35f, 0.35f, 0.35f, 0.5f);
const Colour Widget::sc_editRolloverColour = Colour(0.05f, 0.2f, 0.2f, 0.2f);
const Colour Widget::sc_editSelectedColour = Colour(0.05f, 0.85f, 0.85f, 0.0f);

float Widget::s_selectedColourValue = 0.0f;	///< Value to attenuate a selected widget's colour with, only one selected widget ever

Widget::~Widget()
{
	// Delete list items owned by this widget
	auto nextItem = m_listItems.GetHead();
	while(nextItem != nullptr)
	{
		// Cache off working node
		auto curItem = nextItem;
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
		if (m_alignTo != nullptr)
		{
			drawPos = GetPositionRelative(m_alignTo);
		}
		m_drawPos = drawPos;

		// Determine draw style from selection
		Colour selectColour = m_colour;
		RenderManager & rMan = RenderManager::Get();
		RenderLayer renderLayer = m_debugRender ? RenderLayer::Debug2D : RenderLayer::Gui;
		switch (m_selection)
		{
			case SelectionFlags::Rollover:		if (m_action.IsSet()) { selectColour -= sc_rolloverColour * s_selectedColourValue;	} break;
			case SelectionFlags::Selected:		if (m_action.IsSet()) { selectColour -= sc_selectedColour * s_selectedColourValue;	} break;
			
			// Draw a selection box around a selected widget
			case SelectionFlags::EditRollover:
			{
				selectColour -= sc_editRolloverColour;
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
		if (m_texture != nullptr)
		{
			if (m_scissorRect.IsEqualZeroEpsilon())
			{
				rMan.AddQuad2D(renderLayer, drawPos, m_size.GetVector(), m_texture, TextureOrientation::Normal, selectColour);
			}
			else
			{
				TexCoord tPos(0.0f, 0.0f);
				TexCoord tSize(1.0f - m_scissorRect.GetX(), 1.0f - m_scissorRect.GetY());
				rMan.AddQuad2D(renderLayer, drawPos, m_size.GetVector() - m_scissorRect, m_texture, tPos, tSize, TextureOrientation::Normal, selectColour);
			}
		}

		// Draw fill colour for debug widgets
		if (IsDebugWidget())
		{
			Colour fillColour = selectColour * 0.3f;
			fillColour.SetA(0.95f);
			rMan.AddQuad2D(renderLayer, drawPos, m_size.GetVector(), m_texture, TextureOrientation::Normal, fillColour);
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
		auto nextItem = m_listItems.GetHead();
		Vector2 listDisplayPos = Vector2(drawPos.GetX() + fontDisplaySize, drawPos.GetY() - fontDisplaySize);
		Vector2 itemDisplayPos(listDisplayPos);
		unsigned int numItems = 0;
		while(nextItem != nullptr)
		{
			// If this is the selected item then draw the highlight bar behind the item
			if (numItems == m_selectedListItemId)
			{
				Vector2 hLightPos(listDisplayPos.GetX(), listDisplayPos.GetY() - (fontDisplaySize * numItems));
				Vector2 hLightSize(m_size.GetVector().GetX() - fontDisplaySize, fontDisplaySize);
				rMan.AddQuad2D(renderLayer, hLightPos, hLightSize, nullptr, TextureOrientation::Normal, selectColour - sc_rolloverColour);
			}
			else if (numItems == m_rolloverListItemId)
			{
				Vector2 hLightPos(listDisplayPos.GetX(), listDisplayPos.GetY() - (fontDisplaySize * numItems));
				Vector2 hLightSize(m_size.GetVector().GetX() - fontDisplaySize, fontDisplaySize);
				rMan.AddQuad2D(renderLayer, hLightPos, hLightSize, nullptr, TextureOrientation::Normal, selectColour - sc_selectedColour);
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
	while (curWidget != nullptr)
	{
		curWidget->GetData()->Draw();
		curWidget = curWidget->GetNext();
	}
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
	if (m_alignTo != nullptr)
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
		auto nextItem = m_listItems.GetHead();
		Vector2 listDisplayPos = Vector2(drawPos.GetX() + fontDisplaySize, drawPos.GetY() - fontDisplaySize);
		unsigned int numItems = 0;
		while(nextItem != nullptr)
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
	while (curWidget != nullptr)
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
	while (curWidget != nullptr)
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
	while (cur != nullptr)
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
	while (cur != nullptr)
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
	// Non search case
	if (strcmp(m_name, a_name) == 0)
	{
		return this;
	}

	if (m_children.IsEmpty())
	{
		return nullptr;
	}

	// Exhaustive search through child and siblings to find selected widget
	Widget * foundWidget = nullptr;
	WidgetNode * curWidget = m_children.GetHead();
	while (curWidget != nullptr && foundWidget == nullptr)
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

Widget * Widget::FindSelected()
{
	// Early out for this widget being selected
	if (IsSelected())
	{
		return this;
	}

	if (m_children.IsEmpty())
	{
		return nullptr;
	}

	// Exhaustive search through child and siblings to find selected widget
	WidgetNode * curWidget = m_children.GetHead();
	while (curWidget != nullptr)
	{
		if (curWidget->GetData()->IsSelected())
		{
			return curWidget->GetData();
		}
		// Recurse into children's family to find the widget
		if (!curWidget->GetData()->m_children.IsEmpty())
		{
			if (curWidget->GetData()->IsSelected())
			{
				return curWidget->GetData();
			}
		}
		// Keep searching the widget list
		curWidget = curWidget->GetNext();
	}
	return nullptr;
}

bool Widget::RemoveAlignmentTo(Widget * a_alignedTo)
{
	bool cleared = false;
	if (a_alignedTo == nullptr)
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
	while (cur != nullptr)
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
	if (a_child == nullptr)
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
	while (cur != nullptr && !removed)
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
	strncpy(m_alignToName, a_alignWidget->GetName(), StringUtils::s_maxCharsPerName);
}

void Widget::SetAlignTo(const char * a_alignWidgetName)
{
	strncpy(m_alignToName, a_alignWidgetName, StringUtils::s_maxCharsPerName);
	UpdateAlignTo();
}

void Widget::ClearAlignTo()
{
	m_alignTo = nullptr;
	m_alignToName[0] = '\0';
}

void Widget::UpdateAlignTo()
{
	// Lookup the align to widget if the name is set but the pointer isn't OR
	// if the name and the pointer mismatch
	if ( (m_alignTo == nullptr && m_alignToName[0] != '\0') || 
		 (m_alignTo != nullptr && m_alignToName[0] != '\0' && strcmp(m_alignTo->GetName(), m_alignToName) != 0))
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
	if (a_outputStream != nullptr)
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

		if (m_texture != nullptr)
		{
			menuStream << tabs << StringUtils::s_charTab << "texture: " << m_texture->GetFileName() << StringUtils::s_charLineEnd;
		}

		if (m_scriptFuncName[0] != '\0')
		{
			menuStream << tabs << StringUtils::s_charTab << "action: " << m_scriptFuncName << StringUtils::s_charLineEnd;
		}

		menuStream << tabs << "}" << StringUtils::s_charLineEnd;

		// Serialise any children of this child
		WidgetNode * curChild = m_children.GetHead();
		while (curChild != nullptr)
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
			while (curChild != nullptr)
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
	if (auto newListItem = new LinkedListNode<StringHash>())
	{
		newListItem->SetData(new StringHash(a_newItemName));
		m_listItems.Insert(newListItem);
	}
}

void Widget::RemoveListItem(const char * a_existingItemName)
{
	auto cur = m_listItems.GetHead();
	while(cur != nullptr)
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
	auto nextItem = m_listItems.GetHead();
	while(nextItem != nullptr)
	{
		// Remove all items by caching off the current pointer
		auto curItem = nextItem;
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
		return nullptr;
	}

	unsigned int numItems = 0;
	auto cur = m_listItems.GetHead();
	while(cur != nullptr)
	{
		// Remove item and quit out of the loop if found
		if (numItems == a_itemId)
		{
			return cur->GetData()->GetCString();
		}

		cur = cur->GetNext();
		++numItems;
	}

	return nullptr;
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

void Widget::MeasureText()
{ 
	FontManager::Get().MeasureString2D(m_text, m_fontNameHash, m_fontSize, m_textWidth, m_textHeight); 
}

#ifndef _RELEASE

bool Widget::GetAlignmentSelection(const Vector2 & a_selectionPos, const Vector2 & a_handleSize, Vector2 & a_handlePos_OUT, AlignX & a_xSelected_OUT, AlignY & a_ySelected_OUT)
{
	a_handlePos_OUT = m_drawPos;
	bool inSelectionX = false;
	bool inSelectionY = false;

	// Derive and test position of alignment anchor on this widget on horizontal
	if (a_selectionPos.GetX() >= m_drawPos.GetX() && a_selectionPos.GetX() <= m_drawPos.GetX() + a_handleSize.GetX())
	{
		inSelectionX = true;
		a_xSelected_OUT = AlignX::Left;
	}
	else if (a_selectionPos.GetX() >= m_drawPos.GetX() + (m_size.GetX() * 0.5f) - (a_handleSize.GetX()*0.5f) && a_selectionPos.GetX() <= m_drawPos.GetX() + (m_size.GetX() * 0.5f) + (a_handleSize.GetX()*0.5f))
	{
		inSelectionX = true;
		a_handlePos_OUT += Vector2((m_size.GetX() * 0.5f) - (a_handleSize.GetX()*0.5f), 0.0f);
		a_xSelected_OUT = AlignX::Middle;
	}
	else if (a_selectionPos.GetX() >= m_drawPos.GetX() + m_size.GetX() - a_handleSize.GetX() && a_selectionPos.GetX() <= m_drawPos.GetX() + m_size.GetX())
	{
		inSelectionX = true;
		a_handlePos_OUT += Vector2(m_size.GetX() - a_handleSize.GetX(), 0.0f);
		a_xSelected_OUT = AlignX::Right;
	}

	// Derive and test position of alignment anchor on this widget on vertical
	if (a_selectionPos.GetY() <= m_drawPos.GetY() && a_selectionPos.GetY() >= m_drawPos.GetY() - a_handleSize.GetY())
	{
		inSelectionY = true;
		a_ySelected_OUT = AlignY::Top;
	}
	else if (a_selectionPos.GetY() <= m_drawPos.GetY() - (m_size.GetY() * 0.5f) + (a_handleSize.GetY()*0.5f) && a_selectionPos.GetY() >= m_drawPos.GetY() - (m_size.GetY() * 0.5f) - (a_handleSize.GetY() * 0.5f))
	{
		inSelectionY = true;
		a_handlePos_OUT -= Vector2(0.0f, (m_size.GetY() * 0.5f) - (a_handleSize.GetY()*0.5f));
		a_ySelected_OUT = AlignY::Centre;
	}
	else if (a_selectionPos.GetY() <= m_drawPos.GetY() - m_size.GetY() + a_handleSize.GetY() && a_selectionPos.GetY() >= m_drawPos.GetY() - m_size.GetY())
	{
		inSelectionY = true;
		a_handlePos_OUT -= Vector2(0.0f, m_size.GetY() - a_handleSize.GetY());
		a_ySelected_OUT = AlignY::Bottom;
	}
	return inSelectionX && inSelectionY;
}

void Widget::DrawDebugAlignmentHandleSelectionBox(const Vector2 & a_topLeft, const Vector2 & a_handleSize)
{
	RenderManager & rMan = RenderManager::Get();
	Vector2 startVec = a_topLeft;
	Vector2 endVec = startVec + Vector2(a_handleSize.GetX(), 0.0f);		rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, sc_colourPurple);
	startVec = endVec;	endVec -= Vector2(0.0f, a_handleSize.GetY());	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, sc_colourPurple);
	startVec = endVec;	endVec -= Vector2(a_handleSize.GetX(), 0.0f);	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, sc_colourPurple);
	startVec = endVec;	endVec += Vector2(0.0f, a_handleSize.GetY());	rMan.AddLine2D(RenderLayer::Gui, startVec, endVec, sc_colourPurple);
}

void Widget::DrawDebugAlignment()
{
	InputManager & inMan = InputManager::Get();
	RenderManager & rMan = RenderManager::Get();
	const Vector2 handleSizeVec(sc_alignmentHandleSize, sc_alignmentHandleSize * rMan.GetViewAspect());
	const Vector2 halfSize = handleSizeVec * 0.5f;
	const Vector2 mousePos = inMan.GetMousePosRelative();

	// Detect mouse over handle for anchor and draw a box around it
	Vector2 selectedHandlePos = m_drawPos;
	AlignX selectionX = AlignX::Count;
	AlignY selectionY = AlignY::Count;
	if (GetAlignmentSelection(mousePos, handleSizeVec, selectedHandlePos, selectionX, selectionY))
	{
		DrawDebugAlignmentHandleSelectionBox(selectedHandlePos, handleSizeVec);
	}

	// Detect mouse over handle for parent alignment and draw a box around it
	Vector2 parentSelectedHandlePos = m_alignTo->m_drawPos;
	AlignX parentSelectionX = AlignX::Count;
	AlignY parentSelectionY = AlignY::Count;
	if (m_alignTo->GetAlignmentSelection(mousePos, handleSizeVec, parentSelectedHandlePos, parentSelectionX, parentSelectionY))
	{
		DrawDebugAlignmentHandleSelectionBox(parentSelectedHandlePos, handleSizeVec);
	}

	// Get position of the current anchor
	Vector2 alignPos = m_drawPos;
	switch (m_pos.m_alignAnchor.m_x)
	{
		case AlignX::Middle:		alignPos += Vector2((m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);	break;
		case AlignX::Right:			alignPos += Vector2(m_size.GetX() - handleSizeVec.GetX(), 0.0f);		break;
		default: break;
	}
	switch (m_pos.m_alignAnchor.m_y)
	{
		case AlignY::Centre:		alignPos -= Vector2(0.0f, (m_size.GetY() * 0.5f) - halfSize.GetY());	break;
		case AlignY::Bottom:		alignPos -= Vector2(0.0f, m_size.GetY() - handleSizeVec.GetY());		break;
		default: break;
	}

	// Draw a box around the current anchor
	DrawDebugAlignmentHandleSelectionBox(alignPos, handleSizeVec);
	
	// Get position alignment applies on the parent widget
	Vector2 parentAlignPos = m_alignTo->m_drawPos;
	switch(m_pos.m_align.m_x)
	{
		case AlignX::Middle:		parentAlignPos += Vector2((m_alignTo->m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);		break;
		case AlignX::Right:			parentAlignPos += Vector2(m_alignTo->m_size.GetX() - handleSizeVec.GetX(), 0.0f);			break;
		default: break;
	}
	switch (m_pos.m_align.m_y)
	{
		case AlignY::Centre:		parentAlignPos -= Vector2(0.0f, (m_alignTo->m_size.GetY() * 0.5f) - halfSize.GetY());		break;
		case AlignY::Bottom:		parentAlignPos -= Vector2(0.0f, m_alignTo->m_size.GetY() - handleSizeVec.GetY());			break;
		default: break;
	}

	// Draw a box around the active parent alignment
	DrawDebugAlignmentHandleSelectionBox(parentAlignPos, handleSizeVec);

	// Draw an arrow from anchor to parent
	const Vector2 invHalf = Vector2(halfSize.GetX(), -halfSize.GetY());
	rMan.AddDebugArrow2D(alignPos + invHalf, parentAlignPos + invHalf, sc_colourPurple);
	
	const Colour handleColour = sc_colourPurple - (0.25f, 0.25f, 0.25f, 0.5f);
	DrawDebugAlignmentHandles(handleSizeVec, handleColour, sc_colourPurple);
	m_alignTo->DrawDebugAlignmentHandles(handleSizeVec, handleColour, sc_colourPurple);
}

void Widget::DrawDebugAlignmentHandles(const Vector2 & a_handleSize, const Colour & a_drawColour, const Colour & a_selectedColour)
{
	// Show nine alignment anchors on this widget
	RenderManager & rMan = RenderManager::Get();
	const Vector2 halfSize = a_handleSize * 0.5f;
	Vector2 anchorPos = m_drawPos;
	Colour anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Left && m_pos.m_alignAnchor.m_y == AlignY::Top ? a_selectedColour : a_drawColour;
	rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);
	
	// Middle Top
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Middle && m_pos.m_alignAnchor.m_y == AlignY::Top ? a_selectedColour : a_drawColour;
	anchorPos += Vector2((m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);									rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);

	// Right Top
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Right && m_pos.m_alignAnchor.m_y == AlignY::Top ? a_selectedColour : a_drawColour;
	anchorPos += Vector2((m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);									rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);

	// Left Centre
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Left && m_pos.m_alignAnchor.m_y == AlignY::Centre ? a_selectedColour : a_drawColour;
	anchorPos = Vector2(m_drawPos.GetX(), m_drawPos.GetY() - (m_size.GetY() * 0.5f) + halfSize.GetY());		rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);

	// Middle Centre
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Middle && m_pos.m_alignAnchor.m_y == AlignY::Centre ? a_selectedColour : a_drawColour;
	anchorPos += Vector2((m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);									rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);

	// Right Centre
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Right && m_pos.m_alignAnchor.m_y == AlignY::Centre ? a_selectedColour : a_drawColour;
	anchorPos += Vector2((m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);									rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);

	// Left Bottom
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Left && m_pos.m_alignAnchor.m_y == AlignY::Bottom ? a_selectedColour : a_drawColour;
	anchorPos = Vector2(m_drawPos.GetX(), m_drawPos.GetY() - m_size.GetY() + a_handleSize.GetY());				rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);

	// Middle Bottom
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Middle && m_pos.m_alignAnchor.m_y == AlignY::Bottom ? a_selectedColour : a_drawColour;
	anchorPos += Vector2((m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);									rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);

	// Right Bottom
	anchorColour = m_pos.m_alignAnchor.m_x == AlignX::Right && m_pos.m_alignAnchor.m_y == AlignY::Bottom ? a_selectedColour : a_drawColour;
	anchorPos += Vector2((m_size.GetX() * 0.5f) - halfSize.GetX(), 0.0f);									rMan.AddDebugQuad2D(anchorPos, a_handleSize, anchorColour);
}

#endif
