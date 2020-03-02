#ifndef _ENGINE_DATA_PACK_
#define _ENGINE_DATA_PACK_
#pragma once

#include <ios>

#include "../core/LinearAllocator.h"
#include "../core/LinkedList.h"

#include "Singleton.h"
#include "StringUtils.h"

//\brief A DataPackEntry is an index into the datapack and each entry is stored contiguously at the start of the datapack
struct DataPackEntry
{
	DataPackEntry()
		: m_size(0)
		, m_data(nullptr)
		, m_readOffset(-1)
	{
		m_path[0] = '\0';
	}

	///\brief Alias of ifstream functions so callers can template datapacks or files
	inline bool is_open()
	{
		if (m_readOffset < 0)
		{
			m_readOffset = 0;
			return true;
		}
		return false;
	}
	inline bool good() 
	{ 
		return	m_size > 0 && 
				m_readOffset < (int)m_size && 
				m_data != nullptr && 
				m_path[0] != '\0'; 
	}
	inline bool getline(char * a_buffer_OUT, int a_numCharsMax) 
	{
		char * inBuff = m_data + m_readOffset;
		char * outBuff = a_buffer_OUT;
		bool foundEnd = false;
		int numBytesRead = 0;
		while (!foundEnd && numBytesRead < a_numCharsMax)
		{
			foundEnd = inBuff == '\0' || *inBuff == '\n' || *inBuff == '\r';
			if (!foundEnd)
			{
				*outBuff++ = *inBuff++;
				numBytesRead++;
			}
		}
		int termCharLength = 1;
		if (*inBuff == '\r' && *(inBuff + 1) == '\n')
		{
			termCharLength = 2;
		}
		m_readOffset += numBytesRead + termCharLength;
		*outBuff = '\0';
		return true; 
	}
	inline bool close() { m_readOffset = -1; return true; }
	inline unsigned int tellg() const { return m_readOffset; }
	inline void clear() { close(); }
	inline void seekg(unsigned int a_offset, std::streamoff a_startPos) 
	{ 
		switch (a_startPos)
		{
			case std::ios::beg:
			{
				if (a_offset < m_size)
				{
					m_readOffset = a_offset;
				}
				break;
			}
			case std::ios::cur:
			{
				if (a_offset + m_readOffset < m_size)
				{
					m_readOffset += a_offset;
				}
				break;
			}
			case std::ios::end:
			{
				if (m_size - a_offset >= 0)
				{
					m_readOffset = m_size - a_offset;
				}
				break;
			}
			default: break;
		}
	}

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
	DataPack() : m_loaded(false) { m_relativePath[0] = '\0'; }
	DataPack(const char * a_pathToLoad) { m_relativePath[0] = '\0'; Load(a_pathToLoad); }
	~DataPack() { Unload(); }
	
	typedef LinkedListNode<DataPackEntry> EntryNode;		///< Shorthand for a node pointing to a datapack entry
	typedef LinkedList<DataPackEntry> EntryList;			///< Shorthand for a list of datapack entries maintained by the datapack

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
	inline bool IsLoaded() const { return m_loaded; }
	inline bool HasFilesToWrite() const { return m_manifest.GetLength() > 0; }

	//\brief Extract an entry from the manifest, usually for reading the resource data from
	//\return nullptr if not found
	DataPackEntry * GetEntry(const char * a_path) const;
	void GetAllEntries(const char * a_fileExtensions, EntryList & a_entries_OUT) const;
	void CleanupEntryList(EntryList & a_entries_OUT) const;

	//\brief Check a file exists in the pack already, just like GetEntry save for return type
	bool HasFile(const char * a_path) const;

	//\brief The relative path is used for packing files without absolute path info and also for
	// retrieving files from an absolute path when only the relative is requested. The pack will not work correctly without this
	//\param a_relativePath where the executable is being run from
	void SetRelativePath(const char * a_relativePath);

	static const char * s_defaultDataPackPath;				///< Name of the pack to write and read if not specified

private:

	//\brief Add files of one extension to the data pack, recursively through any sub folders
	bool AddAllFilesInFolder(const char * a_path, const char * a_fileExtension);

	//\brief Operations used in filling lists of files for game systems
	void AddEntryToExternalList(const char * a_fileExtension, EntryList & a_entries_OUT) const;
	void AddEntriesToExternalList(const char * a_fileExtension, EntryList & a_entries_OUT) const;

	EntryList m_manifest;									///< The list of all entries
	LinearAllocator<char> m_resourceData;					///< When a datapack is loaded, the resource data lives here
	char m_relativePath[StringUtils::s_maxCharsPerLine];	///< Relative path when the datapack was made so the pack is always relative
	bool m_loaded;											///< If load has been called on the data pack
};

#endif // _ENGINE_DATA_PACK_
