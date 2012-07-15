#include "../core/Colour.h"

#include "FontManager.h"
#include "InputManager.h"
#include "RenderManager.h"

#include "DebugMenu.h"

template<> DebugMenu * Singleton<DebugMenu>::s_instance = NULL;

void DebugMenu::Startup()
{
	// Setup input callbacks for handling mouse clicks 
	InputManager & inMan = InputManager::Get();
	inMan.RegisterMouseCallback(this, InputManager::eMouseButtonMiddle);

	// Setup properties for the debug menu options
	m_creationMenu.m_widget.SetColour(sc_colourGrey);
	m_creationMenu.m_widget.SetPos(Vector2(0.0f, 0.0f));
	m_creationMenu.m_widget.SetSize(1.0f);
	m_creationMenu.m_widget.SetActive(false);
	m_creationMenu.m_widget.SetFontName(FontManager::Get().GetLoadedFontName("Arial"));
	m_creationMenu.m_widget.SetName("Create!");
}

void DebugMenu::Update(float a_dt)
{
	m_creationMenu.m_widget.Draw();
}

void DebugMenu::OnMenuItemMouseUp(/*Gui::Widget * a_widget*/)
{
	// Set the creation root element  to visible
	InputManager & inMan = InputManager::Get();
	m_creationMenu.m_widget.SetPos(inMan.GetMousePosRelative());
	m_creationMenu.m_widget.SetActive();
}

void DebugMenu::OnMenuItemMouseOver(/*Gui::Widget * a_widget*/)
{

}
