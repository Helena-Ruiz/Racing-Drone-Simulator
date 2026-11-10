// IMPORTANT: Aquesta línia ha d'aparèixer EXACTAMENT en un sol fitxer .cpp del projecte.
#define MINIAUDIO_IMPLEMENTATION
#include "AudioManager.h"
#include <iostream>

AudioManager::AudioManager() : m_initialized(false), m_masterVolume(0.0f) {
}

AudioManager::~AudioManager() {
	Cleanup();
}

bool AudioManager::Init() {
	if ( m_initialized ) return true;

	// Inicialitzem el motor d'àudio principal
	if ( ma_engine_init(NULL, &m_engine) != MA_SUCCESS ) {
		std::cerr << "AudioManager: Error fatal. No s'ha pogut iniciar miniaudio." << std::endl;
		return false;
	}

	m_initialized = true;
	std::cout << "AudioManager: Sistema d'audio inicialitzat correctament." << std::endl;
	return true;
}

void AudioManager::Cleanup() {
	if ( !m_initialized ) return;

	// Aturem música i motor
	m_music.Unload();
	m_droneEngine.Unload();
	m_windSound.Unload();

	// Alliberem tots els efectes de so carregats a la memòria cau
	for ( auto& pair : m_sfxCache ) {
		delete pair.second; // El destructor de Audio cridarà Unload()
	}
	m_sfxCache.clear();

	// Apaguem el motor
	ma_engine_uninit(&m_engine);
	m_initialized = false;
}

void AudioManager::PlaySFX(const std::string& path) {
	if ( !m_initialized ) return;

	// 1. Mirem si el so ja està a la cache
	auto it = m_sfxCache.find(path);
	Audio* sfx = nullptr;

	if ( it != m_sfxCache.end() ) {
		sfx = it->second;
	}
	else {
		// 2. Si no existeix, el carreguem
		sfx = new Audio();
		if ( !sfx->Load(&m_engine, path, false) ) {
			delete sfx;
			return; // Error carregant
		}
		// El guardem al mapa per la propera vegada
		m_sfxCache[ path ] = sfx;
	}

	// 3. Reproduïm
	// Posem el volum segons el Master
	sfx->SetVolume(m_masterVolume);
	sfx->Play();
}

void AudioManager::PlayMusic(const std::string& path) {
	if ( !m_initialized ) return;

	m_music.Stop();

	if ( m_music.Load(&m_engine, path, true) ) {
		m_music.SetVolume(m_masterVolume * 0.75f);
		m_music.Play();
	} 
}

void AudioManager::StopMusic() {
	m_music.Stop();
}

void AudioManager::InitDroneSound() {
	if ( !m_initialized ) return;

	m_droneEngine.Unload();
	m_windSound.Unload();

	if ( m_droneEngine.LoadProcedural(&m_engine) ) {
		m_droneEngine.SetVolume(0.0f);
	}

	//if ( m_windSound.LoadProceduralWind(&m_engine) ) {
		//m_windSound.SetVolume(0.0f);
		//m_windSound.Play();
	//}
}

void AudioManager::UpdateDroneEngine(float throttle, float speed, bool paused, bool estabilitzat) 
{
	
	if ( !m_initialized ) return;

	if ( paused ) {
		if ( m_droneEngine.IsPlaying() ) {
			m_droneEngine.Stop();
		}
		return;
	}
	else 
	{

		if ( !m_droneEngine.IsPlaying() ) m_droneEngine.Play();
		if ( !m_windSound.IsPlaying() ) m_windSound.Play();
	}

	// --- MOTOR (Igual que abans) ---
	if ( throttle < 0.0f ) throttle = 0.0f;
	if ( throttle > 1.0f ) throttle = 1.0f;

	float enginePitch;
	float engineVol;

	if ( estabilitzat )
	{
		enginePitch = 1.1f + throttle * 0.3f;
		engineVol = ( 0.3f + throttle * 0.4f ) * m_masterVolume;
	}
	else
	{
		// ACRO: agresivo
		enginePitch = 0.9f + throttle * 1.3f;
		engineVol = ( 0.05f + throttle * 0.8f ) * m_masterVolume;
	}

	m_droneEngine.SetPitch(enginePitch);
	m_droneEngine.SetVolume(engineVol * 0.05f);

	// --- VENT / TURBULÈNCIA (Basat en Speed) ---

	const float MAX_SPEED_AUDIO = 35.0f; // Velocitat on el vent és màxim
	float windFactor = speed / MAX_SPEED_AUDIO;
	if ( windFactor > 1.0f ) windFactor = 1.0f;
	if ( windFactor < 0.0f ) windFactor = 0.0f;

	// Volum: augmenta exponencialment amb la velocitat (com la resistència de l'aire)
	// El vent sona molt fort a altes velocitats
	float windVol = ( windFactor * windFactor ) * m_masterVolume * 0.8f;

	// Pitch: Simula l'efecte Doppler i la intensitat de la turbulència.
	// A més velocitat -> freqüència més alta (més xiulet)
	float windPitch = 0.7f + ( windFactor * 0.8f );

	m_windSound.SetVolume(windVol);
	m_windSound.SetPitch(windPitch);
}

void AudioManager::SetMasterVolume(float volume) {
	m_masterVolume = volume;
	if ( m_masterVolume < 0.0f ) m_masterVolume = 0.0f;
	if ( m_masterVolume > 1.0f ) m_masterVolume = 1.0f;

	// Actualitzem el volum de la música en temps real
	if ( m_music.IsPlaying() ) {
		m_music.SetVolume(m_masterVolume * 0.5f);
	}

	// Nota: Els SFX s'actualitzaran la propera vegada que es facin sonar amb PlaySFX
}

float AudioManager::GetMasterVolume() const {
	return m_masterVolume;
}