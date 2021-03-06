#include <assert.h>

#include "DataPack.h"
#include "Log.h"
#include "InputManager.h"
#include "FontManager.h"
#include "FileManager.h"
#include "GameFile.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "ScriptManager.h"
#include "StringUtils.h"

#include "Gui.h"

template<> Gui * Singleton<Gui>::s_instance = nullptr;

bool Gui::s_cursorBlink = false;
float Gui::s_cursorBlinkTimer = 0.0f;
const float Gui::s_cursorBlinkTime = 0.55f;
float Gui::s_widgetPulseTimer = 0.0f;

bool Gui::Startup(const char * a_guiPath, const DataPack * a_dataPack)
{
	// Cache off the gui path for later use when loading menus
	strncpy(m_guiPath, a_guiPath, strlen(a_guiPath) + 1);

	// Load in the gui config file
	char fileName[StringUtils::s_maxCharsPerLine];
	sprintf(fileName, "%s%s", a_guiPath, "gui.cfg");

	if (a_dataPack != nullptr && a_dataPack->IsLoaded())
	{
		if (DataPackEntry * guiConfig = a_dataPack->GetEntry(fileName))
		{
			m_configFile.Load(guiConfig);
		}
	}
	else
	{
		m_configFile.Load(fileName);
	}
	
	// Setup the debug menu root element
	m_debugRoot.SetName("DebugMenu");
	m_debugRoot.SetActive(false);
	m_debugRoot.SetDebugWidget();

	// Load menus from data
	LoadMenus(a_guiPath, a_dataPack);

	// There are no menus loaded
	if (m_menus.GetLength() == 0)
	{
		Log::Get().Write(LogLevel::Warning, LogCategory::Engine, "No menus loaded, a default menu has been created.");

		// Create one and add it to the list
		Widget * defaultMenu = new Widget();
		defaultMenu->SetName("New Menu");
		sprintf(fileName, "%s%s", a_guiPath, "newMenu.mnu");
		defaultMenu->SetFilePath(fileName);
		defaultMenu->SetActive(false);

		MenuListNode * newMenuNode = new MenuListNode();
		newMenuNode->SetData(defaultMenu);
		
		m_menus.Insert(newMenuNode);
	}

	// Default the active menu
	if (m_startupMenu == nullptr)
	{
		Log::Get().WriteGameErrorNoParams("No menu found with beginLoaded set to true, defaulting to the first menu loaded!");
		m_startupMenu = m_menus.GetHead()->GetData();
	}
	m_activeMenu = m_startupMenu;
	m_activeMenu->SetActive();

	// Setup the mouse cursor element if present in config file
	m_cursor.SetActive(false);
	if (GameFile::Object * configObj = m_configFile.FindObject("config"))
	{
		if (GameFile::Property * mouseCursorProp = configObj->FindProperty("mouseCursorTexture"))
		{
			m_cursor.SetTexture(TextureManager::Get().GetTexture(mouseCursorProp->GetString(), TextureCategory::Gui));
			m_cursor.SetAlignment(AlignX::Middle, AlignY::Centre);
			m_cursor.SetOffset(Vector2(0.0f, 0.0f));
			m_cursor.SetSize(Vector2(0.16f / RenderManager::Get().GetViewAspect(), 0.16f));
			m_cursor.SetActive(true);
		}
	}
	
	// Setup input callbacks for handling events, left mouse buttons activate gui elements
	InputManager & inMan = InputManager::Get();
	inMan.RegisterMouseCallback(this, &Gui::MouseInputHandler, MouseButton::Left);
	inMan.RegisterMouseCallback(this, &Gui::MouseInputHandler, MouseButton::Right);

	return true;
}

bool Gui::Shutdown()
{
	UnloadMenus();

	return true;
}

bool Gui::Update(float a_dt)
{
	// Update mouse position
	m_cursor.SetOffset(InputManager::Get().GetMousePosRelative()); 
	
	// Process mouse position for selection of widgets
	UpdateSelection();

	// Update the pulsing of selected widgets that have actions
	s_widgetPulseTimer += a_dt * 16.0f;
	Widget::s_selectedColourValue = sin(s_widgetPulseTimer);

	// Draw all elements of the active menu
	if (m_activeMenu != nullptr)
	{
		m_activeMenu->Draw();
	}

	// Draw debug menu elements over the top of the menu
	m_debugRoot.Draw();

	// Draw the name of the current menu
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		FontManager::Get().DrawDebugString2D(m_activeMenu->GetName(), Vector2(-1.0, 1.0f));
	}
	else // Draw game mouse cursor over the top of the gui
	{
		m_cursor.Draw();
	}

	// Blink the text cursor
	s_cursorBlinkTimer += a_dt;
	if (s_cursorBlinkTimer >= s_cursorBlinkTime)
	{
		s_cursorBlink = !s_cursorBlink;
		s_cursorBlinkTimer = 0.0f;
	}
	
	return true;
}

void Gui::ReloadMenus()
{
	UnloadMenus();
	LoadMenus(m_guiPath, nullptr);
	m_activeMenu = m_startupMenu;
	m_activeMenu->SetActive();
}

Widget * Gui::CreateWidget(const Widget::WidgetDef & a_def, Widget * a_parent, bool a_startActive)
{
	// Check for a valid parent
	if (a_parent == nullptr) 
	{	
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Widget creation failed due to an invalid parent.");
		return nullptr;
	}

	// Copy properties over to the managed element
	Widget * newWidget = new Widget();
	newWidget->SetName(a_def.m_name);
	newWidget->SetColour(a_def.m_colour);
	newWidget->SetSize(a_def.m_size);
	newWidget->SetOffset(a_def.m_pos);
	newWidget->SetAlignment(a_def.m_pos.GetAlignment());
	newWidget->SetAlignmentAnchor(a_def.m_pos.GetAlignmentAnchor());
	newWidget->SetFontName(a_def.m_fontNameHash);
	newWidget->SetFontSize(a_def.m_fontSize <= 0.5f ? 1.0f : a_def.m_fontSize);
	newWidget->SetSelectFlags(a_def.m_selectFlags);
	newWidget->SetActive(a_startActive);

	a_parent->AddChild(newWidget);
	
	return newWidget;
}


Widget * Gui::CreateWidget(GameFile::Object * a_widgetFile, Widget * a_parent, bool a_startActive)
{
	// Check for a valid parent
	if (a_parent == nullptr) 
	{	
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Widget creation from file failed due to an invalid parent.");
		return nullptr;
	}

	// Get properties from the file that correspond to widget definitions
	Widget::WidgetDef defFromFile;
	if (GameFile::Property * colour = a_widgetFile->FindProperty("colour"))
	{
		defFromFile.m_colour = colour->GetColour();
	}
	if (GameFile::Property * pos = a_widgetFile->FindProperty("offset"))
	{
		defFromFile.m_pos = pos->GetVector2();
	}
	if (GameFile::Property * size = a_widgetFile->FindProperty("size"))
	{
		defFromFile.m_size = size->GetVector2();
	}
	// Set alignment and anchor
	Alignment align;
	Alignment alignAnchor;
	if (GameFile::Property * alignX = a_widgetFile->FindProperty("alignX"))
	{
		align.SetXFromString(a_widgetFile->FindProperty("alignX")->GetString());
	}
	if (GameFile::Property * alignY = a_widgetFile->FindProperty("alignY"))
	{
		align.SetYFromString(a_widgetFile->FindProperty("alignY")->GetString());
	}
	if (GameFile::Property * alignX = a_widgetFile->FindProperty("alignAnchorX"))
	{
		alignAnchor.SetXFromString(a_widgetFile->FindProperty("alignAnchorX")->GetString());
	}
	if (GameFile::Property * alignY = a_widgetFile->FindProperty("alignAnchorY"))
	{
		alignAnchor.SetYFromString(a_widgetFile->FindProperty("alignAnchorY")->GetString());
	}
	defFromFile.m_pos.SetAlignment(align);
	defFromFile.m_pos.SetAlignmentAnchor(alignAnchor);
	if (GameFile::Property * fontSize = a_widgetFile->FindProperty("fontSize"))
	{
		defFromFile.m_fontSize = fontSize->GetFloat();
	}
	if (GameFile::Property * fontName = a_widgetFile->FindProperty("font"))
	{
		defFromFile.m_fontNameHash = StringHash::GenerateCRC(fontName->GetString());
	}
	else // No font name supplied, use debug
	{
		// Check for a loaded debug font
		if (StringHash * debugFont = FontManager::Get().GetDebugFontName())
		{
			defFromFile.m_fontNameHash = debugFont->GetHash();
		}
	}
	
	// Create the widget
	if (Widget * newWidget = CreateWidget(defFromFile, a_parent, a_startActive))
	{
		// Apply properties not set during creation
		if (GameFile::Property * name = a_widgetFile->FindProperty("name"))
		{
			newWidget->SetName(name->GetString());
		}
		if (GameFile::Property * text = a_widgetFile->FindProperty("text"))
		{
			newWidget->SetText(text->GetString());
		}
		if (GameFile::Property * colour = a_widgetFile->FindProperty("colour"))
		{
			newWidget->SetColour(colour->GetColour());
		}
		if (GameFile::Property * alignTo = a_widgetFile->FindProperty("alignTo"))
		{
			newWidget->SetAlignTo(alignTo->GetString());
		}
		if (GameFile::Property * texture = a_widgetFile->FindProperty("texture"))
		{
			if (Texture * tex = TextureManager::Get().GetTexture(texture->GetString(), TextureCategory::Gui))
			{
				newWidget->SetTexture(tex);
			}
		}
		if (GameFile::Property * action = a_widgetFile->FindProperty("action"))
		{		
			newWidget->SetScriptFuncName(action->GetString());
			newWidget->SetAction(&ScriptManager::Get(), &ScriptManager::OnWidgetAction);
		}
		return newWidget;
	}
	return nullptr;
}

Widget * Gui::FindWidget(const char * a_widgetName)
{
	// Search the active menu first
	Widget * foundWidget = nullptr;
	if (m_activeMenu != nullptr)
	{
		foundWidget = m_activeMenu->Find(a_widgetName);
	}

	// Search through all menus if not found
	if (foundWidget == nullptr)
	{
		MenuListNode * cur = m_menus.GetHead();
		while(cur != nullptr && foundWidget == nullptr)
		{
			foundWidget = cur->GetData()->Find(a_widgetName);
			cur = cur->GetNext();
		}
	}

	return foundWidget;
}

Widget * Gui::FindSelectedWidget()
{
	// Only search the active menu, nothing outside of active can be selected
	Widget * foundWidget = nullptr;
	if (m_activeMenu != nullptr)
	{
		return m_activeMenu->FindSelected();
	}

	return nullptr;
}

void Gui::DestroyWidget(Widget * a_widget)
{
#ifndef _RELEASE
	// First check that this is a debug widget
	if (a_widget->IsDebugWidget())
	{
		assert(m_debugRoot.RemoveFromChildren(a_widget));
		delete a_widget;
	}
	else
	{
#endif
		// Clean up any links
		a_widget->ClearAction();
	
		// Remove any children
		a_widget->RemoveChildren();

		// Remove any align to references or references in parent lists, menus should be the ultimate parent of any widget
		MenuListNode * removed = nullptr;
		bool removedFromChild = false;
		auto cur = m_menus.GetHead();
		while (cur != nullptr)
		{
			MenuListNode * next = cur->GetNext();
			Widget * curParent = cur->GetData();
			if (curParent == a_widget)
			{
				removed = cur;
			}
			else if (cur->GetData()->RemoveFromChildren(a_widget))
			{
				removedFromChild = true;
			}
			
			// Need to exhaustively remove any alignment to this widget
			curParent->RemoveAlignmentTo(a_widget);

			cur = next;
		}
		
		// Nothing else should be hanging on to this, get rid of it
		if (removed != nullptr)
		{
			m_menus.RemoveDelete(removed);
		}
		else if (removedFromChild)
		{
			delete a_widget;
		}
		else // Something went wrong
		{
			assert(false);
		}
#ifndef _RELEASE
	}
#endif
}

bool Gui::MouseInputHandler(bool active)
{
	// The mouse was clicked, check if any elements were rolled over
	bool activated = m_debugRoot.DoActivation();

	if (m_activeMenu != nullptr)
	{
		activated &= m_activeMenu->DoActivation();
	}

	return activated;
}

Widget * Gui::GetActiveWidget()
{
	// Early out if there is no active menu
	if (m_activeMenu == nullptr)
	{
		return nullptr;
	}

	// Iterate through all children of the active menu
	Widget * selected = nullptr;
	DebugMenu & dbgMen = DebugMenu::Get();
	Widget::WidgetNode * curChild = m_activeMenu->GetChildren();
	while (curChild != nullptr)
	{	
		// If we are in editing mode process editing selections
		if (dbgMen.IsDebugMenuEnabled())
		{
			if (curChild->GetData()->IsSelected(SelectionFlags::EditRollover))
			{
				selected = curChild->GetData();
				break;
			}
		}
		else // Otherwise perform normal selection changes
		{
			if (curChild->GetData()->IsSelected())
			{
				selected = curChild->GetData();
				break;
			}
		}
		curChild = curChild->GetNext();
	}

	return selected;
}

bool Gui::LoadMenu(const GameFile & a_menuFile, const DataPack * a_dataPack)
{
	// Load the menu file
	Widget * createdMenu = nullptr;
	if (a_menuFile.IsLoaded())
	{
		// Create a new widget and copy properties from file
		if (GameFile::Object * menuObject = a_menuFile.FindObject("menu"))
		{
			if (GameFile::Property * nameProp = menuObject->FindProperty("name"))
			{
				createdMenu = new Widget();
				createdMenu->SetName(a_menuFile.GetString("menu", "name"));
				if (a_dataPack == nullptr || !a_dataPack->IsLoaded())
				{
					char menuFilePath[StringUtils::s_maxCharsPerLine];
					sprintf(menuFilePath, "%s%s.mnu", m_guiPath, createdMenu->GetName());
					createdMenu->SetFilePath(menuFilePath);
				}
				createdMenu->SetActive(false);

				// TODO Support for non fullscreen menus
				createdMenu->SetSize(Vector2(2.0f, 2.0f));
				createdMenu->SetDrawPos(Vector2(-1.0, 1.0));

				// Load child elements of the menu
				auto childWidget = menuObject->GetChildren();
				while (childWidget != nullptr)
				{
					GameFile::Object * curObject = childWidget->GetData();
					Widget * newChild = CreateWidget(curObject, createdMenu);
					// If not specified in the file, align child widgets to the parent menu
					if (!newChild->HasAlignTo())
					{
						newChild->SetAlignTo(createdMenu);
					}
					childWidget = childWidget->GetNext();
				}

				// Add to list of menus
				m_menus.InsertNew(createdMenu);
			}
			else // No properties present
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading menu file %s, menu does not have a name property.", a_menuFile);
			}

			// Set the active menu to the last menu with the begin loaded property
			if (createdMenu != nullptr && menuObject->FindProperty("beginLoaded"))
			{
				if (menuObject->FindProperty("beginLoaded")->GetBool())
				{
					m_startupMenu = createdMenu;
				}
			}
		}
		else // Unexpected file format, no root element
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading menu file %s, no valid menu parent element.", a_menuFile);
		}

		return true;
	}
	return false;
}

bool Gui::LoadMenus(const char * a_guiPath, const DataPack * a_dataPack)
{
	bool loadSuccess = true;
	if (a_dataPack != nullptr && a_dataPack->IsLoaded())
	{
		DataPack::EntryList menuEntries;
		a_dataPack->GetAllEntries(".mnu", menuEntries);
		DataPack::EntryNode * curNode = menuEntries.GetHead();

		// Load each menu in the list
		bool loadSuccess = true;
		while (curNode != nullptr)
		{
			GameFile menuFile(curNode->GetData());
			loadSuccess &= LoadMenu(menuFile, a_dataPack);
			curNode = curNode->GetNext();
		}

		// Clean up the list of menu data pack entries
		a_dataPack->CleanupEntryList(menuEntries);
	}
	else
	{
		// Get a list of menu files in the gui directory
		FileManager & fileMan = FileManager::Get();
		FileManager::FileList menuFiles;
		fileMan.FillFileList(a_guiPath, menuFiles, ".mnu");

		// Load each menu in the directory
		FileManager::FileListNode * curNode = menuFiles.GetHead();
		while (curNode != nullptr)
		{
			char fullPath[StringUtils::s_maxCharsPerLine];
			sprintf(fullPath, "%s%s", a_guiPath, curNode->GetData()->m_name);
			GameFile menuFile(fullPath);
			loadSuccess &= LoadMenu(menuFile, a_dataPack);
			curNode = curNode->GetNext();
		}

		// Clean up the list of fonts
		fileMan.CleanupFileList(menuFiles);
	}

	return loadSuccess;
}

bool Gui::UnloadMenus()
{
	MenuListNode * cur = m_menus.GetHead();
	while(cur != nullptr)
	{
		// Cache off next pointer
		MenuListNode * next = cur->GetNext();
		
		// Clean up widget and family and delete it
		DestroyWidget(cur->GetData());

		cur = next;
	}

	return true;
}

void Gui::UpdateSelection()
{
	// Always update the debug menu
	m_debugRoot.UpdateSelection(m_cursor.GetPos());
	
	// Any children of the active menu will be updated
	if (!DebugMenu::Get().IsDebugMenuActive() && m_activeMenu != nullptr)
	{
		m_activeMenu->UpdateSelection(m_cursor.GetPos());
	}
}