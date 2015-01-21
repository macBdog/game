#ifndef _ENGINE_DATA_PACK_
#define _ENGINE_DATA_PACK_
#pragma once

#include "../core/LinearAllocator.h"
#include "../core/LinkedList.h"

#include "Singleton.h"
#include "StringUtils.h"

//\brief A DataPackEntry is an index into the datapack and each entry is stored contiguously at the start of the datapack
struct DataPackEntry
{
	DataPackEntry()
		: m_size(0)
		, m_data(NULL)
		, m_readOffset(-1)
	{
		m_path[0] = '\0';
	}

	///\brief Alias of ifstream functions so callers can template datapacks or files
	inline bool is_open() { m_readOffset = 0; return true; }
	inline bool good() { return m_size > 0 && m_readOffset >= (int)m_size && m_data != NULL && m_path[0] != '\0'; }
	inline bool getline(char * a_buffer_OUT, int a_numCharsMax) { /* TODO */ return true; }
	inline bool close() { m_readOffset = -1; return true; }

	size_t m_size;									///< How big the entry in the pack is
	char * m_data;									///< Pointer to the resource data 
	int m_readOffset;								///< How far through reading the file we are
	char m_path[StringUtils::s_maxCharsPerLine];	///< The disk path that the game refers to the resource by
};

//\brief A data pack is an archive of all game data for a game designed to be included with a release build
class DataPack : public Singleton<DataPack>
{
public:

	//\ No work done in the constructor, the idea is the datapack object is created then pumped full of data and written to disk
	DataPack() {}
	DataPack(const char * a_pathToLoad) { Load(a_pathToLoad); }
	~DataPack() { Unload(); }
	
	//\brief Read a pre built data pack from disk and parse it's header so resources can be read from it
	//\param a_path the path to the data pack to read
	//\return true if the object's internal structures were filled with the pack from disk
	bool Load(const char * a_path);

	//\brief Clear all data in the pack and free all memory
	void Unload();

	//\brief Add a disk resource to the datapack that will be recorded in the descriptor and dumped out with the data
	//\param a_path the path to the resource to read and add to the data pack's body and description
	//\return true if the file was found, read and added
	bool AddFile(const char * a_path);

	//\brief Add every file from a folder to the datapack
	//\param a_path to the folder to scan and read each file from
	//\param a_fileExtensions a comma delimitted list of extensions for all the file types to read out of the target folder
	//\return true if at least one file was found and added to the datapack
	bool AddFolder(const char * a_path, const char * a_fileExtensions);

	//\brief Write a copy of the datapack out to disk to be read in later
	//\param a_path Where on the disk relative to the working dir to write the datapack to
	bool Serialize(const char * a_path) const;

	//\brief Status accessors for the user of the datapack
	inline bool IsLoaded() const { return m_manifest.GetLength() > 0; }

	//\brief Extrct an entry from the manifest, usually for reading the resource data from
	//\return NULL if not found
	DataPackEntry * GetEntry(const char * a_path);

private:

	typedef LinkedListNode<DataPackEntry> EntryNode;	///< Shorthand for a node pointing to a datapack entry
	typedef LinkedList<DataPackEntry> EntryList;		///< Shorthand for a list of datapack entries maintained by the datapack

	static const char * s_tempFilePath;					///< When compiling a datapack, the working set minus the manifest is stored on disk instead of memory

	EntryList m_manifest;								///< The list of all entries
	LinearAllocator<char> m_resourceData;				///< When a datapack is loaded, the resource data lives here
};

#endif // _ENGINE_DATA_PACK_
