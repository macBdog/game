#include "FileManager.h"
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

const float TextureManager::s_updateFreq = 1.0f;

TextureManager::TextureManager(float a_updateFreq)
	: m_updateFreq(a_updateFreq)
	, m_updateTimer(0.0f)
{
}

bool TextureManager::Startup(const char * a_texturePath)
{
	// Reset update timer in case we have been shutdown the re started
	 m_updateTimer = 0;

	// Init a pool of memory for each category
	for (unsigned int i = 0; i < eCategoryCount; ++i)
	{
		m_texturePool[i].Init(s_texurePoolSize[i]);

		// This can be removed in all but DEBUG configuration, but its nice when viewing memory
		memset(m_texturePool[i].GetHead(), 0, m_texturePool[i].GetAllocationSizeBytes());
	}

	// Cache off the texture path for non qualified addressing of fonts
	memset(&m_texturePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_texturePath, a_texturePath, strlen(a_texturePath));

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

bool TextureManager::Update(float a_dt)
{
	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
		return false;
	}
	else // Due for an update, scan all textures
	{
		m_updateTimer = 0.0f;
		bool textureReloaded = false;
		// For each texture category
		for (unsigned int i = 0; i < eCategoryCount; ++i)
		{
			// Each texture in the category gets tested
			ManagedTexture * curTex = NULL;
			while ( m_textureMap[i].GetNext(curTex) && curTex != NULL)
			{
				unsigned int curTimeStamp = FileManager::Get().GetFileTimeStamp(curTex->m_path);
				if (curTimeStamp > curTex->m_timeStamp)
				{
					Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "Change detected in %s, reloading.", curTex->m_path);
					textureReloaded = curTex->m_texture.Load(curTex->m_path);
					curTex->m_timeStamp = curTimeStamp;
				}
			}
		}
		return textureReloaded;
	}
}

Texture * TextureManager::GetTexture(const char *a_tgaPath, eTextureCategory a_cat)
{
	// Texture paths are either fully qualified or relative to the config texture dir
	char fileNameBuf[StringUtils::s_maxCharsPerLine];
	if (!strstr(a_tgaPath, ":\\"))
	{
		sprintf(fileNameBuf, "%s%s", m_texturePath, a_tgaPath);
	} 
	else // Already fully qualified
	{
		sprintf(fileNameBuf, "%s", a_tgaPath);
	}
	
	// Get the identifier for the new texture
	StringHash texHash(fileNameBuf);
	unsigned int texId = texHash.GetHash();
	eTextureCategory a_loadedCat = IsTextureLoaded(texId);

	// If it already exists
	if (a_loadedCat != eCategoryNone)
	{
		// Just returned the cached copy
		ManagedTexture * foundTex = NULL;
		m_textureMap[a_loadedCat].Get(texId, foundTex);
		return &foundTex->m_texture;
	}
	else if (ManagedTexture * newTex = m_texturePool[a_cat].Allocate(sizeof(Texture)))
	{
		// Insert the newly allocated texture
		newTex->m_texture.Load(fileNameBuf);
		newTex->m_timeStamp = FileManager::Get().GetFileTimeStamp(fileNameBuf);
		sprintf(newTex->m_path, "%s", fileNameBuf);
		m_textureMap[a_cat].Insert(texId, newTex);
		return &newTex->m_texture;
	}
	else // Report the error
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Texture allocation failed for %s, fileNameBuf");
		return NULL;
	}
   return NULL;
}

TextureManager::eTextureCategory TextureManager::IsTextureLoaded(unsigned int a_tgaPathHash)
{
	// Look through each category for the target texture
	for (unsigned int i = 0; i < eCategoryCount; ++i)
	{
		ManagedTexture * foundTex = NULL;
		if (m_textureMap[i].Get(a_tgaPathHash, foundTex))
		{
			return (eTextureCategory)i;
		}
	}
	return eCategoryNone;
}