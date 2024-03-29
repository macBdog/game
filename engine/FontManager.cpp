#include <iostream>
#include <fstream>

#include "DataPack.h"
#include "RenderManager.h"
#include "TextureManager.h"

#include "FontManager.h"

#include "Font\default.fnt.inc"
#include "Font\default.tga.inc"

using namespace std;	//< For fstream operations

template<> FontManager * Singleton<FontManager>::s_instance = nullptr;

const float FontManager::s_debugFontSize2D = 1.25f;
const float FontManager::s_debugFontSize3D = 10.0f;

bool FontManager::Startup(const char * a_fontPath)
{
	// Sanity check on input arg
	if (a_fontPath == nullptr || a_fontPath[0] == 0)
	{
		return false;
	}

	// First font loaded should be the default from include files
	LoadDefaultFont(defaultFontDefinition);

	// Populate a list of font configuration files
	FileManager::FileList fontFiles;
	FileManager::Get().FillFileList(a_fontPath, fontFiles, ".fnt");
	FileManager::FileListNode * curNode = fontFiles.GetHead();
	
	// Cache off the font path as textures are relative to fonts
	char fontFilePath[StringUtils::s_maxCharsPerLine];
	memset(&fontFilePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&m_fontPath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_fontPath, a_fontPath, sizeof(char) * strlen(a_fontPath) + 1);

	// Load each font in the directory
	bool loadSuccess = true;
	while(curNode != nullptr)
	{
		// File stream for font config file
		sprintf(fontFilePath, "%s%s", m_fontPath, curNode->GetData()->m_name);
		ifstream inputFile(fontFilePath, ifstream::in);
		loadSuccess &= LoadFont<ifstream>(inputFile);
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	FileManager::Get().CleanupFileList(fontFiles);

	return loadSuccess;
}

bool FontManager::Startup(const char * a_fontPath, const DataPack * a_dataPack)
{
	// Sanity check on input arg
	if (a_dataPack == nullptr)
	{
		return false;
	}

	// First font loaded should be the default from include files
	LoadDefaultFont(defaultFontDefinition);

	// Cache off the font path as textures are relative to fonts
	char fontFilePath[StringUtils::s_maxCharsPerLine];
	memset(&fontFilePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&m_fontPath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_fontPath, a_fontPath, sizeof(char) * strlen(a_fontPath) + 1);

	// Populate a list of font configuration files
	DataPack::EntryList fontEntries;
	a_dataPack->GetAllEntries(".fnt", fontEntries);
	DataPack::EntryNode * curNode = fontEntries.GetHead();

	// Load each font in the pack
	bool loadSuccess = true;
	while (curNode != nullptr)
	{
		loadSuccess &= LoadFont<DataPackEntry>(*curNode->GetData());
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	a_dataPack->CleanupEntryList(fontEntries);

	return loadSuccess;
}

bool FontManager::Shutdown()
{
	m_fonts.ForeachAndDelete([&](auto * cur)
	{
		m_fonts.Remove(cur);
	});

	return true;
}

bool FontManager::DrawString(const char * a_string, StringHash * a_fontName, float a_size, Vector2 a_pos, Colour a_colour, RenderLayer a_renderLayer)
{
	return DrawString(a_string, a_fontName->GetHash(), a_size, a_pos, a_colour, a_renderLayer);
}

bool FontManager::DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector2 a_pos, Colour a_colour, RenderLayer a_renderLayer)
{
	Vector stringPos(a_pos.GetX(), a_pos.GetY(), 0.0f);
	return DrawString(a_string, a_fontNameHash, a_size, stringPos, a_colour, a_renderLayer);
}

bool FontManager::DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector a_pos, Colour a_colour, RenderLayer a_renderLayer)
{
	const bool is2D = a_renderLayer != RenderLayer::Debug3D && a_renderLayer != RenderLayer::World;

	FontListNode * curFont = m_fonts.GetHead();
	RenderManager & renderMan = RenderManager::Get();
	const float viewAspect = renderMan.GetViewAspect();
	const char newLine = 10;
	while(curFont != nullptr)
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
				// Handle newline first
				if (a_string[j] == newLine)
				{
					xAdvance = 0.0f;
					const FontChar & defaultChar = font->m_chars[64];
					a_pos.SetY(a_pos.GetY() - (defaultChar.m_height / font->m_sizeY) * sizeWithAspect.GetY());
				}
				else
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
							float zPos = a_pos.GetZ() -((curChar.m_yoffset / font->m_sizeY) * sizeWithAspect.GetY());

							// Align font chars 2D vs 3D
							if (is2D)
							{
								renderMan.AddFontChar(a_renderLayer, curChar.m_charSize, curChar.m_texSize, curChar.m_texCoord, font->m_texture, sizeWithAspect, Vector(xPos, yPos, 0.0f), a_colour);
							}
							else
							{
								renderMan.AddFontChar(a_renderLayer, curChar.m_charSize, curChar.m_texSize, curChar.m_texCoord, font->m_texture, sizeWithAspect, Vector(xPos, a_pos.GetY(), zPos), a_colour);
							}
						}
						xAdvance += (float)(curChar.m_xadvance * sizeRatio.GetX());
					}
					else
					{
						Log::Get().WriteOnce(LogLevel::Warning, LogCategory::Engine, "Unexported font glyph for character.");
					}
				}
			}
			return true;
		}
		curFont = curFont->GetNext();
	}

	// Could not find the font to draw with
	return false;
}

bool FontManager::MeasureString2D(const char * a_string, unsigned int a_fontNameHash, float a_size, float & a_width, float & a_height)
{
	a_width = 0;
	a_height = 0;

	FontListNode * curFont = m_fonts.GetHead();
	RenderManager & renderMan = RenderManager::Get();
	const float viewAspect = renderMan.GetViewAspect();
	const char newLine = 10;
	while(curFont != nullptr)
	{
		if (curFont->GetData()->m_fontName == a_fontNameHash)
		{
			// Draw each character in the string
			Font * font = curFont->GetData();
			a_size *= (float)font->m_sizeX / (float)s_maxFontTexSize;
			const Vector2 sizeWithAspect = Vector2(a_size, a_size);
			float xAdvance = 0.0f;
			
			// Calculate a scaling ratio for the font to match the requested pixel size, font size limit is 1 meg
			const Vector2 sizeRatio(sizeWithAspect.GetX() / font->m_sizeX / viewAspect, a_size / font->m_sizeY);

			const FontChar & defaultChar = font->m_chars[64];
			const float lineHeight = (defaultChar.m_height / font->m_sizeY) * sizeWithAspect.GetY();
			a_height = lineHeight + defaultChar.m_height * sizeRatio.GetY();

			unsigned int textLength = strlen(a_string);
			for (unsigned int j = 0; j < textLength; ++j)
			{
				// Handle newline first
				if (a_string[j] == newLine)
				{
					xAdvance = 0.0f;
					a_height += lineHeight;
				}
				else
				{
					// Safety check for unexported characters
					const FontChar & curChar = font->m_chars[(int)a_string[j]];
					if (curChar.m_width > 0 || curChar.m_height > 0)
					{ 
						xAdvance += curChar.m_xadvance * sizeRatio.GetX();
						if (xAdvance > a_width)
						{
							a_width = xAdvance + (curChar.m_xoffset / font->m_sizeX) * sizeWithAspect.GetX();
						}
					}
				}
			}
			return true;
		}
		curFont = curFont->GetNext();
	}

	// Could not find the font to measure with
	return false;
}

bool FontManager::DrawDebugString2D(const char * a_string, Vector2 a_pos, Colour a_colour, RenderLayer a_renderLayer)
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
	while(curFont != nullptr)
	{
		if (curFont->GetData()->m_fontName == StringHash(a_fontName))
		{
			return &curFont->GetData()->m_fontName;
		}
		curFont = curFont->GetNext();
	}

	return nullptr;
}

const char * FontManager::GetLoadedFontName(unsigned int a_fontNameHash)
{
	FontListNode * curFont = m_fonts.GetHead();
	while(curFont != nullptr)
	{
		if (StringHash(curFont->GetData()->m_fontName) == a_fontNameHash)
		{
			return curFont->GetData()->m_fontName.GetCString();
		}
		curFont = curFont->GetNext();
	}

	return nullptr;
}

const char * FontManager::GetLoadedFontNameForId(int a_fontId)
{
	int fontCount = 0;
	FontListNode * curFont = m_fonts.GetHead();
	while(curFont != nullptr)
	{
		if (fontCount++ == a_fontId)
		{
			return curFont->GetData()->m_fontName.GetCString();
		}
		curFont = curFont->GetNext();
	}

	return nullptr;
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
		return nullptr;
	}
}

bool FontManager::LoadDefaultFont(const char * a_fontDefinition)
{
	// Create a new font to be managed
	RenderManager & renMan = RenderManager::Get();
	FontListNode * newFontNode = new FontListNode();
	newFontNode->SetData(new Font());
	Font * newFont = newFontNode->GetData();

	// Assign preloaded texture and add as the first in the list
	if (m_defaultFontTexture.LoadTGAFromMemory((void*)&defaultFontTextureBuffer, defaultFontTextureBufferLength, true))
	{
		newFont->m_texture = &m_defaultFontTexture;
	}
	else
	{
		return false;
	}

	// Font metadata
	unsigned int numChars = 0;
	unsigned int sizeW = 0;
	unsigned int sizeH = 0;
	unsigned int lineHeight, base, pages;

	// Get the number of chars to parse
	const char * delimeter = "\n";
	const int fontStringSize = strlen(a_fontDefinition);
	char * mutableFont = (char *)malloc(sizeof(char) * fontStringSize + 1);
	if (mutableFont != nullptr)
	{
		memcpy(mutableFont, a_fontDefinition, sizeof(char) * fontStringSize);
		char * line = strtok(mutableFont, delimeter);
		int lineCount = 0;
		while (line != nullptr)
		{
			if (lineCount == 0)	// info face="fontname" etc
			{
				char shortFontName[StringUtils::s_maxCharsPerLine];
				sprintf(shortFontName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "'", 1), true));
				newFont->m_fontName.SetCString(shortFontName);
			}
			else if (lineCount == 1) // common lineHeight=x base=33	
			{
				sscanf_s(line, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d",
				&lineHeight, &base, &sizeW, &sizeH, &pages);
				newFont->m_sizeX = sizeW;
				newFont->m_sizeY = sizeH;
			}
			else if (lineCount == 2)
			{
			}
			else if (lineCount == 3) // chars count=x
			{
				sscanf_s(line, "chars count=%d", &numChars);
				newFont->m_numChars = numChars;
			}
			else
			{
				// Go through each char in the file loading metadata
				for (unsigned int i = 0; i < numChars; ++i)
				{
					int charId, x, y, width, height, xoffset, yoffset, xadvance, page, chnl;
					sscanf_s(line, "char id=%d   x=%d    y=%d    width=%d     height=%d     xoffset=%d     yoffset=%d    xadvance=%d     page=%d  chnl=%d",
						&charId, &x, &y, &width, &height, &xoffset, &yoffset, &xadvance, &page, &chnl);
					FontChar & curChar = newFont->m_chars[charId];
					curChar.m_x = (float)x;
					curChar.m_y = (float)y;
					curChar.m_width = (float)width;
					curChar.m_height = (float)height;
					curChar.m_xoffset = (float)xoffset;
					curChar.m_yoffset = (float)yoffset;
					curChar.m_xadvance = (float)xadvance;

					// This is the glyph size as a ratio of the texture size
					Vector2 sizeRatio(1.0f / newFont->m_sizeX / renMan.GetViewAspect(), 1.0f / newFont->m_sizeY);
					newFont->m_chars[charId].m_charSize = Vector2(curChar.m_width * sizeRatio.GetX(), curChar.m_height * sizeRatio.GetY());

					// Used to generate the position of the character within the texture
					newFont->m_chars[charId].m_texSize = TexCoord(curChar.m_width / newFont->m_sizeX, curChar.m_height / newFont->m_sizeY);
					newFont->m_chars[charId].m_texCoord = TexCoord(curChar.m_x / newFont->m_sizeX, curChar.m_y / newFont->m_sizeY);

					line = strtok(nullptr, delimeter);
				}
				break;
			}
			line = strtok(nullptr, delimeter);
			++lineCount;
		}	

		// Clean up and add font to DB
		free(mutableFont);
		m_fonts.Insert(newFontNode);
	}
	return false;
}
