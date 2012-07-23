#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Widget.h"

const Colour Widget::sc_rolloverColour = Colour(0.35f, 0.35f, 0.35f, 0.5f);

void Widget::Draw()
{
	if (m_active)
	{
		// Determine colour from selection
		Colour selectColour = m_colour;
		switch (m_selection)
		{
			case eSelectionRollover: selectColour += sc_rolloverColour; break;
			default: break;
		}

		// Draw the quad in various states of activation
		if (m_texture != NULL)
		{
			RenderManager::Get().AddQuad2D(RenderManager::eBatchGui, m_pos.GetVector(), m_size.GetVector(), m_texture);
		}
		else // No texture version
		{
			RenderManager::Get().AddQuad2D(RenderManager::eBatchGui, m_pos.GetVector(), m_size.GetVector(), NULL, Texture::eOrientationNormal, selectColour);
		}

		// Draw gui label
		if (m_fontNameHash > 0)
		{
			FontManager::Get().DrawString(m_name, m_fontNameHash, 1.0f, m_pos.GetVector());
		}
	}

	// Draw any children
	if (m_childWidget)
	{
		m_childWidget->Draw();
	}
}

bool Widget::UpdateSelection(WidgetVector a_pos)
{
	// Check if the position is inside the widget
	if (a_pos.GetX() >= m_pos.GetX() && a_pos.GetX() <= m_pos.GetX() + m_size.GetX() && 
		a_pos.GetY() <= m_pos.GetY() && a_pos.GetY() >= m_pos.GetY() - m_size.GetY())
	{
		// Check selection flags
		if ((m_selectFlags & eSelectionRollover) > 0)
		{
			m_selection = eSelectionRollover;
			return true;
		}
	}

	m_selection = eSelectionNone;
	return false;
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
				nextChild->AddSibling(a_sibling);
				return;
			}
			else
			{
				nextChild = nextChild->GetNext();
			}
		}
	}
}