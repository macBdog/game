#include <assert.h>

#include "DebugMenu.h"
#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Widget.h"

using namespace std;	//< For fstream operations

const char * WidgetVector::s_alignXNames[eAlignXCount] = 
{
	"left",
	"middle",
	"right"
};
const char * WidgetVector::s_alignYNames[eAlignYCount] =
{
	"top",
	"centre",
	"bottom"
};

const Colour Widget::sc_rolloverColour = Colour(0.25f, 0.25f, 0.25f, 0.5f);
const Colour Widget::sc_selectedColour = Colour(0.35f, 0.35f, 0.35f, 0.5f);
const Colour Widget::sc_editRolloverColour = Colour(0.05f, 0.2f, 0.2f, 0.2f);
const Colour Widget::sc_editSelectedColour = Colour(0.05f, 0.85f, 0.85f, 0.0f);

Widget::~Widget()
{
	// Iterate through all items and deletes and deallocs
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
		// Determine draw style from selection
		Colour selectColour = m_colour;
		RenderManager & rMan = RenderManager::Get();
		RenderManager::eBatch batch = m_debugRender ? RenderManager::eBatchDebug2D : RenderManager::eBatchGui;
		switch (m_selection)
		{
			case eSelectionRollover:		selectColour -= sc_rolloverColour;		break;
			case eSelectionSelected:		selectColour -= sc_selectedColour;		break;
			
			// Draw a selection box around a selected
			case eSelectionEditRollover:	
			case eSelectionEditSelected:
			{
				selectColour -= sc_editSelectedColour;
				const float extraSelectionSize = 0.05f;
				const Vector2 selectSize = m_size + (m_size * extraSelectionSize);
				Vector2 startVec = m_pos.GetVector();
				startVec.SetX(startVec.GetX() - (selectSize.GetX() - m_size.GetX()) * 0.5f);
				startVec.SetY(startVec.GetY() + (selectSize.GetY() - m_size.GetY()) * 0.5f);
				Vector2 endVec = Vector2(startVec.GetX() + selectSize.GetX(), startVec.GetY());  rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, selectColour);
				startVec = endVec;	endVec -= Vector2(0.0f, selectSize.GetY());	rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, selectColour);
				startVec = endVec;	endVec -= Vector2(selectSize.GetX(), 0.0f);	rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, selectColour);
				startVec = endVec;	endVec += Vector2(0.0f, selectSize.GetY());	rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, selectColour);	
				break;
			}
			default: break;
		}

		// Draw the quad in various states of activation
		if (m_texture != NULL)
		{
			rMan.AddQuad2D(batch, m_pos.GetVector(), m_size.GetVector(), m_texture, Texture::eOrientationNormal, selectColour);
		}

		// Draw fill colour for debug widgets
		if (IsDebugWidget())
		{
			Colour fillColour = selectColour * 0.3f;
			fillColour.SetA(0.95f);
			rMan.AddQuad2D(batch, m_pos.GetVector(), m_size.GetVector(), NULL, Texture::eOrientationNormal, fillColour);
		}

		// Draw widget bounds for editing mode or for debug widgets
		if (DebugMenu::Get().IsDebugMenuEnabled() || IsDebugWidget())
		{
			Colour boxColour = selectColour;
			boxColour.SetA(0.1f);
			Vector2 startVec = m_pos.GetVector();
			Vector2 endVec = Vector2(startVec.GetX() + m_size.GetX(), startVec.GetY());
			rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, boxColour);
			
			startVec = endVec;	endVec -= Vector2(0.0f, m_size.GetY());	rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, boxColour);
			startVec = endVec;	endVec -= Vector2(m_size.GetX(), 0.0f);	rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, boxColour);
			startVec = endVec;	endVec += Vector2(0.0f, m_size.GetY());	rMan.AddLine2D(RenderManager::eBatchGui, startVec, endVec, boxColour);
		}

		// Draw gui label on top of the widget in editing mode
		const float fontDisplaySize = 0.07f;
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			if (m_fontNameHash > 0)
			{
				FontManager::Get().DrawDebugString2D(m_name, m_pos.GetVector(), m_colour, batch);

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
						FontManager::Get().DrawDebugString2D(cursorText, Vector2(m_pos.GetX() + fontDisplaySize, m_pos.GetY() - fontDisplaySize), m_colour, batch);
					}
				}
			}	
		} 

		// Always display text for a widget
		if (m_text[0] != '\0') 
		{
			FontManager::Get().DrawString(m_text, m_fontNameHash, m_fontSize, m_pos, m_colour, batch);
		}

		// Draw any list items
		LinkedListNode<StringHash> * nextItem = m_listItems.GetHead();
		Vector2 listDisplayPos = Vector2(m_pos.GetX() + fontDisplaySize, m_pos.GetY() - fontDisplaySize);
		Vector2 itemDisplayPos(listDisplayPos);
		unsigned int numItems = 0;
		while(nextItem != NULL)
		{
			// If this is the selected item then draw the highlight bar behind the item
			if (numItems == m_selectedListItemId)
			{
				Vector2 hLightPos(listDisplayPos.GetX(), listDisplayPos.GetY() - (fontDisplaySize * numItems));
				Vector2 hLightSize(m_size.GetVector().GetX() - fontDisplaySize, fontDisplaySize);
				rMan.AddQuad2D(batch, hLightPos, hLightSize, NULL, Texture::eOrientationNormal, selectColour - sc_rolloverColour);
			}

			// Now draw the item text
			FontManager::Get().DrawString(nextItem->GetData()->GetCString(), m_fontNameHash, 1.0f, itemDisplayPos, m_colour, batch);
			itemDisplayPos.SetY(itemDisplayPos.GetY() - fontDisplaySize);

			nextItem = nextItem->GetNext();
			++numItems;
		}
	}

	// Draw any sibling widgets
	Widget * curWidget = GetNext();
	while (curWidget != NULL)
	{
		curWidget->Draw();
		curWidget = curWidget->GetNext();
	}

	// Draw any child widgets
	curWidget = GetChild();
	while (curWidget != NULL)
	{
		curWidget->Draw();
		curWidget = curWidget->GetChild();
	}
}

void Widget::UpdateSelection(WidgetVector a_pos)
{
	// Don't select debug menu elements if we aren't in debug mode
	if (IsDebugWidget() && !DebugMenu::Get().IsDebugMenuEnabled())
	{
		m_selection = eSelectionNone;
		return;
	}

	// Check if the mouse position is inside the widget
	if (a_pos.GetX() >= m_pos.GetX() && a_pos.GetX() <= m_pos.GetX() + m_size.GetX() && 
		a_pos.GetY() <= m_pos.GetY() && a_pos.GetY() >= m_pos.GetY() - m_size.GetY())
	{
		// Check selection flags
		if ((m_selectFlags & eSelectionRollover) > 0)
		{
			if (DebugMenu::Get().IsDebugMenuEnabled())
			{
				if (m_selection <= eSelectionEditRollover)
				{
					m_selection = eSelectionEditRollover;
				}
			}
			else
			{
				m_selection = eSelectionRollover;
			}
		}
	}
	else // Selection position not inside bounds
	{
		// Don't unselect a selection by mouseover
		if (m_selection != eSelectionSelected && 
			m_selection != eSelectionEditSelected)
		{
			m_selection = eSelectionNone;
		}
	}

	// Update selection of list items
	if (m_selection > eSelectionNone)
	{
		const float fontDisplaySize = 0.07f;
		LinkedListNode<StringHash> * nextItem = m_listItems.GetHead();
		Vector2 listDisplayPos = Vector2(m_pos.GetX() + fontDisplaySize, m_pos.GetY() - fontDisplaySize);
		unsigned int numItems = 0;
		while(nextItem != NULL)
		{
			Vector2 testPos(listDisplayPos.GetX(), listDisplayPos.GetY() - (fontDisplaySize * numItems));
			Vector2 testSize(m_size.GetVector().GetX() - fontDisplaySize, fontDisplaySize);
			if (a_pos.GetX() >= testPos.GetX() && a_pos.GetX() <= testPos.GetX() + testSize.GetX() && 
				a_pos.GetY() <= testPos.GetY() && a_pos.GetY() >= testPos.GetY() - testSize.GetY())
			{
				m_selectedListItemId = numItems;
				break;
			}

			nextItem = nextItem->GetNext();
			numItems++;
		}
	}

	// Update selection on any sibling widgets
	Widget * curWidget = GetNext();
	while (curWidget != NULL)
	{
		curWidget->UpdateSelection(a_pos);
		curWidget = curWidget->GetNext();
	}

	// Draw any child widgets
	curWidget = GetChild();
	while (curWidget != NULL)
	{
		curWidget->UpdateSelection(a_pos);
		curWidget = curWidget->GetChild();
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
	eSelectionFlags flags = debug.IsDebugMenuEnabled() ? eSelectionEditRollover : eSelectionRollover;
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
	
	// Activate only the next sibling widget
	if (Widget * curWidget = GetNext())
	{
		activated &= curWidget->DoActivation();
	}

	// Activate any child widgets
	if (Widget * curWidget = GetChild())
	{
		activated &= curWidget->DoActivation();
	}

	return activated;
}

bool Widget::IsSelected(eSelectionFlags a_selectMode)
{ 
	// TODO Should be a bittest with every selection flag
	return m_selection == a_selectMode;
}

void Widget::AddChild(Widget * a_child)
{
	// I'm your daddy
	a_child->m_parent = this;

	// In the case of no preexisitng children
	if (m_firstChild == NULL)
	{
		m_firstChild = a_child;
	}
	else // Otherwise append to the list of children
	{
		Widget * nextChild = m_firstChild;
		while (nextChild != NULL)
		{
			// Check for the end of the list
			if (nextChild->GetNext() == NULL)
			{
				nextChild->AddSibling(a_child);
				return;
			}
			else
			{
				nextChild = nextChild->GetNext();
			}
		}
	}
}

void Widget::AddSibling(Widget * a_sibling)
{
	// Your daddy is my daddy
	a_sibling->m_parent = m_parent;

	// In the case of no existing siblings
	if (m_next == NULL)
	{
		m_next = a_sibling;
	}
	else
	{
		// Add to the head of the list (newest get drawn first)
		Widget * nextChild = m_next;
		while (nextChild != NULL)
		{
			// Check for the end of the list
			if (nextChild->GetNext() == NULL)
			{
				nextChild->m_next = a_sibling;
				return;
			}
			else
			{
				nextChild = nextChild->GetNext();
			}
		}
	}
}

void Widget::Orphan()
{
	// Unlink from parents and siblings (disowned)
	if (m_parent != NULL)
	{
		// If first child
		if (m_parent->m_firstChild == this)
		{
			m_parent->m_firstChild = m_next;
		}
		else
		{
			Widget * sibling = m_parent->m_firstChild;
			while (sibling != NULL)
			{
				if (sibling->m_next == this)
				{
					sibling->m_next = sibling->m_next->m_next;
					return;
				}
				sibling = sibling->m_next;
			}
			// Couldn't find myself in the list of siblings, something is wrong
			assert(false);
		}
	}
}

Widget * Widget::Find(const char * a_name)
{
	// Non search cases
	if (strcmp(m_name, a_name) == 0)
	{
		return this;
	}
	if (m_firstChild == NULL)
	{
		return NULL;
	}

	// Exhaustive search through child and siblings to find widget with name
	Widget * foundWidget = NULL;
	Widget * curWidget = m_firstChild;
	while (curWidget != NULL && foundWidget == NULL)
	{
		if (strcmp(curWidget->GetName(), a_name) == 0)
		{
			return curWidget;
		}
		// Recurse into children's family to find the widget
		if (curWidget->m_firstChild != NULL)
		{
			foundWidget = curWidget->Find(a_name);
		}
		// Keep searching the widget list
		curWidget = curWidget->GetNext();
	}
	return foundWidget;
}

void Widget::Activate() 
{ 
	// Check then call the callback
	if (!m_action.IsSet())
	{
		m_action.Execute(this);
	}
	else if (!m_debugRender)
	{
		Log::Get().Write(Log::LL_WARNING, Log::LC_ENGINE, "Widget named %s does not have an action set.", GetName());
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

		m_pos.GetAlignment().GetStringX(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "alignX: "	<< outBuf	<< StringUtils::s_charLineEnd;

		m_pos.GetAlignment().GetStringY(outBuf);
		menuStream << tabs << StringUtils::s_charTab << "alignY: "	<< outBuf	<< StringUtils::s_charLineEnd;

		if (m_texture != NULL)
		{
			menuStream << tabs << StringUtils::s_charTab << "texture: " << m_texture->GetFileName() << StringUtils::s_charLineEnd;
		}

		menuStream << tabs << "}" << StringUtils::s_charLineEnd;

		// Serialise any siblings of this element at this indentation level
		if (m_next != NULL)
		{
			m_next->Serialise(a_outputStream, a_indentCount);
		}

		// Serialise any children of this child
		if (m_firstChild != NULL)
		{
			m_firstChild->Serialise(a_outputStream, ++a_indentCount);
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
			if (m_script[0] != '\0')
			{
				menuOutput << StringUtils::s_charTab		<< "script: "	<< m_script << StringUtils::s_charLineEnd;
			}
			
			// Write the siblings out
			if (m_next != NULL)
			{
				m_next->Serialise(&menuOutput, 1);
			}

			// Write all children out
			if (m_firstChild != NULL)
			{
				m_firstChild->Serialise(&menuOutput, 1);
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
		Log::Get().Write(Log::LL_WARNING, Log::LC_ENGINE, "Trying to get an invalid list item %d from widget called %s.", a_itemId, m_name);
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