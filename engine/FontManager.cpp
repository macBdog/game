#include <vector>

#include <windows.h>
#include <GL/gl.h>

#include "FileManager.h"
#include "Stringutils.h"
#include "Texture.h"

#include "FontManager.h"

const float FontManager::s_debugFontSize = 0.01f;	// Glyph height for debug drawing

/*
std::vector<fontInfo *> fonts;
*/

template<> FontManager * Singleton<FontManager>::s_instance = NULL;

bool FontManager::Init(const char * a_fontPath)
{
	// Populate a list of font configuration files
	FileManager::FileList fontFiles;
	FileManager::Get().FillFileList(a_fontPath, fontFiles, ".fnt");
	FileManager::FileListNode * curNode = fontFiles.GetHead();

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

bool FontManager::LoadFont(const char * a_fontConfigPath)
{
	/*
	FILE * fontFile;
	char *data;
	const int maxFontLineLength = 256;

	data = (char *)malloc(maxFontLineLength * sizeof(char));
	sprintf(data, "gui/fonts/%s.fnt", a_fontName);
	fopen_s(&fontFile, data, "rt");
	
	// Check if the path is valid
	if (fontFile == NULL) {
		printf("GUI ERROR: The font file is missing.\n");
		return;
    }

	// Init the font
	fontInfo * newFont = new fontInfo;
	sprintf(newFont->fontName, "%s", a_fontName);

	// Get the number of chars to parse
	int numChars = 0;
	int sizeW = 0;
	int sizeH = 0;
	int lineHeight, base, pages;
	greadstr(fontFile, data);	// info face="arial" size=32  
	greadstr(fontFile, data);	sscanf_s(data, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d",
												       &lineHeight,  &base,  &sizeW,   &sizeH,	 &pages);
	greadstr(fontFile, data);	// page id=0  
	greadstr(fontFile, data);	sscanf_s(data, "chars count=%d", &numChars);

	// Load the font texture
	sprintf(data, "gui/fonts/%s.tga", a_fontName);
	newFont->texture = loadTextureTGA(data);
	newFont->sizeW = (float)sizeW;
	newFont->sizeH = (float)sizeH;

	// Go through each char in the file loading metadata
	for (int i = 0; i < numChars; ++i)
	{
		greadstr(fontFile, data);
		int charId, x, y, width, height, xoffset, yoffset, xadvance, page, chnl;
		sscanf_s(data, "char id=%d   x=%d    y=%d    width=%d     height=%d     xoffset=%d     yoffset=%d    xadvance=%d     page=%d  chnl=%d", 
						&charId,	 &x,	 &y,	 &width,	  &height,		&xoffset,	   &yoffset,	 &xadvance,		 &page,   &chnl);
		newFont->chars[charId].x = (float)x;
		newFont->chars[charId].y = (float)y;
		newFont->chars[charId].width = (float)width;
		newFont->chars[charId].height = (float)height;
		newFont->chars[charId].xoffset = (float)xoffset;
		newFont->chars[charId].yoffset = (float)yoffset;
		newFont->chars[charId].xadvance = (float)xadvance;
	}

	// Add to list
	fonts.push_back(newFont);

	fclose(fontFile);
	*/
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