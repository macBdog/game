#ifndef _ENGINE_TEXTURE_MANAGER_H_
#define _ENGINE_TEXTURE_MANAGER_H_

#include "../core/HashMap.h"
#include "../core/LinearAllocator.h"

#include "Singleton.h"
#include "StringHash.h"
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
	
	TextureManager() { Startup(); }
	~TextureManager() { Shutdown(); }

	//brief Initialise memory pools on startup, cleanup textures on shutdown
	bool Startup();
	bool Shutdown();

	//\brief Get or load a TGA file into texture memory
	//\param a_tgaPath cstring to identify the texture by
	//\return texture ID of the identified texture
	Texture * GetTexture(const char *a_tgaPath, eTextureCategory a_cat);

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
	
private:

	static const unsigned int s_texurePoolSize[eCategoryCount];		// How much memory is assigned for each category

	LinearAllocator<Texture> m_texturePool[eCategoryCount];			// Memory pool for each texture category
	HashMap<unsigned int, Texture *> m_textureMap[eCategoryCount];	// List of textures for each category
};

#endif /* _ENGINE_TEXTURE_MANAGER_H_ */