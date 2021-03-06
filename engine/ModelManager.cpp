#include "DataPack.h"
#include "FileManager.h"
#include "Log.h"

#include "ModelManager.h"

template<> ModelManager * Singleton<ModelManager>::s_instance = nullptr;

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
	, m_dataPack(nullptr)
{
	m_modelPath[0] = '\0';
}

bool ModelManager::Startup(const char * a_modelPath, DataPack * a_dataPack)
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

	// This can be removed in release configuration, but its nice when viewing memory
	memset(m_modelPool.GetHead(), 0, m_modelPool.GetAllocationSizeBytes());

	// Cache off the model path for non qualified addressing of models
	strncpy(m_modelPath, a_modelPath, sizeof(char) * strlen(a_modelPath) + 1);

	if (a_dataPack != nullptr && a_dataPack->IsLoaded())
	{
		// Cache off the datapack path for loading models from pack
		m_dataPack = a_dataPack;

		// Populate a list of objects
		DataPack::EntryList objEntries;
		a_dataPack->GetAllEntries(".obj", objEntries);
		DataPack::EntryNode * curNode = objEntries.GetHead();

		// Load each model in the pack
		bool loadSuccess = true;
		while (curNode != nullptr)
		{
			loadSuccess &= GetModel(curNode->GetData()->m_path) != nullptr;
			curNode = curNode->GetNext();
		}

		// Clean up the list of objects
		a_dataPack->CleanupEntryList(objEntries);
	}

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
	else // Due for an update, scan all models and their materials
	{
		m_updateTimer = 0.0f;
		bool modelReloaded = false;

		// Each model in the pool gets tested
		ManagedModel * curModel = nullptr;
		auto modelMapIt = m_modelMap.GetIterator();
		while (m_modelMap.GetNext(modelMapIt, curModel) && curModel != nullptr)
		{
			FileManager::Timestamp curModelTimestamp;
			FileManager::Timestamp curMaterialTimestamp;
			char materialPath[StringUtils::s_maxCharsPerLine];
			sprintf(materialPath, "%s%s", m_modelPath, curModel->m_model.GetMaterialFileName());
			bool modelNeedsReload = FileManager::Get().GetFileTimeStamp(curModel->m_path, curModelTimestamp) && curModelTimestamp > curModel->m_modelTimeStamp;
			bool materialNeedsReload = FileManager::Get().GetFileTimeStamp(materialPath, curMaterialTimestamp) && curMaterialTimestamp > curModel->m_materialTimeStamp;
			if (modelNeedsReload || materialNeedsReload)
			{
				// Timestamp is new on either model or material, trigger a reload
				if (modelNeedsReload)
				{
					Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in model %s, reloading.", curModel->m_path);
					curModel->m_modelTimeStamp = curModelTimestamp;
				}
				else
				{
					Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in material %s, reloading.", materialPath);
					curModel->m_materialTimeStamp = curMaterialTimestamp;
				}

				ModelDataPool mdp(m_objectPool, m_loadingVertPool, m_loadingNormalPool, m_loadingUvPool, m_materialPool);
				if (!curModel->m_model.Unload())
				{
					Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Cannot unload model.");
				}
				modelReloaded = curModel->m_model.Load(curModel->m_path, mdp);

				m_loadingVertPool.Reset();
				m_loadingNormalPool.Reset();
				m_loadingUvPool.Reset();
			}
		}
		return modelReloaded;
	}
}

bool ModelManager::ReloadModelsWithTexture(Texture * a_texture)
{
	bool modelReloaded = false;

	// Each model in the pool gets tested
	ManagedModel * curModel = nullptr;
	auto modelIterator = m_modelMap.GetIterator();
	while (m_modelMap.GetNext(modelIterator, curModel) && curModel != nullptr)
	{
		bool modelNeedsReload = false;
		const int numObjects = curModel->m_model.GetNumObjects();
		for (int i = 0l; i < numObjects; ++i)
		{
			if (Material * curMat = curModel->m_model.GetObjectAtIndex(i)->GetMaterial())
			{
				if (curMat->GetDiffuseTexture() == a_texture ||
					curMat->GetNormalTexture() == a_texture ||
					curMat->GetSpecularTexture() == a_texture)
				{
					modelNeedsReload = true;
					break;
				}
			}
		}
		if (modelNeedsReload)
		{
			ModelDataPool mdp(m_objectPool, m_loadingVertPool, m_loadingNormalPool, m_loadingUvPool, m_materialPool);
			if (!curModel->m_model.Unload())
			{
				Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Cannot unload model.");
			}
			modelReloaded = curModel->m_model.Load(curModel->m_path, mdp);

			m_loadingVertPool.Reset();
			m_loadingNormalPool.Reset();
			m_loadingUvPool.Reset();
		}
	}
	return modelReloaded;
}

Model * ModelManager::GetModel(const char * a_modelPath)
{
	// Model paths are either fully qualified or relative to the config model dir
	bool readFromDataPack = m_dataPack != nullptr && m_dataPack->IsLoaded();
	char fileNameBuf[StringUtils::s_maxCharsPerLine];
	char * pathQualifier = readFromDataPack ? "\\" : ":\\";
	if (!strstr(a_modelPath, pathQualifier))
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
		ManagedModel * foundModel = nullptr;
		m_modelMap.Get(modelId, foundModel);
		return &foundModel->m_model;
	}
	else if (ManagedModel * newModel = m_modelPool.Allocate(sizeof(ManagedModel)))
	{
		// Insert the newly allocated model
		bool modelLoaded = false;
		ModelDataPool mdp(m_objectPool, m_loadingVertPool, m_loadingNormalPool, m_loadingUvPool, m_materialPool);
		if (readFromDataPack)
		{
			if (DataPackEntry * packedModel = m_dataPack->GetEntry(fileNameBuf))
			{
				if (newModel->m_model.Load(packedModel, mdp, m_dataPack))
				{
					modelLoaded = true;
					sprintf(newModel->m_path, "%s", fileNameBuf);
					m_modelMap.Insert(modelId, newModel);

					// Reset the temporary loading pools ready for the next load
					m_loadingVertPool.Reset();
					m_loadingNormalPool.Reset();
					m_loadingUvPool.Reset();

					// Return the pointer to the actual model
					return &newModel->m_model;
				}
			}
		}
		else if (newModel->m_model.Load(fileNameBuf, mdp))
		{
			FileManager & fileMan = FileManager::Get();
			modelLoaded = true;
			fileMan.GetFileTimeStamp(fileNameBuf, newModel->m_modelTimeStamp);

			// Also set the timestamp on the associated material. This is the only way to load materials so this is safe.
			char materialFileNameBuf[StringUtils::s_maxCharsPerLine];
			sprintf(materialFileNameBuf, "%s%s", m_modelPath, newModel->m_model.GetMaterialFileName());
			fileMan.GetFileTimeStamp(materialFileNameBuf, newModel->m_materialTimeStamp);
			
			sprintf(newModel->m_path, "%s", fileNameBuf);
			m_modelMap.Insert(modelId, newModel);

			// Reset the temporary loading pools ready for the next load
			m_loadingVertPool.Reset();
			m_loadingNormalPool.Reset();
			m_loadingUvPool.Reset();

			// Return the pointer to the actual model
			return &newModel->m_model;
		}

		// Clean up a bad model load
		if (!modelLoaded)
		{
			//delete newModel;
			m_modelPool.DeAllocate(sizeof(ManagedModel));
			Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Model load failed for %s", fileNameBuf);
			return nullptr;
		}
	}
	else // Report the error
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Model allocation failed for %s, fileNameBuf");
		return nullptr;
	}
   return nullptr;
}

bool ModelManager::IsModelLoaded(unsigned int a_modelPathHash)
{
	// Look through map for the target model
	ManagedModel * foundModel = nullptr;
	if (m_modelMap.Get(a_modelPathHash, foundModel))
	{
		return true;
	}
	
	return false;
}