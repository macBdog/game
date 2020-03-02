#include "Texture.h"

#include <windows.h>

#include "GL/glew.h"
#include <GL/glu.h>
#include <iostream>
#include <fstream>

#include "SDL.h"

#include "Log.h"

using namespace std;

static const char tgaHeader[] = {
		0x2a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	,0x20, 0x03, 0x58, 0x02, 0x18, 0x20, 0x43, 0x52, 0x45, 0x41, 0x54, 0x4f
	,0x52, 0x3a, 0x20, 0x54, 0x68, 0x65, 0x20, 0x47, 0x49, 0x4d, 0x50, 0x27
	,0x73, 0x20, 0x54, 0x47, 0x41, 0x20, 0x46, 0x69, 0x6c, 0x74, 0x65, 0x72
	,0x20, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x2e, 0x32
};

static const GLubyte uTGAcompare[12] = {0,0,2, 0,0,0,0,0,0,0,0,0};
static const GLubyte cTGAcompare[12] = {0,0,10,0,0,0,0,0,0,0,0,0};

bool Texture::LoadTGAFromFile(const char * a_tgaFilePath, bool a_useLinearFilter)
{
	// Early out for no file case
	if (a_tgaFilePath == nullptr)
	{
		return false;
	}

	// Store off the file name
	memcpy(m_filePath, a_tgaFilePath, sizeof(char) * strlen(a_tgaFilePath));

	// Load texture data into memory and check if successful
	ifstream textureFile(a_tgaFilePath, std::ios::binary);

	// Make sure there was actually a file there
	if (!textureFile.good())
	{
		return false;
	}

	textureFile.seekg(0, std::ios::end);
	size_t textureSize = (size_t)textureFile.tellg();
	textureFile.seekg(0, std::ios::beg);
	char * fileBuffer = (char *)malloc(textureSize);
	
	// Read the whole file into a buffer
	bool loadSuccess = false;
	if (textureFile.read(fileBuffer, textureSize))
	{
		loadSuccess = LoadTGAFromMemory(fileBuffer, textureSize, a_useLinearFilter);
	}
	free(fileBuffer);
	return loadSuccess;
}

bool Texture::LoadTGAFromMemory(void * a_texture, size_t a_textureSize, bool a_useLinearFilter)
{
	// Early out for no pointer
	if (a_texture == nullptr || a_textureSize <= 0)
	{
		return false;
	}

	// Load texture data into memory and check if successful
	int x, y, bpp;
	GLubyte * textureData = nullptr;
	textureData = loadTGAFromMemory(a_texture, a_textureSize, x, y, bpp, textureData);
	if (textureData == nullptr) 
	{ 
		return false;
	}

	return GenerateTexture(x, y, bpp, a_useLinearFilter, textureData);
}

bool Texture::LoadFromMemoryAndFree(int a_width, int a_height, int a_bpp, void * a_textureData, bool a_useLinearFilter)
{
	// Early out before any allocation for non power of two texture
	if (!((a_width != 0) && !(a_width & (a_width - 1))) || !((a_height != 0) && !(a_height & (a_height - 1))))
	{
		printf("Can't load non power of two dimension texture\n");
		return false;
	}

	const int bypp = ((a_bpp) >> 3);
	const int textureSize = a_width * a_height * bypp;
	GLubyte * textureData = (GLubyte *)a_textureData;
	if (textureData == nullptr)
	{
		printf("Malloc failed in texture load from blank\n");
		return nullptr;
	}

	return GenerateTexture(a_width, a_height, a_bpp, a_useLinearFilter, textureData);
}

bool Texture::RegenerateTexture(int a_width, int a_height, int a_bpp, void * a_textureData)
{
	// Early out before any allocation for non power of two texture
	if (!((a_width != 0) && !(a_width & (a_width - 1))) || !((a_height != 0) && !(a_height & (a_height - 1))))
	{
		printf("Can't regen non power of two dimension texture\n");
		return false;
	}

	if (a_bpp == 24)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a_width, a_height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)a_textureData);
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, a_width, a_height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)a_textureData);
	}

	return true;
}

bool Texture::GenerateTexture(int a_x, int a_y, int a_bpp, bool a_useLinearFilter, GLubyte * textureData)
{
	GLuint textureID;
	GLenum texFormat, intTexFormat;
    switch (a_bpp) 
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
    
	const int numMips = 8;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
	glTexStorage2D(GL_TEXTURE_2D, numMips, GL_UNSIGNED_BYTE, a_x, a_y);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (a_useLinearFilter)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
    
    glTexImage2D(GL_TEXTURE_2D, 0, intTexFormat, a_x, a_y, 0, texFormat, GL_UNSIGNED_BYTE, textureData);

	if (a_useLinearFilter)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	m_textureId = textureID;

    free(textureData);

    return true;
}

GLubyte * Texture::loadTGAFromMemory(void * a_texture, size_t a_textureSize, int & a_x, int & a_y, int & a_bpp, GLubyte * a_textureData_OUT)
{
	char * input = (char *)a_texture;
    GLubyte *output;
    int loop, size, tmp1, xloop, offset, tgaVersion, bypp, imgDesc;
    static char vendorString[44];
    long filePos;
    unsigned char tmpd[4];
    char *scanLine;
    int idLength;
    int isRLE = 0, xpos, count; // RLE variables
    char pix[4];

	idLength = *input++;
	input++;
        
    // Check this is a TGA
	loop = *input++;
    switch (loop) 
	{
		case 0x02:
		{
			break;
		}
        case 0x0A:
		{
			isRLE = 1;
            break;
		}
        default:
		{
			return (nullptr);
		}
    }

    // Get X and Y
    for (loop = 0; loop < 9; ++loop)
	{
		input++;
	}

	a_x = *input++;
	a_x += *input++ << 8;
	a_y = *input++;
	a_y += *input++ << 8;
	a_bpp = *input++;
	imgDesc = *input++;

	a_x = a_x < 0 ? -a_x : a_x;
	a_y = a_y < 0 ? -a_y : a_y;
    bypp = ((a_bpp)>>3);
    size = (a_x)*(a_y)*bypp;

	// Early out before any allocation for non power of two texture
	if (!((a_x != 0) && !(a_x & (a_x - 1))) || !((a_y != 0) && !(a_y & (a_y - 1))))
	{
		printf("Can't load non power of two dimension texture\n");
		return nullptr;
	}

    output = (GLubyte *)malloc(size);
    
	if (output == nullptr) 
	{
        printf("Malloc failed in texture read\n");
        return nullptr;
    }
	
    // Determine TGA version (new or old)
    filePos = input - (char *)a_texture;
	input = ((char *)a_texture + a_textureSize) - 18;
    memcpy(vendorString, input, 1 * 16);	input += (1*16);
    vendorString[16] = 0;
    if (strcmp(vendorString, "TRUEVISION-XFILE") == 0) 
	{
        tgaVersion = TGAVersion::New;
    } 
	else
	{
        tgaVersion = TGAVersion::Old;
	}
	input = (char *)a_texture + filePos;
    
    // Read in pixel data
    if (isRLE) 
	{
        offset = 0;
        xpos = (a_y) * (a_x) * bypp;
		
        while (offset < xpos) 
		{
            count = *input++;

            if (count & 0x80) 
			{ 
				// Repetition packet
                count &= 0x7F;
                ++count;
                memcpy(pix, input, 1 * bypp);
				input += 1*bypp;

				// Output count pixels
                for (loop = 0; loop < count; ++loop) 
				{
                    memcpy(output+offset, pix, bypp);
                    offset += bypp;
                }
            } 
			else 
			{    
				// Raw packet
                ++count;
                memcpy(output+offset, input, 1 * (bypp*count));
				input += 1 * (bypp*count);
                offset += bypp*count;
            }
        } 
    } 
	else 
	{
        offset = ((a_y)-1) * (a_x)*bypp;
        for (xloop = 0; xloop < (a_y); ++xloop) 
		{
            memcpy(output+offset, input, 1 * ((a_x)*bypp));			
			input += 1 * ((a_x)*bypp);
            offset -= (a_x)*bypp;
        }
    }
    
    // Swap the red and blue channels
    for (loop = 0; loop < size; loop += bypp) 
	{
        tmp1 = output[loop];
        output[loop] = output[loop+2];
        output[loop+2] = tmp1;
    }

    // If we need to flip it horizontally then do so
    if (imgDesc & 0x10) 
	{
        for (loop = 0; loop < (a_y); ++loop) 
		{
            for (xloop = 0; xloop < (a_x)/2; ++xloop) 
			{
                if (a_bpp == 24) 
				{
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
                } 
				else 
				{
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
        for (loop = 0; loop < (a_y)/2; ++loop) 
		{
            memcpy(scanLine, output+(loop*bypp*(a_x)), (a_x)*bypp);
            memcpy(output+(loop*bypp*(a_x)), output+(a_x)*((a_y)-1-loop)*bypp, (a_x)*bypp);
            memcpy(output+(a_x)*((a_y)-1-loop)*bypp, scanLine, (a_x)*bypp);
        }
        free(scanLine);
    }
    return output;
}
