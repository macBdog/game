#include <cstdio>
#include <cstring>
#include <filesystem>

#include "Log.h"

#include "FileManager.h"

template<> FileManager * Singleton<FileManager>::s_instance = nullptr;

bool FileManager::FillFileList(std::string_view a_path, FileList & a_fileList_OUT, std::string_view a_fileSubstring)
{
	// Check there is actually a path supplied
	if (a_path.empty())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Trying to index an invalid path.");
		return false;
	}

	// Check path exists and is a directory
	std::string pathStr(a_path);
	std::error_code ec;
	if (!std::filesystem::is_directory(pathStr, ec))
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "FillFileList: Trying to index invalid path %s", pathStr.c_str());
		return false;
	}

	for (const auto & entry : std::filesystem::directory_iterator(pathStr, ec))
	{
		const auto filename = entry.path().filename().string();

		// Don't add dot-prefixed entries
		if (filename[0] == '.')
		{
			continue;
		}

		// Check substring parameter
		if (!a_fileSubstring.empty() && filename.find(std::string(a_fileSubstring)) == std::string::npos)
		{
			continue;
		}

		// Allocate a new file and set its properties
		FileListNode * newFile = new FileListNode();
		newFile->SetData(new FileInfo());
		newFile->GetData()->m_name = filename;
		newFile->GetData()->m_sizeBytes = 0;
		newFile->GetData()->m_isDir = entry.is_directory();

		// If not a directory set the size
		if (!newFile->GetData()->m_isDir)
		{
			newFile->GetData()->m_sizeBytes = static_cast<unsigned int>(entry.file_size(ec));
		}

		// Add to the file list
		a_fileList_OUT.Insert(newFile);
	}

	if (ec)
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Error completing enumeration in path %s", pathStr.c_str());
	}

	return true;
}

bool FileManager::CheckFilePath(std::string_view a_filePath)
{
	if (a_filePath.empty())
	{
		return false;
	}

	std::string pathStr(a_filePath);
	std::error_code ec;
	return std::filesystem::is_directory(pathStr, ec);
}

void FileManager::CleanupFileList(FileList & a_fileList_OUT)
{
	// Iterate through all objects in this file and clean up memory
	FileListNode * next = a_fileList_OUT.GetHead();
	while(next != nullptr)
	{
		// Cache off next pointer
		FileListNode * cur = next;
		next = cur->GetNext();

		a_fileList_OUT.Remove(cur);
		delete cur->GetData();
		delete cur;
	}
}

bool FileManager::GetFileTimeStamp(std::string_view a_path, Timestamp & a_timestamp_OUT) const
{
	// Check there is actually a path supplied
	if (a_path.empty())
	{
		Log::Get().Write(LogLevel::Error, LogCategory::Engine, "Trying to index an invalid path.");
		return false;
	}

	std::string pathStr(a_path);
	std::error_code ec;
	auto ftime = std::filesystem::last_write_time(pathStr, ec);
	if (ec)
	{
		Log::Get().WriteOnce(LogLevel::Error, LogCategory::Engine, "File modification date NOT retreived for: %s.", pathStr.c_str());
		return false;
	}

	// Convert file_clock time to time_t for calendar breakdown
#if defined(_WIN32)
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		std::chrono::system_clock::time_point(ftime.time_since_epoch()));
#else
	auto fileNow = std::filesystem::file_time_type::clock::now();
	auto sysNow = std::chrono::system_clock::now();
	auto sctp = sysNow + (ftime - fileNow);
#endif
	std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
	std::tm local_tm;
#if defined(_WIN32)
	localtime_s(&local_tm, &tt);
#else
	localtime_r(&tt, &local_tm);
#endif

	a_timestamp_OUT.m_totalDays = (local_tm.tm_year + 1900) * 365 + (local_tm.tm_mon + 1) * 31 + local_tm.tm_mday;
	a_timestamp_OUT.m_totalSeconds = local_tm.tm_hour * 60 * 60 + local_tm.tm_min * 60 + local_tm.tm_sec;
	return true;
}
