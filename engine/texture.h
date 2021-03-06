#ifndef _ENGINE_TEXTURE_H_
#define _ENGINE_TEXTURE_H_

#include "StringUtils.h"

//\brief Texcoords are generated from the orientation hint
namespace TextureOrientation
{
	enum Enum
	{
		Normal = 0,
		FlipHoriz,
		FlipVert,
		FlipBoth,
		Count
	};
}

namespace TGAVersion
{
	enum Enum
	{
		Old = 0,
		New,
		Count
	};
}

typedef unsigned char GLubyte;
typedef unsigned int GLuint;

class Texture
{
public:

	// Assigned texture IDs start from 0
	Texture() : m_textureId(-1) { m_filePath[0] = '\0'; }

	//\brief Load a TGA file into memory and store out the texture ID
	//\param a_tgaFilePath is a const pointer to a c string with the fully qualified path
	//\param a_useLinearFilter is an optional, if set to false, textures will approximate by pixel
	//\return bool true if the texture was loaded succesfullly
	bool LoadTGAFromFile(const char * a_tgaFilePath, bool a_useLinearFilter = true);
	bool LoadTGAFromMemory(void * a_texture, size_t a_textureSize, bool a_useLinearFilter = true);

	//\brief Load from a pre allocated buffer of memory that matches the bpp, so RGBA8 for 32 bpp, will free the memory after creating the texture
	bool LoadFromMemoryAndFree(int a_width, int a_height, int a_bpp, void * a_textureData, bool a_useLinearFilter = true);

	//\brief Reload a texture from a buffer of raw OpenGL format pixel data
	bool RegenerateTexture(int a_width, int a_height, int a_bpp, void * a_textureData);

	//\brief Utility methods for texture member data for convenience
	inline bool IsLoaded() { return m_textureId >= 0; }
	inline unsigned int GetId() { return m_textureId; }
	inline const char * GetFilePath() { return m_filePath; }
	inline const char * GetFileName() { return StringUtils::ExtractFileNameFromPath(m_filePath); }

private:

	//\brief Info stored about each texture <- nice comment asshole
	typedef struct {
		GLubyte		header[6];
		GLuint		bytesPerPixel;
		GLuint		imageSize;
		GLuint		temp;
		GLuint		type;
		GLuint		Height;
		GLuint		Width;
		GLuint		Bpp;
	} TGA;

	// Create the graphics resources used when rendering this texture
	bool GenerateTexture(int a_x, int a_y, int a_bpp, bool a_useLinearFilter, GLubyte * a_textureData);

	//\brief Return a pointer to texture data in memory
	//\param pointer to in memory tga data file to read from
	//\param a_x_OUT ref to int to populate with texture x dimension
	//\param a_y_OUT ref to int to populate with texture y dimension
	//\param a_bpp_OUT ref to int to populate with texture bits per pixel
	//\param a_textureData pointer which will be assigned to the memory containing the texture data binary
	GLubyte * loadTGAFromMemory(void * a_texture, size_t a_textureSize, int & a_x_OUT, int & a_y_OUT, int & a_bpp_OUT, GLubyte * a_textureData_OUT);

	int m_textureId;									///< Texture ID as stored off by the load operation
	char m_filePath[StringUtils::s_maxCharsPerLine];	///< File path stored off during load, fully qualified

};

#endif /* _ENGINE_TEXTURE_H_ */