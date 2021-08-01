#ifndef _ENGINE_FONT_MANAGER_H_
#define _ENGINE_FONT_MANAGER_H_
#pragma once

#include "../core/Colour.h"
#include "../core/LinkedList.h"
#include "../core/Vector.h"

#include "Log.h"
#include "RenderManager.h"
#include "Singleton.h"
#include "StringHash.h"
#include "StringUtils.h"
#include "Texture.h"

#include "FileManager.h"

class DataPack;

//\brief The font class is responsible for loading font configuration files,
//		 font textures and submitting quads to the render manager.
class FontManager : public Singleton<FontManager>
{
public:
	//\ No work done in the constructor, only Init
	FontManager() { m_fontPath[0] = '\0'; }
	~FontManager() { Shutdown(); }

	//\brief Load all fonts in the supplied argument into memory ready for drawing
	//\param a_fontPath pointer to cstring of the path to enumerate font files
	bool Startup(const char * a_fontPath);
	bool Startup(const char * a_fontPath, const DataPack * a_dataPack);
	bool Shutdown();

	//\brief Draw a string using the orthogonal gui render renderLayer
	//\param a_string pointer to cstring with the character to render
	//\param a_fontName name of the loaded font to render with, will fail if not present or unloaded
	//\param a_size float of the height of character boxes, width is derived from this
	//\param a_pos 2D screen space coords to draw the font at, from the top left
	//\param a_colour The colour to tint the font texture to
	//\return true if the glyph quads were submitted to the render manager
	bool DrawString(const char * a_string, StringHash * a_fontName, float a_size, Vector2 a_pos, Colour a_colour = sc_colourWhite, RenderLayer a_renderLayer = RenderLayer::Gui);
	bool DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector2 a_pos, Colour a_colour = sc_colourWhite, RenderLayer a_renderLayer = RenderLayer::Gui);
	bool DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector a_pos, Colour a_colour = sc_colourWhite, RenderLayer a_renderLayer = RenderLayer::World);
	bool DrawDebugString2D(const char * a_string, Vector2 a_pos, Colour a_colour = sc_colourWhite, RenderLayer a_renderLayer = RenderLayer::Debug2D);
	bool DrawDebugString3D(const char * a_string, Vector a_pos, Colour a_colour = sc_colourWhite, float a_size = s_debugFontSize3D);

	//\brief Do the same work as draw string but just accumlate width and height
	//\param a_width_OUT the width the string
	//\param a_height_OUT the height of the string
	bool MeasureString2D(const char * a_string, unsigned int a_fontNameHash, float a_size, float & a_width, float & a_height);

	//\brief Get the symbol for a loaded font name
	//\param pointer to a cstring of the font name to retreieve
	//\return pointer to a StringHash or nullptr for failure
	StringHash * GetLoadedFontName(const char * a_fontName);
	const char * GetLoadedFontName(unsigned int a_fontNameHash);
	const char * GetLoadedFontNameForId(int m_fontId);
	inline int GetNumLoadedFonts() { return m_fonts.GetLength(); }

	//\brief Get a default font name
	StringHash * GetDebugFontName();

private:

	static const float s_debugFontSize2D;				///< Glyph height for debug drawing in ortho mode
	static const float s_debugFontSize3D;				///< Glyph height for debug drawing in debug mode
	static const unsigned int s_maxCharsPerFont = 256u;	///< No non-unicode support needed (yet)
	static const unsigned int s_maxFontTexSize = 1024u; ///< Cannot load fonts greater than a meg

	//\brief Spacing and positioning info about a character in a font
	struct FontChar
	{
		float m_x{ 0.0f };						///< X position in the texture file
		float m_y{ 0.0f };						///< Y position in the texture file
		float m_width{ 0.0f };					///< How wide the glyph is
		float m_height{ 0.0f };					///< How high the glyph is
		float m_xoffset{ 0.0f };				///< X position of the glyph insuide it's box
		float m_yoffset{ 0.0f };				///< Y position of the glyph inside it's box
		float m_xadvance{ 0.0f };				///< How much spacing to put in front of the glyph
		Vector2 m_charSize{0.0f, 0.0f};			///< How much physical space the character takes
		TexCoord m_texSize{ 0.0f, 0.0f };		///< Size of the texture resource to draw with
		TexCoord m_texCoord{ 0.0f, 0.0f };		///< Where in the texture resource is utilised
	};

	//\brief Info about a font and a group of all the character info
	struct Font
	{
		Font() 
			: m_texture(nullptr)
			, m_numChars(0)
			, m_sizeX(0)
			, m_sizeY(0)
		{}
		StringHash	m_fontName;
		FontChar	m_chars[s_maxCharsPerFont];
		Texture* m_texture{ nullptr };
		unsigned int m_numChars{ 0 };
		unsigned int m_sizeX{ 0 };
		unsigned int m_sizeY{ 0 };
	};

	//\brief Alias to store a list of fonts for drawing
	typedef LinkedListNode<Font> FontListNode;
	typedef LinkedList<Font> FontList;

	//\brief Load the font as an include file so the engine is not dependant on external files to draw messages to the screen
	bool LoadDefaultFont(const char * a_fontDefinition);

	//\brief Load a font for use in drawing to the screen. Assumes a texture adjacent to config file.
	//\param a_fontConfigFilePath path to the config file specifying glyph numbers and widths
	//\return True if the load operation was completed successfully
	template <typename TInputData>
	bool LoadFont(TInputData & a_input)
	{
		// Cstrings for reading filenames
		char textureName[StringUtils::s_maxCharsPerLine];
		char texturePath[StringUtils::s_maxCharsPerLine];
		char line[StringUtils::s_maxCharsPerLine];
		memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
		memset(&textureName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
		memset(&texturePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);

		// Create a new font to be managed
		RenderManager & renMan = RenderManager::Get();
		FontListNode * newFontNode = new FontListNode();
		newFontNode->SetData(new Font());
		Font * newFont = newFontNode->GetData();

		// Open the file and parse each line 
		if (a_input.is_open())
		{
			// Font metadata
			unsigned int numChars = 0;
			unsigned int sizeW = 0;
			unsigned int sizeH = 0;
			unsigned int lineHeight, base, pages;

			// Read till the file has more contents or a rule is broken
			while (a_input.good())
			{
				// Get the number of chars to parse
				a_input.getline(line, StringUtils::s_maxCharsPerLine);			// info face="fontname" etc
				char shortFontName[StringUtils::s_maxCharsPerLine];
				sprintf(shortFontName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "\"", 1), true));
				newFont->m_fontName.SetCString(shortFontName);
				a_input.getline(line, StringUtils::s_maxCharsPerLine);			// common lineHeight=x base=33			
				sscanf_s(line, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d",
					&lineHeight, &base, &sizeW, &sizeH, &pages);

				// As we try to render all fonts the same size, fail to load fonts greater than a meg
				if (sizeW > s_maxFontTexSize || sizeH > s_maxFontTexSize)
				{
					Log::Get().Write(LogLevel::Warning, LogCategory::Engine, "Cannot load the font called %s because it's bigger than 1 meg in resolution.", shortFontName);
					a_input.close();
					return false;
				}

				a_input.getline(line, StringUtils::s_maxCharsPerLine);			// page id=0 file="arial.tga"
				sprintf(textureName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "=", 2), true));

				a_input.getline(line, StringUtils::s_maxCharsPerLine);			// chars count=x
				sscanf_s(line, "chars count=%d", &numChars);

				// Load texture for font
				sprintf(texturePath, "%s%s", m_fontPath, textureName);
				newFont->m_texture = TextureManager::Get().GetTexture(texturePath, TextureCategory::Gui, TextureFilter::Linear);
				newFont->m_numChars = numChars;
				newFont->m_sizeX = sizeW;
				newFont->m_sizeY = sizeH;

				// Go through each char in the file loading metadata
				for (unsigned int i = 0; i < numChars; ++i)
				{
					a_input.getline(line, StringUtils::s_maxCharsPerLine);
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
				}

				// There is more info such as kerning here but we don't support it
				break;
			}

			a_input.close();

			m_fonts.Insert(newFontNode);
			return true;
		}
		else
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Could not find font file.");

			// Clean up the font that was allocated
			delete newFontNode->GetData();
			delete newFontNode;

			return false;
		}

		return false;
	}
	
	char m_fontPath[StringUtils::s_maxCharsPerLine];	///< Cache off path to fonts
	FontList m_fonts;									///< Storage for all fonts that are available for drawing
	Texture m_defaultFontTexture;						///< Pointer to a texture in memory loaded from an inc file
};


#endif // _ENGINE_FONT_MANAGER_H_ 
