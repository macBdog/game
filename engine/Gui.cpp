#include "Log.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "StringUtils.h"

#include "Gui.h"

template<> Gui * Singleton<Gui>::s_instance = NULL;

bool Gui::Startup(const char * a_guiPath)
{
	// Load in the gui texture
	char fileName[StringUtils::s_maxCharsPerLine];
	sprintf(fileName, "%s%s", a_guiPath, "gui.cfg");
	m_configFile.Load(fileName);

	// Setup the screen root element
	m_screen.SetName("screen");

	// Setup the mouse cursor element
	sprintf(fileName, "%s%s", a_guiPath, m_configFile.GetString("config", "mouseCursorTexture"));
	m_cursor.SetTexture(TextureManager::Get().GetTexture(fileName, TextureManager::eCategoryGui));
	m_cursor.SetPos(Vector2(0.0f, 0.0f));
	m_cursor.SetSize(Vector2(0.16f / RenderManager::Get().GetViewAspect(), 0.16f));
	m_cursor.SetActive(true);

	// Setup input callbacks for handling events, left mouse buttons activate gui elements
	InputManager & inMan = InputManager::Get();
	inMan.RegisterMouseCallback(this, &Gui::MouseInputHandler, InputManager::eMouseButtonLeft);

	return true;
}

bool Gui::Shutdown()
{
	// Clean up all widgets and family
	DestroyWidget(m_screen.GetChild());
	return true;
}

bool Gui::Update(float a_dt)
{
	// Update mouse position
	m_cursor.SetPos(InputManager::Get().GetMousePosRelative()); 

	// Process mouse position for selection of widgets
	UpdateSelection();

	// Draw the screen and all children (which should be everything)
	m_screen.Draw();

	// Draw mouse cursor over the top of the gui
	m_cursor.Draw();
	return true;
}

Widget * Gui::CreateWidget(const Widget::WidgetDef & a_def, Widget * a_parent, bool a_startActive)
{
	// Copy properties over to the managed element
	Widget * newWidget = new Widget();
	newWidget->SetName(a_def.m_name);
	newWidget->SetColour(a_def.m_colour);
	newWidget->SetSize(a_def.m_size);
	newWidget->SetFontName(a_def.m_fontNameHash);
	newWidget->SetSelectFlags(a_def.m_selectFlags);
	newWidget->SetActive(a_startActive);

	// If parent has children
	if (a_parent->GetChild() != NULL)
	{
		a_parent->GetChild()->AddSibling(newWidget);
	}
	else // Add element as first child 
	{
		a_parent->AddChild(newWidget);
	}

	return newWidget;
}

void Gui::DestroyWidget(Widget * a_widget)
{
	// Activate sibling widgets
	Widget * curWidget = a_widget->GetNext();
	while (curWidget != NULL)
	{
		Widget * next = curWidget->GetNext();
		delete curWidget;
		curWidget = next;
	}

	// Activate any child widgets
	curWidget = a_widget->GetChild();
	while (curWidget != NULL)
	{
		Widget * next = curWidget->GetChild();
		delete curWidget;
		curWidget = next;
	}
}

bool Gui::MouseInputHandler(bool active)
{
	// The mouse was clicked, check if any elements were rolled over
	return m_screen.DoActivation();
}

void Gui::UpdateSelection()
{
	// Any childrend of the root element will be updated
	m_screen.UpdateSelection(m_cursor.GetPos());
}