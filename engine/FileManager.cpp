#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>

#include "Log.h"

#include "FileManager.h"

template<> FileManager * Singleton<FileManager>::s_instance = NULL;

#ifndef WINDOWS
bool FileManager::FillFileList(const char * a_path, FileList & a_fileList_OUT, const char * a_fileSubstring)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	
	// Check there is actually a path supplied
	if (a_path == NULL || !a_path[0])
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Trying to index an invalid path.");
		return false;
	}

	// Check path isn't too deep
	if (strlen(a_path) > StringUtils::s_maxCharsPerLine)
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Cannot recurse files in a directory with such large path: %s", a_path);
		return false;
	}

	// Now try getting the first file of the list
	TCHAR szDir[MAX_PATH];
	StringCchCopy(szDir, MAX_PATH, a_path);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));
	hFind = FindFirstFile(a_path, &findFileData);

	// Check path actually exists
	if (hFind == INVALID_HANDLE_VALUE)
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Trying to index invalid path %s", a_path);
		return false;
	} 

	// Could be an invalid path
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Cannot find a file in path %s", a_path);
		return false;
	} 
	
	do
	{
		// Check substring parameter
		if (!a_fileSubstring || strstr(findFileData.cFileName, a_fileSubstring))
		{
			// Allocate a new file and set its properties
			LinkedListNode<FileInfo> * newFile = new LinkedListNode<FileInfo>();
			newFile->SetData(new FileInfo());
			sprintf(newFile->GetData()->m_name, "%s", findFileData.cFileName);
			newFile->GetData()->m_sizeBytes = 0;
			newFile->GetData()->m_isDir = findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

			// If not a directory set the size
			if (!newFile->GetData()->m_isDir)
			{
				LARGE_INTEGER fileSize;
				fileSize.LowPart = findFileData.nFileSizeLow;
				fileSize.HighPart = findFileData.nFileSizeHigh;
				newFile->GetData()->m_sizeBytes = fileSize.QuadPart; 
			}

			// Add to the file list
			a_fileList_OUT.Insert(newFile);
		}
	}
    while (FindNextFile(hFind, &findFileData) != 0);
 
	// Final error check now iteration is done
   if (GetLastError() != ERROR_NO_MORE_FILES) 
   {
      Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Error completing enumeration in path %s", a_path);
   }

   FindClose(hFind);

   return true;	
}
#else
bool FileManager::PopulateFileList(const char * a_path, FileList & a_fileList)
{
	return false;		
}
#endif

void FileManager::EmptyFileList(FileList & a_fileList_OUT)
{
	// Iterate through all objects in this file looking for a name match
	LinkedListNode<FileInfo> * next = a_fileList_OUT.GetHead();
	while(next != NULL)
	{
		// Cache off next pointer
		LinkedListNode<FileInfo> * cur = next;
		next = cur->GetNext();

		a_fileList_OUT.Remove(cur);
		delete cur->GetData();
		delete cur;
	}
}