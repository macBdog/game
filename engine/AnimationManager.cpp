#include "DebugMenu.h"
#include "WorldManager.h"

#include "AnimationManager.h"

template<> AnimationManager * Singleton<AnimationManager>::s_instance = NULL;

const unsigned int AnimationManager::s_animPoolSize = 16384;					///< How much memory is assigned for all game animations
const float AnimationManager::s_updateFreq = 1.0f;								///< How often the animation manager should check for updates to disk resources

using namespace std;	//< For fstream operations

bool AnimationManager::Startup(const char * a_animPath)
{
	// Cache off path and look for the main game lua file
	strncpy(m_animPath, a_animPath, sizeof(char) * strlen(a_animPath) + 1);

	// Scan all the animation files in the dir for changes to trigger a reload
	FileManager & fileMan = FileManager::Get();
	FileManager::FileList animFiles;
	fileMan.FillFileList(m_animPath, animFiles, ".fbx");

	// Add all the animations for each file in the directory
	FileManager::FileListNode * curNode = animFiles.GetHead();
	while(curNode != NULL)
	{
		// Get a fresh timestamp on the animation file
		char fullPath[StringUtils::s_maxCharsPerLine];
		sprintf(fullPath, "%s%s", m_animPath, curNode->GetData()->m_name);
		FileManager::Timestamp curTimeStamp;
		FileManager::Get().GetFileTimeStamp(fullPath, curTimeStamp);

		LoadAnimationsFromFile(fullPath);
		
		curNode = curNode->GetNext();
	}

	// Clean up the list of anims
	fileMan.CleanupFileList(animFiles);

	return true;
}

bool AnimationManager::Shutdown()
{
	// Clean up any managed animations
	ManagedAnimNode * next = m_anims.GetHead();
	while (next != NULL)
	{
		// Cache off next pointer
		ManagedAnimNode * cur = next;
		next = cur->GetNext();

		m_anims.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	return true;
}

bool AnimationManager::Update(float a_dt)
{
	// Check if an animation needs updating
	int reloadedAnims = 0;
	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
	}
	else // Due for an update, scan all disk resources
	{
		// Test all animations for modification
		m_updateTimer = 0.0f;
		ManagedAnimNode * next = m_anims.GetHead();
		while (next != NULL)
		{
			// Get a fresh timestampt and test it against the stored timestamp
			FileManager::Timestamp curTimeStamp;
			ManagedAnim * curAnim = next->GetData();
			FileManager::Get().GetFileTimeStamp(curAnim->m_path, curTimeStamp);
			if (curTimeStamp > curAnim->m_timeStamp)
			{
				// TODO - remove all loaded anims contained in this file

				reloadedAnims += LoadAnimationsFromFile(curAnim->m_path);
				curAnim->m_timeStamp = curTimeStamp;
				Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in animation file %s, reloading.", curAnim->m_path);
			}
			next = next->GetNext();
		}
	}

	return reloadedAnims > 0;
}

int AnimationManager::LoadAnimationsFromFile(const char * a_fbxPath)
{
	int numAnimsLoaded = 0;
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	ifstream file(a_fbxPath);
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		// Read till the file has more contents or a rule is broken
		unsigned int lineCount = 0;
		bool reachedTakes = false;
		char currentTake[StringUtils::s_maxCharsPerName];
		currentTake[0] = '\0';
		while (file.good())
		{
			file.getline(line, StringUtils::s_maxCharsPerLine);
			lineCount++;

			// If this line starts the animation section
			if (strstr(line, "Takes:  {"))
			{
				reachedTakes = true;
				continue;
			}

			// Parse any comment lines
			if (strstr(line, ";"))
			{
				continue;
			}

			// If a new animation is being defined
			if (reachedTakes &&
				strstr(line, "Current: "))
			{
				const char * takeName = StringUtils::ExtractField(line, ":", 0);
				if (strlen(takeName) != NULL)
				{
					// Read till the channels start
					while (file.good() && strstr(line, "Channel:") == NULL)
					{
						file.getline(line, StringUtils::s_maxCharsPerLine);
						lineCount++;
					}

					// Preamble for each transform manipulattion
					int channelNum = 0;
					int dimensionNum = 0;
					if (strcmp(line, "Channel: \"Transform\" {") == 0)
					{

					}
					

					// Start writing keyframes
					
					// Add managed animation to the list
					FileManager::Timestamp curTimeStamp;
					FileManager::Get().GetFileTimeStamp(a_fbxPath, curTimeStamp);
					ManagedAnim * manAnim = new ManagedAnim(a_fbxPath, takeName, curTimeStamp);
					ManagedAnimNode * manAnimNode = new ManagedAnimNode();
					if (manAnim != NULL && manAnimNode != NULL)
					{
						manAnimNode->SetData(manAnim);
						m_anims.Insert(manAnimNode);
						++numAnimsLoaded;
					}
				}
			}
		}
	}

	return numAnimsLoaded > 0;
}
