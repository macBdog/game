#include "Log.h"

#include "TextureManager.h"

template<> TextureManager * Singleton<TextureManager>::s_instance = NULL;

const unsigned int TextureManager::s_texurePoolSize[eCategoryCount] = 
{
	4096,		// 4 meg for debug textures
	32768,		// 32 for gui
	32768,		// 32 for models
	4096,		// 4 meg for particles
	32768		// 32 for world
};

bool TextureManager::Startup()
{
	// Init a pool of memory for each category
	for (unsigned int i = 0; i < eCategoryCount; ++i)
	{
		m_texturePool[i].Init(s_texurePoolSize[i]);

		// This can be removed in all but DEBUG configuration, but its nice when viewing memory
		memset(m_texturePool[i].GetHead(), 0, m_texturePool[i].GetAllocationSizeBytes());
	}

	return true;
}

bool TextureManager::Shutdown()
{
	// Cleanup memory
	for (unsigned int i = 0; i < eCategoryCount; ++i)
	{
		m_texturePool[i].Done();
	}

	return true;
}

Texture * TextureManager::GetTexture(const char *a_tgaPath, eTextureCategory a_cat)
{
	// Get the identifier for the new texture
	StringHash texHash(a_tgaPath);
	unsigned int texId = texHash.GetHash();
	eTextureCategory a_loadedCat = IsTextureLoaded(texId);

	// If it already exists
	if (a_loadedCat != eCategoryNone)
	{
		// Just returned the cached copy
		Texture * foundTex = NULL;
		m_textureMap[a_loadedCat].Get(texId, foundTex);
		return foundTex;
	}
	else if (Texture * newTex = m_texturePool[a_cat].Allocate(sizeof(Texture)))
	{
		// Insert the newly allocated texture
		newTex->Load(a_tgaPath);
		m_textureMap[a_cat].Insert(texId, newTex);
		return newTex;
	}
	else // Report the error
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Texture allocation failed for %s, a_tgaPath");
		return NULL;
	}
   return NULL;
}

TextureManager::eTextureCategory TextureManager::IsTextureLoaded(unsigned int a_tgaPathHash)
{
	// Look through each category for the target texture
	for (unsigned int i = 0; i < eCategoryCount; ++i)
	{
		Texture * foundTex = NULL;
		if (m_textureMap[i].Get(a_tgaPathHash, foundTex))
		{
			return (eTextureCategory)i;
		}
	}
	return eCategoryNone;
}