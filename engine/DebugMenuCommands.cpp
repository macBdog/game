#include "DataPack.h"
#include "Gui.h"
#include "InputManager.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Widget.h"
#include "WorldManager.h"

#include "DebugMenuCommands.h"

DebugMenuCommand::DebugMenuCommand(const char * a_name, Widget * a_parent, Colour a_colour, EditType a_parentMenu)
: m_gameObjectFunction()
, m_widgetFunction()
, m_alignment(DebugMenuCommandAlign::Below)
, m_widget(nullptr)
, m_parentMenu(a_parentMenu)
{
	m_widget = CreateButton(a_name, a_parent, a_colour);
}

void DebugMenuCommand::SetAlignment(Widget * a_alignedTo, DebugMenuCommandAlign a_alignment)
{
	m_widget->SetAlignTo(a_alignedTo);
	m_alignment = a_alignment;
	SetWidgetAlignment(m_alignment);
}

void DebugMenuCommand::SetWidgetAlignment(DebugMenuCommandAlign a_alignment)
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
	curItem.m_pos = WidgetVector(0.0f, 0.0f);
	curItem.m_pos.SetAlignment(AlignX::Left, AlignY::Top);
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

DebugMenuCommand * DebugMenuCommandRegistry::Create(const char * a_name, Widget * a_parent, Widget * a_alignTo, DebugMenuCommandAlign a_align, Colour a_colour, EditType a_parentMenu)
{
	DebugMenuCommand * newCommand = new DebugMenuCommand(a_name, a_parent, a_colour, a_parentMenu);	
	newCommand->SetAlignment(a_alignTo, a_align);
	CommandNode * newNode = new CommandNode();
	newNode->SetData(newCommand);										
	m_commands.Insert(newNode);
	return newCommand;
}

void DebugMenuCommandRegistry::Startup()
{
	// Create the root of each of the debug sub menus
	Gui & gui = Gui::Get();
	m_btnCreateRoot = DebugMenuCommand::CreateButton("Create!", gui.GetDebugRoot(), sc_colourRed);
	m_btnWidgetRoot = DebugMenuCommand::CreateButton("Change Widget", gui.GetDebugRoot(), sc_colourRed);
	m_btnGameObjectRoot = DebugMenuCommand::CreateButton("Change Object", gui.GetDebugRoot(), sc_colourRed);
	m_btnLightRoot = DebugMenuCommand::CreateButton("Change Light", gui.GetDebugRoot(), sc_colourRed);

	// Create root of the create menu that appears if no objects are selected
	DebugMenuCommand * lastCreatedCommand = nullptr;
	auto newDebugObj = [this, &lastCreatedCommand](const char * a_name) { lastCreatedCommand = Create(a_name, m_btnCreateRoot, m_btnCreateRoot, DebugMenuCommandAlign::Right, sc_colourPurple, EditType::None); };

	newDebugObj("Create Widget");
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::CreateWidget);
	lastCreatedCommand = Create("Create GameObject",		m_btnCreateRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGreen, EditType::None);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::CreateGameObject);
	lastCreatedCommand = Create("Create Light",				m_btnCreateRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourYellow, EditType::None);
	lastCreatedCommand->SetLightFunction(this, &DebugMenuCommandRegistry::CreateLight);
	lastCreatedCommand = Create("Create DataPack",			m_btnCreateRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourBlue, EditType::None);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::CreateDataPack);
	lastCreatedCommand = Create("Show/Hide Physics", m_btnCreateRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourPurple, EditType::None);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ShowHidePhysicsWorld);


	// Change 2D objects
	lastCreatedCommand = Create("Alignment",				m_btnWidgetRoot, m_btnWidgetRoot, DebugMenuCommandAlign::Right, sc_colourBlue, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetAlignment);
	lastCreatedCommand = Create("Align Parent",				m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourBlue, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetAlignmentParent);
	lastCreatedCommand = Create("Offset",					m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourSkyBlue, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetOffset);
	lastCreatedCommand = Create("Shape",					m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourPurple, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetShape);
	lastCreatedCommand = Create("Widget Name",				m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourOrange, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetName);
	lastCreatedCommand = Create("Text", 					m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGreen, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetText);
	lastCreatedCommand = Create("Font", 					m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourSkyBlue, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetFont);
	lastCreatedCommand = Create("FontSize", 				m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourPink, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetFontSize);
	lastCreatedCommand = Create("Colour", 					m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourMauve, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetColour);
	lastCreatedCommand = Create("Texture", 					m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourYellow, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::ChangeWidgetTexture);
	lastCreatedCommand = Create("Delete Widget",			m_btnWidgetRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGrey, EditType::Widget);
	lastCreatedCommand->SetWidgetFunction(this, &DebugMenuCommandRegistry::DeleteWidget);

	// Change 3D objects
	lastCreatedCommand = Create("Model",					m_btnGameObjectRoot, m_btnGameObjectRoot, DebugMenuCommandAlign::Right, sc_colourGreen, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::ChangeGameObjectModel);
	lastCreatedCommand = Create("Name",						m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourOrange, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::ChangeGameObjectName);
	lastCreatedCommand = Create("Clip Type",				m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourYellow, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::ChangeGameObjectClipType);
	lastCreatedCommand = Create("Clip Size",				m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourBlue, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::ChangeGameObjectClipSize);
	lastCreatedCommand = Create("Clip Position",			m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourSkyBlue, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::ChangeGameObjectClipPosition);
	lastCreatedCommand = Create("Set Template",				m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGreen, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::ChangeGameObjectTemplate);
	lastCreatedCommand = Create("Save Template",			m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourPurple, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::SaveGameObjectTemplate);
	lastCreatedCommand = Create("Delete Object",			m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGrey, EditType::GameObject);
	lastCreatedCommand->SetGameObjectFunction(this, &DebugMenuCommandRegistry::DeleteGameObject);

	// Change lights
	lastCreatedCommand = Create("Ambient",					m_btnLightRoot, m_btnLightRoot, DebugMenuCommandAlign::Right, sc_colourOrange, EditType::Light);
	lastCreatedCommand->SetLightFunction(this, &DebugMenuCommandRegistry::ChangeLightAmbient);
	lastCreatedCommand = Create("Diffuse",					m_btnLightRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourPurple, EditType::Light);
	lastCreatedCommand->SetLightFunction(this, &DebugMenuCommandRegistry::ChangeLightDiffuse);
	lastCreatedCommand = Create("Specular",					m_btnLightRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourSkyBlue, EditType::Light);
	lastCreatedCommand->SetLightFunction(this, &DebugMenuCommandRegistry::ChangeLightSpecular);
	lastCreatedCommand = Create("Delete Light",			m_btnGameObjectRoot, lastCreatedCommand->GetWidget(), DebugMenuCommandAlign::Below, sc_colourGrey, EditType::Light);
	lastCreatedCommand->SetLightFunction(this, &DebugMenuCommandRegistry::DeleteLight);
}

void DebugMenuCommandRegistry::Shutdown()
{
	m_commands.ForeachAndDelete([](auto * cur)
	{ 
		Gui::Get().DestroyWidget(cur->GetData()->GetWidget());
	});
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

DebugCommandReturnData DebugMenuCommandRegistry::HandleLeftClick(Widget * a_selectedWidget, GameObject * a_selectedGameObject, Light * a_selectedLight)
{
	DEBUG_COMMANDS_LOOP_BEGIN
		if (debugWidget->IsSelected(SelectionFlags::EditRollover))
		{
			return debugCommand->Execute(a_selectedWidget, a_selectedGameObject, a_selectedLight);
		}
	DEBUG_COMMANDS_LOOP_END

	Hide();

	return DebugCommandReturnData();
}

DebugCommandReturnData DebugMenuCommandRegistry::HandleRightClick(Widget * a_selectedWidget, GameObject * a_selectedGameObject, Light * a_selectedLight)
{
	// Draw the menu from the last clicked position
	DebugCommandReturnData retVal;
	Vector2 menuDrawPos = InputManager::Get().GetMousePosRelative();

	// Determine which way the menus should be drawn in case user clicked too close to right/bottom of screen
	const Vector2 screenSideLimit(0.15f, 0.15f);
	const Vector2 menuDrawSize(m_commands.GetHead()->GetData()->GetWidget()->GetSize());
	Alignment menuAlign;
	menuAlign.m_x = menuDrawPos.GetX() + menuDrawSize.GetX() > 1.0f - screenSideLimit.GetX() ? AlignX::Left : AlignX::Right;
	menuAlign.m_y = menuDrawPos.GetY() - menuDrawSize.GetY() < -1.0 + screenSideLimit.GetY() ? AlignY::Top : AlignY::Bottom;
	SetMenuAlignment(&menuAlign);

	// Commands that affect widgets
	if (a_selectedWidget != nullptr)
	{
		ShowWidgetCommands();
		retVal.m_success = true;
		return retVal;
	}

	// Commands that affect game objects
	if (a_selectedGameObject != nullptr)
	{
		ShowGameObjectCommands();
		retVal.m_success = true;
		return retVal;
	}

	// Commands that affect lights
	if (a_selectedLight != nullptr)
	{
		ShowLightCommands();
		retVal.m_success = true;
		return retVal;
	}

	// Check for showing a root level debug command
	if (a_selectedWidget == nullptr && a_selectedGameObject == nullptr)
	{
		ShowRootCommands();
		retVal.m_success = true;
		return retVal;
	}
	return retVal;
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
	m_btnCreateRoot->SetOffset(InputManager::Get().GetMousePosRelative());
	m_btnCreateRoot->SetActive();

	DEBUG_COMMANDS_LOOP_BEGIN
		if (debugCommand->GetParentMenu() == EditType::None) 
		{
			debugWidget->SetActive();
		}
	DEBUG_COMMANDS_LOOP_END
}

void DebugMenuCommandRegistry::ShowWidgetCommands()
{
	m_btnWidgetRoot->SetOffset(InputManager::Get().GetMousePosRelative());
	m_btnWidgetRoot->SetActive();

	DEBUG_COMMANDS_LOOP_BEGIN
		if (debugCommand->GetParentMenu() == EditType::Widget) 
		{
			debugWidget->SetActive();
		}
	DEBUG_COMMANDS_LOOP_END
}
	
void DebugMenuCommandRegistry::ShowGameObjectCommands()
{
	m_btnGameObjectRoot->SetOffset(InputManager::Get().GetMousePosRelative());
	m_btnGameObjectRoot->SetActive();

	DEBUG_COMMANDS_LOOP_BEGIN
		if (debugCommand->GetParentMenu() == EditType::GameObject) 
		{
			debugWidget->SetActive();
		}
	DEBUG_COMMANDS_LOOP_END
}

void DebugMenuCommandRegistry::ShowLightCommands()
{
	m_btnLightRoot->SetOffset(InputManager::Get().GetMousePosRelative());
	m_btnLightRoot->SetActive();

	DEBUG_COMMANDS_LOOP_BEGIN
		if (debugCommand->GetParentMenu() == EditType::Light) 
		{
			debugWidget->SetActive();
		}
	DEBUG_COMMANDS_LOOP_END
}

void DebugMenuCommandRegistry::HideRootCommands()
{
	m_btnCreateRoot->SetActive(false);

	DEBUG_COMMANDS_LOOP_BEGIN
		if (strcmp(debugWidget->GetName(), "Create Widget") == 0 || 
			strcmp(debugWidget->GetName(), "Create GameObject") == 0 ||
			strcmp(debugWidget->GetName(), "Create Light") == 0) 
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

void DebugMenuCommandRegistry::HideLightCommands()
{

}

void DebugMenuCommandRegistry::Hide()
{
	m_btnCreateRoot->SetActive(false);
	m_btnWidgetRoot->SetActive(false);
	m_btnGameObjectRoot->SetActive(false);
	m_btnLightRoot->SetActive(false);

	DEBUG_COMMANDS_LOOP_BEGIN
		debugWidget->SetActive(false);
	DEBUG_COMMANDS_LOOP_END
}

DebugCommandReturnData DebugMenuCommandRegistry::CreateWidget(Widget * a_widget)
{
	DebugCommandReturnData retVal;

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
	Widget * parentWidget = a_widget != nullptr ? a_widget : gui.GetActiveMenu();
	Widget * newWidget = gui.CreateWidget(curItem, parentWidget);
	newWidget->SetOffset(InputManager::Get().GetMousePosRelative());
		
	Hide();
	retVal.m_success = newWidget != nullptr;
	retVal.m_dirtyFlag = DirtyFlag::GUI;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetAlignment(Widget * a_widget)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Alignment;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetAlignmentParent(Widget * a_widget)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::AlignmentParent;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetOffset(Widget * a_widget)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Pos;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetShape(Widget * a_widget)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Shape;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetName(Widget * a_widget)
{
	Hide();
	sprintf(m_textEditString, "%s", a_widget->GetName());

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Name;
	retVal.m_textEditString = &m_textEditString[0];
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetText(Widget * a_widget)
{
	Hide();
	sprintf(m_textEditString, "%s", a_widget->GetText());

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Text;
	retVal.m_textEditString = &m_textEditString[0];
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetFont(Widget * a_widget)
{
	Hide();
	
	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Font;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetFontSize(Widget * a_widget)
{
	Hide();
	
	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::FontSize;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetColour(Widget * a_widget)
{
	Hide();
	
	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Colour;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeWidgetTexture(Widget * a_widget)
{
	DebugCommandReturnData retVal;
	sprintf(m_resourceSelectPath, "%s", TextureManager::Get().GetTexturePath());
	sprintf(m_resourceSelectExtension, "tga");
	retVal.m_editType = EditType::Widget;
	retVal.m_editMode = EditMode::Texture;
	retVal.m_resourceSelectPath = &m_resourceSelectPath[0];
	retVal.m_resourceSelectExtension = &m_resourceSelectExtension[0];
	retVal.m_success = true;

	Hide();
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::DeleteWidget(Widget * a_widget)
{
	Hide();
	Gui::Get().DestroyWidget(a_widget);
	
	DebugCommandReturnData retVal;
	retVal.m_clearSelection = true;
	retVal.m_dirtyFlag = DirtyFlag::GUI;
	retVal.m_success = a_widget == nullptr;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::CreateDataPack(Widget * a_widget)
{
	DebugCommandReturnData retVal;

	// Write the existing datapack to disk out
	Hide();
	retVal.m_success = DataPack::Get().Serialize(DataPack::s_defaultDataPackPath);
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ShowHidePhysicsWorld(Widget * a_widget)
{
	DebugCommandReturnData retVal;

	// Toggle the state of physics debugging
	Hide();
	const bool curPhysicsDebug = DebugMenu::Get().IsPhysicsDebuggingOn();
	DebugMenu::Get().SetPhysicsDebugging(!curPhysicsDebug);
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::CreateGameObject(GameObject * a_gameObj)
{
	Hide();
	WorldManager::Get().CreateObject();

	DebugCommandReturnData retVal;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeGameObjectModel(GameObject * a_gameObj)
{
	Hide();

	DebugCommandReturnData retVal;
	sprintf(m_resourceSelectPath, "%s", ModelManager::Get().GetModelPath());
	sprintf(m_resourceSelectExtension, "obj");
	retVal.m_editType = EditType::GameObject;
	retVal.m_editMode = EditMode::Model;
	retVal.m_resourceSelectPath = &m_resourceSelectPath[0];
	retVal.m_resourceSelectExtension = &m_resourceSelectExtension[0];
	retVal.m_success = true;

	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeGameObjectName(GameObject * a_gameObj)
{
	Hide();
	sprintf(m_textEditString, "%s", a_gameObj->GetName());

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::GameObject;
	retVal.m_editMode = EditMode::Name;
	retVal.m_textEditString = &m_textEditString[0];
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeGameObjectClipType(GameObject * a_gameObj)
{
	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::GameObject;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeGameObjectClipSize(GameObject * a_gameObj)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editMode = EditMode::ClipSize;
	retVal.m_editType = EditType::GameObject;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeGameObjectClipPosition(GameObject * a_gameObj)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editMode = EditMode::ClipPosition;
	retVal.m_editType = EditType::GameObject;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeGameObjectTemplate(GameObject * a_gameObj)
{
	Hide();
	sprintf(m_resourceSelectPath, "%s", WorldManager::Get().GetTemplatePath());
	sprintf(m_resourceSelectExtension, "tmp");

	DebugCommandReturnData retVal;
	retVal.m_editType = EditType::GameObject;
	retVal.m_editMode = EditMode::Template;
	retVal.m_resourceSelectPath = &m_resourceSelectPath[0];
	retVal.m_resourceSelectExtension = &m_resourceSelectExtension[0];
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::SaveGameObjectTemplate(GameObject * a_gameObj)
{
	Hide();
	sprintf(m_textEditString, "%s", a_gameObj->GetTemplate());

	DebugCommandReturnData retVal;
	retVal.m_textEditString = &m_textEditString[0];
	retVal.m_editMode = EditMode::SaveTemplate;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::DeleteGameObject(GameObject * a_gameObj)
{
	Hide();
	WorldManager::Get().DestroyObject(a_gameObj->GetId());

	DebugCommandReturnData retVal;
	retVal.m_clearSelection = true;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::CreateLight(Light * a_light)
{
	Hide();
	WorldManager::Get().GetCurrentScene()->AddLight("NEW_LIGHT", Vector(0.0f), Quaternion(), Colour(0.5f), Colour(0.5f), Colour(0.5f));

	DebugCommandReturnData retVal;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeLightAmbient(Light * a_light)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editMode = EditMode::Ambient;
	retVal.m_editType = EditType::Light;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeLightDiffuse(Light * a_light)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editMode = EditMode::Diffuse;
	retVal.m_editType = EditType::Light;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::ChangeLightSpecular(Light * a_light)
{
	Hide();

	DebugCommandReturnData retVal;
	retVal.m_editMode = EditMode::Specular;
	retVal.m_editType = EditType::Light;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}

DebugCommandReturnData DebugMenuCommandRegistry::DeleteLight(Light * a_light)
{
	Hide();
	WorldManager::Get().GetCurrentScene()->RemoveLight(a_light->m_name);

	DebugCommandReturnData retVal;
	retVal.m_clearSelection = true;
	retVal.m_dirtyFlag = DirtyFlag::Scene;
	retVal.m_success = true;
	return retVal;
}
