#include <irrKlang.h>

#include "GameObject.h"
#include "Log.h"

#include "SoundManager.h"

template<> SoundManager * Singleton<SoundManager>::s_instance = NULL;
const float SoundManager::s_updateFreq = 1.0f;

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
	return m_engine->play2D(soundNameBuf, false) != NULL;
}

bool SoundManager::PlayMusic(const char * a_musicName)
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
	if (irrklang::ISound * newMusic = m_engine->play2D(musicNameBuf, true))
	{
		SoundNode * newMusicNode = new SoundNode();
		newMusicNode->SetData(newMusic);
		m_music.Insert(newMusicNode);
		return true;
	}
	return false;
}

void SoundManager::StopAllSoundsAndMusic()
{
	m_engine->stopAllSounds();

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
}
