#include "../core/Colour.h"

#include "FontManager.h"
#include "InputManager.h"
#include "Log.h"
#include "RenderManager.h"
#include "Widget.h"

#include "DebugMenu.h"

template<> DebugMenu * Singleton<DebugMenu>::s_instance = NULL;

DebugMenu::DebugMenu()
: m_debugMenuRoot(NULL)
{
	if (!Startup())
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "DebugMenu failed to startup correctly!");
	}
}

bool DebugMenu::Startup()
{
	// Add a listener so the debug menu can be activated
	InputManager::Get().RegisterMouseCallback(this, &DebugMenu::OnActivate, InputManager::eMouseButtonRight);
	Gui & gui = Gui::Get();

	// Create the root of all debug menu items
	Widget::WidgetDef curItem;
	curItem.m_colour = sc_colourRed;
	curItem.m_size = WidgetVector(0.2f, 0.1f);
	curItem.m_fontNameHash = FontManager::Get().GetLoadedFontName("Arial")->GetHash();
	curItem.m_selectFlags = Widget::eSelectionRollover;
	curItem.m_name = "Create!";
	m_debugMenuRoot = gui.CreateWidget(curItem, Gui::Get().GetScreenWidget(), false);

	curItem.m_colour = sc_colourPurple;
	curItem.m_name = "Widget";
	m_debugMenuCreateWidget = gui.CreateWidget(curItem, m_debugMenuRoot, false);

	curItem.m_colour = sc_colourBlue;
	curItem.m_name = "GameObject";
	m_debugMenuCreateGameObject = gui.CreateWidget(curItem, m_debugMenuRoot, false);

	curItem.m_colour = sc_colourGrey;
	curItem.m_name = "Cancel";
	m_debugMenuCancel = gui.CreateWidget(curItem, m_debugMenuRoot, false);

	// Set callback so the debug menu child elements are shown
	if (m_debugMenuRoot == NULL ||
		m_debugMenuCancel == NULL ||
		m_debugMenuCreateWidget == NULL ||
		m_debugMenuCreateGameObject == NULL)
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Debug menu failed to start up correctly.");
		return false;
	}

	// All debug menu items share a handler
	m_debugMenuRoot->SetAction(this, &DebugMenu::OnMenuItemMouseUp);
	m_debugMenuCancel->SetAction(this, &DebugMenu::OnMenuItemMouseUp);
	m_debugMenuCreateWidget->SetAction(this, &DebugMenu::OnMenuItemMouseUp);
	m_debugMenuCreateGameObject->SetAction(this, &DebugMenu::OnMenuItemMouseUp);

	return true;
}

void DebugMenu::Update(float a_dt)
{

}

bool DebugMenu::OnMenuItemMouseUp(Widget * a_widget)
{
	// Check the widget that was activated matches and we don't have other menus up
	if (a_widget == m_debugMenuRoot)
	{
		// Show menu options on the right of the menu
		WidgetVector right = m_debugMenuRoot->GetPos() + WidgetVector(m_debugMenuRoot->GetSize().GetX(), -m_debugMenuRoot->GetSize().GetY());
		WidgetVector height = m_debugMenuRoot->GetSize();
		height.SetX(0.0f);
		m_debugMenuCreateWidget->SetPos(right);
		m_debugMenuCreateGameObject->SetPos(right + height);
		height.SetY(height.GetY() - m_debugMenuRoot->GetSize().GetY() * 2.0f);
		m_debugMenuCancel->SetPos(right + height);
		m_debugMenuCancel->SetActive(true);
		m_debugMenuCreateWidget->SetActive(true);
		m_debugMenuCreateGameObject->SetActive(true);
		return true;
	}
	else if (a_widget == m_debugMenuCreateWidget)
	{
		Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "TODO! Create new GUI widget.");
	}
	else if (a_widget == m_debugMenuCreateGameObject)
	{
		Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "TODO! Create new game object.");
	}
	else // Cancel all menu display
	{
		m_debugMenuRoot->SetActive(false);
		m_debugMenuCancel->SetActive(false);
		m_debugMenuCreateWidget->SetActive(false);
		m_debugMenuCreateGameObject->SetActive(false);
	}
	return false;
}

bool DebugMenu::OnMenuItemMouseOver(Widget * a_widget)
{
	//TODO
	return true;
}

bool DebugMenu::OnActivate(bool a_active)
{
	// Set the creation root element to visible if it isn't already
	if (!m_debugMenuRoot->IsActive())
	{
		InputManager & inMan = InputManager::Get();
		m_debugMenuRoot->SetPos(inMan.GetMousePosRelative());
		m_debugMenuRoot->SetActive(a_active);
	}

	return true;
}