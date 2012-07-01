#include "Log.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Gui.h"

using namespace Gui;

template<> GuiManager * Singleton<GuiManager>::s_instance = NULL;

void Gui::Widget::Draw()
{
	// Draw the quad in various states of activation
	RenderManager::Get().AddQuad2D(RenderManager::eBatchGui, m_pos.GetVector(), m_size.GetVector(), m_texture);
}

bool Gui::GuiManager::Startup(const char * a_guiPath)
{
	// Load in the gui texture
	char fileName[StringUtils::s_maxCharsPerLine];
	sprintf(fileName, "%s%s", a_guiPath, "gui.cfg");
	m_configFile.Load(fileName);

	// Setup the mouse cursor
	sprintf(fileName, "%s%s", a_guiPath, m_configFile.GetString("config", "mouseCursorTexture"));
	m_cursor.SetTexture(TextureManager::Get().GetTexture(fileName, TextureManager::eCategoryGui));
	m_cursor.SetPos(Vector2(0.0f, 0.0f));
	m_cursor.SetSize(Vector2(0.08f / RenderManager::Get().GetViewAspect(), 0.08f));

	return true;
}

bool Gui::GuiManager::Update(float a_dt)
{
	// Draw base level elements
	m_cursor.Draw();

	return true;
}

void Gui::GuiManager::SetMousePos(float a_x, float a_y) 
{ 
	// Calculate position relative to the screen size
	RenderManager & renderMan = RenderManager::Get();
	float relX = a_x / renderMan.GetViewWidth();
	float relY = a_y / renderMan.GetViewHeight();
	m_cursor.SetPos(Vector2(relX*2.0f-1.0f, 1.0f-relY*2.0f)); 
}