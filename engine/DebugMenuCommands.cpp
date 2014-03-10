#include "Gui.h"
#include "InputManager.h"
#include "FontManager.h"
#include "Widget.h"

#include "DebugMenuCommands.h"

DebugMenuCommand::DebugMenuCommand(const char * a_name, Widget * a_parent, Colour a_colour)
: m_gameObjectFunction()
, m_widgetFunction()
, m_alignment(DebugMenuCommandAlign::Below)
, m_widget(NULL)
{
	m_widget = CreateButton(a_name, a_parent, a_colour);
}

void DebugMenuCommand::SetAlignment(Widget * a_alignedTo, DebugMenuCommandAlign::Enum a_alignment)
{
	m_widget->SetAlignTo(a_alignedTo);
	m_alignment = a_alignment;
	SetWidgetAlignment(m_alignment);
}

void DebugMenuCommand::SetWidgetAlignment(DebugMenuCommandAlign::Enum a_alignment)
{
	switch (a_alignment)
	{
		case DebugMenuCommandAlign::Right: 
		{
			m_widget->SetAlignment(AlignX::Right, AlignY::Top);	
			m_widget->SetAlignmentAnchor(AlignX::Left, AlignY::Top);
			break;
		}
		case DebugMenuCommandAlign::Below:
		{
			m_widget->SetAlignment(AlignX::Left, AlignY::Bottom);	
			m_widget->SetAlignmentAnchor(AlignX::Left, AlignY::Top);
			break;
		}
		case DebugMenuCommandAlign::Above:
		{
			m_widget->SetAlignment(AlignX::Left, AlignY::Top);	
			m_widget->SetAlignmentAnchor(AlignX::Left, AlignY::Bottom);
			break;
		}
		case DebugMenuCommandAlign::Left:
		{
			m_widget->SetAlignment(AlignX::Left, AlignY::Top);	
			m_widget->SetAlignmentAnchor(AlignX::Right, AlignY::Top);
			break;
		}
		default: break;
	}
}

Widget * DebugMenuCommand::CreateButton(const char * a_name, Widget * a_parent, Colour a_colour)
{
	// All debug menu elements are created equal
	Widget::WidgetDef curItem;
	curItem.m_size = WidgetVector(0.2f, 0.1f);
	curItem.m_pos = WidgetVector(10.0f, 10.0f);
	curItem.m_pos.SetAlignment(AlignX::Left, AlignY::Bottom);
	curItem.m_pos.SetAlignmentAnchor(AlignX::Left, AlignY::Top);
	curItem.m_selectFlags = SelectionFlags::Rollover;
	curItem.m_colour = a_colour;
	curItem.m_name = a_name;

	// Check for a loaded debug font
	if (StringHash * debugFont = FontManager::Get().GetDebugFontName())
	{
		curItem.m_fontNameHash = debugFont->GetHash();
	}

	Widget * retWidget = Gui::Get().CreateWidget(curItem, a_parent, false);
	retWidget->SetDebugWidget();
	retWidget->SetActive(false);
	return retWidget;
}

DebugMenuCommand * DebugMenuCommandRegistry::Create(const char * a_name, Widget * a_parent, Widget * a_alignTo, DebugMenuCommandAlign::Enum a_align, Colour a_colour)
{
	DebugMenuCommand * newCommand = new DebugMenuCommand(a_name, a_parent, a_colour);	
	newCommand->SetAlignment(a_alignTo, a_align);
	CommandNode * newNode = new CommandNode();											
	newNode->SetData(newCommand);														
	m_commands.Insert(newNode);
	return newCommand;
}

void DebugMenuCommandRegistry::Startup(Widget * a_parent)
{
	m_rootCommand = a_parent;

	DebugMenuCommand * lastCreatedCommand = NULL;
	lastCreatedCommand = Create("Create Widget",			a_parent, a_parent, DebugMenuCommandAlign::Right, sc_colourPurple);
		lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::CreateWidget);
	lastCreatedCommand = Create("Create GameObject",		a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGreen);
	lastCreatedCommand = Create("New Object",				a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Right, sc_colourSkyBlue);
	lastCreatedCommand = Create("From Template",			a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourOrange);
	
	// Change 2D objects
	lastCreatedCommand = Create("Position",					a_parent, a_parent, DebugMenuCommandAlign::Right, sc_colourPurple);
	lastCreatedCommand = Create("Shape",					a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourBlue);
	lastCreatedCommand = Create("Name",						a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourOrange);
	lastCreatedCommand = Create("Text", 					a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGreen);
	lastCreatedCommand = Create("Texture", 					a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourYellow);
	lastCreatedCommand = Create("Delete", 					a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGrey);

	// Change 3D objects
	lastCreatedCommand = Create("Model",					a_parent, a_parent, DebugMenuCommandAlign::Right, sc_colourGreen);
	lastCreatedCommand = Create("Name",						a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourOrange);
	lastCreatedCommand = Create("Clip Type",				a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourYellow);
	lastCreatedCommand = Create("Clip Size",				a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourBlue);
	lastCreatedCommand = Create("Clip Position",			a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourSkyBlue);
	lastCreatedCommand = Create("Save Template",			a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourPurple);
	lastCreatedCommand = Create("Delete",					a_parent, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGrey);
}


void DebugMenuCommandRegistry::Shutdown()
{
	CommandNode * cur = m_commands.GetHead();
	while (cur != NULL)
	{
		CommandNode * next = cur->GetNext();
		Gui::Get().DestroyWidget(cur->GetData()->GetWidget());
		delete cur->GetData();
		delete cur;

		cur = next;
	}
}

bool DebugMenuCommandRegistry::IsActive() const
{
	DEBUG_COMMANDS_LOOP_BEGIN
		if (debugWidget->IsActive()) 
		{
			return true;
		}
	DEBUG_COMMANDS_LOOP_END
	return false;
}

bool DebugMenuCommandRegistry::HandleLeftClick(Widget * a_clickedWidget, Widget * a_selectedWidget, GameObject * a_selectedGameObject)
{
	DEBUG_COMMANDS_LOOP_BEGIN
		if (debugWidget->IsSelected(SelectionFlags::EditRollover))
		{
			debugCommand->Execute(a_selectedWidget, a_selectedGameObject);
			return true;
		}
	DEBUG_COMMANDS_LOOP_END
	return false;
}

bool DebugMenuCommandRegistry::HandleRightClick(Widget * a_clickedWidget, Widget * a_selectedWidget, GameObject * a_selectedGameObject)
{
	// Draw from screen coords or last clicked widget
	Vector2 menuDrawPos = InputManager::Get().GetMousePosRelative();
	if (a_clickedWidget != NULL)
	{
		menuDrawPos = a_clickedWidget->GetPos().GetVector();
	}

	// Determine which way the menus should be drawn in case user clicked too close to right/bottom of screen
	const Vector2 screenSideLimit(0.15f, 0.15f);
	const Vector2 menuDrawSize(m_commands.GetHead()->GetData()->GetWidget()->GetSize());
	Alignment menuAlign;
	menuAlign.m_x = menuDrawPos.GetX() + menuDrawSize.GetX() > 1.0f - screenSideLimit.GetX() ? AlignX::Left : AlignX::Right;
	menuAlign.m_y = menuDrawPos.GetY() - menuDrawSize.GetY() < -1.0 + screenSideLimit.GetY() ? AlignY::Top : AlignY::Bottom;
	SetMenuAlignment(&menuAlign);

	// Check for showing a root level debug command
	if (a_clickedWidget == NULL && a_selectedWidget == NULL && a_selectedGameObject == NULL)
	{
		ShowRootCommands();
		return true;
	}
	return false;
}

void DebugMenuCommandRegistry::SetMenuAlignment(Alignment * a_screenAlign)
{
	DEBUG_COMMANDS_LOOP_BEGIN
		switch (debugCommand->GetAlignment())
		{
			case DebugMenuCommandAlign::Right: // Command should be to the right of its sibling
			{
				if (a_screenAlign->m_x == AlignX::Left)
				{
					debugWidget->SetAlignment(AlignX::Left, AlignY::Top);	
					debugWidget->SetAlignmentAnchor(AlignX::Right, AlignY::Top);
				}
				else
				{
					debugWidget->SetAlignment(AlignX::Right, AlignY::Top);	
					debugWidget->SetAlignmentAnchor(AlignX::Left, AlignY::Top);
				}
				break;
			}
		case DebugMenuCommandAlign::Below:	// Command should be below its sibling
		{
			if (a_screenAlign->m_y == AlignY::Top)
			{
				debugWidget->SetAlignment(AlignX::Left, AlignY::Top);	
				debugWidget->SetAlignmentAnchor(AlignX::Left, AlignY::Bottom);
			}
			else
			{
				debugWidget->SetAlignment(AlignX::Left, AlignY::Bottom);	
				debugWidget->SetAlignmentAnchor(AlignX::Left, AlignY::Top);
			}
			break;
		}
		case DebugMenuCommandAlign::Above:	// Command should be above its sibling
		{
			if (a_screenAlign->m_y == AlignY::Bottom)
			{
				debugWidget->SetAlignment(AlignX::Left, AlignY::Bottom);	
				debugWidget->SetAlignmentAnchor(AlignX::Left, AlignY::Top);
			}
			else
			{
				debugWidget->SetAlignment(AlignX::Left, AlignY::Top);	
				debugWidget->SetAlignmentAnchor(AlignX::Left, AlignY::Bottom);
			}
			break;
		}
		case DebugMenuCommandAlign::Left:	// Command should be left of its sibling
		{
			if (a_screenAlign->m_x == AlignX::Right)
			{
				debugWidget->SetAlignment(AlignX::Right, AlignY::Top);	
				debugWidget->SetAlignmentAnchor(AlignX::Left, AlignY::Top);
			}
			else
			{
				debugWidget->SetAlignment(AlignX::Left, AlignY::Top);	
				debugWidget->SetAlignmentAnchor(AlignX::Right, AlignY::Top);
			}
			break;
		}
			default: break;
		}	
	DEBUG_COMMANDS_LOOP_END
}

void DebugMenuCommandRegistry::ShowRootCommands()
{
	InputManager & inMan = InputManager::Get();
	m_rootCommand->SetOffset(inMan.GetMousePosRelative());
	m_rootCommand->SetActive();

	DEBUG_COMMANDS_LOOP_BEGIN
		if (strcmp(debugWidget->GetName(), "Create Widget") == 0 || 
			strcmp(debugWidget->GetName(), "Create GameObject") == 0) 
		{
			debugWidget->SetActive();
		}
	DEBUG_COMMANDS_LOOP_END
}

void DebugMenuCommandRegistry::ShowWidgetCommands()
{

}
	
void DebugMenuCommandRegistry::ShowGameObjectCommands()
{

}

void DebugMenuCommandRegistry::HideRootCommands()
{
	m_rootCommand->SetActive(false);

	DEBUG_COMMANDS_LOOP_BEGIN
		if (strcmp(debugWidget->GetName(), "Create Widget") == 0 || 
			strcmp(debugWidget->GetName(), "Create GameObject") == 0) 
		{
			debugWidget->SetActive(false);
		}
	DEBUG_COMMANDS_LOOP_END
}

void DebugMenuCommandRegistry::HideWidgetCommands()
{

}

void DebugMenuCommandRegistry::HideGameObjectCommands()
{

}

void DebugMenuCommandRegistry::Hide()
{
	m_rootCommand->SetActive(false);

	DEBUG_COMMANDS_LOOP_BEGIN
		debugWidget->SetActive(false);
	DEBUG_COMMANDS_LOOP_END
}

bool DebugMenuCommandRegistry::CreateWidget(Widget * a_widget)
{
	// Make a new widget
	Widget::WidgetDef curItem;
	curItem.m_colour = sc_colourWhite;
	curItem.m_size = WidgetVector(0.35f, 0.35f);
	
	// Check for a loaded debug font
	if (StringHash * debugFont = FontManager::Get().GetDebugFontName())
	{
		curItem.m_fontNameHash = debugFont->GetHash();
	}

	curItem.m_selectFlags = SelectionFlags::Rollover;
	curItem.m_name = "NEW_WIDGET";

	// Parent is the active menu
	Gui & gui = Gui::Get();
	Widget * parentWidget = a_widget != NULL ? a_widget : gui.GetActiveMenu();
	Widget * newWidget = gui.CreateWidget(curItem, parentWidget);
	newWidget->SetOffset(InputManager::Get().GetMousePosRelative());
	//m_dirtyFlags.Set(DirtyFlag::GUI);
		
	Hide();
	return newWidget != NULL;
}

/*
if (a_widget == m_btnCreateGameObject)
	{
		// Position the create object submenu buttons
		m_btnCreateGameObjectFromTemplate->SetAlignTo(m_btnCreateGameObject);
		m_btnCreateGameObjectFromTemplate->SetPos(firstWidgetAlignment);
		m_btnCreateGameObjectNew->SetAlignTo(m_btnCreateGameObjectFromTemplate);
		m_btnCreateGameObjectNew->SetPos(widgetAlignment);

		m_btnCreateGameObjectFromTemplate->SetActive(true);
		m_btnCreateGameObjectNew->SetActive(true);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnCreateGameObjectFromTemplate)
	{
		m_editType = EditType::GameObject;
		m_editMode = EditMode::Template;
		ShowResourceSelect(WorldManager::Get().GetTemplatePath(), "tmp");

		ShowCreateMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnCreateGameObjectNew)
	{
		// Create a game object
		if (m_btnCreateGameObject->IsActive())
		{
			WorldManager::Get().CreateObject<GameObject>();
		}		
		ShowCreateMenu(false);
		m_handledCommand = true;
	}
	*/