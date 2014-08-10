#include <irrKlang.h>

#include "GameObject.h"
#include "Log.h"

#include "SoundManager.h"

template<> SoundManager * Singleton<SoundManager>::s_instance = NULL;

bool SoundManager::Startup(const char * a_soundPath)
{
	// start the sound engine with default parameters
    if (m_engine = irrklang::createIrrKlangDevice())
    {
		// Cache off path and look for sounds to load
		strncpy(m_soundPath, a_soundPath, sizeof(char) * strlen(a_soundPath) + 1);

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
	// Clean up music handles
	SoundNode * next = m_music.GetHead();
	while (next != NULL)
	{
		// Cache off next pointer
		SoundNode * cur = next;
		next = cur->GetNext();

		m_music.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

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

bool SoundManager::PlaySound(const char * a_soundName)
{
	return m_engine->play2D(a_soundName, false) != NULL;
}

bool SoundManager::PlayMusic(const char * a_musicName)
{
	// Insert the sound into the list of playing sounds so the music can be managed
	if (irrklang::ISound * newMusic = m_engine->play2D(a_musicName, true))
	{
		SoundNode * newMusicNode = new SoundNode();
		newMusicNode->SetData(newMusic);
		m_music.Insert(newMusicNode);
		return true;
	}
	return false;
}