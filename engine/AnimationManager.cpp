#include <iostream>
#include <fstream>

#include "DataPack.h"
#include "DebugMenu.h"
#include "WorldManager.h"

#include "AnimationBlender.h"

#include "AnimationManager.h"

template<> AnimationManager * Singleton<AnimationManager>::s_instance = NULL;

const unsigned int AnimationManager::s_animPoolSize = 67108864;					///< How much memory is assigned for all game animations
const float AnimationManager::s_updateFreq = 1.0f;								///< How often the animation manager should check for updates to disk resources

using namespace std;	//< For fstream operations

bool AnimationManager::Startup(const char * a_animPath, const DataPack * a_dataPack)
{
	// Initialise the anim memory pool
	m_data.Init(s_animPoolSize);

	// Cache off path and look for the main game lua file
	strncpy(m_animPath, a_animPath, sizeof(char) * strlen(a_animPath) + 1);

	bool loadSuccess = true;
	if (a_dataPack != NULL && a_dataPack->IsLoaded())
	{
		// Populate a list of animations
		DataPack::EntryList animEntries;
		a_dataPack->GetAllEntries(".fbx", animEntries);
		DataPack::EntryNode * curNode = animEntries.GetHead();

		// Load each font in the pack
		
		while (curNode != NULL)
		{
			loadSuccess &= LoadAnimations(curNode->GetData()) > 0;
			curNode = curNode->GetNext();
		}

		// Clean up the list of animations
		a_dataPack->CleanupEntryList(animEntries);
	}
	else
	{
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

			loadSuccess &= LoadAnimations(fullPath) > 0;
			curNode = curNode->GetNext();
		}

		// Clean up the list of anims
		fileMan.CleanupFileList(animFiles);
	}

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
	// Don't update if reading from a datapack
	DataPack & dataPack = DataPack::Get();
	if (dataPack.IsLoaded())
	{
		return true;
	}

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
				reloadedAnims += LoadAnimations(curAnim->m_path);
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

int AnimationManager::LoadAnimations(const char * a_fbxPath)
{
	// Early out for no file case
	if (a_fbxPath == NULL)
	{
		return false;
	}

	// Anim name is the filename missing the extension because Blender's exporter only exports one take per file
	char animNameBuf[StringUtils::s_maxCharsPerName];
	strcpy(&animNameBuf[0], StringUtils::ExtractFileNameFromPath(a_fbxPath));
	if (strstr(&animNameBuf[0], ".fbx") != NULL)
	{
		animNameBuf[strlen(animNameBuf) - 4] = '\0';
	}

	ifstream file(a_fbxPath, ios::binary);
	return LoadData<ifstream>(file, animNameBuf);
}

int AnimationManager::LoadAnimations(DataPackEntry * a_packedModel)
{
	// Early out for no pack
	if (a_packedModel == NULL)
	{
		return false;
	}

	// Anim name is the data pack path missing the extension because Blender's exporter only exports one take per file
	char animNameBuf[StringUtils::s_maxCharsPerName];
	strcpy(&animNameBuf[0], StringUtils::ExtractFileNameFromPath(a_packedModel->m_path));
	if (strstr(&animNameBuf[0], ".fbx") != NULL)
	{
		animNameBuf[strlen(animNameBuf) - 4] = '\0';
	}

	return LoadData<DataPackEntry>(*a_packedModel, animNameBuf);
}
