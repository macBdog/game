#ifndef _ENGINE_FONT_MANAGER_H_
#define _ENGINE_FONT_MANAGER_H_
#pragma once

#include "../core/Colour.h"
#include "../core/LinkedList.h"
#include "../core/Vector.h"

#include "RenderManager.h"
#include "Singleton.h"
#include "StringHash.h"
#include "StringUtils.h"
#include "Texture.h"

#include "FileManager.h"

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
	bool Shutdown();

	//\brief Draw a string using the orthogonal gui render batch
	//\param a_string pointer to cstring with the character to render
	//\param a_fontName name of the loaded font to render with, will fail if not present or unloaded
	//\param a_size float of the height of character boxes, width is derived from this
	//\param a_pos 2D screen space coords to draw the font at, from the top left
	//\param a_colour The colour to tint the font texture to
	//\return true if the glyph quads were submitted to the render manager
	bool DrawString(const char * a_string, StringHash * a_fontName, float a_size, Vector2 a_pos, Colour a_colour = sc_colourWhite, RenderManager::eBatch a_batch = RenderManager::eBatchGui);
	bool DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector2 a_pos, Colour a_colour = sc_colourWhite, RenderManager::eBatch a_batch = RenderManager::eBatchGui);
	bool DrawString(const char * a_string, unsigned int a_fontNameHash, float a_size, Vector a_pos, Colour a_colour = sc_colourWhite, RenderManager::eBatch a_batch = RenderManager::eBatchWorld);
	bool DrawDebugString2D(const char * a_string, Vector2 a_pos, Colour a_colour = sc_colourWhite, RenderManager::eBatch a_batch = RenderManager::eBatchDebug2D);
	bool DrawDebugString3D(const char * a_string, float a_size, Vector a_pos, Colour a_colour = sc_colourWhite);

	//\brief Get the symbol for a loaded font name
	//\param pointer to a cstring of the font name to retreieve
	//\return pointer to a StringHash or NULL for failure
	StringHash * GetLoadedFontName(const char * a_fontName);
	const char * GetLoadedFontName(unsigned int a_fontNameHash);

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
		float m_x;						///< X position in the texture file
		float m_y;						///< Y position in the texture file
		float m_width;					///< How wide the glyph is
		float m_height;					///< How high the glyph is
		float m_xoffset;				///< X position of the glyph insuide it's box
		float m_yoffset;				///< Y position of the glyph inside it's box
		float m_xadvance;				///< How much spacing to put in front of the glyph
		unsigned int m_displayListId;	///< The generated display list ID for drawing each character
	};

	//\brief Info about a font and a group of all the character info
	struct Font
	{
		StringHash	m_fontName;
		FontChar	m_chars[s_maxCharsPerFont];
		Texture *	m_texture;
		unsigned int m_numChars;
		unsigned int m_sizeX;
		unsigned int m_sizeY;
	};

	//\brief Alias to store a list of fonts for drawing
	typedef LinkedListNode<Font> FontListNode;
	typedef LinkedList<Font> FontList;

	//\brief Load a font for use in drawing to the screen. Assumes a texture adjacent to config file.
	//\param a_fontConfigFilePath path to the config file specifying glyph numbers and widths
	//\return True if the load operation was completed successfully
	bool LoadFont(const char * a_fontConfigFilePath);
	
	char m_fontPath[StringUtils::s_maxCharsPerLine];	///< Cache off path to fonts
	FontList m_fonts;									///< Storage for all fonts that are available for drawing
};


#endif // _ENGINE_FONT_MANAGER_H_ 
