#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Widget.h"

const Colour Widget::sc_rolloverColour = Colour(0.35f, 0.35f, 0.35f, 0.5f);

void Widget::Draw()
{
	if (m_active && !(IsDebugWidget() && !DebugMenu::Get().IsDebugMenuEnabled()))
	{
		// Determine colour from selection
		Colour selectColour = m_colour;
		switch (m_selection)
		{
			case eSelectionRollover: selectColour += sc_rolloverColour; break;
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

		// Draw gui label
		if (m_fontNameHash > 0)
		{
			FontManager::Get().DrawString(m_name, m_fontNameHash, 1.0f, m_pos.GetVector(), selectColour);
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
			m_selection = eSelectionRollover;
		}
	}
	else // Selection position not inside bounds
	{
		m_selection = eSelectionNone;
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
	bool activated = false;
	if (IsSelected())
	{
		// Perform whatever the widget's function is
		Activate();
		activated = true;
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