#pragma once

#include <ios>
#include <string>
#include <string_view>

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
			foundEnd = *inBuff == '\0' || *inBuff == '\n' || *inBuff == '\r';
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

	///\brief Get the path as a string_view
	inline std::string_view GetPath() const { return m_path; }

	size_t m_size;									///< How big the entry in the pack is
	char * m_data;									///< Pointer to the resource data
	int m_readOffset;								///< How far through reading the file we are
	char m_path[StringUtils::s_maxCharsPerLine];	///< The disk path - fixed size for binary serialization
};

//\brief A data pack is an archive of all game data for a game designed to be included with a release build
class DataPack : public Singleton<DataPack>
{
public:

	//\ No work done in the constructor, the idea is the datapack object is created then pumped full of data and written to disk
	DataPack() : m_loaded(false) { }
	DataPack(std::string_view a_pathToLoad) { Load(a_pathToLoad); }
	~DataPack() { Unload(); }

	typedef LinkedListNode<DataPackEntry> EntryNode;		///< Shorthand for a node pointing to a datapack entry
	typedef LinkedList<DataPackEntry> EntryList;			///< Shorthand for a list of datapack entries maintained by the datapack

	//\brief Read a pre built data pack from disk and parse it's header so resources can be read from it
	bool Load(std::string_view a_path);

	//\brief Clear all data in the pack and free all memory
	void Unload();

	//\brief Add a disk resource to the datapack
	bool AddFile(std::string_view a_path);

	//\brief Add every file from a folder to the datapack
	bool AddFolder(std::string_view a_path, std::string_view a_fileExtensions);

	//\brief Write a copy of the datapack out to disk
	bool Serialize(std::string_view a_path) const;

	//\brief Status accessors for the user of the datapack
	inline bool IsLoaded() const { return m_loaded; }
	inline bool HasFilesToWrite() const { return m_manifest.GetLength() > 0; }

	//\brief Extract an entry from the manifest
	DataPackEntry * GetEntry(std::string_view a_path) const;
	void GetAllEntries(std::string_view a_fileExtensions, EntryList & a_entries_OUT) const;
	void CleanupEntryList(EntryList & a_entries_OUT) const;

	//\brief Check a file exists in the pack already
	bool HasFile(std::string_view a_path) const;

	//\brief Set the relative path for packing/retrieving files
	void SetRelativePath(std::string_view a_relativePath);

	static const char * s_defaultDataPackPath;				///< Name of the pack to write and read if not specified

private:

	//\brief Add files of one extension to the data pack recursively
	bool AddAllFilesInFolder(std::string_view a_path, std::string_view a_fileExtension);

	//\brief Operations used in filling lists of files for game systems
	void AddEntryToExternalList(std::string_view a_filePath, EntryList & a_entries_OUT) const;
	void AddEntriesToExternalList(std::string_view a_fileExtension, EntryList & a_entries_OUT) const;

	EntryList m_manifest{};									///< The list of all entries
	LinearAllocator<char> m_resourceData{};					///< When a datapack is loaded, the resource data lives here
	std::string m_relativePath;								///< Relative path when the datapack was made
	bool m_loaded{ false };									///< If load has been called on the data pack
};
