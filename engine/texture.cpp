#include "texture.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <fstream>

#include "SDL.h"

static int TEXTURE_DEPTH;

static char tgaHeader[] = {
	 0x2a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	,0x20, 0x03, 0x58, 0x02, 0x18, 0x20, 0x43, 0x52, 0x45, 0x41, 0x54, 0x4f
	,0x52, 0x3a, 0x20, 0x54, 0x68, 0x65, 0x20, 0x47, 0x49, 0x4d, 0x50, 0x27
	,0x73, 0x20, 0x54, 0x47, 0x41, 0x20, 0x46, 0x69, 0x6c, 0x74, 0x65, 0x72
	,0x20, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x2e, 0x32
};

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

/*
 * TGA loading bizniss
 */
GLubyte uTGAcompare[12] = {0,0,2, 0,0,0,0,0,0,0,0,0};
GLubyte cTGAcompare[12] = {0,0,10,0,0,0,0,0,0,0,0,0};

/* 
 * load in bitmap as a GL texture in three modes
 */
GLuint loadTextureBMP(char* file, int mode) {
	GLuint texture;
	SDL_Surface *TextureImage; 

    TextureImage = SDL_LoadBMP(file); 
	if (TextureImage == NULL) {
        printf("Couldn't load the texture reason: %s\n",  SDL_GetError());
        return FALSE;
	}
	//generate three textures
	glGenTextures(1, &texture);
	
	switch (mode) {
		case 3:
			// make a mip mapped tex that can be of any size
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage->w, TextureImage->h, GL_BGR_EXT, GL_UNSIGNED_BYTE, TextureImage->pixels);
			break;
		case 2:
			// make a linear tex suitable for 2D shapes
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage->w, TextureImage->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, TextureImage->pixels);
			break;
		case 1:
			// make a low detail nearest tex
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage->w, TextureImage->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, TextureImage->pixels);	
			break;
		default:
			return FALSE;
			break;
	}
	
	//clean up
	if (TextureImage) { SDL_FreeSurface(TextureImage); }
    return texture;
}

/*
 * loading a TGA file
 */
GLuint loadTextureTGA(const char *filename) {
    int x, y, bpp;
    GLubyte *textureData;
    GLuint textureID;
    GLenum texFormat, intTexFormat;

    textureData = loadTGA(filename, &x, &y, &bpp);
    if (textureData == NULL) { 
        return (0);
    }

    switch (bpp) {
        case 24:
            texFormat = GL_RGB;
            if (TEXTURE_DEPTH == 16)
                intTexFormat = GL_RGB5;
            else
                intTexFormat = GL_RGB8;
            break;
        case 32:
            texFormat = GL_RGBA;
            if (TEXTURE_DEPTH == 16)
                intTexFormat = GL_RGBA4;
            else
                intTexFormat = GL_RGBA8;
            break;
        default:
            texFormat = GL_RGBA;
            intTexFormat = GL_RGBA;
    }
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, intTexFormat, x, y, 0, texFormat,
            GL_UNSIGNED_BYTE, textureData);

    free(textureData);

    return (textureID);
}

/*
 * TGA file version
 */
#define TGA_OLD 0
#define TGA_NEW 1

GLubyte *loadTGA(const char *filename, int *x, int *y, int *bpp) {
    FILE *input;
    GLubyte *output;
    int loop, size, tmp1, xloop, offset, tgaVersion, bypp, imgDesc;
    static char vendorString[44];
    long filePos;
    unsigned char tmpd[4];
    char *scanLine;
    int idLength;
    int isRLE = 0, xpos, count; /* RLE variables */
    char pix[4];

    fopen_s(&input, filename, "rb");
    if (input == NULL) {
        printf("Texture file failed open: \"%s\"\n", filename);
        return (NULL);
    }

    idLength = fgetc(input);
    fgetc(input);
        
    /*
     * Check this is a TGA
     */
    loop = fgetc(input);
    switch (loop) {
        case 0x02:
            break;
        case 0x0A:
            isRLE = 1;
            break;
        default:
        return (NULL);
    }

    /*
     * Get X and Y
     */
    for (loop = 0; loop < 9; ++loop)
        fgetc(input);

    *x = fgetc(input);
    *x += fgetc(input) << 8;
    *y = fgetc(input);
    *y += fgetc(input) << 8;
    *bpp = fgetc(input);
    imgDesc = fgetc(input);

    bypp = ((*bpp)>>3);
    size = (*x)*(*y)*bypp;

    output = (GLubyte *)malloc(size);
    
	if (output == NULL) {
        printf("Malloc failed in texture read\n");
        return (NULL);
    }

    /*
     * Determine TGA version (new or old)
     */
    filePos = ftell(input);
    fseek(input, -18, SEEK_END);
    fread(vendorString, 1, 16, input);
    vendorString[16] = 0;
    if (strcmp(vendorString, "TRUEVISION-XFILE") == 0) {
        tgaVersion = TGA_NEW;
    } else
        tgaVersion = TGA_OLD;
    fseek(input, filePos, SEEK_SET); 

    /*
     * Read in pixel data
     */
    if (isRLE) {
        offset = 0;
        xpos = (*y)*(*x)*bypp;
        while (offset < xpos) {
            count = fgetc(input);
            if (count & 0x80) { /* repitition packet */
                count &= 0x7F;
                ++count;
                fread(pix, 1, bypp, input); /* read 1 pixel */
                for (loop = 0; loop < count; ++loop) {/* output count pixels */
                    memcpy(output+offset, pix, bypp);
                    offset += bypp;
                }
            } else {    /* raw packet */
                ++count;
                fread(output+offset, 1, bypp*count, input);
                offset += bypp*count;
            }
        } /* while loop */
    } else {
        offset = ((*y)-1) * (*x)*bypp;
        for (xloop = 0; xloop < (*y); ++xloop) {
            fread(output+offset, 1, (*x)*bypp, input);
            offset -= (*x)*bypp;
        }
    }
    
    /*
     * Swap the red and blue channels
     */
    for (loop = 0; loop < size; loop += bypp) {
        tmp1 = output[loop];
        output[loop] = output[loop+2];
        output[loop+2] = tmp1;
    }

    /*
     * if we need to flip it horizintally then do so
     */
    if (imgDesc & 0x10) {
        for (loop = 0; loop < (*y); ++loop) {
            for (xloop = 0; xloop < (*x)/2; ++xloop) {
                if (*bpp == 24) {
                    offset = loop*(*x)*3 + (xloop*3);
                    tmp1 = loop*(*x)*3 + ((*x)*3)-((xloop+1)*3);
                    tmpd[0] = output[offset];
                    tmpd[1] = output[offset+1];
                    tmpd[2] = output[offset+2];

                    output[offset] = output[tmp1 - offset];
                    output[offset+1] = output[tmp1 - offset +1];
                    output[offset+2] = output[tmp1 - offset +2];
                    
                    output[tmp1 - offset] = tmpd[0];
                    output[tmp1 - offset +1] = tmpd[1];
                    output[tmp1 - offset +2] = tmpd[2];
                } else {
                    offset = loop*(*x)*4 + (xloop*4);
                    tmp1 = loop*(*x)*4 + ((*x)*4)-((xloop+1)*4);
                    tmpd[0] = output[offset];
                    tmpd[1] = output[offset+1];
                    tmpd[2] = output[offset+2];
                    tmpd[3] = output[offset+3];

                    output[offset] = output[tmp1];
                    output[offset+1] = output[tmp1+1];
                    output[offset+2] = output[tmp1+2];
                    output[offset+3] = output[tmp1+3];
                    
                    output[tmp1] = tmpd[0];
                    output[tmp1 +1] = tmpd[1];
                    output[tmp1 +2] = tmpd[2];
                    output[tmp1 +3] = tmpd[3];
                }
            }
        }
    }

    /*
     * If we need to flip it vertical then do so
     */
    if (!(imgDesc & 0x20)) {
        scanLine = (char *)malloc((*x)*(bypp));
        for (loop = 0; loop < (*y)/2; ++loop) {
            memcpy(scanLine, output+(loop*bypp*(*x)), (*x)*bypp);
            memcpy(output+(loop*bypp*(*x)),
                    output+(*x)*((*y)-1-loop)*bypp, (*x)*bypp);
            memcpy(output+(*x)*((*y)-1-loop)*bypp, scanLine, (*x)*bypp);
        }
        free(scanLine);
    }

    fclose(input);
    return output;
}

int writeTGAFromData(int x, int y, int bpp, const char *data, const char *filename) {
    FILE *output;
    int size;

    /*
     * Put X, Y, and bpp into the tga header.
     */
    tgaHeader[12] = x % 256;
    tgaHeader[13] = x >> 8;
    tgaHeader[14] = y % 256;
    tgaHeader[15] = y >> 8;

    /*
     * Calculate size
     */
    size = x * y * ((bpp+7) / 8);

    /*
     * Open output and write the stuff
     */
     fopen_s(&output, filename, "w");
    
	if (output == NULL) {
        printf("writeTGAFromData failed to open %s for writing\n", filename);
        return (-1);
    }

    fwrite(tgaHeader, 60, 1, output);
    fwrite(data, size, 1, output);

    return (0);
}
