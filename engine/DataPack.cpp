#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "DataPack.h"

using namespace std;	//< For fstream operations

template<> DataPack * Singleton<DataPack>::s_instance = NULL;

const char * s_tempFilePath = "dataPackTemp.tmp";			///< When compiling a datapack, the working set minus the manifest is stored on disk instead of memory

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
	// First get the size of the file
	struct stat sizeResult;
	if (stat(a_path, &sizeResult) == 0)
	{
		const size_t sizeBytes = (size_t)sizeResult.st_size;
		DataPackEntry * newEntry = new DataPackEntry();
		newEntry->m_size = sizeBytes;
		strcpy(newEntry->m_path, a_path);

		EntryNode * newNode = new EntryNode();
		newNode->SetData(newEntry);

		m_manifest.Insert(newNode);
		return true;
	}
	return false;
}

bool DataPack::AddFolder(const char * a_path, const char * a_fileExtensions)
{
	return false;
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
				
				// Now write the resource
				size_t writeByteCount = 0;
				ifstream resourceFile(curEntry->m_path, ios::in | ios::binary);
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
				cur = cur->GetNext();
			}
			return true;
		}
	}
	return false;
}

DataPackEntry * DataPack::GetEntry(const char * a_path)
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
