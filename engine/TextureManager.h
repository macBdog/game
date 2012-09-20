#ifndef _ENGINE_TEXTURE_MANAGER_H_
#define _ENGINE_TEXTURE_MANAGER_H_

#include "../core/HashMap.h"
#include "../core/LinearAllocator.h"

#include "FileManager.h"
#include "Singleton.h"
#include "StringHash.h"
#include "StringUtils.h"
#include "Texture.h"

//\brief TextureManager keeps track of all textures in the game and the memory
//		 required for them. It handles hot loading of all texture resources
//		 and will only load a unique texture by patch once.
class TextureManager : public Singleton<TextureManager>
{
public:

	//\brief Textures are stored in reference to engine systems
	enum eTextureCategory
	{
		eCategoryNone = -1,
		eCategoryDebug,
		eCategoryGui,
		eCategoryModel,
		eCategoryParticle,
		eCategoryWorld,

		eCategoryCount
	};
	
	//\brief A abbreviation of a a set of flags to use when generating textures
	enum eTextureFilter
	{
		eTextureFilterInvalid = -1,		///< Means we don't care what filter is applied
		eTextureFilterLinear,			///< Normal stretchy blurry mode
		eTextureFilterNearest,			///< Pixelised nearest neighbour

		eTextureFilterCount,
	};

	//\brief Ctor calls through to startup
	TextureManager(float a_updateFreq = s_updateFreq);
	~TextureManager() { Shutdown(); }

	//brief Initialise memory pools on startup, cleanup textures on shutdown
	bool Startup(const char * a_texturePath, bool a_useLinearTextureFilter = true);
	bool Shutdown();

	//\brief Update will poll for texture changes and reload any textures that have a newer version than on disk
	//\return true if a texture was old and needed to be reloaded
	bool Update(float a_dt);

	//\brief Get or load a TGA file into texture memory
	//\param a_tgaPath cstring to identify the texture by
	//\return texture ID of the identified texture
	Texture * GetTexture(const char *a_tgaPath, eTextureCategory a_cat, eTextureFilter a_currentFilter = eTextureFilterInvalid);

	//\brief Functions to check if a texture has already been loaded
	//\param a_tgaPathHash is the identified for the texture
	//\return -1 the category that the texture is loaded into, none if not loaded
	eTextureCategory IsTextureLoaded(unsigned int a_tgaPathHash);
	inline eTextureCategory IsTextureLoaded(const char *a_tgaPath) { return IsTextureLoaded(StringHash(a_tgaPath).GetHash()); }

	//\brief Reload a single texture without changing the IDs
	//\param a_cat the category the texture is found in. If not supplied, an exhaustive search is performed
	//\return true in the texture was found and reloaded successfully
	bool ReloadTexture(unsigned int a_tgaPathHash, eTextureCategory a_cat = eCategoryNone);
	inline bool ReloadTexture(const char *a_tgaPath, eTextureCategory a_cat = eCategoryNone) { return ReloadTexture(StringHash(a_tgaPath).GetHash(), a_cat); }

	//\brief Wholesale reload of single or multiple categories
	bool ReloadTextureCategory(eTextureCategory a_cat);
	bool ReloadAllTextureCategories();

	//\brief Get the fully qualified texture path
	//\return A pointer to a c string containing the texture path
	inline const char * GetTexturePath() { return m_texturePath; }
	
private:

	//\brief A managed texture contains the actual texture data as well as extra information
	//		 that enables it to be version checked and hot reloaded 
	struct ManagedTexture
	{
		Texture  m_texture;											///< The actual texture
		FileManager::Timestamp m_timeStamp;							///< Datestamp for checking a newer version
		char m_path[StringUtils::s_maxCharsPerLine];				///< The full path for reloading
	};

	typedef HashMap<unsigned int, ManagedTexture *> TextureMap;		///< Alias for a hash map of managed textures

	static const unsigned int s_texurePoolSize[eCategoryCount];		///< How much memory is assigned for each category
	static const float s_updateFreq;								///< How often the texture manager should check for updates

	LinearAllocator<ManagedTexture> m_texturePool[eCategoryCount];	///< Memory pool for each texture category
	TextureMap m_textureMap[eCategoryCount];						///< List of textures for each category
	char m_texturePath[StringUtils::s_maxCharsPerLine];				///< Cache off texture path 
	float m_updateFreq;												///< How often the texture manager should check for changes
	float m_updateTimer;											///< If we are due for a scan and update of textures
	eTextureFilter m_filterMode;									///< Filtering rule to apply, can make exceptions on a per texture basis
};

#endif /* _ENGINE_TEXTURE_MANAGER_H_ */