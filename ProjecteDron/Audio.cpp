#include "Audio.h"
#include <iostream>
#include <cmath>
#include <cstdlib> 

static float getSawWave(float phase) {
	float t = phase - floorf(phase);
	return 2.0f * t - 1.0f;
}

Audio::Audio()
	: m_soundPtr(nullptr), m_bufferInit(false), m_isProcedural(false)
{
	// Inicialitzem estructures a 0 per seguretat
	std::memset(&m_sound, 0, sizeof(m_sound));
	std::memset(&m_engineBuffer, 0, sizeof(m_engineBuffer));
}

Audio::~Audio() {
	Unload();
}

void Audio::Unload() {
	if ( m_soundPtr ) {

		ma_sound_stop(m_soundPtr);
		ma_sound_uninit(&m_sound);
		m_soundPtr = nullptr;
	}

	if ( m_bufferInit ) {
		ma_audio_buffer_uninit(&m_engineBuffer);
		m_bufferInit = false;
	}

	m_engineData.clear();
}

bool Audio::Load(ma_engine* engine, const std::string& path, bool loop) {
	Unload();

	ma_result res = ma_sound_init_from_file(engine, path.c_str(),
		MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC,
		NULL, NULL, &m_sound);

	if ( res != MA_SUCCESS ) {
		std::cerr << "Audio: Error carregant fitxer '" << path << "'" << std::endl;
		return false;
	}

	m_soundPtr = &m_sound;
	ma_sound_set_looping(m_soundPtr, loop ? MA_TRUE : MA_FALSE);
	m_isProcedural = false;

	return true;
}

bool Audio::LoadProcedural(ma_engine* engine) {
	Unload();

	// --- Generació de l'ona de so del motor (Lògica original del teu codi) ---
	const ma_uint32 sampleRate = 48000;
	const ma_uint32 channels = 1;
	const float durationSec = 2.0f; // Buffer de 2 segons (es repetirà)
	const ma_uint64 frameCount = ( ma_uint64 ) ( sampleRate * durationSec );

	m_engineData.resize(( size_t ) ( frameCount * channels ));

	const float baseFreq = 550.0f;
	const float detuneAmount = 1.015f;
	const float volMotors = 0.70f;
	const float volWind = 0.15f;

	for ( ma_uint64 i = 0; i < frameCount; ++i ) {
		float t = ( float ) i / ( float ) sampleRate;
		float ph1 = t * baseFreq;
		float ph2 = t * ( baseFreq * detuneAmount );

		float motor1 = getSawWave(ph1);
		float motor2 = getSawWave(ph2);
		float sub1 = sinf(2.0f * 3.14159f * ( baseFreq * 0.5f ) * t);

		float motors = ( motor1 + motor2 * 0.8f ) * 0.5f + ( sub1 * 0.2f );
		float white = ( ( float ) rand() / ( float ) RAND_MAX ) * 2.0f - 1.0f;

		float propMod = 0.5f * ( 1.0f + sinf(2.0f * 3.14159f * baseFreq * t) );
		float wind = white * propMod;

		float sample = motors * volMotors + wind * volWind;

		// Soft clipping
		sample = tanh(sample * 1.5f);
		if ( sample > 1.0f ) sample = 1.0f;
		if ( sample < -1.0f ) sample = -1.0f;

		m_engineData[ ( size_t ) i ] = sample;
	}

	// Configuració del buffer de miniaudio
	ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
		ma_format_f32, channels, frameCount, m_engineData.data(), NULL
	);

	if ( ma_audio_buffer_init(&bufferConfig, &m_engineBuffer) != MA_SUCCESS ) {
		std::cerr << "Audio: Error creant buffer procedural" << std::endl;
		return false;
	}
	m_bufferInit = true;

	// Crear el so a partir del buffer
	ma_result res = ma_sound_init_from_data_source(engine, &m_engineBuffer,
		MA_SOUND_FLAG_DECODE, NULL, &m_sound);

	if ( res != MA_SUCCESS ) {
		std::cerr << "Audio: Error inicialitzant so procedural" << std::endl;
		return false;
	}

	m_soundPtr = &m_sound;
	ma_sound_set_looping(m_soundPtr, MA_TRUE); // El motor sempre fa loop
	m_isProcedural = true;

	return true;
}

bool Audio::LoadProceduralWind(ma_engine * engine) {
	Unload();

	const ma_uint32 sampleRate = 48000;
	const ma_uint32 channels = 1; // Mono és suficient per al vent, però l'engine ho posiciona
	const float durationSec = 4.0f; // Un loop llarg per evitar repetició obvia
	const ma_uint64 frameCount = ( ma_uint64 ) ( sampleRate * durationSec );

	m_engineData.resize(( size_t ) ( frameCount * channels ));

	// Estats per al filtre de Soroll Rosa (Algorisme de Paul Kellet)
	float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;

	for ( ma_uint64 i = 0; i < frameCount; ++i ) {
		// Generar Soroll Blanc (-1 a 1)
		float white = ( ( float ) rand() / ( float ) RAND_MAX ) * 2.0f - 1.0f;

		// Filtrar a Soroll Rosa (aproximació)
		// Això suavitza els aguts i potencia els greus
		b0 = 0.99886f * b0 + white * 0.0555179f;
		b1 = 0.99332f * b1 + white * 0.0750759f;
		b2 = 0.96900f * b2 + white * 0.1538520f;
		b3 = 0.86650f * b3 + white * 0.3104856f;
		b4 = 0.55000f * b4 + white * 0.5329522f;
		b5 = -0.7616f * b5 - white * 0.0168980f;

		float pink = b0 + b1 + b2 + b3 + b4 + b5 + white * 0.5362f;
		float sample = pink * 0.11f; // Ajust de guany per evitar saturació

		// Fade in/out als extrems del buffer per evitar "clic" al fer loop
		if ( i < 1000 ) sample *= ( ( float ) i / 1000.0f );
		if ( i > frameCount - 1000 ) sample *= ( ( float ) ( frameCount - i ) / 1000.0f );

		// Clamp
		if ( sample > 1.0f ) sample = 1.0f;
		if ( sample < -1.0f ) sample = -1.0f;

		m_engineData[ ( size_t ) i ] = sample;
	}

	// Configurar buffer miniaudio
	ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
		ma_format_f32, channels, frameCount, m_engineData.data(), NULL
	);

	if ( ma_audio_buffer_init(&bufferConfig, &m_engineBuffer) != MA_SUCCESS ) {
		return false;
	}
	m_bufferInit = true;

	ma_result res = ma_sound_init_from_data_source(engine, &m_engineBuffer,
		MA_SOUND_FLAG_DECODE, NULL, &m_sound);

	if ( res != MA_SUCCESS ) return false;

	m_soundPtr = &m_sound;
	ma_sound_set_looping(m_soundPtr, MA_TRUE);

	return true;
}

void Audio::Play() {
	if ( m_soundPtr ) {
		// Si és un efecte (no loop procedural), el reiniciem al principi per si es prem molt ràpid
		if ( !m_isProcedural && !ma_sound_is_looping(m_soundPtr) ) {
			ma_sound_seek_to_pcm_frame(m_soundPtr, 0);
		}
		ma_sound_start(m_soundPtr);
	}
}

void Audio::Pause() {
	if ( m_soundPtr ) ma_sound_stop(m_soundPtr);
}

void Audio::Stop() {
	if ( m_soundPtr ) {
		ma_sound_stop(m_soundPtr);
		ma_sound_seek_to_pcm_frame(m_soundPtr, 0);
	}
}

void Audio::SetVolume(float volume) {
	if ( m_soundPtr ) ma_sound_set_volume(m_soundPtr, volume);
}

void Audio::SetPitch(float pitch) {
	if ( m_soundPtr ) ma_sound_set_pitch(m_soundPtr, pitch);
}

bool Audio::IsPlaying() {
	return ( m_soundPtr && ma_sound_is_playing(m_soundPtr) );
}