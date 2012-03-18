#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <windows.h>
#include <GL/gl.h>

unsigned int loadTextureTGA(const char *filename);
GLubyte *loadTGA(const char *filename, int *x, int *y, int *bpp);

#endif /* _TEXTURE_H_ */