#ifndef _ENGINE_FILE_MANAGER_
#define _ENGINE_FILE_MANAGER_
#pragma once

#include "../core/Delegate.h"
#include "../core/LinkedList.h"

#include "Singleton.h"
#include "StringUtils.h"

///\brief The file manager handles regular opening, reading and writing of files but 
//		  can also create a delegate for any system that cares about modifications in
//		  a list of files. It uses file system events to callback to systems with an 
//		  updated file or just notification.
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
		
		inline bool operator > (const Timestamp & a_val) 
		{ 
			if (m_totalDays > a_val.m_totalDays) 
			{ 
				return true;	
			} 
			else if (m_totalDays == a_val.m_totalDays) 
			{ 
				return m_totalSeconds > a_val.m_totalSeconds; 
			} 
			return false;
		}
		inline bool operator < (const Timestamp & a_val) 
		{ 
			if (m_totalDays < a_val.m_totalDays) 
			{ 
				return true;	
			} 
			else if (m_totalDays == a_val.m_totalDays) 
			{ 
				return m_totalSeconds < a_val.m_totalSeconds; 
			} 
			return false;
		}
	};

	//\brief Types of file modification
	enum eModificationType
	{
		eModificationType_Change = 0,	///< A file's modification date is new
		eModificationType_Create,		///< A file was created in a managed directory
		eModificationType_Delete,		///< A file was deleted in a managed directory

		eModificationType_Count,
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
	bool CheckFilePath(const char * a_filePath);
	void CleanupFileList(FileList & a_fileList_OUT);

	//\brief Convenience functions for file lists
	//\param a_fileList const ref to the fileList being counted
	//\param a_countFiles bool true if files should be included in the count
	//\param a_countDirs bool true if directories should be included in the count
	//\return The number of files and folders in the file list
	inline unsigned int CountFiles(const FileList & a_fileList, bool a_countFiles, bool a_countDirs) const
	{
		unsigned int numFiles = 0;
		FileListNode * next = a_fileList.GetHead();
		while(next != NULL)
		{
			FileInfo * curFile = next->GetData();
			if (curFile->m_isDir)
			{
				if (a_countDirs)
				{
					++numFiles;
				}
			}
			else // Is a file
			{
				if (a_countFiles)
				{
					++numFiles;
				}
			}
			next = next->GetNext();
		}
		return numFiles;
	}

	//\brief Load the game file and parse it into data, will allocate new entries
	//		 in the list so be sure to use the helper function or delete entries to avoid leaks
	//\parma a_modifiedCallback Function pointer to some function to be called when file in the path are modified
	//\param a_filePath cString of the path to enumerate
	//\param a_fileList_OUT the list of FileInfo to add to
	//\param a_fileSubstring optionally exclude all files without this substring in the path
	template <typename TObj, typename TMethod>
	inline bool FillManagedFileList(TObj * a_callerObject, TMethod a_callback, const char * a_filePath, FileList &a_fileList_OUT, const char * a_fileSubstring = NULL)
	{
		// Add an event to the list of items to be processed
		// TODO memory management! Kill std new with a rusty fork
		FileEventNode * newFileNode = new FileEventNode();
		newFileNode->SetData(new FileEvent());
	
		// Set data for the new event
		FileEvent * newEvent = newFileNode->GetData();
		sprintf(newEvent->m_fileName, "%s", a_filePath);
		newEvent->m_delegate.SetCallback(a_callerObject, a_callback);
		m_events.Insert(newInputNode);

		// The filelist itself is regular
		return FillFileList(a_filePath, a_fileList_OUT, a_fileSubstring);
	}
	inline void CleanupManagedFileList(FileList & a_fileList_OUT) { CleanupFileList(a_fileList_OUT); }

	//\brief Utility function to return a file's time stamp, is RELATIVE to each day.
	//\param a_path the path to the file to be interrogated
	//\param an Timestamp ref which will be written with to the file's last modification time, 0 for file not found
	//\return bool true if the file was found
	bool GetFileTimeStamp(const char * a_path, Timestamp & a_timestamp_OUT) const;

private:

	//\brief Storage for an input event and it's callback
	struct FileEvent
	{
		FileEvent() 
			: m_src(eModificationType_Change)
			, m_delegate()
			{ sprintf(m_fileName, ""); }

		eModificationType m_src;							///< What event happened
		char m_fileName[StringUtils::s_maxCharsPerName];	///< The file that was affected
		Delegate<bool, bool> m_delegate;					///< Pointer to object to call when it happens
	};

	//\brief Alias to store a list of file handle callbacks
	typedef LinkedListNode<FileEvent> FileEventNode;
	typedef LinkedList<FileEvent> FileEventList;
}; 

#endif // _ENGINE_FILE_MANAGER_
