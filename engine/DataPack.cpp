#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "zlib.h"

#include "FileManager.h"
#include "Log.h"

#include "DataPack.h"

using namespace std;	//< For fstream operations

template<> DataPack * Singleton<DataPack>::s_instance = nullptr;

const char * DataPack::s_defaultDataPackPath = "datapack.dtz";

void DataPack::Unload()
{
	// Clean up any entries in the manifest
	EntryNode * next = m_manifest.GetHead();
	while (next != nullptr)
	{
		// Cache off next pointer
		EntryNode * cur = next;
		next = cur->GetNext();

		m_manifest.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	// Unload resource data
	m_resourceData.Done();

	m_loaded = false;
}

bool DataPack::Load(std::string_view a_path)
{
	if (IsLoaded())
	{
		Unload();
	}

	// The relative path needs to exist
	if (m_relativePath.empty())
	{
		assert(false);
		return false;
	}

	// Decompress and read file
	std::string pathStr(a_path);
	gzFile inputFile = gzopen(pathStr.c_str(), "rb");

	// Open the file and parse the datapack
	if (inputFile)
	{
		// First is the number of entries in the manifest and the resource data size
		int numEntries = 0;
		size_t packSize = 0;
		gzread(inputFile, (char *)&numEntries, sizeof(int));
		gzread(inputFile, (char *)&packSize, sizeof(size_t));
		m_resourceData.Init(packSize);

		// Read each entry
		for (int i = 0; i < numEntries; ++i)
		{
			DataPackEntry * newEntry = new DataPackEntry();
			gzread(inputFile, (char *)&newEntry->m_size, sizeof(size_t));
			gzread(inputFile, (char *)&newEntry->m_path, sizeof(char) * StringUtils::s_maxCharsPerLine);
			char * resourceHead = m_resourceData.Allocate(newEntry->m_size);
			newEntry->m_data = resourceHead;
			const int resourceSize = (int)newEntry->m_size;
			gzread(inputFile, resourceHead, resourceSize);
			resourceHead += resourceSize;

			// Add to manifest
			EntryNode * newNode = new EntryNode();
			newNode->SetData(newEntry);
			m_manifest.Insert(newNode);
		}
		gzclose(inputFile);
		m_loaded = true;
		return true;
	}
	return false;
}

bool DataPack::AddFile(std::string_view a_path)
{
	// The relative path needs to exist
	if (m_relativePath.empty())
	{
		assert(false);
		return false;
	}

	// First get the size of the file
	std::string pathStr(a_path);
	struct stat sizeResult;
	if (stat(pathStr.c_str(), &sizeResult) == 0)
	{
		const size_t sizeBytes = (size_t)sizeResult.st_size;

		// First check that the file isn't already in the pack
		if (!HasFile(a_path))
		{
			// Strip out the absolute part of the file path
			std::string relPath(a_path);
			size_t relPos = relPath.find(m_relativePath);
			if (relPos != std::string::npos)
			{
				relPath = relPath.substr(relPos + m_relativePath.size());
			}

			DataPackEntry * newEntry = new DataPackEntry();
			newEntry->m_size = sizeBytes;
			strncpy(newEntry->m_path, relPath.c_str(), StringUtils::s_maxCharsPerLine);

			EntryNode * newNode = new EntryNode();
			newNode->SetData(newEntry);

			m_manifest.Insert(newNode);
		}
		return true;
	}
	return false;
}

bool DataPack::AddFolder(std::string_view a_path, std::string_view a_fileExtensions)
{
	bool filesAdded = false;
	if (!a_fileExtensions.empty())
	{
		// Support a list of extension types
		std::string extensions(a_fileExtensions);
		size_t pos = 0;
		size_t comma;
		while ((comma = extensions.find(',', pos)) != std::string::npos)
		{
			filesAdded |= AddAllFilesInFolder(a_path, extensions.substr(pos, comma - pos));
			pos = comma + 1;
		}
		filesAdded |= AddAllFilesInFolder(a_path, extensions.substr(pos));
	}
	return filesAdded;
}

bool DataPack::AddAllFilesInFolder(std::string_view a_path, std::string_view a_fileExtension)
{
	// Scan all the path for files of the correct extension
	FileManager & fileMan = FileManager::Get();
	FileManager::FileList filesToPack;
	fileMan.FillFileList(a_path, filesToPack, a_fileExtension);

	// Add all files of correct path
	int numFilesAdded = 0;
	FileManager::FileListNode * curNode = filesToPack.GetHead();
	while(curNode != nullptr)
	{
		std::string fullPath = std::string(a_path) + curNode->GetData()->m_name;

		if (curNode->GetData()->m_isDir)
		{
			numFilesAdded += AddAllFilesInFolder(fullPath, a_fileExtension);
		}
		else
		{
			AddFile(fullPath);
			++numFilesAdded;
		}
		curNode = curNode->GetNext();
	}
	return numFilesAdded > 0;
}

bool DataPack::Serialize(std::string_view a_path) const
{
	// First compute the complete size of all resources in the pack
	size_t packSize = 0;
	EntryNode * cur = m_manifest.GetHead();
	while (cur != nullptr)
	{
		DataPackEntry * curEntry = cur->GetData();
		packSize += curEntry->m_size;
		cur = cur->GetNext();
	}

	const int numEntries = m_manifest.GetLength();
	std::string tempFilePath = std::string(a_path) + ".tmp";
	ofstream outputFile(tempFilePath, ios::out | ios::binary);
	if (outputFile.is_open())
	{
		// First write the number of entries in the manifest and the total resource size
		outputFile.write((char *)&numEntries, sizeof(int));
		outputFile.write((char *)&packSize, sizeof(size_t));

		// Write each file in the manifest
		EntryNode * cur = m_manifest.GetHead();
		while (cur != nullptr)
		{
			DataPackEntry * curEntry = cur->GetData();

			// First write the entry header so the reading function knows how far to read
			outputFile.write((char *)&curEntry->m_size, sizeof(size_t));
			outputFile.write((char *)&curEntry->m_path, sizeof(char) * StringUtils::s_maxCharsPerLine);

			// Now write the resource by opening and reading it from the disk
			std::string diskPath;

			// Special case for game.json as it lives outside the data folder
			if (std::string_view(curEntry->m_path).find("game.json") != std::string_view::npos)
			{
				diskPath = curEntry->m_path;
			}
			else
			{
				diskPath = m_relativePath + curEntry->m_path;
			}
			size_t writeByteCount = 0;
			ifstream resourceFile(diskPath, ifstream::in | ifstream::binary);
			if (resourceFile.is_open())
			{
				// Read and write the resource file in one pass
				char * resourceBuffer = (char *)malloc(sizeof(char) * curEntry->m_size);
				memset(resourceBuffer, 0, sizeof(char) * curEntry->m_size);
				resourceFile.read(resourceBuffer, curEntry->m_size);
				outputFile.write(resourceBuffer, curEntry->m_size);
				writeByteCount += curEntry->m_size;
				resourceFile.close();
				free(resourceBuffer);
			}
			else
			{
				Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot open file %s for writing to the datapack, this will corrupt the datapack.", diskPath.c_str());
			}
			cur = cur->GetNext();
		}
	}
	outputFile.close();

	// Now compress the file
	FILE * inputFile = fopen(tempFilePath.c_str(), "rb");
	gzFile outfile = gzopen(s_defaultDataPackPath, "wb");
	char readBuffer[128];
	int bytesRead = 0;
	while ((bytesRead = fread(readBuffer, 1, sizeof(readBuffer), inputFile)) > 0)
	{
		gzwrite(outfile, readBuffer, bytesRead);
	}
	fclose(inputFile);
	gzclose(outfile);

	return true;
}

DataPackEntry * DataPack::GetEntry(std::string_view a_path) const
{
	EntryNode * cur = m_manifest.GetHead();
	while (cur != nullptr)
	{
		DataPackEntry * curEntry = cur->GetData();
		if (std::string_view(curEntry->m_path) == a_path)
		{
			return curEntry;
		}
		cur = cur->GetNext();
	}
	return nullptr;
}

void DataPack::GetAllEntries(std::string_view a_fileExtensions, EntryList & a_entries_OUT) const
{
	// Support a list of extension types
	std::string extensions(a_fileExtensions);
	size_t pos = 0;
	size_t comma;
	while ((comma = extensions.find(',', pos)) != std::string::npos)
	{
		AddEntriesToExternalList(extensions.substr(pos, comma - pos), a_entries_OUT);
		pos = comma + 1;
	}
	AddEntriesToExternalList(extensions.substr(pos), a_entries_OUT);
}

void DataPack::CleanupEntryList(EntryList & a_entries_OUT) const
{
	// Delete the nodes, all data is owned by the datapack
	EntryNode * next = a_entries_OUT.GetHead();
	while (next != nullptr)
	{
		// Cache off next pointer
		EntryNode * cur = next;
		next = cur->GetNext();

		a_entries_OUT.Remove(cur);
		delete cur;
	}
}

bool DataPack::HasFile(std::string_view a_path) const
{
	EntryNode * cur = m_manifest.GetHead();
	while (cur != nullptr)
	{
		const DataPackEntry * curEntry = cur->GetData();
		if (std::string_view(curEntry->m_path) == a_path)
		{
			return true;
		}
		cur = cur->GetNext();
	}
	return false;
}

void DataPack::SetRelativePath(std::string_view a_relativePath)
{
	m_relativePath = a_relativePath;
}

void DataPack::AddEntriesToExternalList(std::string_view a_fileExtension, EntryList & a_entries_OUT) const
{
	EntryNode * cur = m_manifest.GetHead();
	while (cur != nullptr)
	{
		DataPackEntry * curEntry = cur->GetData();
		if (std::string_view(curEntry->m_path).find(a_fileExtension) != std::string_view::npos)
		{
			// Data is owned by the datapack
			EntryNode * newNode = new EntryNode();
			newNode->SetData(curEntry);
			a_entries_OUT.Insert(newNode);
		}
		cur = cur->GetNext();
	}
}

void DataPack::AddEntryToExternalList(std::string_view a_filePath, EntryList & a_entries_OUT) const
{
	EntryNode * cur = m_manifest.GetHead();
	while (cur != nullptr)
	{
		DataPackEntry * curEntry = cur->GetData();
		if (std::string_view(curEntry->m_path) == a_filePath)
		{
			// Data is owned by the datapack
			EntryNode * newNode = new EntryNode();
			newNode->SetData(curEntry);
			a_entries_OUT.Insert(newNode);
			return;
		}
		cur = cur->GetNext();
	}
}
