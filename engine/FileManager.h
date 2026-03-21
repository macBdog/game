#ifndef _ENGINE_FILE_MANAGER_
#define _ENGINE_FILE_MANAGER_
#pragma once

#include <string>
#include <string_view>

#include "../core/Delegate.h"
#include "../core/LinkedList.h"

#include "Singleton.h"
#include "StringUtils.h"

//\brief Types of file modification
enum class ModificationType : unsigned char
{
	Change = 0,	///< A file's modification date is new
	Create,		///< A file was created in a managed directory
	Delete,		///< A file was deleted in a managed directory
	Count,
};

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
		std::string m_name;
		unsigned int m_sizeBytes{ 0 };
		bool m_isDir{ false };
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

	//\brief How to pass lists of file info around
	typedef LinkedListNode<FileInfo> FileListNode;
	typedef LinkedList<FileInfo> FileList;

	//\brief Load the game file and parse it into data, will allocate new entries
	bool FillFileList(std::string_view a_filePath, FileList & a_fileList_OUT, std::string_view a_fileSubstring = {});
	bool CheckFilePath(std::string_view a_filePath);
	void CleanupFileList(FileList & a_fileList_OUT);

	//\brief Convenience functions for file lists
	inline unsigned int CountFiles(const FileList & a_fileList, bool a_countFiles, bool a_countDirs) const
	{
		unsigned int numFiles = 0;
		FileListNode * next = a_fileList.GetHead();
		while(next != nullptr)
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

	//\brief Load the game file and parse it into data with a modification callback
	template <typename TObj, typename TMethod>
	inline bool FillManagedFileList(TObj * a_callerObject, TMethod a_callback, std::string_view a_filePath, FileList &a_fileList_OUT, std::string_view a_fileSubstring = {})
	{
		// Add an event to the list of items to be processed
		FileEventNode * newFileNode = new FileEventNode();
		newFileNode->SetData(new FileEvent());

		// Set data for the new event
		FileEvent * newEvent = newFileNode->GetData();
		newEvent->m_fileName = a_filePath;
		newEvent->m_delegate.SetCallback(a_callerObject, a_callback);
		m_events.Insert(newEvent);

		// The filelist itself is regular
		return FillFileList(a_filePath, a_fileList_OUT, a_fileSubstring);
	}
	inline void CleanupManagedFileList(FileList & a_fileList_OUT) { CleanupFileList(a_fileList_OUT); }

	//\brief Utility function to return a file's time stamp
	bool GetFileTimeStamp(std::string_view a_path, Timestamp & a_timestamp_OUT) const;

private:

	//\brief Storage for an input event and it's callback
	struct FileEvent
	{
		ModificationType m_src{ ModificationType::Change };		///< What event happened
		std::string m_fileName;									///< The file that was affected
		Delegate<bool, bool> m_delegate;						///< Pointer to object to call when it happens
	};

	//\brief Alias to store a list of file handle callbacks
	typedef LinkedListNode<FileEvent> FileEventNode;
	typedef LinkedList<FileEvent> FileEventList;

	FileEventList m_events{};
};

#endif // _ENGINE_FILE_MANAGER_
