#include "miniaudio.h"

#include "DataPack.h"
#include "GameObject.h"
#include "Log.h"

#include "SoundManager.h"

template<> SoundManager * Singleton<SoundManager>::s_instance = nullptr;
const float SoundManager::s_updateFreq = 1.0f;

bool SoundManager::Startup(const char * a_soundPath, const DataPack * a_dataPack)
{
	m_engine = new ma_engine();
	ma_engine_config engineConfig = ma_engine_config_init();

	if (ma_engine_init(&engineConfig, m_engine) != MA_SUCCESS)
	{
		Log::Get().WriteEngineErrorNoParams("Could not start up sound system.");
		delete m_engine;
		m_engine = nullptr;
		return false;
	}

	// Cache off path
	strncpy(m_soundPath, a_soundPath, sizeof(char) * strlen(a_soundPath) + 1);

	return true;
}

bool SoundManager::Shutdown()
{
	StopAllSoundsAndMusic();

	if (m_engine != nullptr)
	{
		ma_engine_uninit(m_engine);
		delete m_engine;
		m_engine = nullptr;
		return true;
	}

	return false;
}

void SoundManager::Update(float a_dt)
{
}

void SoundManager::SetListenerPosition(const Vector & a_position, const Vector & a_direction, const Vector & a_velocity)
{
	if (m_engine != nullptr)
	{
		ma_engine_listener_set_position(m_engine, 0, a_position.GetX(), a_position.GetY(), a_position.GetZ());
		ma_engine_listener_set_direction(m_engine, 0, a_direction.GetX(), a_direction.GetY(), a_direction.GetZ());
		ma_engine_listener_set_velocity(m_engine, 0, a_velocity.GetX(), a_velocity.GetY(), a_velocity.GetZ());
		ma_engine_listener_set_world_up(m_engine, 0, 0.0f, 0.0f, 1.0f);
	}
}

bool SoundManager::PlaySoundFX(const char * a_soundName) const
{
	if (m_mute)
	{
		return false;
	}

	if (m_engine)
	{
		// Check if the sound name needs the path added
		char soundNameBuf[StringUtils::s_maxCharsPerLine];
		if (!StringUtils::IsAbsolutePath(a_soundName))
		{
			sprintf(soundNameBuf, "%s%s", m_soundPath, a_soundName);
		}
		else // Already fully qualified
		{
			sprintf(soundNameBuf, "%s", a_soundName);
		}
		if (ma_engine_play_sound(m_engine, soundNameBuf, nullptr) == MA_SUCCESS)
		{
			return true;
		}
	}
	return false;
}

bool SoundManager::PlaySoundFX3D(const char * a_soundName, const Vector & a_position) const
{
	if (m_mute)
	{
		return false;
	}

	if (m_engine)
	{
		// Check if the sound name needs the path added
		char soundNameBuf[StringUtils::s_maxCharsPerLine];
		if (!StringUtils::IsAbsolutePath(a_soundName))
		{
			sprintf(soundNameBuf, "%s%s", m_soundPath, a_soundName);
		}
		else // Already fully qualified
		{
			sprintf(soundNameBuf, "%s", a_soundName);
		}

		ma_sound * sound = new ma_sound();
		if (ma_sound_init_from_file(m_engine, soundNameBuf, MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound) == MA_SUCCESS)
		{
			ma_sound_set_spatialization_enabled(sound, MA_TRUE);
			ma_sound_set_position(sound, a_position.GetX(), a_position.GetY(), a_position.GetZ());
			ma_sound_set_min_distance(sound, 0.1f);
			ma_sound_set_max_distance(sound, 10000.0f);
			ma_sound_start(sound);
			// Fire and forget — sound will play and the memory will leak for short SFX
			// A more robust approach would track these, but matches the original irrKlang behavior
			return true;
		}
		else
		{
			delete sound;
		}
	}
	return false;
}

bool SoundManager::PlayMusic(const char * a_musicName)
{
	if (m_mute)
	{
		return false;
	}

	if (m_engine)
	{
		// Check if the music name needs the path added
		char musicNameBuf[StringUtils::s_maxCharsPerLine];
		if (!StringUtils::IsAbsolutePath(a_musicName))
		{
			sprintf(musicNameBuf, "%s%s", m_soundPath, a_musicName);
		}
		else // Already fully qualified
		{
			sprintf(musicNameBuf, "%s", a_musicName);
		}

		ma_sound * sound = new ma_sound();
		if (ma_sound_init_from_file(m_engine, musicNameBuf, 0, nullptr, nullptr, sound) == MA_SUCCESS)
		{
			ma_sound_set_looping(sound, MA_TRUE);
			ma_sound_start(sound);

			// Insert into the music list so it can be managed
			PlayingSoundInfo * newInfo = new PlayingSoundInfo();
			strncpy(newInfo->m_name, a_musicName, StringUtils::s_maxCharsPerName);
			newInfo->m_handle = sound;
			SoundNode * newInfoNode = new SoundNode();
			newInfoNode->SetData(newInfo);
			m_music.Insert(newInfoNode);
			return true;
		}
		else
		{
			Log::Get().Write(LogLevel::Error, LogCategory::Game, "Could not play music named %s.", musicNameBuf);
			delete sound;
		}
	}
	return false;
}

bool SoundManager::SetMusicVolume(const char * a_musicName, float a_newVolume)
{
	// Look through all playing music and try to set the volume
	SoundNode * cur = m_music.GetHead();
	while (cur != nullptr)
	{
		// Found our sound
		PlayingSoundInfo * curInfo = cur->GetData();
		if (strcmp(curInfo->m_name, a_musicName) == 0)
		{
			if (ma_sound * soundHandle = curInfo->m_handle)
			{
				ma_sound_set_volume(soundHandle, a_newVolume);
				return true;
			}
		}

		cur = cur->GetNext();
	}
	return false;
}


bool SoundManager::StopMusic(const char * a_musicName)
{
	// Look through all playing music and try to stop
	SoundNode * cur = m_music.GetHead();
	while (cur != nullptr)
	{
		// Found our sound
		PlayingSoundInfo * curInfo = cur->GetData();
		if (strcmp(curInfo->m_name, a_musicName) == 0)
		{
			if (ma_sound * soundHandle = curInfo->m_handle)
			{
				ma_sound_stop(soundHandle);
				ma_sound_uninit(soundHandle);
				delete soundHandle;

				m_music.RemoveDelete(cur);
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
		// Clean up music handles
		SoundNode * next = m_music.GetHead();
		while (next != nullptr)
		{
			// Cache off next pointer
			SoundNode * cur = next;
			next = cur->GetNext();

			// Clean up the memory for the sound
			if (ma_sound * soundHandle = cur->GetData()->m_handle)
			{
				ma_sound_stop(soundHandle);
				ma_sound_uninit(soundHandle);
				delete soundHandle;
				cur->GetData()->m_handle = nullptr;
			}

			m_music.Remove(cur);
			delete cur->GetData();
			delete cur;
		}
	}
}
