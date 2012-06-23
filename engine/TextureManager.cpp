#include "TextureManager.h"

template<> TextureManager * Singleton<TextureManager>::s_instance = NULL;

int TextureManager::GetTexture(const char *a_tgaPath, eTextureCategory a_cat)
{
   return -1;
}