#include <iostream>
#include <fstream>

//#include "FileManager.h"
#include "Log.h"
#include "Texture.h"

#include "FontManager.h"

using namespace std;	//< For fstream operations

template<> FontManager * Singleton<FontManager>::s_instance = NULL;

const float FontManager::s_debugFontSize = 0.01f;

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
			strncpy(newFont->m_fontName, fontName, strlen(fontName) - strlen(strstr(fontName, "\"")));
			
			file.getline(line, StringUtils::s_maxCharsPerLine);			// common lineHeight=x base=33			
			sscanf_s(line, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d",
								   &lineHeight,  &base,  &sizeW,   &sizeH,	 &pages);

			file.getline(line, StringUtils::s_maxCharsPerLine);			// page id=0 file="arial.tga"
			sprintf(textureName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "=", 2), true));

			file.getline(line, StringUtils::s_maxCharsPerLine);			// chars count=x
			sscanf_s(line, "chars count=%d", &numChars);
			
			// Load texture for font
			newFont->m_texture = new Texture();
			sprintf(texturePath, "%s%s", m_fontPath, textureName);
			newFont->m_texture->Load(texturePath);
			newFont->m_numChars = numChars;

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
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Could not find font file %s ", fontFilePath);

		// Clean up the font that was allocated
		delete newFontNode->GetData();
		delete newFontNode;

		return false;
	}
	
	return false;
}

/*
void shutdownFonts()
{
	for (unsigned int i = 0; i < fonts.size(); ++i)
	{
		delete fonts.at(i);
	}
}

void drawStringDefault(const char * text, unsigned int size, float X, float Y) 
{
	drawString(text, fonts.at(0)->fontName, size, X, Y);
}

void drawString(const char text[], const char * fontName, unsigned int size, float X, float Y) 
{
	// Choose the correct font 
	fontString * newString = new fontString;

	for (unsigned int i = 0; i < fonts.size(); ++i)
	{
		if (strcmp(fonts.at(i)->fontName, fontName) == 0)
		{
			newString->fontIndex = i;
			break;
		}
	}

	// Copy the rest of the string properties
	strcpy(newString->text, text);
	newString->X = X;
	newString->Y = Y;
	newString->size = size;
	strings.push_back(newString);
}

void draw3dString(const char text[], const char * fontName, unsigned int size, float X, float Y, float Z) 
{
	// Choose the correct font 
	fontString3d * newString = new fontString3d;

	for (unsigned int i = 0; i < fonts.size(); ++i)
	{
		if (strcmp(fonts.at(i)->fontName, fontName) == 0)
		{
			newString->fontIndex = i;
			break;
		}
	}

	// Copy the rest of the string properties
	strcpy(newString->text, text);
	newString->X = X;
	newString->Y = Y;
	newString->Z = Z;
	newString->size = size;
	strings3d.push_back(newString);
}

void drawStrings()
{
	// For each of the strings
	for (unsigned int i = 0; i < strings.size(); ++i)
	{
		// Select the correct font
		const fontString * curString = strings.at(i);
		const fontInfo * curFont = fonts.at(curString->fontIndex);

		// Draw each character in the string
		unsigned int textLength = strlen(curString->text);
		float xAdvance = 0.0f;
		const float sizeMultiplier = (float)curString->size / 8000.0f;

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, curFont->texture); 
		glPushMatrix();
			glTranslatef(curString->X, curString->Y+sizeMultiplier * 28.0f, 0.0f);

		for (unsigned int j = 0; j < textLength; ++j)
		{
			// Space character
			if (curString->text[j] == 32) {
				xAdvance += sizeMultiplier * 15.0f;
			}
			else
			{
				const fontChar & curChar = curFont->chars[curString->text[j]];
				float texCoordX = curChar.x / curFont->sizeW;
				float texCoordY = curChar.y / curFont->sizeH;
				float texCoordWidth = curChar.width / curFont->sizeW;
				float texCoordHeight = curChar.height / curFont->sizeH;

				float width = curChar.width * sizeMultiplier;
				float height = curChar.height * sizeMultiplier * ASPECT_RATIO;
				float xoffset = curChar.xoffset * sizeMultiplier;
				float yoffset = curChar.yoffset * sizeMultiplier * ASPECT_RATIO;

				glPushMatrix();
					glBegin(GL_QUADS);
						glTexCoord2f(texCoordX,	texCoordY);										// top left
						glVertex3f(xAdvance + xoffset,			0.0f - yoffset, 0.0f);	

						glTexCoord2f(texCoordX,					texCoordY + texCoordHeight);	// bottom left
						glVertex3f(xAdvance + xoffset,			0.0f - yoffset - height, 0.0f);	

						glTexCoord2f(texCoordX + texCoordWidth, texCoordY + texCoordHeight);	// bottom right
						glVertex3f(xAdvance + xoffset + width,	0.0f - yoffset - height, 0.0f);
						
						glTexCoord2f(texCoordX + texCoordWidth, texCoordY);						// top right
						glVertex3f(xAdvance + xoffset + width,	0.0f - yoffset, 0.0f);
					glEnd();
				glPopMatrix();

				xAdvance += (float)curChar.xadvance * sizeMultiplier;
			}
		}

		glPopMatrix();

		// Delete the string now it's finished with
		delete curString;
	}

	strings.clear();
}

void draw3dStrings()
{
	// For each of the strings
	for (unsigned int i = 0; i < strings3d.size(); ++i)
	{
		// Select the correct font
		const fontString3d * curString = strings3d.at(i);
		const fontInfo * curFont = fonts.at(curString->fontIndex);

		// Draw each character in the string
		unsigned int textLength = strlen(curString->text);
		float xAdvance = 0.0f;
		const float sizeMultiplier = (float)curString->size / 8000.0f;

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, curFont->texture); 
		glPushMatrix();
			glTranslatef(curString->X, curString->Y+sizeMultiplier * 28.0f, curString->Z);

		for (unsigned int j = 0; j < textLength; ++j)
		{
			// Space character
			if (curString->text[j] == 32) {
				xAdvance += sizeMultiplier * 15.0f;
			}
			else
			{
				const fontChar & curChar = curFont->chars[curString->text[j]];
				float texCoordX = curChar.x / curFont->sizeW;
				float texCoordY = curChar.y / curFont->sizeH;
				float texCoordWidth = curChar.width / curFont->sizeW;
				float texCoordHeight = curChar.height / curFont->sizeH;

				float width = curChar.width * sizeMultiplier;
				float height = curChar.height * sizeMultiplier * ASPECT_RATIO;
				float xoffset = curChar.xoffset * sizeMultiplier;
				float yoffset = curChar.yoffset * sizeMultiplier * ASPECT_RATIO;

				glPushMatrix();
					glBegin(GL_QUADS);
						glTexCoord2f(texCoordX,	texCoordY);										// top left
						glVertex3f(xAdvance + xoffset,			0.0f - yoffset, 0.0f);	

						glTexCoord2f(texCoordX,					texCoordY + texCoordHeight);	// bottom left
						glVertex3f(xAdvance + xoffset,			0.0f - yoffset - height, 0.0f);	

						glTexCoord2f(texCoordX + texCoordWidth, texCoordY + texCoordHeight);	// bottom right
						glVertex3f(xAdvance + xoffset + width,	0.0f - yoffset - height, 0.0f);
						
						glTexCoord2f(texCoordX + texCoordWidth, texCoordY);						// top right
						glVertex3f(xAdvance + xoffset + width,	0.0f - yoffset, 0.0f);
					glEnd();
				glPopMatrix();

				xAdvance += (float)curChar.xadvance * sizeMultiplier;
			}
		}

		glPopMatrix();

		// Delete the string now it's finished with
		delete curString;
	}

	strings3d.clear();
}

*/