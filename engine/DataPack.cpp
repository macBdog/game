#include <assert.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "FileManager.h"
#include "Log.h"

#include "DataPack.h"

using namespace std;	//< For fstream operations

template<> DataPack * Singleton<DataPack>::s_instance = NULL;

const char * DataPack::s_defaultDataPackPath = "datapack.dtp";	///< Name of the pack to write and read if not specified

void DataPack::Unload()
{
	// Clean up any entries in the manifest
	EntryNode * next = m_manifest.GetHead();
	while (next != NULL)
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
}

bool DataPack::Load(const char * a_path)
{
	if (IsLoaded())
	{
		Unload();
	}

	// The relative path needs to exist so the paths the game is looking for can be reconstructed
	if (m_relativePath[0] == '\0' || strlen(m_relativePath) <= 0)
	{
		assert(false);
		return false;
	}

	ifstream inputFile(a_path, ios::in | ios::binary);

	// Open the file and parse the datapack
	if (inputFile.is_open())
	{
		// First is the number of entries in the manifest and the resource data size
		int numEntries = 0;
		size_t packSize = 0;
		inputFile.read((char *)&numEntries, sizeof(int));
		inputFile.read((char *)&packSize, sizeof(size_t));
		m_resourceData.Init(packSize);

		// Read each entry
		for (int i = 0; i < numEntries; ++i)
		{
			DataPackEntry * newEntry = new DataPackEntry();
			inputFile.read((char *)&newEntry->m_size, sizeof(size_t));
			inputFile.read((char *)&newEntry->m_path, sizeof(char) * StringUtils::s_maxCharsPerLine);
			char * resourceHead = m_resourceData.Allocate(newEntry->m_size);
			newEntry->m_data = resourceHead;
			const int resourceSize = (int)newEntry->m_size;
			for (int j = 0; j < resourceSize; ++j)
			{
				char c = 0;
				inputFile.read(&c, sizeof(char));
				*resourceHead = c;
				++resourceHead;
			}

			// Add to manifest
			EntryNode * newNode = new EntryNode();
			newNode->SetData(newEntry);
			m_manifest.Insert(newNode);
		}
		inputFile.close();
		return true;
	}
	return false;
}

bool DataPack::AddFile(const char * a_path)
{
	// The relative path needs to exist so it can be stripped before adding to the datapack
	if (m_relativePath[0] == '\0' || strlen(m_relativePath) <= 0)
	{
		assert(false);
		return false;
	}

	// First get the size of the file
	struct stat sizeResult;
	if (stat(a_path, &sizeResult) == 0)
	{
		const size_t sizeBytes = (size_t)sizeResult.st_size;

		// First check that the file isn't already in the pack
		if (!HasFile(a_path))
		{
			// Strip out the absolute part of the file path
			char relPath[StringUtils::s_maxCharsPerLine];
			strcpy(relPath, a_path);
			const char * relSub = strstr(a_path, m_relativePath);
			if (relSub)
			{
				strcpy(relPath, relSub + strlen(m_relativePath));
			}

			DataPackEntry * newEntry = new DataPackEntry();
			newEntry->m_size = sizeBytes;
			strcpy(newEntry->m_path, relPath);

			EntryNode * newNode = new EntryNode();
			newNode->SetData(newEntry);

			m_manifest.Insert(newNode);
		}
		return true;
	}
	return false;
}

bool DataPack::AddFolder(const char * a_path, const char * a_fileExtensions)
{
	bool filesAdded = false;
	const char * delimeter = ",";
	if (a_fileExtensions && a_fileExtensions[0] != '\0')
	{
		// Support a list of extension types
		if (strstr(a_fileExtensions, ",") != NULL)
		{
			char extensionsTokenized[StringUtils::s_maxCharsPerName];
			strcpy(extensionsTokenized, a_fileExtensions);
			char * listTok = strtok(&extensionsTokenized[0], delimeter);
			while(listTok != NULL)
			{
				filesAdded = AddAllFilesInFolder(a_path, listTok);
				listTok = strtok(NULL, delimeter);
			}
		}
		else
		{
			filesAdded = AddAllFilesInFolder(a_path, a_fileExtensions);
		}
	}
	return filesAdded;
}

bool DataPack::AddAllFilesInFolder(const char * a_path, const char * a_fileExtension)
{
	// Scan all the path for files of the correct extension
	FileManager & fileMan = FileManager::Get();
	FileManager::FileList filesToPack;
	fileMan.FillFileList(a_path, filesToPack, a_fileExtension);

	// Add all files of correct path
	int numFilesAdded = 0;
	FileManager::FileListNode * curNode = filesToPack.GetHead();
	while(curNode != NULL)
	{
		// Get a fresh timestamp on the animation file
		char fullPath[StringUtils::s_maxCharsPerLine];
		sprintf(fullPath, "%s%s", a_path, curNode->GetData()->m_name);

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

bool DataPack::Serialize(const char * a_path) const
{
	if (IsLoaded())
	{
		// First compute the complete size of all resources in the pack
		size_t packSize = 0;
		EntryNode * cur = m_manifest.GetHead();
		while (cur != NULL)
		{
			DataPackEntry * curEntry = cur->GetData();
			packSize += curEntry->m_size;
			cur = cur->GetNext();
		}

		const int numEntries = m_manifest.GetLength();
		ofstream outputFile(a_path, ios::out | ios::binary);
		if (outputFile.is_open())
		{
			// First write the number of entries in the manifest and the total resource size
			outputFile.write((char *)&numEntries, sizeof(int));
			outputFile.write((char *)&packSize, sizeof(size_t));

			// Write each file in the manifest
			EntryNode * cur = m_manifest.GetHead();
			while (cur != NULL)
			{
				DataPackEntry * curEntry = cur->GetData();

				// First write the entry header so the reading function knows how far to read
				outputFile.write((char *)&curEntry->m_size, sizeof(size_t));
				outputFile.write((char *)&curEntry->m_path, sizeof(char) * StringUtils::s_maxCharsPerLine);
				
				// Now write the resource by opening and reading it from the disk
				char diskPath[StringUtils::s_maxCharsPerLine];

				// Special case for game.cfg as it lives outside the data folder
				if (strstr(curEntry->m_path, "game.cfg") != 0)
				{ 
					sprintf(diskPath, "%s", curEntry->m_path);
				}
				else
				{
					sprintf(diskPath, "%s%s", m_relativePath, curEntry->m_path);
				}
				size_t writeByteCount = 0;
				ifstream resourceFile(diskPath, ifstream::in | ifstream::binary);
				if (resourceFile.is_open())
				{
					while (resourceFile.good() && writeByteCount < curEntry->m_size)
					{
						char c = resourceFile.get();
						outputFile.write(&c, sizeof(char));
						++writeByteCount;
					}
					resourceFile.close();
				}
				else
				{
					Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Cannot open file %s for writing to the datapack, this will corrupt the datapack.", diskPath);
				}
				cur = cur->GetNext();
			}
			return true;
		}
	}
	return false;
}

DataPackEntry * DataPack::GetEntry(const char * a_path) const
{
	// Write each file in the manifest
	EntryNode * cur = m_manifest.GetHead();
	while (cur != NULL)
	{
		DataPackEntry * curEntry = cur->GetData();
		if (strcmp(curEntry->m_path, a_path) == 0)
		{
			return curEntry;
		}
		cur = cur->GetNext();
	}
	return NULL;
}

void DataPack::GetAllEntries(const char * a_fileExtensions, EntryList & a_entries_OUT) const
{
	// Support a list of extension types
	const char * delimeter = ",";
	if (strstr(a_fileExtensions, ",") != NULL)
	{
		char extensionsTokenized[StringUtils::s_maxCharsPerName];
		strcpy(extensionsTokenized, a_fileExtensions);
		char * listTok = strtok(&extensionsTokenized[0], delimeter);
		while (listTok != NULL)
		{
			AddEntriesToExternalList(listTok, a_entries_OUT);
			listTok = strtok(NULL, delimeter);
		}
	}
	else
	{
		AddEntriesToExternalList(a_fileExtensions, a_entries_OUT);
	}
}

void DataPack::CleanupEntryList(EntryList & a_entries_OUT) const
{
	// Delete the nodes, all data is owned by the datapack
	EntryNode * next = a_entries_OUT.GetHead();
	while (next != NULL)
	{
		// Cache off next pointer
		EntryNode * cur = next;
		next = cur->GetNext();

		a_entries_OUT.Remove(cur);
		delete cur;
	}
}

bool DataPack::HasFile(const char * a_path) const
{
	EntryNode * cur = m_manifest.GetHead();
	while (cur != NULL)
	{
		const DataPackEntry * curEntry = cur->GetData();
		if (strcmp(curEntry->m_path, a_path) == 0)
		{
			return true;
		}
		cur = cur->GetNext();
	}
	return false;
}

void DataPack::SetRelativePath(const char * a_relativePath)
{ 
	strcpy(m_relativePath, a_relativePath); 
}

void DataPack::AddEntriesToExternalList(const char * a_fileExtension, EntryList & a_entries_OUT) const
{
	EntryNode * cur = m_manifest.GetHead();
	while (cur != NULL)
	{
		DataPackEntry * curEntry = cur->GetData();
		if (strstr(curEntry->m_path, a_fileExtension) != 0)
		{
			// Data is owned by the datapack
			EntryNode * newNode = new EntryNode();
			newNode->SetData(curEntry);
			a_entries_OUT.Insert(newNode);
		}
		cur = cur->GetNext();
	}
}

void DataPack::AddEntryToExternalList(const char * a_filePath, EntryList & a_entries_OUT) const
{
	EntryNode * cur = m_manifest.GetHead();
	while (cur != NULL)
	{
		DataPackEntry * curEntry = cur->GetData();
		if (strcmp(curEntry->m_path, a_filePath) == 0)
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