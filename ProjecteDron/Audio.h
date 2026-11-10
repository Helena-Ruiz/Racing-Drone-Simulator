#pragma once
#include <string>
#include <vector>
#include "miniaudio.h"

class Audio {
public:
	Audio();
	~Audio();
	void Unload();

	// Mètodes de càrrega
	bool Load(ma_engine* engine, const std::string& path, bool loop = false);
	bool LoadProcedural(ma_engine* engine);
	bool LoadProceduralWind(ma_engine* engine);

	// Mètodes de control
	void Play();
	void Pause();
	void Stop();
	void SetVolume(float volume);
	void SetPitch(float pitch);
	bool IsPlaying();

private:
	ma_sound m_sound;
	ma_sound* m_soundPtr;

	// Per a so procedural
	ma_audio_buffer m_engineBuffer;
	std::vector<float> m_engineData;
	bool m_bufferInit;
	bool m_isProcedural;
};