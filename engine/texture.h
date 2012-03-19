#ifndef _ENGINE_TEXTURE_H_
#define _ENGINE_TEXTURE_H_

#include <windows.h>
#include <GL/gl.h>

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
	}
	
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

	//\brief Load a TGA file into memory and store out the texture ID
	bool Load(const char *a_tgaFilePath);
	inline bool IsLoaded() { return m_textureId > 0; }

private:

	//\brief Return a pointer to texture data in memory
	GLubyte *loadTGA(const char *a_tgaFilePath, unsigned int *a_x, unsigned int *a_y, unsigned int *a_bpp);

	int m_textureId;		// Texture ID as stored off by the load operation

};

#endif /* _ENGINE_TEXTURE_H_ */