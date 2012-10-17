#include <iostream>
#include <fstream>

#include "Log.h"
#include "RenderManager.h"
#include "TextureManager.h"

#include "FontManager.h"

using namespace std;	//< For fstream operations

template<> FontManager * Singleton<FontManager>::s_instance = NULL;

const float FontManager::s_debugFontSize = 1.25f;

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
	strncpy(m_fontPath, a_fontPath, strlen(a_fontPath));

	// Load each font in the directory
	bool loadSuccess = true;
	while(curNode != NULL)
	{
		loadSuccess &= LoadFont(curNode->GetData()->m_name);
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	FileManager::Get().EmptyFileList(fontFiles);

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

bool FontManager::LoadFont(const char * a_fontName)
{
	// Cstrings for reading filename
	char line[StringUtils::s_maxCharsPerLine];
	char fontFilePath[StringUtils::s_maxCharsPerLine];
	char textureName[StringUtils::s_maxCharsPerLine];
	char texturePath[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&fontFilePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&textureName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&texturePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	sprintf(fontFilePath, "%s%s", m_fontPath, a_fontName);

	// File stream for font config file
	ifstream file(fontFilePath);
	unsigned int lineCount = 0;
	
	// Create a new font to be managed
	FontListNode * newFontNode = new FontListNode();
	newFontNode->SetData(new Font());
	Font * newFont = newFontNode->GetData();

	// Open the file and parse each line 
	if (file.is_open())
	{
		// Font metadata
		unsigned int numChars = 0;
		unsigned int sizeW = 0;
		unsigned int sizeH = 0;
		unsigned int lineHeight, base, pages;

		// Read till the file has more contents or a rule is broken
		while (file.good())
		{
			// Get the number of chars to parse
			file.getline(line, StringUtils::s_maxCharsPerLine);			// info face="fontname" etc
		    char * fontName = strstr(line, "\"") + 1;
			char shortFontName[StringUtils::s_maxCharsPerLine];
			sprintf(shortFontName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "\"", 1), true));
			newFont->m_fontName.SetCString(shortFontName);

			file.getline(line, StringUtils::s_maxCharsPerLine);			// common lineHeight=x base=33			
			sscanf_s(line, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d",
								   &lineHeight,  &base,  &sizeW,   &sizeH,	 &pages);

			// As we try to render all fonts the same size, fail to load fonts greater than a meg
			if (sizeW > s_maxFontTexSize || sizeH > s_maxFontTexSize)
			{
				Log::Get().Write(Log::LL_WARNING, Log::LC_ENGINE, "Cannot load the font called %s because it's bigger than meg in resolution.", shortFontName);
				file.close();
				return false;
			}

			file.getline(line, StringUtils::s_maxCharsPerLine);			// page id=0 file="arial.tga"
			sprintf(textureName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "=", 2), true));

			file.getline(line, StringUtils::s_maxCharsPerLine);			// chars count=x
			sscanf_s(line, "chars count=%d", &numChars);
			
			// Load texture for font
			sprintf(texturePath, "%s%s", m_fontPath, textureName);
			newFont->m_texture = TextureManager::Get().GetTexture(texturePath, TextureManager::eCategoryGui, TextureManager::eTextureFilterLinear);
			newFont->m_numChars = numChars;
			newFont->m_sizeX = sizeW;
			newFont->m_sizeY = sizeH;

			// Go through each char in the file loading metadata
			for (unsigned int i = 0; i < numChars; ++i)
			{
				file.getline(line, StringUtils::s_maxCharsPerLine);
				int charId, x, y, width, height, xoffset, yoffset, xadvance, page, chnl;
				sscanf_s(line, "char id=%d   x=%d    y=%d    width=%d     height=%d     xoffset=%d     yoffset=%d    xadvance=%d     page=%d  chnl=%d", 
								&charId,	 &x,	 &y,	 &width,	  &height,		&xoffset,	   &yoffset,	 &xadvance,		 &page,   &chnl);
				newFont->m_chars[charId].m_x = (float)x;
				newFont->m_chars[charId].m_y = (float)y;
				newFont->m_chars[charId].m_width = (float)width;
				newFont->m_chars[charId].m_height = (float)height;
				newFont->m_chars[charId].m_xoffset = (float)xoffset;
				newFont->m_chars[charId].m_yoffset = (float)yoffset;
				newFont->m_chars[charId].m_xadvance = (float)xadvance;
			}

			// There is more info such as kerning here but we don't care about it
			break;
		}

		file.close();

		m_fonts.Insert(newFontNode);

		return true;
	}
	else
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Could not find font file %s ", fontFilePath);

		// Clean up the font that was allocated
		delete newFontNode->GetData();
		delete newFontNode;

		return false;
	}
	
	return false;
}

bool FontManager::DrawString(const char * a_string, StringHash * a_fontName, float a_size, Vector2 a_pos, Colour a_colour, RenderManager::eBatch a_batch)
{
	return DrawString(a_string, a_fontName->GetHash(), a_size, a_pos, a_colour, a_batch);
}

bool FontManager::DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector2 a_pos, Colour a_colour, RenderManager::eBatch a_batch)
{
	FontListNode * curFont = m_fonts.GetHead();
	while(curFont != NULL)
	{
		if (curFont->GetData()->m_fontName == a_fontNameHash)
		{
			// Draw each character in the string
			Font * font = curFont->GetData();
			RenderManager & renderMan = RenderManager::Get();
			float xAdvance = 0.0f;

			// Calculate a scaling ratio for the font to match the requested pixel size, font size limit is 1 meg
			a_size *= (float)font->m_sizeX / (float)s_maxFontTexSize;
			Vector2 sizeRatio(a_size / font->m_sizeX / renderMan.GetViewAspect(), a_size / font->m_sizeY);

			unsigned int textLength = strlen(a_string);
			for (unsigned int j = 0; j < textLength; ++j)
			{
				// Safety check for unexported characters
				const FontChar & curChar = font->m_chars[(int)a_string[j]];
				if (curChar.m_width > 0 || curChar.m_height > 0)
				{ 
					// This is the glyph size as a ratio of the texture size
					Vector2 charSize(curChar.m_width * sizeRatio.GetX(), curChar.m_height * sizeRatio.GetY());

					// Do not add a quad for a space
					if (a_string[j] != ' ') 
					{
						TexCoord texSize(curChar.m_width/font->m_sizeX, curChar.m_height/font->m_sizeY);
						TexCoord texCoord(curChar.m_x/font->m_sizeX, curChar.m_y/font->m_sizeY);

						float xPos = a_pos.GetX() + xAdvance + ((curChar.m_xoffset / font->m_sizeX) * a_size);
						float yPos = a_pos.GetY() - ((curChar.m_yoffset / font->m_sizeY) * a_size);
						renderMan.AddQuad2D(a_batch, Vector2(xPos, yPos), charSize, font->m_texture, texCoord, texSize, Texture::eOrientationNormal, a_colour);
					}
					xAdvance += (float)(curChar.m_xadvance * sizeRatio.GetX());
				}
				else
				{
					Log::Get().WriteOnce(Log::LL_WARNING, Log::LC_ENGINE, "Unexported font glyph for character.");
				}
			}
			return true;
		}
		curFont = curFont->GetNext();
	}

	// Could not find the font to draw with
	return false;
}


bool FontManager::DrawDebugString(const char * a_string, Vector2 a_pos, Colour a_colour, RenderManager::eBatch a_batch)
{
	// Use the first loaded font as the debug font
	if (m_fonts.GetLength() > 0)
	{
		return DrawString(a_string, &m_fonts.GetHead()->GetData()->m_fontName, s_debugFontSize, a_pos, a_colour, a_batch);
	}
	else // Not fonts loaded
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Cannot load any font to draw error message on the screen.", false);
	}

	return false;
}

bool FontManager::DrawDebugString3D(const char * a_string, float a_size, Vector a_pos, Colour a_colour)
{
	// Use the first loaded font as the debug font
	if (m_fonts.GetLength() > 0)
	{
		Vector2 pos(a_pos.GetX(), a_pos.GetY());
		return DrawString(a_string, &m_fonts.GetHead()->GetData()->m_fontName, s_debugFontSize, pos, a_colour, RenderManager::eBatchDebug);
	}
	else // Not fonts loaded
	{
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Cannot load any font to draw 3D message on the screen.", false);
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
		Log::Get().WriteOnce(Log::LL_ERROR, Log::LC_ENGINE, "Cannot find a debug font to draw with! Only hope now is reading stdout!");
		return NULL;
	}
	
}