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
	InputManager::Get().RegisterMouseCallback(this, &DebugMenu::OnActivate, InputManager::eMouseButtonMiddle);

	// Create the root of all debug menu items
	Widget::WidgetDef root;
	root.m_colour = sc_colourGrey;
	root.m_size = WidgetVector(0.2f, 0.1f);
	root.m_fontNameHash = FontManager::Get().GetLoadedFontName("Arial")->GetHash();
	root.m_selectFlags = Widget::eSelectionRollover;
	root.m_name = "Create!";

	// Add it to the gui manager to start deactivated
	m_debugMenuRoot = Gui::Get().CreateWidget(root, Gui::Get().GetScreenWidget(), false);

	// Set callback so the debug menu chuild elements are shown
	if (m_debugMenuRoot == NULL)
	{
		return false;
	}

	m_debugMenuRoot->SetAction(this, &DebugMenu::OnMenuItemMouseUp);
	return true;
}

void DebugMenu::Update(float a_dt)
{

}

bool DebugMenu::OnMenuItemMouseUp(Widget * a_widget)
{
	// Check the widget that was activated matches
	if (m_debugMenuRoot == a_widget)
	{
		// TODO: Activate the child menu elements
		Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "Debug menu options go here!");
		a_widget->SetActive(false);
		return true;
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
	// Set the creation root element  to visible
	InputManager & inMan = InputManager::Get();
	m_debugMenuRoot->SetPos(inMan.GetMousePosRelative());
	m_debugMenuRoot->SetActive(a_active);

	return true;
}