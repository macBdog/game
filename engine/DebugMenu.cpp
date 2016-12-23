#include "../core/Colour.h"
#include "../core/MathUtils.h"

#include "CameraManager.h"
#include "FontManager.h"
#include "InputManager.h"
#include "Log.h"
#include "ModelManager.h"
#include "RenderManager.h"
#include "ScriptManager.h"
#include "StringHash.h"
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
, m_timePaused(false)
, m_debugPhysics(false)
, m_gameTimeScale(1.0f)
, m_dirtyFlags()
, m_lastMousePosRelative(0.0f)
, m_editType(EditType::None)
, m_editMode(EditMode::None)
, m_colourPicker(nullptr)
, m_colourPickerTextureMemory(nullptr)
, m_widgetToEdit(nullptr)
, m_gameObjectToEdit(nullptr)
, m_lightToEdit(nullptr)
, m_resourceSelect(nullptr)
, m_resourceSelectList(nullptr)
, m_btnResourceSelectOk(nullptr)
, m_btnResourceSelectCancel(nullptr)
, m_textInput(nullptr)
, m_textInputField(nullptr)
, m_btnTextInputOk(nullptr)
, m_btnTextInputCancel(nullptr)
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
#ifdef _RELEASE
	return true;
#endif

	Gui & gui = Gui::Get();
	InputManager & inMan = InputManager::Get();
	FontManager & fontMan = FontManager::Get();

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

	// Widgets to be used by script for debugging to the screen
	for (unsigned int i = 0; i < sc_numScriptDebugWidgets; ++i)
	{
		curItem.m_selectFlags = SelectionFlags::Rollover;
		curItem.m_colour = sc_colourWhite;
		curItem.m_name = "ScriptDebugWidget";
		curItem.m_fontSize = 2.0f;
		curItem.m_fontNameHash = fontMan.GetDebugFontName()->GetHash();
		if (m_scriptDebugWidgets[i] = gui.CreateWidget(curItem, gui.GetDebugRoot(), false))
		{
			m_scriptDebugWidgets[i]->SetDebugWidget();
			m_scriptDebugWidgets[i]->SetActive(false);
			m_scriptDebugWidgets[i]->SetAlwaysDraw();
		}
	}

	// Register global key and mouse listeners. Note these will be processed after the button callbacks
	inMan.RegisterKeyCallback(this, &DebugMenu::OnEnable, SDLK_TAB);
	inMan.RegisterKeyCallback(this, &DebugMenu::OnReload, SDLK_F5);
	inMan.RegisterKeyCallback(this, &DebugMenu::OnTimePause, SDLK_p);
	inMan.RegisterAlphaKeyCallback(this, &DebugMenu::OnAlphaKeyDown, InputType::KeyDown); 
	inMan.RegisterAlphaKeyCallback(this, &DebugMenu::OnAlphaKeyUp, InputType::KeyUp); 
	inMan.RegisterMouseCallback(this, &DebugMenu::OnActivate, MouseButton::Right);
	inMan.RegisterMouseCallback(this, &DebugMenu::OnSelect, MouseButton::Left);

	// Process vector cursor vertices to account for display aspect
	for (unsigned int i = 0; i < 4; ++i)
	{
		sc_vectorCursor[i].SetY(sc_vectorCursor[i].GetY() * RenderManager::Get().GetViewAspect());
	}

	// Setup the colour picker texture
	size_t colourPickerTextureMemorySize = sc_colourPickerTextureSize * sc_colourPickerTextureSize * ((sc_colourPickerTextureBpp) >> 3);
	m_colourPickerTextureMemory = malloc(colourPickerTextureMemorySize);
	int * currentPixel = (int*)m_colourPickerTextureMemory;
	for (int i = 0; i < sc_colourPickerTextureSize; ++i)
	{
		for (int j = 0; j < sc_colourPickerTextureSize; ++j)
		{
			float h = (float)j / sc_colourPickerTextureSize;
			float s = sinf(PI * (1.0f - (float)i / (float)sc_colourPickerTextureSize));
			float v = (float)i / (float)sc_colourPickerTextureSize;
			float r, g, b = 0.0f;
			Colour::HSVtoRGB(h, s, v, r, g, b);
			char * curChannel = (char*)currentPixel;
			*curChannel = (char)(r * 255.0f);
			curChannel++;
			*curChannel = (char)(g * 255.0f);
			curChannel++;
			*curChannel = (char)(b * 255.0f);
			curChannel++;
			*curChannel = 255u;
			currentPixel++;
		}
	}
	if (m_colourPickerTexture.LoadFromMemoryAndFree(sc_colourPickerTextureSize, sc_colourPickerTextureSize, sc_colourPickerTextureBpp, m_colourPickerTextureMemory))
	{
		m_colourPickerTextureMemory = nullptr;
	}

	// Setup the colour picker
	m_colourPicker = DebugMenuCommand::CreateButton("ChooseColour", gui.GetDebugRoot(), sc_colourWhite);
	m_colourPicker->SetTexture(&m_colourPickerTexture);
	m_colourPicker->SetSize(Vector2(0.5f, 0.5f));
	m_colourPicker->SetPos(WidgetVector(-0.25f, 0.25f));

	return true;
}

void DebugMenu::Shutdown() 
{ 
	m_commands.Shutdown(); 
	if (m_colourPickerTextureMemory)
	{
		free(m_colourPickerTextureMemory);
	}
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
	const Vector2 mousePos = inMan.GetMousePosRelative();
	const Vector2 amountToMove = (inMan.GetMousePosRelative() - m_lastMousePosRelative) * 10.0f;
	const float moveAmount = amountToMove.GetX() + amountToMove.GetY();
	if (m_editType == EditType::Widget && m_widgetToEdit != NULL)
	{
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
			case EditMode::FontSize:
			{
				m_widgetToEdit->SetFontSize((Vector2::Vector2Zero() - mousePos).LengthSquared() * 3.0f);
				break;
			}
			case EditMode::Colour:
			{
				const float fracX = (mousePos.GetX() + 0.25f) * 2.0f;
				const float fracY = MathUtils::Clamp(0.0f, ((mousePos.GetY() + 0.25f) * 2.0f), 1.0f);
				float h = MathUtils::Clamp(0.0f, fracX, 1.0f);
				float s = sinf(PI * (1.0f - fracY));
				float v = fracY;
				float r, g, b = 0.0f;
				Colour::HSVtoRGB(h, s, v, r, g, b);
				Colour modColour(r, g, b, 1.0f);
				m_widgetToEdit->SetColour(modColour);
				break;
			}
			default: break;
		}
	}
	else if (m_gameObjectToEdit != NULL)
	{
		if (m_editType == EditType::GameObject)
		{
			if (m_editMode == EditMode::ClipPosition)
			{
				Vector curPos = m_gameObjectToEdit->GetClipOffset();	
				if (inMan.IsKeyDepressed(SDLK_x))			m_gameObjectToEdit->SetClipOffset(curPos + Vector(moveAmount, 0.0f, 0.0f));		
				else if (inMan.IsKeyDepressed(SDLK_y))		m_gameObjectToEdit->SetClipOffset(curPos + Vector(0.0f, moveAmount, 0.0f));
				else if (inMan.IsKeyDepressed(SDLK_z))		m_gameObjectToEdit->SetClipOffset(curPos + Vector(0.0f, 0.0f, moveAmount));
				m_dirtyFlags.Set(DirtyFlag::Scene);
			}
			else if (m_editMode == EditMode::ClipSize)
			{
				Vector curSize = m_gameObjectToEdit->GetClipSize();	
				if (inMan.IsKeyDepressed(SDLK_x))			m_gameObjectToEdit->SetClipSize(curSize + Vector(moveAmount, 0.0f, 0.0f));
				else if (inMan.IsKeyDepressed(SDLK_y))		m_gameObjectToEdit->SetClipSize(curSize + Vector(0.0f, moveAmount, 0.0f));
				else if (inMan.IsKeyDepressed(SDLK_z))		m_gameObjectToEdit->SetClipSize(curSize + Vector(0.0f, 0.0f, moveAmount));
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


	if (m_editType == EditType::Light && m_lightToEdit != NULL)
	{
		if (!IsDebugMenuActive())
		{
			if (inMan.IsKeyDepressed(SDLK_x))
			{
				if (inMan.IsKeyDepressed(SDLK_LALT))
				{
					const Quaternion moveAmount(MathUtils::Deg2Rad(Vector(moveAmount * 16.0f, 0.0f, 0.0f)));
					m_lightToEdit->m_dir *= moveAmount;
				}
				else
				{
					m_lightToEdit->m_pos += Vector(moveAmount, 0.0f, 0.0f);
				}
				m_dirtyFlags.Set(DirtyFlag::Scene);
			} 
			else if (inMan.IsKeyDepressed(SDLK_y))
			{
				if (inMan.IsKeyDepressed(SDLK_LALT))
				{
					const Quaternion moveAmount(MathUtils::Deg2Rad(Vector(0.0f, moveAmount * 16.0f, 0.0f)));
					m_lightToEdit->m_dir *= moveAmount;
				}
				else
				{
					m_lightToEdit->m_pos += Vector(0.0f, moveAmount, 0.0f);
				}
				m_dirtyFlags.Set(DirtyFlag::Scene);				
			}
			else if (inMan.IsKeyDepressed(SDLK_z))
			{
				if (inMan.IsKeyDepressed(SDLK_LALT))
				{
					const Quaternion moveAmount(MathUtils::Deg2Rad(Vector(0.0f, 0.0f, moveAmount * 16.0f)));
					m_lightToEdit->m_dir *= moveAmount;
				}
				else
				{
					m_lightToEdit->m_pos += Vector(0.0f, 0.0f, moveAmount);
				}
				m_dirtyFlags.Set(DirtyFlag::Scene);
			}
		}
		switch (m_editMode)
		{
			case EditMode::Ambient:
			{
				const Vector2 colourVec = Vector2::Vector2Zero() - mousePos;
				const float colorMag = colourVec.LengthSquared() * 3.0f;
				Colour modColour(colourVec.GetX()*3.0f, colourVec.GetY()*3.0f, colorMag, 1.0f);
				m_lightToEdit->m_ambient = modColour;
				break;
			}
			case EditMode::Diffuse:
			{
				const Vector2 colourVec = Vector2::Vector2Zero() - mousePos;
				const float colorMag = colourVec.LengthSquared() * 3.0f;
				Colour modColour(colourVec.GetX()*3.0f, colourVec.GetY()*3.0f, colorMag, 1.0f);
				m_lightToEdit->m_diffuse = modColour;
				break;
			}
			case EditMode::Specular:
			{
				const Vector2 colourVec = Vector2::Vector2Zero() - mousePos;
				const float colorMag = colourVec.LengthSquared() * 3.0f;
				Colour modColour(colourVec.GetX()*3.0f, colourVec.GetY()*3.0f, colorMag, 1.0f);
				m_lightToEdit->m_specular = modColour;
				break;
			}
			default: break;
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
				case DirtyFlag::GUI:	
				{
					Gui::Get().GetActiveMenu()->Serialise();				
					changesSaved = true; 
					break;
				}
				case DirtyFlag::Scene:	
				{
					Scene * curScene = WorldManager::Get().GetCurrentScene();
					curScene->Serialise();		
					curScene->ResetFileDateStamp();
					changesSaved = true; 
					break;
				}
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
		const char * newResource = m_resourceSelectList->GetSelectedListItem();
		switch (m_editType)
		{
			case EditType::Widget: 
			{
				// Early out for no widget selected
				if (m_widgetToEdit == NULL) { break; }

				// Setting a texture on a widget
				if (m_editMode == EditMode::Texture)
				{
					if (newResource != NULL && newResource[0] != '\0' && strlen(newResource)  > 0)
					{
						char tgaBuf[StringUtils::s_maxCharsPerLine];
						sprintf(tgaBuf, "%s%s", TextureManager::Get().GetTexturePath(), newResource);
						m_widgetToEdit->SetTexture(TextureManager::Get().GetTexture(tgaBuf, TextureCategory::Gui));
					}
					else // Clear the texture
					{
						m_widgetToEdit->SetTexture(NULL);
					}
					m_dirtyFlags.Set(DirtyFlag::GUI);
				}
				else if (m_editMode == EditMode::Font)
				{
					if (m_resourceSelectList->GetSelectedListItem())
					{
						m_widgetToEdit->SetFontName(StringHash::GenerateCRC(m_resourceSelectList->GetSelectedListItem()));
					}
				}
				break;
			}
			case EditType::GameObject:
			{
				WorldManager & worldMan = WorldManager::Get();
				if (m_editMode == EditMode::Template)
				{
					// Create new object
					if (newResource != NULL && newResource[0] != '\0' && strlen(newResource)  > 0)
					{
						if (GameObject * newGameObj = worldMan.CreateObject(newResource))
						{
							// Set properties of new object to match that of old object
							newGameObj->SetPos(m_gameObjectToEdit->GetPos());

							// Destroy old object
							if (m_gameObjectToEdit)
							{
								worldMan.DestroyObject(m_gameObjectToEdit->GetId());
							}
							m_gameObjectToEdit = newGameObj;
					
							m_dirtyFlags.Set(DirtyFlag::Scene);
						}
						else
						{
							Log::Get().WriteEngineErrorNoParams("Failed to set template on game object.");
						}
					}
				}

				// Early out for no object
				if (m_gameObjectToEdit == NULL) { break; }

				// Setting a model on a game object
				if (m_editMode == EditMode::Model)
				{
					if (newResource != NULL && newResource[0] != '\0' && strlen(newResource)  > 0)
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
					else // Clear the model
					{
						m_gameObjectToEdit->SetModel(NULL);
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
	m_commands.HandleRightClick(m_widgetToEdit, m_gameObjectToEdit, m_lightToEdit);
	return true;
}

bool DebugMenu::OnReload(bool a_active)
{
#ifndef _RELEASE
	ScriptManager::Get().ReloadScripts();
#endif

	return true;
}

bool DebugMenu::OnTimePause(bool a_active)
{
#ifndef _RELEASE
	m_timePaused = !m_timePaused;
#endif

	return true;
}

bool DebugMenu::OnSelect(bool a_active)
{
#ifndef _RELEASE
	const Vector2 mousePos = InputManager::Get().GetMousePosRelative();

	// Respond to a click and set internal state if it's been handled by a command
	if (m_commands.IsActive())
	{
		DebugCommandReturnData retVal = m_commands.HandleLeftClick(m_widgetToEdit, m_gameObjectToEdit, m_lightToEdit);
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

		// If the commmand requires choosing a font
		if (retVal.m_editType == EditType::Widget && retVal.m_editMode == EditMode::Font)
		{
			ShowFontSelect();
		}

		// If the command requires choosing a colour
		if (retVal.m_editMode == EditMode::Colour)
		{
			ShowColourPicker();
		}

		// If the command cleared the selection of items
		if (retVal.m_clearSelection)
		{
			if (m_widgetToEdit != NULL)
			{
				m_widgetToEdit->ClearSelection();
			}
			m_widgetToEdit = NULL;
			m_gameObjectToEdit = NULL;
			m_lightToEdit = NULL;
		}
		return retVal.m_success;
	}

	// Stop any mouse bound editing on click
	if (m_editMode == EditMode::Pos  || 
		m_editMode == EditMode::Shape ||
		m_editMode == EditMode::FontSize)
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

	// Don't play around with widget selection while we are changing alignment
	if (m_editType == EditType::Widget && m_editMode == EditMode::Alignment)
	{
		// Handle change to alignment
		if (m_widgetToEdit != NULL)
		{
			Vector2 handlePos;
			AlignX::Enum selectionX = AlignX::Count;
			AlignY::Enum selectionY = AlignY::Count;
			bool setAlignment = false;
			if (m_widgetToEdit->GetAlignmentSelection(mousePos, Widget::sc_alignmentHandleSize, handlePos, selectionX, selectionY))
			{
				m_widgetToEdit->SetOffset(Vector2::Vector2Zero());
				m_widgetToEdit->SetAlignmentAnchor(selectionX, selectionY);
				m_dirtyFlags.Set(DirtyFlag::GUI);
				setAlignment = true;
			}
			if (m_widgetToEdit->GetAlignTo()->GetAlignmentSelection(mousePos, Widget::sc_alignmentHandleSize, handlePos, selectionX, selectionY))
			{
				m_widgetToEdit->SetOffset(Vector2::Vector2Zero());
				m_widgetToEdit->SetAlignment(selectionX, selectionY);
				m_dirtyFlags.Set(DirtyFlag::GUI);
				setAlignment = true;
			}

			// Exit from alignment edit mode
			if (!setAlignment)
			{
				m_editType = EditType::None;
				m_editMode = EditMode::None;
				m_widgetToEdit->ClearSelection();
				m_widgetToEdit = NULL;
			}
		}
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
		// Handle widget selection when changing alignment parent
		if (m_editType == EditType::Widget && 
			m_editMode == EditMode::AlignmentParent &&
			m_widgetToEdit != NULL &&
			m_widgetToEdit != newSelectedWidget)
		{
			m_widgetToEdit->SetOffset(Vector2::Vector2Zero());
			m_widgetToEdit->SetAlignTo(newSelectedWidget);
			m_dirtyFlags.Set(DirtyFlag::GUI);
		}

		// Clear selection of old widget
		if (m_widgetToEdit != NULL && m_widgetToEdit != newSelectedWidget)
		{
			m_widgetToEdit->ClearSelection();
		}
		m_editType = EditType::Widget;
		m_widgetToEdit = newSelectedWidget;
		m_widgetToEdit->SetSelection(SelectionFlags::EditSelected);
		m_gameObjectToEdit = NULL;
		m_lightToEdit = NULL;
	}
	else // Cancel selections
	{
		// Hide the colour picker
		if (m_editMode == EditMode::Colour)
		{
			HideColourPicker();
		}
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
		// Picking point is the mouse cursor transformed to 3D space in cam direction
		bool selectedObject = false;
		const float pickDepth = 10000.0f;
		RenderManager & renMan = RenderManager::Get();
		CameraManager & camMan = CameraManager::Get();
		Matrix camMat = camMan.GetCameraMatrix();
		Vector camPos = camMan.GetWorldPos();

		// Translate by mouse position for picking
		Vector mouseOffset3D = camMat.GetInverse().Transform(Vector(mousePos.GetX() * 0.1f, mousePos.GetY() * 0.05f, 0.0f));
		Vector pickStart = camPos + mouseOffset3D;
		Vector pickEnd = camMat.GetInverse().Transform(Vector(0.0f, 0.0, -pickDepth)) + (mouseOffset3D * 1000.0f);

		// Do picking with all the game objects in the scene
		if (Scene * curScene = WorldManager::Get().GetCurrentScene())
		{
			// Pick an arbitrary object (would have to sort to get the closest)
			GameObject * foundObject = curScene->GetSceneObject(pickStart, pickEnd);
			if (foundObject != NULL)
			{
				m_gameObjectToEdit = foundObject;
				m_widgetToEdit = NULL;
				m_lightToEdit = NULL;
				m_editType = EditType::GameObject;
				selectedObject = true;
			}
		}

		// Couldn't find a game object, try a light
		if (!selectedObject)
		{
			if (Scene * curScene = WorldManager::Get().GetCurrentScene())
			{
				Light * foundLight = curScene->GetLight(pickStart, pickEnd);
				if (foundLight != NULL)
				{
					m_lightToEdit = foundLight;
					m_gameObjectToEdit = NULL;
					m_widgetToEdit = NULL;
					m_editType = EditType::Light;
					m_editMode = EditMode::None;
					selectedObject = true;
				}
			}
		}

		// Couldn't find anything, clear selection flags
		if (!selectedObject)
		{
			m_widgetToEdit = NULL;
			m_gameObjectToEdit = NULL;
			m_lightToEdit = NULL;
			m_editType = EditType::None;
			m_editMode = EditMode::None;
		}

	}

	// Cancel all menu display
	if (m_gameObjectToEdit == NULL && m_widgetToEdit == NULL)
	{
		m_commands.Hide();
	}

#endif

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

		SDL_Keycode lastKey = inMan.GetLastKey();
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
			SDL_Keycode lastKey = inMan.GetLastKey();
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

	// Add a null option for clearing selection
	m_resourceSelectList->ClearListItems();
	m_resourceSelectList->AddListItem("");

	// Add resource list to widget
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

void DebugMenu::ShowFontSelect()
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
	m_resourceSelectList->ClearListItems();
	
	// Add fonts list to widget
	FontManager & fontMan = FontManager::Get();
	
	const int numFonts = fontMan.GetNumLoadedFonts();
	for (int i = 0; i < numFonts; ++i)
	{
		m_resourceSelectList->AddListItem(fontMan.GetLoadedFontNameForId(i));
	}
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

void DebugMenu::ShowColourPicker()
{
	if (m_colourPicker != NULL)
	{
		m_colourPicker->SetActive();
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
			m_scriptDebugWidgets[i]->SetActive(true);
			const WidgetVector boxSize = WidgetVector(m_scriptDebugWidgets[i]->GetTextWidth(), m_scriptDebugWidgets[i]->GetTextHeight());
			const WidgetVector boxPos = WidgetVector(a_posX, a_posY - (i*0.5f*m_scriptDebugWidgets[i]->GetSize().GetY()));
			m_scriptDebugWidgets[i]->SetOffset(boxPos);
			m_scriptDebugWidgets[i]->SetSize(boxSize);
			return true;
		}
	}

	Log::Get().WriteGameErrorNoParams("Ran out of debug DebugPrint resources!");
	return false;
}

void DebugMenu::Draw()
{
#ifndef _RELEASE
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
		Vector curLineX = gridStart + Vector(x*gridMeasurement, 0.0f, -1.0f);
		renMan.AddLine(RenderLayer::Debug3D, curLineX, curLineX + Vector(0.0f, gridMeasurement * (float)(gridSize), 0.0f), sc_colourGreyAlpha);	
	}

	// Gridlines on the Y axis
	for (unsigned int y = 0; y < gridSize+1; ++y)
	{
		Vector curLineY = gridStart + Vector(0.0f, y*gridMeasurement, -1.0f);
		renMan.AddLine(RenderLayer::Debug3D, curLineY, curLineY + Vector(gridMeasurement * (float)(gridSize), 0.0f, 0.0f), sc_colourGreyAlpha);
	}

	// Draw an identity matrix nearby the origin (not directly on to avoid Z fighting)
	Matrix fakeIdentity = Matrix::Identity();
	fakeIdentity.Translate(Vector(0.0f, 0.0f, EPSILON));
	renMan.AddDebugTransform(fakeIdentity);

	// Draw selection box around objects - slightly larger than clip size so there is no z fighting
	const float extraSelectionSize = 0.025f;
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

	// Draw selection box around lights
	if (m_lightToEdit != NULL)
	{
		renMan.AddDebugSphere(m_lightToEdit->m_pos, Light::s_lightDrawSize + extraSelectionSize, sc_colourRed);
	}
	
	// Draw selected widget alignment
	if (m_editMode == EditMode::Alignment && m_widgetToEdit != NULL)
	{
		m_widgetToEdit->DrawDebugAlignment();
	}

	// Draw magnitude of size editing
	Vector2 mousePos = InputManager::Get().GetMousePosRelative();
	if (m_widgetToEdit != NULL && m_editMode == EditMode::FontSize)
	{
		char sizeBuf[16];
		sprintf(sizeBuf, "%f", m_widgetToEdit->GetFontSize());
		renMan.AddDebugArrow2D(Vector2::Vector2Zero(), mousePos, sc_colourWhite);
		fontMan.DrawDebugString2D(sizeBuf, mousePos, sc_colourWhite);
	}

	// Draw colour editing widget
	if (m_widgetToEdit != NULL && m_editMode == EditMode::Colour)
	{
		char colourBuf[64];
		const Colour widgetColour = m_widgetToEdit->GetColour();
		widgetColour.GetString(colourBuf);
		renMan.AddDebugArrow2D(Vector2::Vector2Zero(), mousePos, widgetColour);
		fontMan.DrawDebugString2D(colourBuf, mousePos, widgetColour);
	}

	// Draw light editing widget
	if (m_lightToEdit != NULL && m_editType == EditType::Light && m_editMode != EditMode::None)
	{
		Colour colourVal(0.0f);
		switch (m_editMode)
		{
			case EditMode::Ambient: colourVal = m_lightToEdit->m_ambient; break; 
			case EditMode::Diffuse: colourVal = m_lightToEdit->m_diffuse; break;
			case EditMode::Specular: colourVal = m_lightToEdit->m_specular; break;
			default: break;
		}
		char colourBuf[64];
		colourVal.GetString(colourBuf);
		renMan.AddDebugArrow2D(Vector2::Vector2Zero(), mousePos, colourVal);
		fontMan.DrawDebugString2D(colourBuf, mousePos, colourVal);
	}

	// Show mouse pos at cursor
	char mouseBuf[16];
	sprintf(mouseBuf, "%.2f, %.2f", mousePos.GetX(), mousePos.GetY());
	Vector2 displayPos(mousePos.GetX() + sc_cursorSize, mousePos.GetY() - sc_cursorSize);
	fontMan.DrawDebugString2D(mouseBuf, displayPos, sc_colourGreen);

	// Draw mouse cursor
	for (int i = 0; i < 3; ++i)
	{
		renMan.AddLine2D(RenderLayer::Debug2D, mousePos+sc_vectorCursor[i], mousePos+sc_vectorCursor[i+1], sc_colourGreen);
	}
	renMan.AddLine2D(RenderLayer::Debug2D, mousePos+sc_vectorCursor[3], mousePos+sc_vectorCursor[0], sc_colourGreen);
#endif
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

void DebugMenu::HideColourPicker()
{
	if (m_colourPicker != NULL)
	{
		m_colourPicker->SetActive(false);
	}
}