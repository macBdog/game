#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Widget.h"

using namespace std;	//< For fstream operations

const Colour Widget::sc_rolloverColour = Colour(0.25f, 0.25f, 0.25f, 0.5f);
const Colour Widget::sc_selectedColour = Colour(0.35f, 0.35f, 0.35f, 0.5f);
const Colour Widget::sc_editRolloverColour = Colour(0.55f, 0.25f, 0.25f, 0.5f);
const Colour Widget::sc_editSelectedColour = Colour(1.0f, 0.15f, 1.0f, 0.75f);

void Widget::Draw()
{
	if (m_active && !(IsDebugWidget() && !DebugMenu::Get().IsDebugMenuEnabled()))
	{
		// Determine colour from selection
		Colour selectColour = m_colour;
		switch (m_selection)
		{
			case eSelectionRollover:		selectColour += sc_rolloverColour;		break;
			case eSelectionEditRollover:	selectColour += sc_editRolloverColour;	break;
			case eSelectionSelected:		selectColour += sc_selectedColour;		break;
			case eSelectionEditSelected:	selectColour += sc_editSelectedColour;	break;
			default: break;
		}

		// Draw the quad in various states of activation
		RenderManager::eBatch batch = m_debugRender ? RenderManager::eBatchDebug : RenderManager::eBatchGui;
		if (m_texture != NULL)
		{
			RenderManager::Get().AddQuad2D(batch, m_pos.GetVector(), m_size.GetVector(), m_texture);
		}
		else // No texture version
		{
			RenderManager::Get().AddQuad2D(batch, m_pos.GetVector(), m_size.GetVector(), NULL, Texture::eOrientationNormal, selectColour);
		}

		// Draw gui label on top of the widget
		if (m_fontNameHash > 0)
		{
			FontManager::Get().DrawString(m_name, m_fontNameHash, 1.0f, m_pos.GetVector(), m_colour, IsDebugWidget() ? RenderManager::eBatchDebug : RenderManager::eBatchGui);
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

	// Check if the position is inside the widget
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
	// In the case of no preexisitng children
	if (m_childWidget == NULL)
	{
		m_childWidget = a_child;
	}
	else // Otherwise append to the list of children
	{
		Widget * nextChild = m_childWidget;
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
	// In the case of no existing siblings
	if (m_nextWidget == NULL)
	{
		m_nextWidget = a_sibling;
	}
	else
	{
		// Add to the head of the list (newest get drawn first)
		Widget * nextChild = m_nextWidget;
		while (nextChild != NULL)
		{
			// Check for the end of the list
			if (nextChild->GetNext() == NULL)
			{
				nextChild->m_nextWidget = a_sibling;
				return;
			}
			else
			{
				nextChild = nextChild->GetNext();
			}
		}
	}
}

void Widget::Activate() 
{ 
	// Check then call the callback
	if (!m_action.IsSet())
	{
		m_action.Execute(this);
	}
	else
	{
		Log::Get().Write(Log::LL_WARNING, Log::LC_ENGINE, "Widget named %s does not have an action set.", GetName());
	}
}

void Widget::Serialise(std::ofstream * a_outputStream, unsigned int a_indentCount)
{
	char outBuf[StringUtils::s_maxCharsPerName];
	const char * lineEnd = "\n";
	const char * tab = "\t";

	// Can only write if 
	if (a_outputStream != NULL)
	{
		// Generate the correct tab amount
		char tabs[StringUtils::s_maxCharsPerName];
		memset(&tabs[0], '\t', a_indentCount);
		tabs[a_indentCount] = '\0';

		std::ofstream & menuStream = *a_outputStream;
		menuStream << tabs << "widget" << lineEnd;
		menuStream << tabs << "{" << lineEnd;
		menuStream << tabs << tab << "name: "	<< m_name				<< lineEnd;
		
		m_pos.GetString(outBuf);
		menuStream << tabs << tab << "pos: "	<< outBuf	<< lineEnd;

		m_size.GetString(outBuf);
		menuStream << tabs << tab <<"size: "	<< outBuf	<< lineEnd;
		menuStream << tabs << "}" << lineEnd;

		// Serialise any children of this child
		Widget * nextChild = m_childWidget;
		while (nextChild != NULL)
		{
			nextChild->Serialise(a_outputStream, ++a_indentCount);
			nextChild = nextChild->GetNext();
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
			menuOutput << "menu"	<< lineEnd;
			menuOutput << "{"		<< lineEnd;
			menuOutput << tab		<< "name: "		<< m_name << lineEnd;
			
			// Write all children out
			Widget * nextChild = m_childWidget;
			while (nextChild != NULL)
			{
				nextChild->Serialise(&menuOutput, 1);
				nextChild = nextChild->GetNext();
			}

			menuOutput << "}" << lineEnd;
		}

		menuOutput.close();
	}
}