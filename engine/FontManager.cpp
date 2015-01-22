#include <iostream>
#include <fstream>

#include "DataPack.h"
#include "RenderManager.h"
#include "TextureManager.h"

#include "FontManager.h"

using namespace std;	//< For fstream operations

template<> FontManager * Singleton<FontManager>::s_instance = NULL;

const float FontManager::s_debugFontSize2D = 1.25f;
const float FontManager::s_debugFontSize3D = 10.0f;

bool FontManager::Startup(const char * a_fontPath)
{
	// Sanity check on input arg
	if (a_fontPath == NULL || a_fontPath[0] == 0)
	{
		return false;
	}

	// Populate a list of font configuration files
	FileManager::FileList fontFiles;
	FileManager::Get().FillFileList(a_fontPath, fontFiles, ".fnt");
	FileManager::FileListNode * curNode = fontFiles.GetHead();
	
	// Cache off the font path as textures are relative to fonts
	memset(&m_fontPath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_fontPath, a_fontPath, sizeof(char) * strlen(a_fontPath) + 1);

	// Load each font in the directory
	bool loadSuccess = true;
	while(curNode != NULL)
	{
		// File stream for font config file
		ifstream inputFile(curNode->GetData()->m_name);
		loadSuccess &= LoadFont<ifstream>(inputFile);
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	FileManager::Get().CleanupFileList(fontFiles);

	return loadSuccess;
}

bool FontManager::Startup(const DataPack * a_dataPack)
{
	// Sanity check on input arg
	if (a_dataPack == NULL)
	{
		return false;
	}

	// Populate a list of font configuration files
	DataPack::EntryList fontEntries;
	a_dataPack->GetAllEntries(".fnt", fontEntries);
	DataPack::EntryNode * curNode = fontEntries.GetHead();

	// Load each font in the pack
	bool loadSuccess = true;
	while (curNode != NULL)
	{
		loadSuccess &= LoadFont<DataPackEntry>(*curNode->GetData());
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	DataPack::Get().CleanupEntryList(fontEntries);

	return loadSuccess;
}

bool FontManager::Shutdown()
{
	FontListNode * next = m_fonts.GetHead();
	while(next != NULL)
	{
		// Cache off next pointer
		FontListNode * cur = next;
		next = cur->GetNext();

		m_fonts.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	return true;
}

bool FontManager::DrawString(const char * a_string, StringHash * a_fontName, float a_size, Vector2 a_pos, Colour a_colour, RenderLayer::Enum a_renderLayer)
{
	return DrawString(a_string, a_fontName->GetHash(), a_size, a_pos, a_colour, a_renderLayer);
}

bool FontManager::DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector2 a_pos, Colour a_colour, RenderLayer::Enum a_renderLayer)
{
	Vector stringPos(a_pos.GetX(), a_pos.GetY(), 0.0f);
	return DrawString(a_string, a_fontNameHash, a_size, stringPos, a_colour, a_renderLayer);
}

bool FontManager::DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector a_pos, Colour a_colour, RenderLayer::Enum a_renderLayer)
{
	const bool is2D = a_renderLayer != RenderLayer::Debug3D && a_renderLayer != RenderLayer::World;
	FontListNode * curFont = m_fonts.GetHead();
	RenderManager & renderMan = RenderManager::Get();
	const float viewAspect = renderMan.GetViewAspect();
	while(curFont != NULL)
	{
		if (curFont->GetData()->m_fontName == a_fontNameHash)
		{
			// Draw each character in the string
			Font * font = curFont->GetData();
			float xAdvance = 0.0f;

			// Calculate a scaling ratio for the font to match the requested pixel size, font size limit is 1 meg
			a_size *= (float)font->m_sizeX / (float)s_maxFontTexSize;
			const Vector2 sizeWithAspect = Vector2(is2D ? a_size : a_size * viewAspect, a_size);
			const Vector2 sizeRatio(sizeWithAspect.GetX() / font->m_sizeX / viewAspect, a_size / font->m_sizeY);

			unsigned int textLength = strlen(a_string);
			for (unsigned int j = 0; j < textLength; ++j)
			{
				// Safety check for unexported characters
				const FontChar & curChar = font->m_chars[(int)a_string[j]];
				if (curChar.m_width > 0 || curChar.m_height > 0)
				{ 
					// Do not add a quad for a space
					if (a_string[j] != ' ') 
					{
						float xPos = a_pos.GetX() + xAdvance + ((curChar.m_xoffset / font->m_sizeX) * sizeWithAspect.GetX());
						float yPos = a_pos.GetY() - ((curChar.m_yoffset / font->m_sizeY) * sizeWithAspect.GetY());
						float zPos = a_pos.GetZ() - ((curChar.m_yoffset / font->m_sizeY) * sizeWithAspect.GetY());

						// Align font chars 2D vs 3D
						if (is2D)
						{
							renderMan.AddFontChar(a_renderLayer, curChar.m_displayListId, sizeWithAspect, Vector(xPos, yPos, 0.0f), a_colour);
						}
						else
						{
							zPos = a_pos.GetZ() - (curChar.m_yoffset / font->m_sizeY);
							renderMan.AddFontChar(a_renderLayer, curChar.m_displayListId3D, sizeWithAspect, Vector(xPos, a_pos.GetY(), zPos), a_colour);
						}
					}
					xAdvance += (float)(curChar.m_xadvance * sizeRatio.GetX());
				}
				else
				{
					Log::Get().WriteOnce(LogLevel::Warning, LogCategory::Engine, "Unexported font glyph for character.");
				}
			}
			return true;
		}
		curFont = curFont->GetNext();
	}

	// Could not find the font to draw with
	return false;
}


bool FontManager::DrawDebugString2D(const char * a_string, Vector2 a_pos, Colour a_colour, RenderLayer::Enum a_renderLayer)
{
	// Use the first loaded font as the debug font
	if (m_fonts.GetLength() > 0)
	{
		return DrawString(a_string, &m_fonts.GetHead()->GetData()->m_fontName, s_debugFontSize2D, a_pos, a_colour, a_renderLayer);
	}
	else // Not fonts loaded
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Cannot load any font to draw error message on the screen.", false);
	}

	return false;
}

bool FontManager::DrawDebugString3D(const char * a_string, Vector a_pos, Colour a_colour, float a_size)
{
	// Use the first loaded font as the debug font
	if (m_fonts.GetLength() > 0)
	{
		return DrawString(a_string, m_fonts.GetHead()->GetData()->m_fontName.GetHash(), a_size, a_pos, a_colour, RenderLayer::Debug3D);
	}
	else // Not fonts loaded
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Cannot load any font to draw 3D message on the screen.", false);
	}

	return false;
}

StringHash * FontManager::GetLoadedFontName(const char * a_fontName)
{
	FontListNode * curFont = m_fonts.GetHead();
	while(curFont != NULL)
	{
		if (curFont->GetData()->m_fontName == StringHash(a_fontName))
		{
			return &curFont->GetData()->m_fontName;
		}
		curFont = curFont->GetNext();
	}

	return NULL;
}

const char * FontManager::GetLoadedFontName(unsigned int a_fontNameHash)
{
	FontListNode * curFont = m_fonts.GetHead();
	while(curFont != NULL)
	{
		if (StringHash(curFont->GetData()->m_fontName) == a_fontNameHash)
		{
			return curFont->GetData()->m_fontName.GetCString();
		}
		curFont = curFont->GetNext();
	}

	return NULL;
}

const char * FontManager::GetLoadedFontNameForId(int a_fontId)
{
	int fontCount = 0;
	FontListNode * curFont = m_fonts.GetHead();
	while(curFont != NULL)
	{
		if (fontCount++ == a_fontId)
		{
			return curFont->GetData()->m_fontName.GetCString();
		}
		curFont = curFont->GetNext();
	}

	return NULL;
}

StringHash * FontManager::GetDebugFontName()
{
	if (m_fonts.GetLength() > 0)
	{
		return &m_fonts.GetHead()->GetData()->m_fontName;
	}
	else // No fonts loaded
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "Cannot find a debug font to draw with! Only hope now is reading stdout!");
		return NULL;
	}
	
}