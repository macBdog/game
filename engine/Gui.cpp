#include "Log.h"
#include "InputManager.h"
#include "FileManager.h"
#include "GameFile.h"
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

	// Setup the debug menu root element
	m_debugRoot.SetName("DebugMenuRoot");
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
		defaultMenu->SetActive(false);

		MenuListNode * newMenuNode = new MenuListNode();
		newMenuNode->SetData(defaultMenu);
		
		m_menus.Insert(newMenuNode);
	}
	
	// Set the active menu to the first loaded menu
	m_activeMenu = m_menus.GetHead()->GetData();

	// Setup the mouse cursor element
	sprintf(fileName, "%s%s", a_guiPath, m_configFile.GetString("config", "mouseCursorTexture"));
	m_cursor.SetTexture(TextureManager::Get().GetTexture(fileName, TextureManager::eCategoryGui));
	m_cursor.SetPos(Vector2(0.0f, 0.0f));
	m_cursor.SetSize(Vector2(0.16f / RenderManager::Get().GetViewAspect(), 0.16f));
	m_cursor.SetActive(true);

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
		if (GameFile::Object * menuObject = menuFile->GetObject("menu"))
		{
			if (GameFile::Property * nameProp = menuFile->GetProperty(menuObject, "name"))
			{
				Widget * newWidget = new Widget();
				newWidget->SetName(menuFile->GetString("menu", "name"));
				newWidget->SetFilePath(a_menuFile);
				newWidget->SetActive(false);

				// Add to list of menus
				MenuListNode * newMenuNode = new MenuListNode();
				newMenuNode->SetData(newWidget);
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

	return false;
}

bool Gui::LoadMenus(const char * a_guiPath)
{
	// Get a list of menu files in the gui directory
	FileManager & fileMan = FileManager::Get();
	FileManager::FileList menuFiles;
	FileManager::Get().FillFileList(a_guiPath, menuFiles, ".mnu");

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
	FileManager::Get().EmptyFileList(menuFiles);

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