#ifndef _ENGINE_TEXTURE_MANAGER_H_
#define _ENGINE_TEXTURE_H_

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
	
	//\brief Get or load a TGA file into texture memory
	//\param a_tgaPath cstring to identify the texture by
	//\return texture ID of the identified texture
	int GetTexture(const char *a_tgaPath, eTextureCategory a_cat);

	//\brief Functions to check if a texture has already been loaded
	//\param a_tgaPathHash is the identified for the texture
	//\return -1 if the texture is already loaded otherwise return the texture ID
	int IsTextureLoaded(unsigned int a_tgaPathHash);
	inline int IsTextureLoaded(const char *a_tgaPath) { return IsTextureLoaded(StringHash(a_tgaPath).GetHash()); }

	//\brief Reload a single texture without changing the IDs
	//\param a_cat the category the texture is found in. If not supplied, an exhaustive search is performed
	//\return true in the texture was found and reloaded successfully
	bool ReloadTexture(unsigned int a_tgaPathHash, eTextureCategory a_cat = eCategoryNone);
	inline bool ReloadTexture(const char *a_tgaPath, eTextureCategory a_cat = eCategoryNone) { return ReloadTexture(StringHash(a_tgaPath).GetHash(), a_cat); }

	//\brief Wholesale reload of single or multiple categories
	bool ReloadTextureCategory(eTextureCategory a_cat);
	bool ReloadAllTextureCategories();
	
private:

	//\brief Handle to a texture's address in memory and it's identifier
	struct TextureHandle
	{
		unsigned int m_texturePathHash;
	};

	//\brief Memory pool for each texture category
	LinearAllocator m_texturePool[eCategoryCount];
};

#endif /* _ENGINE_TEXTURE_MANAGER_H_ */