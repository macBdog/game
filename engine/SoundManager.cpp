#include <irrKlang.h>

#include "DataPack.h"
#include "GameObject.h"
#include "Log.h"

#include "SoundManager.h"

template<> SoundManager * Singleton<SoundManager>::s_instance = NULL;
const float SoundManager::s_updateFreq = 1.0f;

bool SoundManager::Startup(const char * a_soundPath, const DataPack * a_dataPack)
{
	// start the sound engine with default parameters
	if (m_engine = irrklang::createIrrKlangDevice(irrklang::ESOD_AUTO_DETECT))
    {
		// Cache off path and look for sounds to load
		strncpy(m_soundPath, a_soundPath, sizeof(char) * strlen(a_soundPath) + 1);

		if (a_dataPack != NULL && a_dataPack->IsLoaded())
		{
			// Populate a list of sounds
			DataPack::EntryList soundEntries;
			a_dataPack->GetAllEntries(".ogg,.wav", soundEntries);
			DataPack::EntryNode * curNode = soundEntries.GetHead();

			// Load each font in the pack
			bool loadSuccess = true;
			while (curNode != NULL)
			{
				// TODO
				curNode = curNode->GetNext();
			}

			// Clean up the list of sounds
			a_dataPack->CleanupEntryList(soundEntries);
		}
		return true;
	}
	else
	{
		Log::Get().WriteEngineErrorNoParams("Could not start up sound system.");
	}

	return false;
}

bool SoundManager::Shutdown()
{
	StopAllSoundsAndMusic();

	// Shutdown the engine that plays the sound
	if (m_engine != NULL)
	{
		m_engine->drop();
		m_engine = NULL;
		return true;
	}

	return false;
}

void SoundManager::Update(float a_dt)
{

}

bool SoundManager::PlaySound(const char * a_soundName) const
{
	if (m_engine)
	{
		// Check if the sound name needs the path added
		char soundNameBuf[StringUtils::s_maxCharsPerLine];
		if (!strstr(a_soundName, ":\\"))
		{
			sprintf(soundNameBuf, "%s%s", m_soundPath, a_soundName);
		}
		else // Already fully qualified
		{
			sprintf(soundNameBuf, "%s", a_soundName);
		}
		if (irrklang::ISound * soundHandle = m_engine->play2D(soundNameBuf, false))
		{
			// Not holding on to the reference for any reason so destruct it right away
			soundHandle->drop();
			return true;
		}
	}
	return false;
}

bool SoundManager::PlayMusic(const char * a_musicName)
{
	if (m_engine)
	{
		// Check if the music name needs the path added
		char musicNameBuf[StringUtils::s_maxCharsPerLine];
		if (!strstr(a_musicName, ":\\"))
		{
			sprintf(musicNameBuf, "%s%s", m_soundPath, a_musicName);
		}
		else // Already fully qualified
		{
			sprintf(musicNameBuf, "%s", a_musicName);
		}

		// Insert the sound into the list of playing sounds so the music can be managed
		if (irrklang::ISound * soundHandle = m_engine->play2D(musicNameBuf, true, false, true, irrklang::ESM_AUTO_DETECT, true))
		{
			PlayingSoundInfo * newInfo = new PlayingSoundInfo();
			strncpy(newInfo->m_name, a_musicName, StringUtils::s_maxCharsPerName);
			newInfo->m_handle = soundHandle;
			SoundNode * newInfoNode = new SoundNode();
			newInfoNode->SetData(newInfo);
			m_music.Insert(newInfoNode);
			return true;
		}
		else
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Game, "Could not play music named %s.", musicNameBuf);
		}
		
	}
	return false;
}

bool SoundManager::SetMusicVolume(const char * a_musicName, float a_newVolume)
{
	// Look through all playing music and try to set the volume
	SoundNode * cur = m_music.GetHead();
	while (cur != NULL)
	{
		// Found our sound
		PlayingSoundInfo * curInfo = cur->GetData();
		if (strcmp(curInfo->m_name, a_musicName) == 0)
		{
			if (irrklang::ISound * soundHandle = curInfo->m_handle)
			{
				soundHandle->setVolume(a_newVolume);
				return true;
			}
		}
		
		cur = cur->GetNext();
	}
	return false;
}

void SoundManager::StopAllSoundsAndMusic()
{
	if (m_engine)
	{
		m_engine->stopAllSounds();

		// Clean up music handles
		SoundNode * next = m_music.GetHead();
		while (next != NULL)
		{
			// Cache off next pointer
			SoundNode * cur = next;
			next = cur->GetNext();

			// Clean up the memory for the sound
			if (irrklang::ISound * soundHandle = cur->GetData()->m_handle)
			{
				soundHandle->drop();
				cur->GetData()->m_handle = NULL;
			}

			m_music.Remove(cur);
			delete cur->GetData();
			delete cur;
		}
	}
}
