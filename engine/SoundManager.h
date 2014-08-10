#ifndef _ENGINE_SOUND_MANAGER
#define _ENGINE_SOUND_MANAGER
#pragma once

#include "../core/LinkedList.h"
#include "GameFile.h"
#include "Singleton.h"

class irrklang::ISoundEngine;
class irrklang::ISound;

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
	bool Startup(const char * a_soundPath);
	bool Shutdown();

	void Update(float a_dt);

	bool PlaySound(const char * a_soundName);
	bool PlayMusic(const char * a_musicName);
	bool StopMusic();
	bool FadeOutMusic();
	bool FadeInMusic();

private:

	static const float s_updateFreq;							///< How often the script manager should check for script updates

	typedef LinkedListNode<irrklang::ISound> SoundNode;			///< Alias for a linked list node with a sound pointer
	typedef LinkedList<irrklang::ISound> SoundList;				///< Linked list full of sounds

	SoundList m_music;											///< Any music that has been played
	char m_soundPath[StringUtils::s_maxCharsPerLine];			///< Cache off path to sounds 
	float m_updateFreq;											///< How often the manager should check for changes to sound
	float m_updateTimer;										///< If we are due for a scan and update of sound
	irrklang::ISoundEngine * m_engine;							///< Pointer to engine to play the sounds
};

#endif //_ENGINE_SOUND_MANAGER