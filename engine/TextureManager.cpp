#include "FileManager.h"
#include "Log.h"

#include "TextureManager.h"
#include "ModelManager.h"

template<> TextureManager * Singleton<TextureManager>::s_instance = nullptr;

const unsigned int TextureManager::s_texurePoolSize[TextureManager::s_numPools] =
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
	, m_dataPack(nullptr)
	, m_filterMode(TextureFilter::Invalid)
{
}

bool TextureManager::Startup(std::string_view a_texturePath, bool a_useLinearTextureFilter)
{
	m_texturePath = a_texturePath;
	return Init(a_useLinearTextureFilter);
}

bool TextureManager::Startup(std::string_view a_texturePath, DataPack * a_dataPack, bool a_useLinearTextureFilter)
{
	m_dataPack = a_dataPack;
	m_texturePath = a_texturePath;
	return Init(a_useLinearTextureFilter);
}

bool TextureManager::Init(bool a_useLinearTextureFilter)
{
	// Reset update timer in case we have been shutdown the re started
	m_updateTimer = 0;

	// Init a pool of memory for each category
	for (unsigned int i = 0; i < static_cast<unsigned int>(TextureCategory::Count); ++i)
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
	for (unsigned int i = 0; i < static_cast<unsigned int>(TextureCategory::Count); ++i)
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
		for (unsigned int i = 0; i < static_cast<unsigned int>(TextureCategory::Count); ++i)
		{
			// Each texture in the category gets tested
			ManagedTexture * curTex = nullptr;
			auto textureIterator = m_textureMap[i].GetIterator();
			while (m_textureMap[i].GetNext(textureIterator, curTex) && curTex != nullptr)
			{
				FileManager::Timestamp curTimeStamp;
				if (FileManager::Get().GetFileTimeStamp(curTex->m_path, curTimeStamp))
				{
					if (curTimeStamp > curTex->m_timeStamp)
					{
						Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in texture %s, reloading.", curTex->m_path.c_str());
						textureReloaded = curTex->m_texture.LoadTGAFromFile(curTex->m_path.c_str());
						curTex->m_timeStamp = curTimeStamp;

						// Check any models that use this texture and reload them
						ModelManager::Get().ReloadModelsWithTexture(&curTex->m_texture);
					}
				}
			}
		}
		return textureReloaded;
	}
}

Texture * TextureManager::GetTexture(std::string_view a_tgaPath, TextureCategory a_cat, TextureFilter a_currentFilter)
{
	// Texture paths are either fully qualified or relative to the config texture dir
	const int tCat = static_cast<int>(a_cat);
	bool readFromDataPack = m_dataPack != nullptr && m_dataPack->IsLoaded();
	std::string fileName;
	if (!readFromDataPack && !StringUtils::IsAbsolutePath(a_tgaPath))
	{
		// Strip out any leading slashes
		size_t slashPos = a_tgaPath.find('/');
		if (slashPos == std::string_view::npos) slashPos = a_tgaPath.find('\\');
		if (slashPos != std::string_view::npos)
		{
			fileName = m_texturePath + std::string(a_tgaPath.substr(slashPos));
		}
		else
		{
			fileName = m_texturePath + std::string(a_tgaPath);
		}
	}
	else // Already fully qualified
	{
		fileName = a_tgaPath;
	}

	// Get the identifier for the new texture
	StringHash texHash(fileName);
	unsigned int texId = texHash.GetHash();
	TextureCategory loadedCat = IsTextureLoaded(texId);

	// If it already exists
	if (loadedCat != TextureCategory::None)
	{
		// Just returned the cached copy
		ManagedTexture * foundTex = nullptr;
		m_textureMap[static_cast<int>(loadedCat)].Get(texId, foundTex);
		return &foundTex->m_texture;
	}
	else if (ManagedTexture * newTex = m_texturePool[tCat].Allocate(sizeof(ManagedTexture)))
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
			if (DataPackEntry * packedTexture = m_dataPack->GetEntry(fileName))
			{
				if (newTex->m_texture.LoadTGAFromMemory((void *)packedTexture->m_data, packedTexture->m_size, a_currentFilter == TextureFilter::Linear))
				{
					newTex->m_path = fileName;
					m_textureMap[tCat].Insert(texId, newTex);
					return &newTex->m_texture;
				}
				else
				{
					Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture load from pack failed for %s", fileName.c_str());
					return nullptr;
				}
			}
			else
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture load from pack failed for %s", fileName.c_str());
				return nullptr;
			}
		}
		else
		{
			// Insert the newly allocated texture
			if (newTex->m_texture.LoadTGAFromFile(fileName.c_str(), a_currentFilter == TextureFilter::Linear))
			{
				FileManager::Get().GetFileTimeStamp(fileName, newTex->m_timeStamp);
				newTex->m_path = fileName;
				m_textureMap[tCat].Insert(texId, newTex);
				return &newTex->m_texture;
			}
			else
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture load from disk failed for %s", fileName.c_str());
				return nullptr;
			}
		}
	}
	else // Report the error
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Texture allocation failed for %s", fileName.c_str());
		return nullptr;
	}
	return nullptr;
}

TextureCategory TextureManager::IsTextureLoaded(unsigned int a_tgaPathHash)
{
	// Look through each category for the target texture
	for (unsigned int i = 0; i < static_cast<unsigned int>(TextureCategory::Count); ++i)
	{
		ManagedTexture * foundTex = nullptr;
		if (m_textureMap[i].Get(a_tgaPathHash, foundTex))
		{
			return (TextureCategory)i;
		}
	}
	return TextureCategory::None;
}