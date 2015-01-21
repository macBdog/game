#include "DebugMenu.h"
#include "WorldManager.h"

#include "AnimationBlender.h"

#include "AnimationManager.h"

template<> AnimationManager * Singleton<AnimationManager>::s_instance = NULL;

const unsigned int AnimationManager::s_animPoolSize = 67108864;					///< How much memory is assigned for all game animations
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
#ifndef _RELEASE
	// Check if an animation needs updating
	if (m_updateTimer < m_updateFreq)
	{
		m_updateTimer += a_dt;
		return true;
	}
	else // Due for an update, scan all disk resources
	{
		// Test all animations for modification
		int reloadedAnims = 0;
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
				reloadedAnims += LoadAnimationsFromFile(curAnim->m_path);
				curAnim->m_timeStamp = curTimeStamp;
				Log::Get().Write(LogLevel::Info, LogCategory::Engine, "Change detected in animation file %s, reloading.", curAnim->m_path);
			}
			next = next->GetNext();
		}
		return reloadedAnims > 0;
	}
#endif
	return true;
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
			return blend->PlayAnimation(foundAnim->m_data, foundAnim->m_numKeys, foundAnim->m_frameRate, foundAnim->m_name);
		}
	}

	return false;
}

int AnimationManager::LoadAnimationsFromFile(const char * a_fbxPath)
{
	int fileFrameRate = 24;
	const unsigned int maxAnimFileLineChars = StringUtils::s_maxCharsPerLine * 2;
	char line[maxAnimFileLineChars];
	memset(&line, 0, sizeof(char) * maxAnimFileLineChars);
	ifstream file(a_fbxPath);
	
	// Keep track of all the animations that have been added to their frame rate can be appended
	ManagedAnimList addedAnims;

	// Open the file and parse each line 
	if (file.is_open())
	{
		// Read till the file has more contents or a rule is broken
		unsigned int lineCount = 0;
		bool reachedTakes = false;
		bool finishedWithTakes = false;
		char currentTake[StringUtils::s_maxCharsPerName];
		currentTake[0] = '\0';
		while (file.good())
		{
			file.getline(line, maxAnimFileLineChars);
			lineCount++;

			// If the line is too long then its some vertex data we dont care about
			if (strlen(line) >= maxAnimFileLineChars-1)
			{
				lineCount--;
				file.clear();
				continue;
			}

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

			// Keep skipping before we get to the takes section
			if (!reachedTakes)
			{
				continue;
			}

			// Get the frame rate for the animations in this file
			if (strstr(line, "FrameRate: "))
			{
				sscanf(StringUtils::TrimString(line), "FrameRate: \"%d\"", &fileFrameRate);
			}

			// If a new animation is being defined, read each component separately
			int animLength = 0;
			const int numChannels = 3;
			const int numComponents = 3;
			KeyComp * activeKey = NULL;
			int numKeys[numChannels][numComponents];
			LinearAllocator<KeyComp> * inputKeys[numChannels][numComponents];
			for (int i = 0; i < numChannels; ++i)
			{
				for (int j = 0; j < numComponents; ++j)
				{
					numKeys[i][j] = 0;
					inputKeys[i][j] = NULL;
				}
			}

			if (!finishedWithTakes &&
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

					// Preamble for each transform manipulation
					for (int chanCount = 0; chanCount < numChannels; ++chanCount)
					{
						// Channel: T/R/S
						file.getline(line, maxAnimFileLineChars);		lineCount++;

						for (int compCount = 0; compCount < numComponents; ++compCount)
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

							// Allocate for the data to be read
							numKeys[chanCount][compCount] = keyCount;
							if (inputKeys[chanCount][compCount] == NULL)
							{
								inputKeys[chanCount][compCount] = new LinearAllocator<KeyComp>();
								inputKeys[chanCount][compCount]->Init(sizeof(KeyComp) * keyCount);
							}
							activeKey = inputKeys[chanCount][compCount]->GetHead();

							// Key:
							file.getline(line, maxAnimFileLineChars);		lineCount++;

							// Read all the keys
							for (int k = 0; k < keyCount; ++k)
							{
								int time = 0;
								float key = 0.0f;
								file.getline(line, maxAnimFileLineChars);		lineCount++;
								const char * timeString = StringUtils::ExtractField(line, ",", 0);
								sscanf(StringUtils::TrimString(line), "%d,%f,L,", &time, &key);

								// Convert super long time string into something more managable
								if (timeString[0] == '0') 
								{
									time = 0;
								}
								else // Chop the extraneous precision off the end of the time value
								{
									const int numLength = strlen(timeString);
									const int maxPrecision = 6;
									if (numLength > maxPrecision)
									{
										char choppedNum[16];
										strncpy(&choppedNum[0], timeString, numLength - maxPrecision);
										choppedNum[numLength - maxPrecision] = '\0';
										time = atoi(choppedNum);
									}
								}
								activeKey->m_time = time;
								activeKey->m_value = key;
								activeKey++;

								if (time > animLength)
								{
									animLength = time;
								}
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

					// Compile the keys and components into a stream of frames
					int totalFrameCount = 0;
					KeyFrame * firstFrame = NULL;
					KeyComp * currentKeyComp[numChannels][numComponents];
					for (int i = 0; i < numChannels; ++i)
					{
						for (int j = 0; j < numComponents; ++j)
						{
							currentKeyComp[i][j] = inputKeys[i][j]->GetHead();
						}
					}

					// Each key can have have a different number of frames. Read and hold on to the last value until a time passes with a new value for each channel
					int keyProgress[numChannels][numComponents];
					memset(&keyProgress[0][0], 0, sizeof(int) * numChannels * numComponents);
					bool newKeyToSet = true;
					for (int timeCount = 0; timeCount < animLength; ++timeCount)
					{
						if (newKeyToSet)
						{
							KeyFrame curKey;
							KeyComp * tX = inputKeys[0][0]->GetHead() + keyProgress[0][0];
							KeyComp * tY = inputKeys[0][1]->GetHead() + keyProgress[0][1];
							KeyComp * tZ = inputKeys[0][2]->GetHead() + keyProgress[0][2];
							KeyComp * rX = inputKeys[1][0]->GetHead() + keyProgress[1][0];
							KeyComp * rY = inputKeys[1][1]->GetHead() + keyProgress[1][1];
							KeyComp * rZ = inputKeys[1][2]->GetHead() + keyProgress[1][2];
							KeyComp * sX = inputKeys[2][0]->GetHead() + keyProgress[2][0];
							KeyComp * sY = inputKeys[2][1]->GetHead() + keyProgress[2][1];
							KeyComp * sZ = inputKeys[2][2]->GetHead() + keyProgress[2][2];
								
							// Add a new key into the stream
							curKey.m_pos = Vector(tX->m_value, tY->m_value, tZ->m_value);
							curKey.m_rot = Vector(rX->m_value, rY->m_value, rZ->m_value);
							curKey.m_scale = Vector(sX->m_value, sY->m_value, sZ->m_value);
							curKey.m_time = timeCount;

							KeyFrame * newKey = m_data.Allocate(sizeof(KeyFrame));
							*newKey = curKey;
							++totalFrameCount;
							if (firstFrame == NULL)
							{
								firstFrame = newKey;
							}
						}

						// Advance component to next frame if there is another frame for the channel component after the time we are at
						newKeyToSet = false;
						for (int i = 0; i < numChannels; ++i)
						{
							for (int j = 0; j < numComponents; ++j)
							{
								// If there is another key to read
								if (numKeys[i][j] > keyProgress[i][j])
								{
									// And time has moved past this key
									if (timeCount >= (inputKeys[i][j]->GetHead() + keyProgress[i][j] + 1)->m_time)
									{
										keyProgress[i][j]++;
										newKeyToSet = true;
									}
								}
							}
						}
					}

					// Add the new managed animation to the list
					FileManager::Timestamp curTimeStamp;
					FileManager::Get().GetFileTimeStamp(a_fbxPath, curTimeStamp);

					// Anim name is the filename missing the extension because Blender's exporter only exports one take per file
					char animNameBuf[StringUtils::s_maxCharsPerName];
					strcpy(&animNameBuf[0], StringUtils::ExtractFileNameFromPath(a_fbxPath));
					if (strstr(&animNameBuf[0], ".fbx") != NULL)
					{
						animNameBuf[strlen(animNameBuf) - 4] = '\0';
					}
					ManagedAnim * manAnim = new ManagedAnim(a_fbxPath, animNameBuf, curTimeStamp);
					manAnim->m_numKeys = totalFrameCount;
					manAnim->m_data = firstFrame;
					manAnim->m_frameRate = fileFrameRate;
					ManagedAnimNode * manAnimNode = new ManagedAnimNode();
					if (manAnim != NULL && manAnimNode != NULL)
					{
						manAnimNode->SetData(manAnim);
						m_anims.Insert(manAnimNode);

						// Add to list for appending framerate
						addedAnims.Insert(manAnimNode);
					}

					finishedWithTakes = true;
				}	
			}
		}
	
		file.close();
	}

	// Update frame rate on all managed anims
	int numAddedAnims = 0;
	ManagedAnimNode * curNode = addedAnims.GetHead();
	while (curNode != NULL)
	{
		curNode->GetData()->m_frameRate = fileFrameRate;
		curNode = curNode->GetNext();
	}

	return numAddedAnims > 0;
}
