#ifndef _ENGINE_FONT_H_
#define _ENGINE_FONT_H_
#pragma once

class Font
{
public:

struct fontChar
{
	float x;
	float y;
	float width;
	float height;
	float xoffset;
	float yoffset;
	float xadvance;
};

struct fontInfo
{
	char fontName[64];
	fontChar chars[256];
	unsigned int texture;
	float sizeW;
	float sizeH;
};

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
};

void initFonts(void);
void loadFont(const char * a_fontName);
void shutdownFonts(void);
void drawStringDefault(const char * tex, unsigned int size, float X, float Y);
void drawString(const char text[], const char * fontName, unsigned int size, float X, float Y);
void draw3dString(const char text[], const char * fontName, unsigned int size, float X, float Y, float Z);
void drawStrings();
void draw3dStrings();

private:

};


#endif /* _ENGINE_FONT_H_ */
