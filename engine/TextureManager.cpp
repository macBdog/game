#include "FileManager.h"
#include "Log.h"

#include "TextureManager.h"

template<> TextureManager * Singleton<TextureManager>::s_instance = NULL;

const unsigned int TextureManager::s_texurePoolSize[TextureCategory::Count] = 
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
	, m_dataPack(NULL)
{
	m_texturePath[0] = '\0';
	m_filterMode = TextureFilter::Invalid;
}

bool TextureManager::Startup(const char * a_texturePath, bool a_useLinearTextureFilter)
{
	// Cache off the texture path for non qualified loading of textures
	memset(&m_texturePath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_texturePath, a_texturePath, sizeof(char) * strlen(a_texturePath) + 1);
	
	return Init(a_useLinearTextureFilter);
}

bool TextureManager::Startup(const char * a_texturePath,  DataPack * a_dataPack, bool a_useLinearTextureFilter)
{
	// Cache off the datapack path for loading textures from pack
	m_dataPack = a_dataPack;
	memset(&m_texturePath, 0, StringUtils::s_maxCharsPerLine);
	strncpy(m_texturePath, a_texturePath, sizeof(char) * strlen(a_texturePath) + 1);

	return Init(a_useLinearTextureFilter);
}

bool TextureManager::Init(bool a_useLinearTextureFilter)
{
	// Reset update timer in case we have been shutdown the re started
	m_updateTimer = 0;

	// Init a pool of memory for each category
	for (unsigned int i = 0; i < TextureCategory::Count; ++i)
	{
		if (m_texturePool[i].Init(s_texurePoolSize[i]))
		{
			// This can be removed in all but DEBUG configuration, but its nice when viewing memory
			memset(m_texturePool[i].GetHead(), 0, m_texturePool[i].GetAllocationSizeBytes());
		}
		else // Allocation of the pool failed in Init
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture memory allocation for texture categpry %d", i);
			return false;
		}
	}

	// Set filtering rule
	m_filterMode = a_useLinearTextureFilter ? TextureFilter::Linear : TextureFilter::Nearest;

	return true;
}

bool TextureManager::Shutdown()
{
	// Cleanup memory
	for (unsigned int i = 0; i < TextureCategory::Count; ++i)
	{
		m_texturePool[i].Done();
	}

	return true;
}

bool TextureManager::Update(float a_dt)
{
#ifdef _RELEASE
	return true;
#endif

	DataPack & dataPack = DataPack::Get();
	if (dataPack.IsLoaded())
	{
		return true;
	}

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
		for (unsigned int i = 0; i < TextureCategory::Count; ++i)
		{
			// Each texture in the category gets tested
			ManagedTexture * curTex = NULL;
			while ( m_textureMap[i].GetNext(curTex) && curTex != NULL)
			{
				FileManager::Timestamp curTimeStamp;
				if (FileManager::Get().GetFileTimeStamp(curTex->m_path, curTimeStamp))
				{
					if (curTimeStamp > curTex->m_timeStamp)
					{
						Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in texture %s, reloading.", curTex->m_path);
						textureReloaded = curTex->m_texture.LoadFromFile(curTex->m_path);
						curTex->m_timeStamp = curTimeStamp;
					}
				}
			}
		}
		return textureReloaded;
	}
}

Texture * TextureManager::GetTexture(const char * a_tgaPath, TextureCategory::Enum a_cat, TextureFilter::Enum a_currentFilter)
{
	// Texture paths are either fully qualified or relative to the config texture dir
	bool readFromDataPack = m_dataPack != NULL && m_dataPack->IsLoaded();
	char fileNameBuf[StringUtils::s_maxCharsPerLine];
	char * pathQualifier = readFromDataPack ? "\\" : ":\\";
	if (!strstr(a_tgaPath, pathQualifier))
	{
		// Strip out any leading slashes
		const char * filenameOnly = strstr(a_tgaPath, "\\");
		if (filenameOnly != NULL)
		{
			sprintf(fileNameBuf, "%s%s", m_texturePath, filenameOnly);
		} 
		else
		{
			sprintf(fileNameBuf, "%s%s", m_texturePath, a_tgaPath);
		}
	} 
	else // Already fully qualified
	{
		sprintf(fileNameBuf, "%s", a_tgaPath);
	}
	
	// Get the identifier for the new texture
	StringHash texHash(fileNameBuf);
	unsigned int texId = texHash.GetHash();
	TextureCategory::Enum loadedCat = IsTextureLoaded(texId);

	// If it already exists
	if (loadedCat != TextureCategory::None)
	{
		// Just returned the cached copy
		ManagedTexture * foundTex = NULL;
		m_textureMap[loadedCat].Get(texId, foundTex);
		return &foundTex->m_texture;
	}
	else if (ManagedTexture * newTex = m_texturePool[a_cat].Allocate(sizeof(ManagedTexture)))
	{
		// If the filter is not specified, use the default
		if (a_currentFilter == TextureFilter::Invalid)
		{
			a_currentFilter = m_filterMode;
		}

		// If loading from a datapack
		if (readFromDataPack)
		{
			// Insert the newly allocated texture
			if (DataPackEntry * packedTexture = m_dataPack->GetEntry(fileNameBuf))
			{
				if (newTex->m_texture.LoadFromMemory((void *)packedTexture->m_data, packedTexture->m_size, a_currentFilter == TextureFilter::Linear))
				{
					sprintf(newTex->m_path, "%s", fileNameBuf);
					m_textureMap[a_cat].Insert(texId, newTex);
					return &newTex->m_texture;
				}
				else
				{
					Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture load from pack failed for %s", fileNameBuf);
					return NULL;
				}
			}
			else
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture load from pack failed for %s", fileNameBuf);
				return NULL;
			}
		}
		else
		{
			// Insert the newly allocated texture
			if (newTex->m_texture.LoadFromFile(fileNameBuf, a_currentFilter == TextureFilter::Linear))
			{
				FileManager::Get().GetFileTimeStamp(fileNameBuf, newTex->m_timeStamp);
				sprintf(newTex->m_path, "%s", fileNameBuf);
				m_textureMap[a_cat].Insert(texId, newTex);
				return &newTex->m_texture;
			}
			else
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture load from disk failed for %s", fileNameBuf);
				return NULL;
			}
		}
	}
	else // Report the error
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture allocation failed for %s", fileNameBuf);
		return NULL;
	}
   return NULL;
}

TextureCategory::Enum TextureManager::IsTextureLoaded(unsigned int a_tgaPathHash)
{
	// Look through each category for the target texture
	for (unsigned int i = 0; i < TextureCategory::Count; ++i)
	{
		ManagedTexture * foundTex = NULL;
		if (m_textureMap[i].Get(a_tgaPathHash, foundTex))
		{
			return (TextureCategory::Enum)i;
		}
	}
	return TextureCategory::None;
}