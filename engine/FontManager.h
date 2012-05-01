#ifndef _ENGINE_FONT_MANAGER_H_
#define _ENGINE_FONT_MANAGER_H_
#pragma once

#include "../core/Colour.h"
#include "../core/Vector.h"

#include "Singleton.h"
#include "Texture.h"

//\brief The font class is responsible for loading font configuration files,
//		 font textures and submitting quads to the render manager.
class FontManager : public Singleton<FontManager>
{
public:

	//\ No work done in the constructor, only Init
	FontManager() {}

	//\brief Load all fonts in the supplied argument into memory ready for drawing
	//\param a_fontPath pointer to cstring of the path to enumerate font files
	bool Init(const char * a_fontPath);

	//\brief Draw a string using the orthogonal gui render batch
	//\param a_string pointer to cstring with the character to render
	//\param a_fontName name of the loaded font to render with, will fail if not present or unloaded
	//\param a_size float of the height of character boxes, width is derived from this
	//\param a_pos 2D screen space coords to draw the font at
	//\param a_colour The colour to tint the font texture to
	//\return true if the glyph quads were submitted to the render manager
	bool DrawString(const char * a_string, const char * a_fontName, float a_size, Vector2D a_pos, Colour a_colour = sc_colourWhite);
	bool DrawDebugString(const char * a_string, Vector2D a_pos, Colour a_colour = sc_colourWhite);
	bool DrawString3D(const char * a_string, const char * a_fontName, float a_size, Vector a_pos, Colour a_colour = sc_colourWhite);

private:

	static const unsigned int s_maxCharsPerFont = 256;	// No non-unicode support needed (yet)

	//\breif Spacing and positioning info about a character in a font
	struct FontChar
	{
		float m_x;			//< X position in the texture file
		float m_y;			//< Y position in the texture file
		float m_width;		//< How wide the glyph is
		float m_height;		//< How high the glyph is
		float m_xoffset;	//< X position of the glyph insuide it's box
		float m_yoffset;	//< Y position of the glyph inside it's box
		float m_xadvance;	//< How much spacing to put in front of the glyph
	};

	struct fontInfo
	{
		char fontName[64];
		FontChar m_chars[s_maxCharsPerFont];
		unsigned int texture;
		float sizeW;
		float sizeH;
	};

	//\brief Load a font for use in drawing to the screen. Assumes a texture adjacent to config file.
	//\param a_fontConfigFilePath path to the config file specifying glyph numbers and widths
	bool LoadFont(const char * a_fontConfigFilePath);

	/*
	struct fontString
	{
		char text[256];
		int fontIndex;
		unsigned int size;
		float X;
		float Y;
	};

	struct fontString3d : fontString
	{
		float Z;
	};*/

	static const float s_debugFontSize;			// Glyph height for debug drawing
};


#endif // _ENGINE_FONT_MANAGER_H_ 
