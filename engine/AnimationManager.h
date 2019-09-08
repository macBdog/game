#ifndef _ENGINE_ANIMATION_MANAGER_
#define _ENGINE_ANIMATION_MANAGER_
#pragma once

#include "../core/LinearAllocator.h"
#include "../core/LinkedList.h"
#include "../core/Matrix.h"

#include "FileManager.h"
#include "Singleton.h"
#include "StringHash.h"

class DataPack;
struct DataPackEntry;
class GameObject;

//\brief A KeyFrame stores data to apply to a model at a certain time
struct KeyFrame
{
	KeyFrame()
		: m_time(0)
		, m_pos(0.0f)
		, m_rot(0.0f)
		, m_scale(1.0f)
		, m_transformName() { }
	int m_time;					///< What relative time the keyframe is applied
	Vector m_pos;					///< Where the keyframe locates the transform
	Vector m_rot;					///< Three axis of rotation
	Vector m_scale;					///< Scale in each dimension
	StringHash m_transformName;		///< What the keyframe locates 
};

//\brief AnimationManager loads animations for disk resources and supplies to animation blenders
class AnimationManager : public Singleton<AnimationManager>
{
public:

	//\ No work done in the constructor, only Init
	AnimationManager(float a_updateFreq = s_updateFreq) 
		: m_updateFreq(a_updateFreq)
		, m_updateTimer(0.0f)
		, m_data(NULL)
		{ m_animPath[0] = '\0'; }
	~AnimationManager() { Shutdown(); }

	//\brief Set clear colour buffer and depth buffer setup 
    bool Startup(const char * a_animPath, const DataPack * a_dataPack);
	bool Shutdown();

	//\brief Update and reload animation data
	bool Update(float a_dt);

	//\brief Play an animation on a game object
	bool PlayAnimation(GameObject * a_gameObj, const StringHash & a_animName);

private:

	static const unsigned int s_animPoolSize;					///< How much memory is assigned for all game animations
	static const float s_updateFreq;							///< How often the animation manager should check for resource updates

	//\brief A Key Component is a component of a keyframe used only when loading the animation
	struct KeyComp
	{
		KeyComp() : m_value(0.0f), m_time(0) { }
		float m_value;
		int m_time;
	};

	//\brief A managed animation stores animation data and metadata about the file resource
	struct ManagedAnim
	{
		ManagedAnim(const char * a_animName)
			: m_timeStamp()
			, m_numKeys(0)
			, m_name(a_animName)
			, m_data(NULL)
		{
			m_path[0] = '\0';
		}
		ManagedAnim(const char * a_animPath, const char * a_animName, const FileManager::Timestamp & a_timeStamp)
			: m_timeStamp(a_timeStamp)	
			, m_numKeys(0)
			, m_name(a_animName)
			, m_data(NULL)
		{ 
			strncpy(&m_path[0], a_animPath, StringUtils::s_maxCharsPerLine);
		}
		FileManager::Timestamp m_timeStamp;						///< When the anim file was last edited
		char m_path[StringUtils::s_maxCharsPerLine];			///< Where the anim resides for reloading
		StringHash m_name;										///< What the anim is called
		int m_numKeys;											///< How many keys are in the animation
		int m_frameRate;										///< The speed at which the animation should be played
		KeyFrame * m_data;										///< Pointer to the keyframe data
	};

	typedef LinkedListNode<ManagedAnim> ManagedAnimNode;		///< Alias for a linked list node that points to a managed animation
	typedef LinkedList<ManagedAnim> ManagedAnimList;			///< Alias for a linked list of managed animations

	//\brief Load each take from an ascii FBX file into managed animations
	//\param a_fbxPath the path to the file
	//\return int the number of animations loaded
	int LoadAnimations(const char * a_fbxPath);
	int LoadAnimations(DataPackEntry * a_packedModel);

	//\brief Load function will work with either datapack entry or input stream
	template <typename TInputData>
	int LoadData(TInputData & a_input, const char * a_animName)
	{
		int fileFrameRate = 24;
		const unsigned int maxAnimFileLineChars = StringUtils::s_maxCharsPerLine * 2;
		char line[maxAnimFileLineChars];
		memset(&line, 0, sizeof(char) * maxAnimFileLineChars);

		// Keep track of all the animations that have been added to their frame rate can be appended
		ManagedAnimList addedAnims;

		// Open the file and parse each line 
		if (a_input.is_open())
		{
			// Read till the file has more contents or a rule is broken
			unsigned int lineCount = 0;
			bool reachedTakes = false;
			bool finishedWithTakes = false;
			char currentTake[StringUtils::s_maxCharsPerName];
			currentTake[0] = '\0';
			while (a_input.good())
			{
				a_input.getline(line, maxAnimFileLineChars);
				lineCount++;

				// If the line is too long then its some vertex data we dont care about
				if (strlen(line) >= maxAnimFileLineChars - 1)
				{
					lineCount--;
					a_input.clear();
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
						while (a_input.good() && strstr(line, "Channel:") == NULL)
						{
							a_input.getline(line, maxAnimFileLineChars);
							lineCount++;
						}
						const char * transformName = StringUtils::ExtractField(line, ":", 1);

						// Preamble for each transform manipulation
						for (int chanCount = 0; chanCount < numChannels; ++chanCount)
						{
							// Channel: T/R/S
							a_input.getline(line, maxAnimFileLineChars);		lineCount++;

							for (int compCount = 0; compCount < numComponents; ++compCount)
							{
								// Channel: X/Y/Z
								a_input.getline(line, maxAnimFileLineChars);		lineCount++;

								// Default: 
								a_input.getline(line, maxAnimFileLineChars);		lineCount++;

								// KeyVer:
								a_input.getline(line, maxAnimFileLineChars);		lineCount++;

								// KeyCount:
								int keyCount = 0;
								a_input.getline(line, maxAnimFileLineChars);		lineCount++;
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
								a_input.getline(line, maxAnimFileLineChars);		lineCount++;

								// Read all the keys
								for (int k = 0; k < keyCount; ++k)
								{
									int time = 0;
									float key = 0.0f;
									a_input.getline(line, maxAnimFileLineChars);		lineCount++;
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
								a_input.getline(line, maxAnimFileLineChars);		lineCount++;

								// }
								a_input.getline(line, maxAnimFileLineChars);		lineCount++;
							}

							// LayerType: 
							a_input.getline(line, maxAnimFileLineChars);		lineCount++;

							// }
							a_input.getline(line, maxAnimFileLineChars);		lineCount++;
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
									if (keyProgress[i][j] + 1 < numKeys[i][j])
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
						ManagedAnim * manAnim = NULL;
						if (DataPack::Get().IsLoaded())
						{
							manAnim = new ManagedAnim(a_animName);
						}
						else
						{
							char animPath[StringUtils::s_maxCharsPerLine];
							sprintf(animPath, "%s%s.fbx", m_animPath, a_animName);
							FileManager::Timestamp curTimeStamp;
							FileManager::Get().GetFileTimeStamp(animPath, curTimeStamp);
							manAnim = new ManagedAnim(animPath, a_animName, curTimeStamp);
						}
						if (manAnim != NULL)
						{
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
						}
						else
						{
							Log::Get().WriteEngineErrorNoParams("Memory allocation failure in animation manager.");
						}

						finishedWithTakes = true;
					}
				}
			}

			a_input.close();
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

	ManagedAnimList m_anims;									///< List of all the scripts found on disk at startup
	LinearAllocator<KeyFrame> m_data;							///< Keyframe data shared with blenders
	char m_animPath[StringUtils::s_maxCharsPerLine];			///< Cache off path to animation data 
	float m_updateFreq;											///< How often the script manager should check for changes to shaders
	float m_updateTimer;										///< If we are due for a scan and update of scripts
	KeyFrame * m_last;											///< Last allocated animation memory
};

#endif // _ENGINE_ANIMATION_MANAGER
