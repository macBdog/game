#include "../core/Colour.h"
#include "../core/MathUtils.h"

#include "CameraManager.h"
#include "FontManager.h"
#include "InputManager.h"
#include "Log.h"
#include "ModelManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "Widget.h"
#include "WorldManager.h"

#include "DebugMenu.h"

template<> DebugMenu * Singleton<DebugMenu>::s_instance = NULL;

const float DebugMenu::sc_gameTimeScaleFast = 3.0f;		// Value of game time scale in fast mode
const float DebugMenu::sc_gameTimeScaleSlow = 0.1f;		// Value of game time scale in slow mode
const float DebugMenu::sc_cursorSize = 0.075f;			// Cursor definition
Vector2 DebugMenu::sc_vectorCursor[4] = 
{ 
	Vector2(0.0f, 0.0f),
	Vector2(sc_cursorSize, -sc_cursorSize),
	Vector2(sc_cursorSize*0.3f, -sc_cursorSize*0.7f),
	Vector2(0.0f, -sc_cursorSize)
};

DebugMenu::DebugMenu()
: m_enabled(false) 
, m_gameTimeScale(1.0f)
, m_dirtyFlags()
, m_lastMousePosRelative(0.0f)
, m_editType(EditType::None)
, m_editMode(EditMode::None)
, m_widgetToEdit(NULL)
, m_gameObjectToEdit(NULL)
, m_resourceSelect(NULL)
, m_resourceSelectList(NULL)
, m_btnResourceSelectOk(NULL)
, m_btnResourceSelectCancel(NULL)
, m_textInput(NULL)
, m_textInputField(NULL)
, m_btnTextInputOk(NULL)
, m_btnTextInputCancel(NULL)
{
	for (unsigned int i = 0; i < sc_numScriptDebugWidgets; ++i)
	{
		m_scriptDebugWidgets[i] = NULL;
	}

	if (!Startup())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "DebugMenu failed to startup correctly!");
	}
}

bool DebugMenu::Startup()
{
	Gui & gui = Gui::Get();
	InputManager & inMan = InputManager::Get();

	// Create the debug menu functionality
	m_commands.Startup();
	
	// Create the resource selection dialog
	Widget::WidgetDef curItem;
	curItem.m_size = WidgetVector(0.95f, 1.5f);
	curItem.m_pos = WidgetVector(10.0f, 10.0f);

	// Check for a loaded debug font
	if (StringHash * debugFont = FontManager::Get().GetDebugFontName())
	{
		curItem.m_fontNameHash = debugFont->GetHash();
	}

	curItem.m_selectFlags = SelectionFlags::None;
	curItem.m_colour = sc_colourBlue;
	curItem.m_name = "Resource Select";
	m_resourceSelect = gui.CreateWidget(curItem, gui.GetDebugRoot(), false);
	m_resourceSelect->SetDebugWidget();

	// Create list box for resources
	curItem.m_size = WidgetVector(0.85f, 1.2f);
	curItem.m_selectFlags = SelectionFlags::Rollover;
	curItem.m_colour = sc_colourPurple;
	curItem.m_name="Resource List";
	m_resourceSelectList = gui.CreateWidget(curItem, m_resourceSelect, false);
	m_resourceSelectList->SetDebugWidget();

	// Ok and Cancel buttons on the resource select dialog
	m_btnResourceSelectOk = DebugMenuCommand::CreateButton("Ok", m_resourceSelect, sc_colourOrange);
	m_btnResourceSelectOk->SetAction(this, &DebugMenu::OnMenuItemMouseUp);
	m_btnResourceSelectCancel = DebugMenuCommand::CreateButton("Cancel", m_resourceSelect, sc_colourGrey);
	m_btnResourceSelectCancel->SetAction(this, &DebugMenu::OnMenuItemMouseUp);

	// Create text input box for naming objects and files
	curItem.m_size = WidgetVector(0.85f, 0.4f);
	curItem.m_colour = sc_colourBlue;
	curItem.m_name = "Text Entry";
	m_textInput = gui.CreateWidget(curItem, gui.GetDebugRoot(), false);
	m_textInput->SetDebugWidget();

	curItem.m_size = WidgetVector(0.8f, 0.15f);
	curItem.m_colour = sc_colourGrey;
	curItem.m_name = "Value";
	m_textInputField = gui.CreateWidget(curItem, m_textInput, false);
	m_textInputField->SetShowTextCursor(true);
	m_textInputField->SetDebugWidget();

	// Ok and Cancel buttons on the text input dialog
	m_btnTextInputOk = DebugMenuCommand::CreateButton("Ok", m_textInput, sc_colourOrange);
	m_btnTextInputOk->SetAction(this, &DebugMenu::OnMenuItemMouseUp);
	m_btnTextInputCancel = DebugMenuCommand::CreateButton("Cancel", m_textInput, sc_colourGrey);
	m_btnTextInputCancel->SetAction(this, &DebugMenu::OnMenuItemMouseUp);

	// Widgets to be used by script
	for (unsigned int i = 0; i < sc_numScriptDebugWidgets; ++i)
	{
		curItem.m_selectFlags = SelectionFlags::Rollover;
		curItem.m_colour = sc_colourWhite;
		curItem.m_name = "ScriptDebugWidget";
		curItem.m_fontSize = 2.0f;
		if (m_scriptDebugWidgets[i] = gui.CreateWidget(curItem, gui.GetDebugRoot(), false))
		{
			m_scriptDebugWidgets[i]->SetDebugWidget();
			m_scriptDebugWidgets[i]->SetActive(false);
			m_scriptDebugWidgets[i]->SetDebugWidget();
			m_scriptDebugWidgets[i]->SetAlwaysDraw();
		}
	}

	// Register global key and mouse listeners. Note these will be processed after the button callbacks
	inMan.RegisterKeyCallback(this, &DebugMenu::OnEnable, SDLK_TAB);
	inMan.RegisterAlphaKeyCallback(this, &DebugMenu::OnAlphaKeyDown, InputType::KeyDown); 
	inMan.RegisterAlphaKeyCallback(this, &DebugMenu::OnAlphaKeyUp, InputType::KeyUp); 
	inMan.RegisterMouseCallback(this, &DebugMenu::OnActivate, MouseButton::Right);
	inMan.RegisterMouseCallback(this, &DebugMenu::OnSelect, MouseButton::Left);

	// Process vector cursor vertices to account for display aspect
	for (unsigned int i = 0; i < 4; ++i)
	{
		sc_vectorCursor[i].SetY(sc_vectorCursor[i].GetY() * RenderManager::Get().GetViewAspect());
	}

	return true;
}

void DebugMenu::Update(float a_dt)
{
	InputManager & inMan = InputManager::Get();

	// Handle variable time scale
	ResetGameTimeScale();
	if (inMan.IsKeyDepressed(SDLK_EQUALS))
	{
		m_gameTimeScale = sc_gameTimeScaleFast;
	}
	else if (inMan.IsKeyDepressed(SDLK_MINUS))
	{
		m_gameTimeScale = sc_gameTimeScaleSlow;
	}

	// Handle editing actions tied to mouse move
	if (m_editType == EditType::Widget && m_widgetToEdit != NULL)
	{
		Vector2 mousePos = inMan.GetMousePosRelative();
		switch (m_editMode)
		{
			case EditMode::Pos:
			{	
				m_widgetToEdit->SetOffset(mousePos);
				m_dirtyFlags.Set(DirtyFlag::GUI);
				break;
			}
			case EditMode::Shape:	
			{
				m_widgetToEdit->SetSize(Vector2(mousePos.GetX() - m_widgetToEdit->GetPos().GetX(),
												m_widgetToEdit->GetPos().GetY() - mousePos.GetY()));
				m_dirtyFlags.Set(DirtyFlag::GUI);
				break;
			}
			default: break;
		}
	}
	else if (m_gameObjectToEdit != NULL)
	{
		Vector2 amountToMove = (inMan.GetMousePosRelative() - m_lastMousePosRelative) * 10.0f;
		float moveAmount = amountToMove.GetX() + amountToMove.GetY();
		if (m_editType == EditType::GameObject)
		{
			if (m_editMode == EditMode::ClipPosition)
			{
				Vector curPos = m_gameObjectToEdit->GetClipOffset();	
				if (inMan.IsKeyDepressed(SDLK_x))
				{
					m_gameObjectToEdit->SetClipOffset(curPos + Vector(moveAmount, 0.0f, 0.0f));
				}
				else if (inMan.IsKeyDepressed(SDLK_y))
				{
					m_gameObjectToEdit->SetClipOffset(curPos + Vector(0.0f, moveAmount, 0.0f));
				}
				else if (inMan.IsKeyDepressed(SDLK_z))
				{
					m_gameObjectToEdit->SetClipOffset(curPos + Vector(0.0f, 0.0f, moveAmount));
				}
				m_dirtyFlags.Set(DirtyFlag::Scene);
			}
			else if (m_editMode == EditMode::ClipSize)
			{
				Vector curSize = m_gameObjectToEdit->GetClipSize();	
				if (inMan.IsKeyDepressed(SDLK_x))
				{
					m_gameObjectToEdit->SetClipSize(curSize + Vector(moveAmount, 0.0f, 0.0f));
				}
				else if (inMan.IsKeyDepressed(SDLK_y))
				{
					m_gameObjectToEdit->SetClipSize(curSize + Vector(0.0f, moveAmount, 0.0f));
				}
				else if (inMan.IsKeyDepressed(SDLK_z))
				{
					m_gameObjectToEdit->SetClipSize(curSize + Vector(0.0f, 0.0f, moveAmount));
				}
				m_dirtyFlags.Set(DirtyFlag::Scene);
			}
			else if (!IsDebugMenuActive())
			{
				// Move object in all dimensions separately
				Vector curPos = m_gameObjectToEdit->GetPos();			
				if (inMan.IsKeyDepressed(SDLK_x))
				{
					if (inMan.IsKeyDepressed(SDLK_LALT))
					{
						m_gameObjectToEdit->AddRot(Vector(moveAmount * 16.0f, 0.0f, 0.0f));
					}
					else
					{
						m_gameObjectToEdit->SetPos(curPos + Vector(moveAmount, 0.0f, 0.0f));
					}
					m_dirtyFlags.Set(DirtyFlag::Scene);
				} 
				else if (inMan.IsKeyDepressed(SDLK_y))
				{
					if (inMan.IsKeyDepressed(SDLK_LALT))
					{
						m_gameObjectToEdit->AddRot(Vector(0.0f, moveAmount * 16.0f, 0.0f));
					}
					else
					{
						m_gameObjectToEdit->SetPos(curPos + Vector(0.0f, moveAmount, 0.0f));
					}
					m_dirtyFlags.Set(DirtyFlag::Scene);				
				}
				else if (inMan.IsKeyDepressed(SDLK_z))
				{
					if (inMan.IsKeyDepressed(SDLK_LALT))
					{
						m_gameObjectToEdit->AddRot(Vector(0.0f, 0.0f, moveAmount * 16.0f));
					}
					else
					{
						m_gameObjectToEdit->SetPos(curPos + Vector(0.0f, 0.0f, moveAmount));
					}
					m_dirtyFlags.Set(DirtyFlag::Scene);
				}
			}
		}
	}

	// Draw all widgets with updated coords
	Draw();
	
	// Cache off the last mouse pos
	m_lastMousePosRelative = inMan.GetMousePosRelative();
}

bool DebugMenu::SaveChanges()
{
	// Save resources to disk if dirty
	bool changesSaved = false;
	for (unsigned int i = 0; i < DirtyFlag::Count; ++i)
	{
		DirtyFlag::Enum curFlag = (DirtyFlag::Enum)i;
		if (m_dirtyFlags.IsBitSet(curFlag))
		{
			// Handle each flag type
			switch (curFlag)
			{
				case DirtyFlag::GUI:	Gui::Get().GetActiveMenu()->Serialise();				changesSaved = true; break;
				case DirtyFlag::Scene:	WorldManager::Get().GetCurrentScene()->Serialise();		changesSaved = true; break;
				default: break;
			}
			m_dirtyFlags.Clear(curFlag);
		}
	}

	return changesSaved;
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

	if (a_widget == m_btnResourceSelectOk)
	{
		// Have just selected a resource for some object
		switch (m_editType)
		{
			case EditType::Widget: 
			{
				// Early out for no widget selected
				if (m_widgetToEdit == NULL) { break; }

				// Setting a texture on a widget
				if (m_editMode == EditMode::Texture)
				{
					char tgaBuf[StringUtils::s_maxCharsPerLine];
					sprintf(tgaBuf, "%s%s", TextureManager::Get().GetTexturePath(), m_resourceSelectList->GetSelectedListItem());
					m_widgetToEdit->SetTexture(TextureManager::Get().GetTexture(tgaBuf, TextureCategory::Gui));
					m_dirtyFlags.Set(DirtyFlag::GUI);
				}
				break;
			}
			case EditType::GameObject:
			{
				WorldManager & worldMan = WorldManager::Get();
				if (m_editMode == EditMode::Template)
				{
					// Delete the old object
					if (m_gameObjectToEdit)
					{
						worldMan.DestroyObject(m_gameObjectToEdit->GetId());
					}
					worldMan.CreateObject<GameObject>(m_resourceSelectList->GetSelectedListItem());
					m_dirtyFlags.Set(DirtyFlag::Scene);
				}

				// Early out for no object
				if (m_gameObjectToEdit == NULL) { break; }

				// Setting a model on a game object
				if (m_editMode == EditMode::Model)
				{
					char objBuf[StringUtils::s_maxCharsPerLine];
					sprintf(objBuf, "%s%s", ModelManager::Get().GetModelPath(), m_resourceSelectList->GetSelectedListItem());
					
					// Load the model and set it as the current model to edit
					if (Model * newModel = ModelManager::Get().GetModel(objBuf))
					{
						m_gameObjectToEdit->SetModel(newModel);
						m_dirtyFlags.Set(DirtyFlag::Scene);
					}
				}
				break;
			}
			default: break;
		}

		HideResoureSelect();
		m_editType = EditType::None;
		m_editMode = EditMode::None;
	}
	else if (a_widget == m_btnResourceSelectCancel)
	{
		HideResoureSelect();
		m_editType = EditType::None;
		m_editMode = EditMode::None;
	}
	else if (a_widget == m_btnTextInputOk)
	{
		if (m_editType == EditType::Widget)
		{
			if (m_editMode == EditMode::Name)
			{
				// Editing the name of a widget
				if (m_widgetToEdit != NULL)
				{
					m_widgetToEdit->SetName(m_textInputField->GetText());
					m_dirtyFlags.Set(DirtyFlag::GUI);
				}
			}
			else if (m_editMode == EditMode::Text)
			{
				// Editing the text of a widget
				if (m_widgetToEdit != NULL)
				{
					m_widgetToEdit->SetText(m_textInputField->GetText());
					m_dirtyFlags.Set(DirtyFlag::GUI);
				}
			}
		}
		else if (m_editType == EditType::GameObject)
		{
			// Editing the name of an object
			if (m_editMode == EditMode::Name)
			{
				if (m_gameObjectToEdit != NULL)
				{
					m_gameObjectToEdit->SetName(m_textInputField->GetText());
					m_dirtyFlags.Set(DirtyFlag::Scene);
				}
			}
			else if (m_editMode == EditMode::SaveTemplate)
			{
				// Editing the template of an object
				if (m_gameObjectToEdit != NULL)
				{
					char templateString[StringUtils::s_maxCharsPerName];
					if (strstr(m_textInputField->GetText(), ".tmp"))
					{
						sprintf(templateString, "%s", m_textInputField->GetText());
					}
					else
					{
						sprintf(templateString, "%s.tmp", m_textInputField->GetText());
					}
					m_gameObjectToEdit->SetTemplate(templateString);
					m_dirtyFlags.Set(DirtyFlag::Scene);
				}
			}
		}

		HideTextInput();
		m_editType = EditType::None;
		m_editMode = EditMode::None;
	}
	else if (a_widget == m_btnTextInputCancel)
	{
		HideTextInput();
		m_editType = EditType::None;
		m_editMode = EditMode::None;
	}

	// Save anything dirty to file
	return SaveChanges();
}

bool DebugMenu::OnActivate(bool a_active)
{
	// Do nothing if the debug menu isn't enabled
	if (!m_enabled)
	{
		return false;
	}

	// If there is both a widget and object selected then we are in an error state
	if (m_widgetToEdit != NULL && m_gameObjectToEdit != NULL)
	{
		m_gameObjectToEdit = NULL;
	}

	// Let the commands handle what to do
	m_commands.HandleRightClick(m_widgetToEdit, m_gameObjectToEdit);
	return true;
}

bool DebugMenu::OnSelect(bool a_active)
{
	// Respond to a click and set internal state if it's been handled by a command
	if (m_commands.IsActive())
	{
		DebugCommandReturnData retVal = m_commands.HandleLeftClick(m_widgetToEdit, m_gameObjectToEdit);
		m_editMode = retVal.m_editMode;
		m_editType = retVal.m_editType;

		// If the command touched a resource that needs writing
		if (retVal.m_dirtyFlag > DirtyFlag::None)
		{
			m_dirtyFlags.Set(retVal.m_dirtyFlag);
		}
		// If the command required a resource to be selected
		if (retVal.m_resourceSelectPath != NULL)
		{
			ShowResourceSelect(retVal.m_resourceSelectPath, retVal.m_resourceSelectExtension);
		}
		// If the command requires some input of text
		if (retVal.m_textEditString != NULL)
		{
			ShowTextInput(retVal.m_textEditString);
		}
		return retVal.m_success;
	}

	// Stop any mouse bound editing on click
	if (m_editMode == EditMode::Pos  || m_editMode == EditMode::Shape)
	{
		m_editMode = EditMode::None;

		// Changed a property, save the file
		m_dirtyFlags.Set(DirtyFlag::GUI);
	}

	// Don't play around with widget selection while a menu is up
	if (m_textInput->IsActive() || m_resourceSelect->IsActive())
	{
		return false;
	}

	// Cancel previous selection
	if (!IsDebugMenuActive() && m_editMode == EditMode::None)
	{
		if (m_widgetToEdit != NULL)
		{
			m_editType = EditType::None;
			m_editMode = EditMode::None;
			m_widgetToEdit->ClearSelection();
			m_widgetToEdit = NULL;
		}
	}

	// Find the first widget that is rolled over in edit mode
	if (Widget * newSelectedWidget = Gui::Get().GetActiveWidget())
	{
		// Clear selection of old widget
		if (m_widgetToEdit != NULL && m_widgetToEdit != newSelectedWidget)
		{
			m_widgetToEdit->ClearSelection();
		}
		m_editType = EditType::Widget;
		m_widgetToEdit = newSelectedWidget;
		m_widgetToEdit->SetSelection(SelectionFlags::EditSelected);
		m_gameObjectToEdit = NULL;
	}
	else // Cancel selections
	{
		if (m_widgetToEdit != NULL)
		{
			m_editType = EditType::None;
			m_editMode = EditMode::None;
			m_widgetToEdit->ClearSelection();
			m_widgetToEdit = NULL;
		}
	}

	// If we don't already have a widget
	if (m_widgetToEdit == NULL)
	{
		// Do picking with all the game objects in the scene
		if (Scene * curScene = WorldManager::Get().GetCurrentScene())
		{
			// Picking point is the mouse cursor transformed to 3D space in cam direction
			const float pickDepth = 100.0f;
			const float persp = 0.47f;
			RenderManager & renMan = RenderManager::Get();
			CameraManager & camMan = CameraManager::Get();
			Vector2 mousePos = InputManager::Get().GetMousePosRelative();
			Matrix camMat = camMan.GetViewMatrix();
			Vector camPos = camMan.GetWorldPos();
			Vector mouseInput = Vector(	mousePos.GetX() * renMan.GetViewAspect() * pickDepth * persp, 
										0.0f, 
										mousePos.GetY() * pickDepth * persp);
			Vector pickEnd = camPos + camMat.GetLook() * pickDepth;
			pickEnd += camMat.Transform(mouseInput);

			// Pick an arbitrary object (would have to sort to get the closest)
			GameObject * foundObject = curScene->GetSceneObject(camPos, pickEnd);
			if (foundObject != NULL)
			{
				m_gameObjectToEdit = foundObject;
				m_widgetToEdit = NULL;
				m_editType = EditType::GameObject;
			}
			else
			{
				m_gameObjectToEdit = NULL;
				m_editType = EditType::None;
				m_editMode = EditMode::None;
			}
		}
	}

	// Cancel all menu display
	if (m_gameObjectToEdit == NULL && m_widgetToEdit == NULL)
	{
		m_commands.Hide();
	}

	return false;
}

bool DebugMenu::OnEnable(bool a_toggle)
{
	m_enabled = !m_enabled;
	return m_enabled;
}

bool DebugMenu::OnAlphaKeyDown(bool a_unused)
{
	InputManager & inMan = InputManager::Get();

	// Respond to typing in a text input box
	if (m_textInput->IsActive())
	{
		char newName[StringUtils::s_maxCharsPerName];
		newName[0] = '\0';
		strncpy(newName, m_textInputField->GetText(), sizeof(char) * strlen(m_textInputField->GetText()) + 1);

		SDLKey lastKey = inMan.GetLastKey();
		if (lastKey == SDLK_BACKSPACE)
		{
			// Delete a character off the end of the name
			unsigned int nameLength = strlen(newName);
			if (nameLength > 0)
			{
				newName[nameLength - 1] = '\0';
				m_textInputField->SetText(newName);
			}
		}
		else // Some other alpha key, append to the name
		{
			// TODO move this check to string utils and input manager
			int keyVal = (int)lastKey;
			const unsigned int alphaStart = 97; // ASCII for a key
			const unsigned int alphaEnd = 122;  // ASCII for z key
			const unsigned int numStart = 48;	// ASCII for 0
			const unsigned int numEnd = 57;		// ASCII for 9
			if ((keyVal >= alphaStart && keyVal <= alphaEnd) ||
				(keyVal >= numStart && keyVal <= numEnd) ||
				lastKey == SDLK_UNDERSCORE)
			{
				// Handle upper case letters when a shift is held
				if ((keyVal >= alphaStart && keyVal <= alphaEnd) && 
					(inMan.IsKeyDepressed(SDLK_LSHIFT) || inMan.IsKeyDepressed(SDLK_RSHIFT)))
				{
					keyVal -= 32;
				}

				sprintf(newName, "%s%c", m_textInputField->GetText(), keyVal);
				m_textInputField->SetText(newName);
			}
		}

		return true;
	}
	else // Other debug menu keys
	{
		if (m_enabled)
		{
			// Clear the log
			SDLKey lastKey = inMan.GetLastKey();
			if (lastKey == SDLK_BACKSPACE)
			{
				Log::Get().ClearRendering();
			}
		}
	}

	return false;
}

bool DebugMenu::OnAlphaKeyUp(bool a_unused)
{
	// Save to disk file at the end of an action
	return SaveChanges();	
}

void DebugMenu::ShowResourceSelect(const char * a_startingPath, const char * a_fileExtensionFilter)
{
	// Position and display the elements of the dialog
	m_resourceSelect->SetActive();
	m_resourceSelectList->SetActive();
	m_btnResourceSelectOk->SetActive();
	m_btnResourceSelectCancel->SetActive();

	// Position buttons on the panel
	const float buttonSpacingX = 0.025f;
	const float buttonSpacingY = buttonSpacingX * RenderManager::Get().GetViewAspect();
	Vector2 parentSize = m_resourceSelect->GetSize();
	Vector2 parentPos = Vector2(-parentSize.GetX()*0.5f, 0.75f);
	m_resourceSelect->SetOffset(parentPos);

	// Position the list of resources
	m_resourceSelectList->SetOffset(Vector2(parentPos.GetX() + buttonSpacingX*2.0f, parentPos.GetY() - buttonSpacingY*2.0f));

	// Position the Ok and Cancel buttons
	Vector2 buttonSize = m_btnResourceSelectOk->GetSize();
	Vector2 buttonPos = Vector2(parentPos.GetX() + buttonSpacingX,
								parentPos.GetY() - parentSize.GetY() + buttonSize.GetY() + buttonSpacingY);
	m_btnResourceSelectOk->SetOffset(buttonPos);

	buttonPos.SetX(parentPos.GetX() + parentSize.GetX() - buttonSize.GetX() - buttonSpacingX);
	m_btnResourceSelectCancel->SetOffset(buttonPos);

	// Add resource list to widget
	m_resourceSelectList->ClearListItems();
	FileManager & fileMan = FileManager::Get();
	FileManager::FileList resourceFiles;
	fileMan.FillFileList(a_startingPath, resourceFiles, a_fileExtensionFilter);

	// Add each resource file in the directory
	FileManager::FileListNode * curNode = resourceFiles.GetHead();
	while(curNode != NULL)
	{
		m_resourceSelectList->AddListItem(curNode->GetData()->m_name);

		curNode = curNode->GetNext();
	}

	// Clean up file list
	fileMan.CleanupFileList(resourceFiles);
}

void DebugMenu::ShowTextInput(const char * a_startingText)
{
	// Position and display the elements of the dialog
	m_textInput->SetActive();
	m_textInputField->SetActive();
	m_btnTextInputOk->SetActive();
	m_btnTextInputCancel->SetActive();

	// Position buttons on the panel
	const float buttonSpacingX = 0.025f;
	const float buttonSpacingY = buttonSpacingX * RenderManager::Get().GetViewAspect();
	Vector2 parentSize = m_textInput->GetSize();
	Vector2 parentPos = Vector2(-parentSize.GetX()*0.5f, 0.75f);
	m_textInput->SetOffset(parentPos);

	// Position the list of resources
	m_textInputField->SetOffset(Vector2(parentPos.GetX() + buttonSpacingX*2.0f, parentPos.GetY() - buttonSpacingY*2.0f));

	// Position the Ok and Cancel buttons
	Vector2 buttonSize = m_btnTextInputOk->GetSize();
	Vector2 buttonPos = Vector2(parentPos.GetX() + buttonSpacingX,
								parentPos.GetY() - parentSize.GetY() + buttonSize.GetY() + buttonSpacingY);
	m_btnTextInputOk->SetOffset(buttonPos);

	buttonPos.SetX(parentPos.GetX() + parentSize.GetX() - buttonSize.GetX() - buttonSpacingX);
	m_btnTextInputCancel->SetOffset(buttonPos);

	// Show the starting text if reqd
	if (a_startingText != NULL)
	{
		m_textInputField->SetText(a_startingText);
	}
}

bool DebugMenu::ShowScriptDebugText(const char * a_text, float a_posX, float a_posY)
{
	Gui & gui = Gui::Get();
	for (unsigned int i = 0; i < sc_numScriptDebugWidgets; ++i)
	{
		if (m_scriptDebugWidgets[i] != NULL && !m_scriptDebugWidgets[i]->IsActive())
		{
			m_scriptDebugWidgets[i]->SetText(a_text);
			m_scriptDebugWidgets[i]->SetOffset(WidgetVector(a_posX, a_posY - (i*0.5f*m_scriptDebugWidgets[i]->GetSize().GetY())));
			m_scriptDebugWidgets[i]->SetActive(true);
			return true;
		}
	}

	Log::Get().WriteGameErrorNoParams("Ran out of debug DebugPrint resources!");
	return false;
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

	// Don't draw lines over the menu
	if (!IsDebugMenuActive())
	{
		// Draw 2D gridlines
		renMan.AddLine2D(RenderLayer::Debug2D, Vector2(-1.0f, 0.0f), Vector2(1.0f, 0.0f), sc_colourGreyAlpha);
		renMan.AddLine2D(RenderLayer::Debug2D, Vector2(0.0f, 1.0f),  Vector2(0.0f, -1.0f), sc_colourGreyAlpha);
	}

	// Draw 3D gridlines
	const unsigned int gridSize = 10;
	const float gridMeasurement = 1.0f;
	Vector gridStart(-((float)gridSize * 0.5f) * gridMeasurement, -((float)gridSize * 0.5f) * gridMeasurement, 0.0f);	
	
	// Gridlines on the X axis
	for (unsigned int x = 0; x < gridSize+1; ++x)
	{
		Vector curLineX = gridStart + Vector(x*gridMeasurement, 0.0f, 0.0f);
		renMan.AddLine(RenderLayer::Debug3D, curLineX, curLineX + Vector(0.0f, gridMeasurement * (float)(gridSize), 0.0f), sc_colourGreyAlpha);	
	}

	// Gridlines on the Y axis
	for (unsigned int y = 0; y < gridSize+1; ++y)
	{
		Vector curLineY = gridStart + Vector(0.0f, y*gridMeasurement, 0.0f);
		renMan.AddLine(RenderLayer::Debug3D, curLineY, curLineY + Vector(gridMeasurement * (float)(gridSize), 0.0f, 0.0f), sc_colourGreyAlpha);
	}

	// Draw an identity matrix nearby the origin (not directly on to avoid Z fighting)
	Matrix fakeIdentity = Matrix::Identity();
	fakeIdentity.Translate(Vector(0.0f, 0.0f, EPSILON));
	renMan.AddDebugMatrix(fakeIdentity);

	// Draw selection box around objects - slightly larger than clip size so there is no z fighting
	const float extraSelectionSize = 0.01f;
	if (m_gameObjectToEdit != NULL)
	{
		switch (m_gameObjectToEdit->GetClipType())
		{
			case ClipType::Box:
			case ClipType::AxisBox:
			{
				renMan.AddDebugAxisBox(m_gameObjectToEdit->GetClipPos(), m_gameObjectToEdit->GetClipSize() + extraSelectionSize, sc_colourRed);
				break;
			}
			case ClipType::Sphere:
			{
				renMan.AddDebugSphere(m_gameObjectToEdit->GetClipPos(), m_gameObjectToEdit->GetClipSize().GetX() + extraSelectionSize, sc_colourRed); 
				break;
			}
			default: break;
		}
	}
	
	// Show mouse pos at cursor
	char mouseBuf[16];
	Vector2 mousePos = InputManager::Get().GetMousePosRelative();
	sprintf(mouseBuf, "%.2f, %.2f", mousePos.GetX(), mousePos.GetY());
	Vector2 displayPos(mousePos.GetX() + sc_cursorSize, mousePos.GetY() - sc_cursorSize);
	fontMan.DrawDebugString2D(mouseBuf, displayPos, sc_colourGreen);

	// Draw mouse cursor
	for (int i = 0; i < 3; ++i)
	{
		renMan.AddLine2D(RenderLayer::Debug2D, mousePos+sc_vectorCursor[i], mousePos+sc_vectorCursor[i+1], sc_colourGreen);
	}
	renMan.AddLine2D(RenderLayer::Debug2D, mousePos+sc_vectorCursor[3], mousePos+sc_vectorCursor[0], sc_colourGreen);
}

void DebugMenu::PostRender()
{
	// Clear the debug widgets, script will have to add text next frame for them
	for (unsigned int i = 0; i < sc_numScriptDebugWidgets; ++i)
	{
		if (m_scriptDebugWidgets[i] != NULL)
		{
			m_scriptDebugWidgets[i]->SetActive(false);
			m_scriptDebugWidgets[i]->SetText("");
		}
	}
}

void DebugMenu::HideResoureSelect()
{
	m_resourceSelect->SetActive(false);
	m_resourceSelectList->SetActive(false);
	m_btnResourceSelectOk->SetActive(false);
	m_btnResourceSelectCancel->SetActive(false);
}

void DebugMenu::HideTextInput()
{
	m_textInput->SetActive(false);
	m_textInputField->SetActive(false);
	m_btnTextInputOk->SetActive(false);
	m_btnTextInputCancel->SetActive(false);
}