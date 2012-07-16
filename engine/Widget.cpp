#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Widget.h"

using namespace Gui;

void Widget::Draw()
{
	if (m_active)
	{
		// Draw the quad in various states of activation
		if (m_texture != NULL)
		{
			RenderManager::Get().AddQuad2D(RenderManager::eBatchGui, m_pos.GetVector(), m_size.GetVector(), m_texture);
		}
		else // No texture version
		{
			RenderManager::Get().AddQuad2D(RenderManager::eBatchGui, m_pos.GetVector(), m_size.GetVector(), NULL, Texture::eOrientationNormal, m_colour);
		}

		// Draw gui label
		if (m_fontName != NULL)
		{
			FontManager::Get().DrawString(m_name, m_fontName, m_size.GetY(), m_pos.GetVector());
		}
	}
}
