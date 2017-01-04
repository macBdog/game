#ifndef _ENGINE_SOUND_MANAGER
#define _ENGINE_SOUND_MANAGER
#pragma once

#include "../core/LinkedList.h"
#include "GameFile.h"
#include "Singleton.h"

class DataPack;
namespace irrklang
{
	class ISoundEngine;
	class ISound;
}

class SoundManager : public Singleton<SoundManager>
{

public:

	SoundManager(float a_updateFreq = s_updateFreq) 
		: m_updateFreq(a_updateFreq)
		, m_updateTimer(0.0f)
		, m_engine(NULL)
		{ 
			m_soundPath[0] = '\0'; 
		}
	~SoundManager() { Shutdown(); }

	//\brief Lifecycle functions
	bool Startup(const char * a_soundPath, const DataPack * a_dataPack);
	bool Shutdown();

	void Update(float a_dt);
	void SetListenerPosition(const Vector & a_position, const Vector & a_direction, const Vector & a_velocity);
	bool PlaySoundFX(const char * a_soundName) const;
	bool PlaySoundFX3D(const char * a_soundName, const Vector & a_position) const;
	bool PlayMusic(const char * a_musicName);
	bool StopMusic(const char * a_musicName);
	bool SetMusicVolume(const char * a_musicName, float a_newVolume);
	void StopAllSoundsAndMusic();
	bool FadeOutMusic();
	bool FadeInMusic();

private:

	//\brief Storage for a pointer to a sound interface and a name so the sound can be changed later
	struct PlayingSoundInfo
	{
		PlayingSoundInfo() : m_handle(NULL)
		{
			m_name[0] = '\0';
		}

		irrklang::ISound * m_handle;
		char m_name[StringUtils::s_maxCharsPerName];
	};

	static const float s_updateFreq;							///< How often the script manager should check for script updates

	typedef LinkedListNode<PlayingSoundInfo> SoundNode;			///< Alias for a linked list node with a sound pointer
	typedef LinkedList<PlayingSoundInfo> SoundList;				///< Linked list full of sounds

	SoundList m_music;											///< Any music that has been played
	char m_soundPath[StringUtils::s_maxCharsPerLine];			///< Cache off path to sounds 
	float m_updateFreq;											///< How often the manager should check for changes to sound
	float m_updateTimer;										///< If we are due for a scan and update of sound
	irrklang::ISoundEngine * m_engine;							///< Pointer to engine to play the sounds
};

#endif //_ENGINE_SOUND_MANAGER