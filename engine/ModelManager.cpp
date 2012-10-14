#include "FileManager.h"
#include "Log.h"

#include "ModelManager.h"

template<> ModelManager * Singleton<ModelManager>::s_instance = NULL;

const unsigned int ModelManager::s_modelPoolSize = 65536;			// 64 meg for all models
const unsigned int ModelManager::s_loadingVertPoolSize = 32768;		// 32 meg for temporary loading of model vertices
const unsigned int ModelManager::s_loadingNormalPoolSize = 32768;	// 32 meg for temporary loading of model normals
const unsigned int ModelManager::s_loadingUvPoolSize = 32768;		// 32 meg for temporary loading texture coords

const float ModelManager::s_updateFreq = 1.0f;

ModelManager::ModelManager(float a_updateFreq)
	: m_updateFreq(a_updateFreq)
	, m_updateTimer(0.0f)
{
}

bool ModelManager::Startup(const char * a_modelPath)
{
	// Reset update timer in case we have been shutdown the re started
	 m_updateTimer = 0;

	// Init model memory pool
	m_modelPool.Init(s_modelPoolSize);

	// Init temporary loading pools
	m_loadingVertPool.Init(s_loadingVertPoolSize);
	m_loadingNormalPool.Init(s_loadingNormalPoolSize);
	m_loadingUvPool.Init(s_loadingUvPoolSize);

	// This can be removed in all but DEBUG configuration, but its nice when viewing memory
	memset(m_modelPool.GetHead(), 0, m_modelPool.GetAllocationSizeBytes());

	// Cache off the model path for non qualified addressing of models
	memset(&m_modelPath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_modelPath, a_modelPath, strlen(a_modelPath));

	return true;
}

bool ModelManager::Shutdown()
{
	// Cleanup memory
	m_modelPool.Done();

	return true;
}

bool ModelManager::Update(float a_dt)
{
	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
		return false;
	}
	else // Due for an update, scan all models
	{
		m_updateTimer = 0.0f;
		bool modelReloaded = false;

		// Each model in the pool gets tested
		ManagedModel * curModel = NULL;
		while ( m_modelMap.GetNext(curModel) && curModel != NULL)
		{
			FileManager::Timestamp curTimestamp;
			if (FileManager::Get().GetFileTimeStamp(curModel->m_path, curTimestamp))
			{
				// Timestamp is new, trigger a reload
				if (curTimestamp > curModel->m_timeStamp)
				{
					Log::Get().Write(Log::LL_INFO, Log::LC_ENGINE, "Change detected in model %s, reloading.", curModel->m_path);
					modelReloaded = curModel->m_model.Load(curModel->m_path, m_loadingVertPool, m_loadingNormalPool, m_loadingUvPool);
					curModel->m_timeStamp = curTimestamp;

					m_loadingVertPool.Reset();
					m_loadingNormalPool.Reset();
					m_loadingUvPool.Reset();
				}
			}
		}
		
		return modelReloaded;
	}
}

Model * ModelManager::GetModel(const char * a_modelPath)
{
	// Model paths are either fully qualified or relative to the config model dir
	char fileNameBuf[StringUtils::s_maxCharsPerLine];
	if (!strstr(a_modelPath, ":\\"))
	{
		sprintf(fileNameBuf, "%s%s", m_modelPath, a_modelPath);
	} 
	else // Already fully qualified
	{
		sprintf(fileNameBuf, "%s", a_modelPath);
	}
	
	// Get the identifier for the new model
	StringHash modelHash(fileNameBuf);
	unsigned int modelId = modelHash.GetHash();

	// If it already exists
	if (IsModelLoaded(modelId))
	{
		// Just returned the cached copy
		ManagedModel * foundModel = NULL;
		m_modelMap.Get(modelId, foundModel);
		return &foundModel->m_model;
	}
	else if (ManagedModel * newModel = m_modelPool.Allocate(sizeof(ManagedModel)))
	{
		// Insert the newly allocated model
		if (newModel->m_model.Load(fileNameBuf, m_loadingVertPool, m_loadingNormalPool, m_loadingUvPool))
		{
			FileManager::Get().GetFileTimeStamp(fileNameBuf, newModel->m_timeStamp);
			sprintf(newModel->m_path, "%s", fileNameBuf);
			m_modelMap.Insert(modelId, newModel);
			return &newModel->m_model;
		}
		else
		{
			delete newModel;
			Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Model load failed for %s", fileNameBuf);
			return NULL;
		}

		// Reset the temporary loading pools ready for the next load
		m_loadingVertPool.Reset();
		m_loadingNormalPool.Reset();
		m_loadingUvPool.Reset();
	}
	else // Report the error
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Model allocation failed for %s, fileNameBuf");
		return NULL;
	}
   return NULL;
}

bool ModelManager::IsModelLoaded(unsigned int a_modelPathHash)
{
	// Look through map for the target model
	ManagedModel * foundModel = NULL;
	if (m_modelMap.Get(a_modelPathHash, foundModel))
	{
		return true;
	}
	
	return false;
}