#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <windows.h>
#include <GL/gl.h>

GLuint loadTextureBMP(char* file, int mode);
GLuint loadTextureTGA(const char *filename);
GLubyte *loadTGA(const char *filename, int *x, int *y, int *bpp);
int writeTGAFromData(int x, int y, int bpp, const char *data, const char *filename);

#endif /* _TEXTURE_H_ */