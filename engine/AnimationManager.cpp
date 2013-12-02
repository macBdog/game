#include "DebugMenu.h"
#include "WorldManager.h"

#include "AnimationBlender.h"

#include "AnimationManager.h"

template<> AnimationManager * Singleton<AnimationManager>::s_instance = NULL;

const unsigned int AnimationManager::s_animPoolSize = 16777216;					///< How much memory is assigned for all game animations
const float AnimationManager::s_updateFreq = 1.0f;								///< How often the animation manager should check for updates to disk resources

using namespace std;	//< For fstream operations

bool AnimationManager::Startup(const char * a_animPath)
{
	// Initialise the anim memory pool
	m_data.Init(s_animPoolSize);

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

	// Clean up the memory pool
	m_data.Done();

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

bool AnimationManager::PlayAnimation(GameObject * a_gameObj, const StringHash & a_animName)
{
	// Find the animation
	ManagedAnim * foundAnim = NULL;
	ManagedAnimNode * curAnim = m_anims.GetHead();
	while (curAnim != NULL)
	{
		if (curAnim->GetData()->m_name == a_animName)
		{
			foundAnim = curAnim->GetData();
			break;
		}
		curAnim = curAnim->GetNext();
	}

	// Play it on the game object's blender
	if (a_gameObj && foundAnim)
	{
		if (!a_gameObj->HasAnimationBlender())
		{
			AnimationBlender * newBlend = new AnimationBlender(a_gameObj);
			a_gameObj->SetAnimationBlender(newBlend);
		}
		if (AnimationBlender * blend = a_gameObj->GetAnimationBlender())
		{
			return blend->PlayAnimation(foundAnim->m_data, foundAnim->m_numKeys, foundAnim->m_name);
		}
	}

	return false;
}

int AnimationManager::LoadAnimationsFromFile(const char * a_fbxPath)
{
	int numAnimsLoaded = 0;
	const unsigned int maxAnimFileLineChars = StringUtils::s_maxCharsPerLine * 2;
	char line[maxAnimFileLineChars];
	memset(&line, 0, sizeof(char) * maxAnimFileLineChars);
	ifstream file(a_fbxPath);
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		// Read till the file has more contents or a rule is broken
		unsigned int lineCount = 0;
		int frameRate = 24;
		bool reachedTakes = false;
		bool finishedWithTakes = false;
		char currentTake[StringUtils::s_maxCharsPerName];
		currentTake[0] = '\0';
		while (file.good())
		{
			file.getline(line, maxAnimFileLineChars);
			lineCount++;

			// If this line starts the animation section
			if (strstr(line, "Takes:"))
			{
				reachedTakes = true;
				continue;	
			}

			// Parse any comment lines
			if (strstr(line, ";"))
			{
				continue;
			}

			// Get the frame rate for the animations in this file
			if (strstr(line, "FrameRate: "))
			{
				sscanf(StringUtils::TrimString(line), "FrameRate: \"%d\"", &frameRate);
			}

			// If a new animation is being defined
			int maxKeys = 0;
			KeyFrame * currentTake = NULL;
			KeyFrame * currentChannel = NULL;
			KeyFrame * currentTransform = NULL;
			if (reachedTakes &&
				!finishedWithTakes &&
				strstr(line, "Current: "))
			{
				const char * takeName = StringUtils::ExtractField(line, ":", 1);
				if (strlen(takeName) != NULL)
				{
					// Read till the channels start
					while (file.good() && strstr(line, "Channel:") == NULL)
					{
						file.getline(line, maxAnimFileLineChars);
						lineCount++;
					}
					const char * transformName = StringUtils::ExtractField(line, ":", 1);

					// Preamble for each transform manipulattion
					const int numChannels = 3;
					const int numComponents = 3;
					for (int i = 0; i < numChannels; ++i)
					{
						// Channel: T/R/S
						file.getline(line, maxAnimFileLineChars);		lineCount++;

						// Reset data pointer to take so all components line up
						if (currentTake != NULL)
						{
							currentChannel = currentTake;
						}
						for (int j = 0; j < numComponents; ++j)
						{
							// Channel: X/Y/Z
							file.getline(line, maxAnimFileLineChars);		lineCount++;

							// Default: 
							file.getline(line, maxAnimFileLineChars);		lineCount++;

							// KeyVer:
							file.getline(line, maxAnimFileLineChars);		lineCount++;

							// KeyCount:
							int keyCount = 0;
							file.getline(line, maxAnimFileLineChars);		lineCount++;
							sscanf(StringUtils::TrimString(line), "KeyCount: %d", &keyCount);

							if (keyCount > maxKeys)
							{
								maxKeys = keyCount;
							}

							// Key:
							file.getline(line, maxAnimFileLineChars);		lineCount++;

							// Setup the current take
							if (currentTake == NULL)
							{
								currentTake = m_data.Allocate(sizeof(KeyFrame) * keyCount);
								currentChannel = currentTake;
							}
							currentChannel->m_transformName.SetCString(transformName);

							// Read all the keys
							for (int k = 0; k < keyCount; ++k)
							{
								int time = 0;
								float key = 0.0f;
								file.getline(line, maxAnimFileLineChars);		lineCount++;
								sscanf(StringUtils::TrimString(line), "%d,%f,L,", &time, &key);

								// TODO: Support for scale channel
								if (i == 2)
								{
									continue;
								}

								// Set data based on which channel and component we are on
								int compOrder = i == 0 ? 3 : i - 1;
								currentChannel->m_prs.SetValue(compOrder * 4 + j, key);
								currentChannel++;
							}

							// Colour:
							file.getline(line, maxAnimFileLineChars);		lineCount++;

							// }
							file.getline(line, maxAnimFileLineChars);		lineCount++;
						}
						
						// LayerType: 
						file.getline(line, maxAnimFileLineChars);		lineCount++;

						// }
						file.getline(line, maxAnimFileLineChars);		lineCount++;
					}

					// Add the new managed animation to the list
					FileManager::Timestamp curTimeStamp;
					FileManager::Get().GetFileTimeStamp(a_fbxPath, curTimeStamp);
					ManagedAnim * manAnim = new ManagedAnim(a_fbxPath, takeName, curTimeStamp);
					manAnim->m_data = currentTake;
					manAnim->m_numKeys = maxKeys;

					ManagedAnimNode * manAnimNode = new ManagedAnimNode();
					if (manAnim != NULL && manAnimNode != NULL)
					{
						manAnimNode->SetData(manAnim);
						m_anims.Insert(manAnimNode);
						++numAnimsLoaded;
					}

					finishedWithTakes = true;
				}	
			}
		}
	}

	return numAnimsLoaded > 0;
}
