#ifndef _ENGINE_FILE_MANAGER_
#define _ENGINE_FILE_MANAGER_
#pragma once

#include "../core/LinkedList.h"

#include "StringUtils.h"

class FileManager
{
public:
	
	//\brief Basic info about a dir or file
	struct FileInfo
	{
		char m_name[StringUtils::s_maxCharsPerLine];
		unsigned int m_sizeBytes;
		bool m_isDir;
	};

	//\brief How to pass lists of file info around
	typedef LinkedList<FileInfo> FileList;

	//\brief Load the game file and parse it into data, will allocate new entries
	//		 in the list so be sure to use the helper function or delete entries to avoid leaks
	//\param a_filePath cString of the path to enumerate
	//\param a_fileList_OUT the list of FileInfo to add to
    bool FillFileList(const char * a_filePath, FileList & a_fileList_OUT);
	void EmptyFileList(FileList & a_fileList_OUT);

private:

};

#endif // _ENGINE_FILE_MANAGER_
