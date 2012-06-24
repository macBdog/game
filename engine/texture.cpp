#include "Texture.h"

#include <GL/glu.h>
#include <iostream>
#include <fstream>

#include "SDL.h"

#include "Log.h"

static const char tgaHeader[] = {
		0x2a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	,0x20, 0x03, 0x58, 0x02, 0x18, 0x20, 0x43, 0x52, 0x45, 0x41, 0x54, 0x4f
	,0x52, 0x3a, 0x20, 0x54, 0x68, 0x65, 0x20, 0x47, 0x49, 0x4d, 0x50, 0x27
	,0x73, 0x20, 0x54, 0x47, 0x41, 0x20, 0x46, 0x69, 0x6c, 0x74, 0x65, 0x72
	,0x20, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x2e, 0x32
};

static const GLubyte uTGAcompare[12] = {0,0,2, 0,0,0,0,0,0,0,0,0};
static const GLubyte cTGAcompare[12] = {0,0,10,0,0,0,0,0,0,0,0,0};

bool Texture::Load(const char *a_tgaFilePath)
{
    int x, y, bpp;
    GLubyte *textureData;
    GLuint textureID;
    GLenum texFormat, intTexFormat;

	// Early out for no file case
	if (a_tgaFilePath == NULL)
	{
		return false;
	}

	// Load texture data into memory and check if successful
    textureData = loadTGA(a_tgaFilePath, x, y, bpp);
    if (textureData == NULL) 
	{ 
        return false;
    }

    switch (bpp) 
	{
        case 24:
		{
            texFormat = GL_RGB;
            intTexFormat = GL_RGB8;
		}
		break;
        case 32:
		{
            texFormat = GL_RGBA;
            intTexFormat = GL_RGBA8;
		}
        break;
        default:
		{
            texFormat = GL_RGBA;
            intTexFormat = GL_RGBA;
		}
		break;
    }
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, intTexFormat, x, y, 0, texFormat, GL_UNSIGNED_BYTE, textureData);

	m_textureId = textureID;

    free(textureData);

    return true;
}

GLubyte * Texture::loadTGA(const char *a_tgaFilePath, int &a_x, int &a_y, int &a_bpp)
{
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

    fopen_s(&input, a_tgaFilePath, "rb");
    if (input == NULL) 
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Texture file failed open: %s", a_tgaFilePath);
        return NULL;
    }

    idLength = fgetc(input);
    fgetc(input);
        
    // Check this is a TGA
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

    // Get X and Y
    for (loop = 0; loop < 9; ++loop)
	{
        fgetc(input);
	}

    a_x = fgetc(input);
    a_x += fgetc(input) << 8;
    a_y = fgetc(input);
    a_y += fgetc(input) << 8;
    a_bpp = fgetc(input);
    imgDesc = fgetc(input);

    bypp = ((a_bpp)>>3);
    size = (a_x)*(a_y)*bypp;

    output = (GLubyte *)malloc(size);
    
	if (output == NULL) {
        printf("Malloc failed in texture read\n");
        return (NULL);
    }

    // Determine TGA version (new or old)
    filePos = ftell(input);
    fseek(input, -18, SEEK_END);
    fread(vendorString, 1, 16, input);
    vendorString[16] = 0;
    if (strcmp(vendorString, "TRUEVISION-XFILE") == 0) 
	{
        tgaVersion = eTGAVersionNew;
    } 
	else
	{
        tgaVersion = eTGAVersionOld;
	}
    fseek(input, filePos, SEEK_SET); 

    // Read in pixel data
    if (isRLE) {
        offset = 0;
        xpos = (a_y)*(a_x)*bypp;
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
        offset = ((a_y)-1) * (a_x)*bypp;
        for (xloop = 0; xloop < (a_y); ++xloop) {
            fread(output+offset, 1, (a_x)*bypp, input);
            offset -= (a_x)*bypp;
        }
    }
    
    // Swap the red and blue channels
    for (loop = 0; loop < size; loop += bypp) {
        tmp1 = output[loop];
        output[loop] = output[loop+2];
        output[loop+2] = tmp1;
    }

    // If we need to flip it horizintally then do so
    if (imgDesc & 0x10) {
        for (loop = 0; loop < (a_y); ++loop) {
            for (xloop = 0; xloop < (a_x)/2; ++xloop) {
                if (a_bpp == 24) {
                    offset = loop*(a_x)*3 + (xloop*3);
                    tmp1 = loop*(a_x)*3 + ((a_x)*3)-((xloop+1)*3);
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
                    offset = loop*(a_x)*4 + (xloop*4);
                    tmp1 = loop*(a_x)*4 + ((a_x)*4)-((xloop+1)*4);
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

    // If we need to flip to compensate for texture compression
	if (isRLE == 0) 
	{
        scanLine = (char *)malloc((a_x)*(bypp));
        for (loop = 0; loop < (a_y)/2; ++loop) {
            memcpy(scanLine, output+(loop*bypp*(a_x)), (a_x)*bypp);
            memcpy(output+(loop*bypp*(a_x)),
                    output+(a_x)*((a_y)-1-loop)*bypp, (a_x)*bypp);
            memcpy(output+(a_x)*((a_y)-1-loop)*bypp, scanLine, (a_x)*bypp);
        }
        free(scanLine);
    }

    fclose(input);
    return output;
}
