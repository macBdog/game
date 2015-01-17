#include "FileManager.h"
#include "Log.h"

#include "ModelManager.h"

template<> ModelManager * Singleton<ModelManager>::s_instance = NULL;

const unsigned int ModelManager::s_modelPoolSize = 65536;			// 64k for managed model info
const unsigned int ModelManager::s_loadingVertPoolSize = 65536;		// 64k verts for temporary loading of model vertices
const unsigned int ModelManager::s_loadingNormalPoolSize = 65536;	// 64k normals temporary loading of model normals
const unsigned int ModelManager::s_loadingUvPoolSize = 65536;		// 64k uv coords temporary loading texture coords
const unsigned int ModelManager::s_objectPoolSize = 65536;			// 64k for objects
const unsigned int ModelManager::s_materialPoolSize = 65536;		// 64k for materials

const float ModelManager::s_updateFreq = 1.0f;

ModelManager::ModelManager(float a_updateFreq)
	: m_updateFreq(a_updateFreq)
	, m_updateTimer(0.0f)
{
	m_modelPath[0] = '\0';
}

bool ModelManager::Startup(const char * a_modelPath)
{
	// Reset update timer in case we have been shutdown the re started
	 m_updateTimer = 0;

	// Init model object and material memory pool
	m_modelPool.Init(s_modelPoolSize);
	m_objectPool.Init(s_objectPoolSize);
	m_materialPool.Init(s_materialPoolSize);

	// Init temporary loading pools
	m_loadingVertPool.Init(s_loadingVertPoolSize * sizeof(Vector));
	m_loadingNormalPool.Init(s_loadingNormalPoolSize * sizeof(Vector));
	m_loadingUvPool.Init(s_loadingUvPoolSize * sizeof(TexCoord));

	// This can be removed in all but DEBUG configuration, but its nice when viewing memory
	memset(m_modelPool.GetHead(), 0, m_modelPool.GetAllocationSizeBytes());

	// Cache off the model path for non qualified addressing of models
	strncpy(m_modelPath, a_modelPath, sizeof(char) * strlen(a_modelPath) + 1);

	return true;
}

bool ModelManager::Shutdown()
{
	// Cleanup memory
	m_modelPool.Done();
	m_objectPool.Done();
	m_materialPool.Done();
	m_loadingVertPool.Done();
	m_loadingNormalPool.Done();
	m_loadingUvPool.Done();

	return true;
}

bool ModelManager::Update(float a_dt)
{
#ifdef _RELEASE
	return true;
#endif

	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
		return false;
	}
	else // Due for an update, scan all models and their materials
	{
		m_updateTimer = 0.0f;
		bool modelReloaded = false;

		// Each model in the pool gets tested
		ManagedModel * curModel = NULL;
		while ( m_modelMap.GetNext(curModel) && curModel != NULL)
		{
			FileManager::Timestamp curModelTimestamp;
			FileManager::Timestamp curMaterialTimestamp;
			char materialPath[StringUtils::s_maxCharsPerLine];
			sprintf(materialPath, "%s%s", m_modelPath, curModel->m_model.GetMaterialFileName());
			bool modelNeedsReload = FileManager::Get().GetFileTimeStamp(curModel->m_path, curModelTimestamp) && curModelTimestamp > curModel->m_timeStamp;
			bool materialNeedsReload = FileManager::Get().GetFileTimeStamp(materialPath, curMaterialTimestamp) && curMaterialTimestamp > curModel->m_timeStamp;
			if (modelNeedsReload || materialNeedsReload)
			{
				// Timestamp is new on either model or material, trigger a reload
				if (modelNeedsReload)
				{
					Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in model %s, reloading.", curModel->m_path);
					curModel->m_timeStamp = curModelTimestamp;
				}
				else
				{
					Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in material %s, reloading.", materialPath);
					curModel->m_timeStamp = curMaterialTimestamp;
				}

				ModelDataPool mdp(m_objectPool, m_loadingVertPool, m_loadingNormalPool, m_loadingUvPool, m_materialPool);
				modelReloaded = curModel->m_model.Load(curModel->m_path, mdp);

				m_loadingVertPool.Reset();
				m_loadingNormalPool.Reset();
				m_loadingUvPool.Reset();
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
		ModelDataPool mdp(m_objectPool, m_loadingVertPool, m_loadingNormalPool, m_loadingUvPool, m_materialPool);
		if (newModel->m_model.Load(fileNameBuf, mdp))
		{
			FileManager::Get().GetFileTimeStamp(fileNameBuf, newModel->m_timeStamp);
			sprintf(newModel->m_path, "%s", fileNameBuf);
			m_modelMap.Insert(modelId, newModel);

			// Reset the temporary loading pools ready for the next load
			m_loadingVertPool.Reset();
			m_loadingNormalPool.Reset();
			m_loadingUvPool.Reset();

			// Return the pointer to the actual model
			return &newModel->m_model;
		}
		else
		{
			//delete newModel;
			m_modelPool.DeAllocate(sizeof(ManagedModel));
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Model load failed for %s", fileNameBuf);
			return NULL;
		}
	}
	else // Report the error
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Model allocation failed for %s, fileNameBuf");
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