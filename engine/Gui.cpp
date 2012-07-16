#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Gui.h"

using namespace Gui;

template<> GuiManager * Singleton<GuiManager>::s_instance = NULL;

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
	m_cursor.SetActive(true);

	return true;
}

bool Gui::GuiManager::Update(float a_dt)
{
	// Update mouse position
	m_cursor.SetPos(InputManager::Get().GetMousePosRelative()); 

	// Draw base level elements
	m_cursor.Draw();

	return true;
}
