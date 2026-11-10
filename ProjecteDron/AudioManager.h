#pragma once
#include <string>
#include <map>
#include "miniaudio.h"
#include "Audio.h"
#include "Defines.h"

class AudioManager {
public:
	// Singleton
	static AudioManager& Get() {
		static AudioManager instance;
		return instance;
	}

	bool Init();
	void Cleanup();

	void PlaySFX(const std::string& path);
	void PlayMusic(const std::string& path);
	void StopMusic();

	void InitDroneSound();
	void UpdateDroneEngine(float throttle, float speed, bool paused, bool estabilitzat);

	void SetMasterVolume(float volume);
	float GetMasterVolume() const;

private:
	AudioManager();
	~AudioManager();

	// Prohibir còpia
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	ma_engine m_engine;
	bool m_initialized;
	float m_masterVolume;

	std::map<std::string, Audio*> m_sfxCache;
	Audio m_music;
	Audio m_droneEngine;
	Audio m_windSound;
};