#include "Log.h"
#include "InputManager.h"
#include "FileManager.h"
#include "GameFile.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "ScriptManager.h"
#include "StringUtils.h"

#include "Gui.h"

template<> Gui * Singleton<Gui>::s_instance = NULL;

bool Gui::s_cursorBlink = false;
float Gui::s_cursorBlinkTimer = 0.0f;
const float Gui::s_cursorBlinkTime = 0.55f;

bool Gui::Startup(const char * a_guiPath)
{
	// Cache off the gui path for later use when loading menus
	strncpy(m_guiPath, a_guiPath, strlen(a_guiPath) + 1);

	// Load in the gui config file
	char fileName[StringUtils::s_maxCharsPerLine];
	sprintf(fileName, "%s%s", a_guiPath, "gui.cfg");
	m_configFile.Load(fileName);

	// Setup the debug menu root element
	m_debugRoot.SetName("DebugMenu");
	m_debugRoot.SetActive(false);
	m_debugRoot.SetDebugWidget();

	// Load menus from data
	LoadMenus(a_guiPath);

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
	
	// Set the active menu to the first loaded menu
	m_activeMenu = m_menus.GetHead()->GetData();

	// Setup the mouse cursor element if present in config file
	m_cursor.SetActive(false);
	if (GameFile::Object * configObj = m_configFile.FindObject("config"))
	{
		if (GameFile::Property * mouseCursorProp = configObj->FindProperty("mouseCursorTexture"))
		{
			sprintf(fileName, "%s%s", a_guiPath, mouseCursorProp->GetString());
			m_cursor.SetTexture(TextureManager::Get().GetTexture(fileName, TextureCategory::Gui));
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

	// Draw all elements of the active menu
	if (m_activeMenu != NULL)
	{
		m_activeMenu->Draw();
	}

	// Draw debug menu elements over the top of the menu
	m_debugRoot.Draw();

	// Draw mouse cursor over the top of the gui
	if (!DebugMenu::Get().IsDebugMenuEnabled())
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

Widget * Gui::CreateWidget(const Widget::WidgetDef & a_def, Widget * a_parent, bool a_startActive)
{
	// Check for a valid parent
	if (a_parent == NULL) 
	{	
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Widget creation failed due to an invalid parent.");
		return NULL;
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
	if (a_parent == NULL) 
	{	
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Widget creation from file failed due to an invalid parent.");
		return NULL;
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
		return newWidget;
	}
	return NULL;
}

Widget * Gui::FindWidget(const char * a_widgetName)
{
	// Search the active menu first
	Widget * foundWidget = NULL;
	if (m_activeMenu != NULL)
	{
		foundWidget = m_activeMenu->Find(a_widgetName);
	}

	// Search through all menus if not found
	if (foundWidget == NULL)
	{
		MenuListNode * cur = m_menus.GetHead();
		while(cur != NULL && foundWidget == NULL)
		{
			foundWidget = cur->GetData()->Find(a_widgetName);
			cur = cur->GetNext();
		}
	}

	return foundWidget;
}

void Gui::DestroyWidget(Widget * a_widget)
{
	// Clean up any links
	a_widget->ClearAction();
	
	// Remove any align to references or references in parent lists
	MenuListNode * cur = m_menus.GetHead();
	while(cur != NULL)
	{
		cur->GetData()->RemoveAlignmentTo(a_widget);
		cur->GetData()->RemoveFromChildren(a_widget);
		cur = cur->GetNext();
	}

	// Delete child widgets
	Widget::WidgetNode * curWidget = a_widget->GetChildren();
	while (curWidget != NULL)
	{
		Widget::WidgetNode * next = curWidget->GetNext();
		DestroyWidget(curWidget->GetData());
		delete curWidget;
		curWidget = next;
	}

	// Nothing else should be hanging on to this, get rid of it
	delete a_widget;
}

bool Gui::MouseInputHandler(bool active)
{
	// The mouse was clicked, check if any elements were rolled over
	bool activated = m_debugRoot.DoActivation();

	if (m_activeMenu != NULL)
	{
		activated &= m_activeMenu->DoActivation();
	}

	return activated;
}

Widget * Gui::GetActiveWidget()
{
	// Early out if there is no active menu
	if (m_activeMenu == NULL)
	{
		return NULL;
	}

	// Iterate through all children of the active menu
	Widget * selected = NULL;
	DebugMenu & dbgMen = DebugMenu::Get();
	Widget::WidgetNode * curChild = m_activeMenu->GetChildren();
	while (curChild != NULL)
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

bool Gui::LoadMenu(const char * a_menuFile)
{
	// Load the menu file
	GameFile * menuFile = new GameFile();
	if (menuFile->Load(a_menuFile))
	{
		// Create a new widget and copy properties from file
		if (GameFile::Object * menuObject = menuFile->FindObject("menu"))
		{
			if (GameFile::Property * nameProp = menuObject->FindProperty("name"))
			{
				Widget * parentMenu = new Widget();
				parentMenu->SetName(menuFile->GetString("menu", "name"));
				parentMenu->SetFilePath(a_menuFile);
				parentMenu->SetActive(false);

				// TODO Support for non fullscreen menus
				parentMenu->SetSize(Vector2(2.0f, 2.0f));
				parentMenu->SetDrawPos(Vector2(-1.0, 1.0));

				// Load child elements of the menu
				LinkedListNode<GameFile::Object> * childWidget = menuObject->GetChildren();
				while (childWidget != NULL)
				{
					GameFile::Object * curObject = childWidget->GetData();
					Widget * newChild = CreateWidget(curObject, parentMenu);
					// If not specified in the file, align child widgets to the parent menu
					if (!newChild->HasAlignTo())
					{
						newChild->SetAlignTo(parentMenu);
					}
					childWidget = childWidget->GetNext();
				}

				// Add to list of menus
				MenuListNode * newMenuNode = new MenuListNode();
				newMenuNode->SetData(parentMenu);
				m_menus.Insert(newMenuNode);
			}
			else // No properties present
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading menu file %s, menu does not have a name property.", a_menuFile);
			}
		}
		else // Unexpected file format, no root element
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error loading menu file %s, no valid menu parent element.", a_menuFile);
		}

		return true;
	}

	// Cleanup and return
	delete menuFile;
	return false;
}

bool Gui::LoadMenus(const char * a_guiPath)
{
	// Get a list of menu files in the gui directory
	FileManager & fileMan = FileManager::Get();
	FileManager::FileList menuFiles;
	fileMan.FillFileList(a_guiPath, menuFiles, ".mnu");

	// Load each menu in the directory
	bool loadSuccess = true;
	FileManager::FileListNode * curNode = menuFiles.GetHead();
	while(curNode != NULL)
	{
		char fullPath[StringUtils::s_maxCharsPerLine];
		sprintf(fullPath, "%s%s", a_guiPath, curNode->GetData()->m_name);
		loadSuccess &= LoadMenu(fullPath);
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	fileMan.CleanupFileList(menuFiles);

	return loadSuccess;
}

bool Gui::UnloadMenus()
{
	MenuListNode * next = m_menus.GetHead();
	while(next != NULL)
	{
		// Cache off next pointer
		MenuListNode * cur = next;
		next = cur->GetNext();

		// Remove from list
		m_menus.Remove(cur);
		
		// Clean up all widgets and family
		DestroyWidget(cur->GetData());
		delete cur;
	}

	return true;
}

void Gui::UpdateSelection()
{
	// Any children of the active menu will be updated
	m_debugRoot.UpdateSelection(m_cursor.GetPos());
	
	if (!DebugMenu::Get().IsDebugMenuActive() && m_activeMenu != NULL)
	{
		m_activeMenu->UpdateSelection(m_cursor.GetPos());
	}
}