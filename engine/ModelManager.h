#ifndef _ENGINE_MODEL_MANAGER_H_
#define _ENGINE_MODEL_MANAGER_H_

#include "../core/HashMap.h"
#include "../core/LinearAllocator.h"

#include "FileManager.h"
#include "Singleton.h"
#include "StringHash.h"
#include "StringUtils.h"
#include "Model.h"

//\brief ModelManager keeps track of all models in the game and the memory
//		 required for them. It handles hot loading of all model resources
//		 and will only load a unique model by path once.
class ModelManager : public Singleton<ModelManager>
{
public:
	
	//\brief Ctor calls through to startup
	ModelManager(float a_updateFreq = s_updateFreq);
	~ModelManager() { Shutdown(); }

	//brief Initialise memory pools on startup, cleanup models on shutdown
	bool Startup(const char * a_modelPath);
	bool Shutdown();

	//\brief Update will poll for model changes and reload any models that have a newer version than on disk
	//\return true if a model was old and needed to be reloaded
	bool Update(float a_dt);

	//\brief Get or load a TGA file into model memory
	//\param a_tgaPath cstring to identify the model by
	//\return model ID of the identified model
	Model * GetModel(const char *a_modelPath);

	//\brief Functions to check if a model has already been loaded
	//\param a_tgaPathHash is the identified for the model
	//\return -1 the category that the model is loaded into, none if not loaded
	bool IsModelLoaded(unsigned int a_tgaPathHash);
	inline bool IsModelLoaded(const char *a_tgaPath) { return IsModelLoaded(StringHash(a_tgaPath).GetHash()); }

	//\brief Reload a single model without changing the IDs
	//\param a_cat the category the model is found in. If not supplied, an exhaustive search is performed
	//\return true in the model was found and reloaded successfully
	bool ReloadModel(unsigned int a_modelPathHash);
	inline bool Reloadmodel(const char *a_modelPath) { return ReloadModel(StringHash(a_modelPath).GetHash()); }

	//\brief Wholesale reload of models
	bool ReloadAllModels();

	//\brief Get the fully qualified model path
	//\return A pointer to a c string containing the model path
	inline const char * GetModelPath() { return m_modelPath; }

	//\brief While a model is loading, it stores data in these memory pools temporarily
	static const unsigned int s_loadingVertPoolSize;			///< The maximum number of verts that could be loaded from model file
	static const unsigned int s_loadingNormalPoolSize;			///< The maximum number of normals that could be loaded from a model file
	static const unsigned int s_loadingUvPoolSize;				///< The maximum number of tex coords that could be loaded from a model file

	static const unsigned int s_objectPoolSize;					///< The maximum number and size of objects that could be loaded from a model file
	static const unsigned int s_materialPoolSize;				///< The maximum number and size of materials that could be loaded from a model file
	
private:

	static const unsigned int s_modelPoolSize;					///< How much memory is assigned for each category

	static const float s_updateFreq;							///< How often the model manager should check for updates

	//\brief A managed model contains the actual model data as well as extra information
	//		 that enables it to be version checked and hot reloaded 
	struct ManagedModel
	{
		Model  m_model;											///< The actual model
		FileManager::Timestamp m_timeStamp;						///< Datestamp for checking a newer version
		char m_path[StringUtils::s_maxCharsPerLine];			///< The full path for reloading
	};

	typedef HashMap<unsigned int, ManagedModel *> ModelMap;

	LinearAllocator<ManagedModel> m_modelPool;					///< Memory pool for each model category
	
	LinearAllocator<Vector> m_loadingVertPool;					///< Temporary pool of vectors used when reading a model file
	LinearAllocator<Vector> m_loadingNormalPool;				///< Temporary pool of normals used when reading a model file
	LinearAllocator<TexCoord> m_loadingUvPool;					///< Temporary pool of tex corrds used when reading a model file

	LinearAllocator<Object> m_objectPool;						///< Pool for object storage
	LinearAllocator<Material> m_materialPool;					///< Pool for material storage

	ModelMap m_modelMap;										///< List of models for each category
	char m_modelPath[StringUtils::s_maxCharsPerLine];			///< Cache off model path 
	float m_updateFreq;											///< How often the model manager should check for changes
	float m_updateTimer;										///< If we are due for a scan and update of models
};

#endif /* _ENGINE_MODEL_MANAGER_H_ */