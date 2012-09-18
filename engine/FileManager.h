#ifndef _ENGINE_FILE_MANAGER_
#define _ENGINE_FILE_MANAGER_
#pragma once

#include "../core/LinkedList.h"

#include "Singleton.h"
#include "StringUtils.h"

class FileManager : public Singleton<FileManager>
{
public:
	
	//\brief Basic info about a dir or file
	struct FileInfo
	{
		char m_name[StringUtils::s_maxCharsPerLine];
		unsigned int m_sizeBytes;
		bool m_isDir;
	};

	//\brief Info for storing a timestamp
	struct Timestamp
	{
		Timestamp() : m_totalDays(0), m_totalSeconds(0) {}

		unsigned int m_totalDays;
		unsigned int m_totalSeconds;
		
		bool operator > (const Timestamp & a_val) { return m_totalDays > a_val.m_totalDays && m_totalSeconds > a_val.m_totalSeconds; }
		bool operator < (const Timestamp & a_val) { return m_totalDays < a_val.m_totalDays && m_totalSeconds < a_val.m_totalSeconds; }
	};

	//\brief How to pass lists of file info around
	typedef LinkedListNode<FileInfo> FileListNode;
	typedef LinkedList<FileInfo> FileList;

	//\brief Load the game file and parse it into data, will allocate new entries
	//		 in the list so be sure to use the helper function or delete entries to avoid leaks
	//\param a_filePath cString of the path to enumerate
	//\param a_fileList_OUT the list of FileInfo to add to
	//\param a_fileSubstring optionally exclude all files without this substring in the path
    bool FillFileList(const char * a_filePath, FileList & a_fileList_OUT, const char * a_fileSubstring = NULL);
	void EmptyFileList(FileList & a_fileList_OUT);

	//\brief Load the game file and parse it into data, will allocate new entries
	//		 in the list so be sure to use the helper function or delete entries to avoid leaks
	//\parma a_modifiedCallback Function pointer to some function to be called when file in the path are modified
	//\param a_filePath cString of the path to enumerate
	//\param a_fileList_OUT the list of FileInfo to add to
	//\param a_fileSubstring optionally exclude all files without this substring in the path
	bool FillManagedFileList(void * a_modifiedCallback, const char * a_filePath, FileList &a_fileList_OUT, const char * a_fileSubstring = NULL);
	void EmptyManagedFileList(FileList & a_fileList_OUT);

	//\brief Utility function to return a file's time stamp, is RELATIVE to each day.
	//\param a_path the path to the file to be interrogated
	//\param an Timestamp ref which will be written with to the file's last modification time, 0 for file not found
	//\return bool true if the file was found
	bool GetFileTimeStamp(const char * a_path, Timestamp & a_timestamp_OUT) const;

private:
	// TODO: The file manager should cache off a function pointer for anything calling for
	//		 a list of files. It should also generate a checksum for file modification in
	//		 each managed path. At some update frequency the manager should recalculate
	//		 the checksums of managed paths and call back to the system that ordered that
	//		 managed path with a fresh file listing.
}; 

#endif // _ENGINE_FILE_MANAGER_
