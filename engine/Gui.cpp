#include "Log.h"
#include "InputManager.h"
#include "FileManager.h"
#include "GameFile.h"
#include "LuaScript.h"
#include "RenderManager.h"
#include "TextureManager.h"
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

	// Load in the gui texture
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
		Log::Get().Write(Log::LL_WARNING, Log::LC_ENGINE, "No menus loaded, a default menu has been created.");

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
			m_cursor.SetTexture(TextureManager::Get().GetTexture(fileName, TextureManager::eCategoryGui));
			m_cursor.SetPos(Vector2(0.0f, 0.0f));
			m_cursor.SetSize(Vector2(0.16f / RenderManager::Get().GetViewAspect(), 0.16f));
			m_cursor.SetActive(true);
		}
	}
	
	// Setup input callbacks for handling events, left mouse buttons activate gui elements
	InputManager & inMan = InputManager::Get();
	inMan.RegisterMouseCallback(this, &Gui::MouseInputHandler, InputManager::eMouseButtonLeft);
	inMan.RegisterMouseCallback(this, &Gui::MouseInputHandler, InputManager::eMouseButtonRight);

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
	m_cursor.SetPos(InputManager::Get().GetMousePosRelative()); 

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
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Widget creation failed due to an invalid parent.");
		return NULL;
	}

	// Copy properties over to the managed element
	Widget * newWidget = new Widget();
	newWidget->SetName(a_def.m_name);
	newWidget->SetColour(a_def.m_colour);
	newWidget->SetSize(a_def.m_size);
	newWidget->SetPos(a_def.m_pos);
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


Widget * Gui::CreateWidget(GameFile::Object * a_widgetFile, Widget * a_parent, bool a_startActive)
{
	// Check for a valid parent
	if (a_parent == NULL) 
	{	
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Widget creation from file failed due to an invalid parent.");
		return NULL;
	}

	// Get properties from the file that correspond to widget definitions
	Widget::WidgetDef defFromFile;
	if (GameFile::Property * size = a_widgetFile->FindProperty("size"))
	{
		defFromFile.m_size = size->GetVector2();
	}
	if (GameFile::Property * pos = a_widgetFile->FindProperty("pos"))
	{
		defFromFile.m_pos = pos->GetVector2();
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
		if (GameFile::Property * texture = a_widgetFile->FindProperty("texture"))
		{
			if (Texture * tex = TextureManager::Get().GetTexture(texture->GetString(), TextureManager::eCategoryGui))
			{
				newWidget->SetTexture(tex);
			}
		}

		return newWidget;
	}

	return NULL;
}

void Gui::DestroyWidget(Widget * a_widget)
{
	// Clean up any links
	a_widget->ClearAction();
	a_widget->Orphan();

	// Delete sibling widgets
	Widget * curWidget = a_widget->GetNext();
	while (curWidget != NULL)
	{
		Widget * next = curWidget->GetNext();
		delete curWidget;
		curWidget = next;
	}

	// Delete any child widgets
	curWidget = a_widget->GetChild();
	while (curWidget != NULL)
	{
		Widget * next = curWidget->GetChild();
		delete curWidget;
		curWidget = next;
	}

	// Destroy the parent
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
	Widget * curChild = m_activeMenu->GetChild();
	DebugMenu & dbgMen = DebugMenu::Get();
	while (curChild != NULL)
	{
		Widget * curSibling = curChild->GetNext();
		while (curSibling != NULL)
		{
			// If we are in editing mode process editing selections
			if (dbgMen.IsDebugMenuEnabled())
			{
				if (curSibling->IsSelected(Widget::eSelectionEditRollover))
				{
					selected = curSibling;
					break;
				}
			}
			else // Otherwise perform normal selection changes
			{
				if (curSibling->IsSelected())
				{
					selected = curSibling;
					break;
				}
			}
			curSibling = curSibling->GetNext();
		}

		// Do the same for debug children
		if (dbgMen.IsDebugMenuEnabled())
		{
			if (curChild->IsSelected(Widget::eSelectionEditRollover))
			{
				selected = curChild;
				break;
			}
		}
		else // Normal children
		{
			if (curChild->IsSelected())
			{
				selected = curChild;
				break;
			}
		}

		curChild = curChild->GetChild();
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
			if (GameFile::Property * nameProp = menuFile->FindProperty(menuObject, "name"))
			{
				Widget * parentMenu = new Widget();
				parentMenu->SetName(menuFile->GetString("menu", "name"));
				parentMenu->SetFilePath(a_menuFile);
				parentMenu->SetActive(false);

				// Load the script
				if (GameFile::Property * scriptProp = menuFile->FindProperty(menuObject, "script"))
				{
					// Open Lua and load the standard libraries
					lua_State *lua = luaL_newstate();	
					luaL_requiref(lua, "io", luaopen_io, 1);
					luaL_requiref(lua, "base", luaopen_base, 1);
					luaL_requiref(lua, "table", luaopen_table, 1);
					luaL_requiref(lua, "string", luaopen_string, 1);
					luaL_requiref(lua, "math", luaopen_math, 1);

					// Construct path to script using menu path
					char scriptPath[StringUtils::s_maxCharsPerLine];
					sprintf(scriptPath, "%sscripts\\%s", m_guiPath, menuFile->GetString("menu", "script"));
					if (luaL_loadfile(lua, scriptPath) == 0)
					{
					   // Call code located in script file...
					   if (lua_pcall(lua, 0, LUA_MULTRET, 0) != 0)
					   {
						   // Report ant remove error message from stack
						   Log::Get().Write(Log::LL_ERROR, Log::LC_GAME, lua_tostring(lua, -1));
						   lua_pop(lua, 1);
					   }
					}
					else
					{
						Log::Get().Write(Log::LL_ERROR, Log::LC_GAME, "Cannot find script file %s referred to by gui file %s.", scriptPath, a_menuFile);
					}
					lua_close (lua);
				}

				// Load child elements of the menu
				GameFile::Object * childWidget = menuObject->m_firstChild;
				while (childWidget != NULL)
				{
					CreateWidget(childWidget, parentMenu);
					childWidget = childWidget->m_next;
				}

				// Add to list of menus
				MenuListNode * newMenuNode = new MenuListNode();
				newMenuNode->SetData(parentMenu);
				m_menus.Insert(newMenuNode);
			}
			else // No properties present
			{
				Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Error loading menu file %s, menu does not have a name property.", a_menuFile);
			}
		}
		else // Unexpected file format, no root element
		{
			Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Error loading menu file %s, no valid menu parent element.", a_menuFile);
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
	fileMan.EmptyFileList(menuFiles);

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