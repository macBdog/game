#include "../core/Colour.h"

#include "FontManager.h"
#include "InputManager.h"
#include "Log.h"
#include "RenderManager.h"
#include "Widget.h"

#include "DebugMenu.h"

template<> DebugMenu * Singleton<DebugMenu>::s_instance = NULL;

// Cursor definitions
const float DebugMenu::sc_cursorSize = 0.075f;
Vector2 DebugMenu::sc_vectorCursor[4] = 
{ 
	Vector2(0.0f, 0.0f),
	Vector2(sc_cursorSize, -sc_cursorSize),
	Vector2(sc_cursorSize*0.3f, -sc_cursorSize*0.7f),
	Vector2(0.0f, -sc_cursorSize)
};

DebugMenu::DebugMenu()
: m_enabled(false) 
, m_widgetToEdit(NULL)
, m_btnCreateRoot(NULL)
, m_btnCancel(NULL)
, m_btnCreateWidget(NULL)
, m_btnCreateGameObject(NULL)
, m_btnChangeRoot(NULL)
, m_btnChangePos(NULL)
, m_btnChangeShape(NULL)
, m_btnChangeType(NULL)
, m_btnChangeTexture(NULL)
{
	if (!Startup())
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "DebugMenu failed to startup correctly!");
	}
}

bool DebugMenu::Startup()
{
	// Add a ketboard listener so the debug menu can be enabled
	InputManager & inMan = InputManager::Get();
	inMan.RegisterKeyCallback(this, &DebugMenu::OnEnable, SDLK_TAB);

	// Add a listener so the debug menu can be activated
	inMan.RegisterMouseCallback(this, &DebugMenu::OnActivate, InputManager::eMouseButtonRight);
	inMan.RegisterMouseCallback(this, &DebugMenu::OnSelect, InputManager::eMouseButtonLeft);
	Gui & gui = Gui::Get();

	// Create the root of all debug menu items
	m_btnCreateRoot = CreateButton("Create!", sc_colourRed, gui.GetDebugRoot());
	m_btnCancel = CreateButton("Cancel", sc_colourGrey, m_btnCreateRoot);
	m_btnCreateWidget = CreateButton("Widget", sc_colourPurple, m_btnCreateRoot);
	m_btnCreateGameObject = CreateButton("GameObject", sc_colourBlue, m_btnCreateRoot);
	m_btnChangeRoot = CreateButton("Change", sc_colourRed, gui.GetDebugRoot());
	m_btnChangePos = CreateButton("Position", sc_colourPurple, m_btnChangeRoot);
	m_btnChangeShape = CreateButton("Shape", sc_colourBlue, m_btnChangeRoot);
	m_btnChangeType = CreateButton("Type", sc_colourOrange, m_btnChangeRoot);
	m_btnChangeTexture = CreateButton("Texture", sc_colourYellow, m_btnChangeRoot);

	// Process vector cursors for display aspect
	for (unsigned int i = 0; i < 4; ++i)
	{
		sc_vectorCursor[i].SetY(sc_vectorCursor[i].GetY() * RenderManager::Get().GetViewAspect());
	}
	return true;
}

void DebugMenu::Update(float a_dt)
{
	Draw();
}

bool DebugMenu::OnMenuItemMouseUp(Widget * a_widget)
{
	// Do nothing if the debug menu isn't enabled
	if (!IsDebugMenuEnabled())
	{
		return false;
	}
	
	// Check the widget that was activated matches and we don't have other menus up
	Gui & gui = Gui::Get();
	if (a_widget == m_btnCreateRoot)
	{
		// Show menu options on the right of the menu
		WidgetVector right = m_btnCreateRoot->GetPos() + WidgetVector(m_btnCreateRoot->GetSize().GetX(), -m_btnCreateRoot->GetSize().GetY());
		WidgetVector height = m_btnCreateRoot->GetSize();
		height.SetX(0.0f);
		m_btnCreateWidget->SetPos(right);
		m_btnCreateGameObject->SetPos(right + height);
		height.SetY(height.GetY() - m_btnCreateRoot->GetSize().GetY() * 2.0f);
		m_btnCancel->SetPos(right + height);
		
		ShowCreateMenu(true);
		return true;
	}
	else if (a_widget == m_btnCreateWidget)
	{
		// Make a new widget
		Widget::WidgetDef curItem;
		curItem.m_colour = sc_colourWhite;
		curItem.m_size = WidgetVector(0.35f, 0.35f);
		curItem.m_fontNameHash = FontManager::Get().GetLoadedFontName("Arial")->GetHash();
		curItem.m_selectFlags = Widget::eSelectionRollover;
		curItem.m_name = "NEW_WIDGET";

		// TODO: The parent of this should be the screen that is currently being worked in
		Widget * parentWidget = m_widgetToEdit != NULL ? m_widgetToEdit : gui.GetRootWidget();
		Widget * newWidget = Gui::Get().CreateWidget(curItem, parentWidget);
		newWidget->SetPos(InputManager::Get().GetMousePosRelative());
		
		// Cancel menu display
		ShowCreateMenu(false);
	}
	else if (a_widget == m_btnCreateGameObject)
	{
		Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "TODO! Create new game object.");
	}
	else if (a_widget == m_btnCancel)
	{
		// Cancel all menu display
		ShowCreateMenu(false);
		ShowChangeMenu(false);
	}
	else if (a_widget == m_btnChangeRoot)
	{
		// Show menu options on the right of the menu
		WidgetVector right = m_btnChangeRoot->GetPos() + WidgetVector(m_btnChangeRoot->GetSize().GetX(), -m_btnChangeRoot->GetSize().GetY());
		WidgetVector height = m_btnChangeRoot->GetSize();
		height.SetX(0.0f);
		m_btnChangePos->SetPos(right);
		m_btnChangeShape->SetPos(right + height);

		height.SetY(height.GetY() - m_btnChangeRoot->GetSize().GetY() * 2.0f);
		m_btnChangeType->SetPos(right + height);

		height.SetY(height.GetY() - m_btnChangeRoot->GetSize().GetY());
		m_btnChangeTexture->SetPos(right + height);

		height.SetY(height.GetY() - m_btnChangeRoot->GetSize().GetY());
		m_btnCancel->SetPos(right + height);

		ShowChangeMenu(true);
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
	// Do nothing if the debug menu isn't enabled
	if (!m_enabled)
	{
		return false;
	}

	// Set the creation root element to visible if it isn't already
	InputManager & inMan = InputManager::Get();
	if (m_widgetToEdit != NULL)
	{
		if (!IsDebugMenuActive())
		{
			m_btnChangeRoot->SetPos(inMan.GetMousePosRelative());
			m_btnChangeRoot->SetActive(a_active);
		}
	}
	else if (!m_btnCreateRoot->IsActive())
	{
		m_btnCreateRoot->SetPos(inMan.GetMousePosRelative());
		m_btnCreateRoot->SetActive(a_active);
	}

	return true;
}

bool DebugMenu::OnSelect(bool a_active)
{
	// Cancle previous selection
	if (!IsDebugMenuActive())
	{
		if (m_widgetToEdit != NULL)
		{
			m_widgetToEdit->SetSelection(Widget::eSelectionNone);
			m_widgetToEdit = NULL;
		}
	}

	// Find the first widget that is rolled over in edit mode
	if (Widget * newEditedWidget = Gui::Get().GetActiveWidget())
	{
		m_widgetToEdit = newEditedWidget;
		m_widgetToEdit->SetSelection(Widget::eSelectionEditSelected);
		return true;
	}

	return false;
}

bool DebugMenu::OnEnable(bool a_toggle)
{
	m_enabled = !m_enabled;
	return m_enabled;
}

void DebugMenu::Draw()
{
	// Draw nothing if the debug menu isn't enabled
	if (!m_enabled)
	{
		return;
	}

	RenderManager & renMan = RenderManager::Get();
	FontManager & fontMan = FontManager::Get();

	// Draw gridlines
	renMan.AddLine2D(Vector2(-1.0f, 0.0f), Vector2(1.0f, 0.0f), sc_colourGreyAlpha);
	renMan.AddLine2D(Vector2(0.0f, 1.0f),  Vector2(0.0f, -1.0f), sc_colourGreyAlpha);
	
	// Show mouse pos at cursor
	char mouseBuf[16];
	Vector2 mousePos = InputManager::Get().GetMousePosRelative();
	sprintf(mouseBuf, "%.2f, %.2f", mousePos.GetX(), mousePos.GetY());
	Vector2 displayPos(mousePos.GetX() + sc_cursorSize, mousePos.GetY() - sc_cursorSize);
	fontMan.DrawDebugString(mouseBuf, displayPos, sc_colourGreen);

	// Draw mouse cursor
	for (int i = 0; i < 3; ++i)
	{
		renMan.AddLine2D(mousePos+sc_vectorCursor[i], mousePos+sc_vectorCursor[i+1], sc_colourGreen);
	}
	renMan.AddLine2D(mousePos+sc_vectorCursor[3], mousePos+sc_vectorCursor[0], sc_colourGreen);
}

Widget * DebugMenu::CreateButton(const char * a_name, Colour a_colour, Widget * a_parent)
{
	Widget::WidgetDef curItem;
	curItem.m_size = WidgetVector(0.2f, 0.1f);
	curItem.m_fontNameHash = FontManager::Get().GetLoadedFontName("Arial")->GetHash();
	curItem.m_selectFlags = Widget::eSelectionRollover;
	curItem.m_colour = a_colour;
	curItem.m_name = a_name;
	
	Widget * newWidget = Gui::Get().CreateWidget(curItem, a_parent, false);
	newWidget->SetDebugWidget();
	newWidget->SetAction(this, &DebugMenu::OnMenuItemMouseUp);

	return newWidget;
}

void DebugMenu::ShowCreateMenu(bool a_show)
{
	m_btnCreateRoot->SetActive(a_show);
	m_btnCancel->SetActive(a_show);
	m_btnCreateWidget->SetActive(a_show);
	m_btnCreateGameObject->SetActive(a_show);
}

void DebugMenu::ShowChangeMenu(bool a_show)
{
	m_btnCancel->SetActive(a_show);
	m_btnChangeRoot->SetActive(a_show);
	m_btnChangePos->SetActive(a_show);
	m_btnChangeShape->SetActive(a_show);
	m_btnChangeType->SetActive(a_show);
	m_btnChangeTexture->SetActive(a_show);
}