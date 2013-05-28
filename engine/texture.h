#ifndef _ENGINE_TEXTURE_H_
#define _ENGINE_TEXTURE_H_

#include <windows.h>
#include <GL/glew.h>

#include "StringUtils.h"

class Texture
{
public:

	//\brief Texcoords are generated from the orientation hint
	enum eOrientation
	{
		eOrientationNormal = 0,
		eOrientationFlipHoriz,
		eOrientationFlipVert,
		eOrientationFlipBoth,

		eOrientationCount
	};
	
	// Assigned texture IDs start from 0
	Texture() : m_textureId(-1) { m_filePath[0] = '\0'; }

	//\brief Load a TGA file into memory and store out the texture ID
	//\param a_tgaFilePath is a const pointer to a c string with the fully qualified path
	//\param a_useLinearFilter is an optional, if set to false, textures will approximate by pixel
	//\return bool true if the texture was loaded succesfullly
	bool Load(const char *a_tgaFilePath, bool a_useLinearFilter = true);

	//\brief Utility methods for texture member data for convenience
	inline bool IsLoaded() { return m_textureId >= 0; }
	inline unsigned int GetId() { return m_textureId; }
	inline const char * GetFilePath() { return m_filePath; }
	inline const char * GetFileName() { return StringUtils::ExtractFileNameFromPath(m_filePath); }

private:

	enum eTGAVersion
	{
		eTGAVersionOld = 0,
		eTGAVersionNew,

		eTGAVersionCount
	};

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

	//\brief Return a pointer to texture data in memory
	GLubyte *loadTGA(const char *a_tgaFilePath, int &a_x, int &a_y, int &a_bpp);

	int m_textureId;			///< Texture ID as stored off by the load operation
	char m_filePath[StringUtils::s_maxCharsPerLine];	///< File path stored off during load, fully qualified

};

#endif /* _ENGINE_TEXTURE_H_ */