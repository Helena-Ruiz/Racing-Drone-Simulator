#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include "imgui.h"
#include "Application.h"
#include "json.hpp"
#include <vector>

using json = nlohmann::json;

struct SettingsData {
    int selectedMap;
    int selectedGamepad;
    int selectedMode;
    int resolutionIndex;
    float volume;
    int qualityPreset;
	bool m_isFullscreen;
	int currentDroneModel = 0;
};


class Menu {
public:
    static bool simulationPaused;
    static bool showMainMenu;
    static bool showSettings;
    static bool showPastSimulations;
    static bool isLoading;           // estat de c�rrega
    static float loadingProgress;    // progr�s de c�rrega
    static float loadingStartTime;   // temps d'inici de c�rregav
    static int loadingState;
    static int selectedGamepad;
    static int selectedMap;
    static MODE_VOL selectedMode;
    static Application* m_app; // cambiar de puntero de instancia a est�tico
    static float ConectionLost;
    static float LowSignal;
    static int qualityPreset;
    static float volume;
    static int resolution_index;
    static Texture* backgroundTex;
    static bool showMapSelection;
    static std::vector<Texture*> mapTextures;
    static bool showCredits;
	static bool fullscren;

	static bool showDroneCustomization;
	static std::vector<Texture*> droneTextures;
	static void LoadDroneTextures();

	// Configuració del dron
	static int selectedDroneModel;
	static float currentMass;
	static float currentMaxThrust;
	static float currentTurnResponsiveness;
	static float currentDrag;
	static float currentMotorLag;
	static std::vector<DroneConfig> droneConfigs;        // Configuració actual de cada dron
	static std::vector<DroneConfig> defaultDroneConfigs; // Configuració de fàbrica (per Reset)
	static void SwitchDroneModel(int newModelIndex);
	static void ResetToDefaults();
	
	static void RenderRaceFinishedOverlay();
    static void RenderCredits();
    static void RenderMapSelection();
    static std::string FormatTime(float timeInSeconds);
    static void RenderReplay();
    static void RenderConnectionLostOverlay();
	static void RenderLowBatteryOverlay();
	static void RenderLowBattery2Overlay();
    static void RenderLowSignalOverlay();
    static Dron* getdorne() { return &m_app->getmscene().getDron(); }
    static void ResetSimulation(Application* app);
    static MODE_VOL GetSelectedMode() { return selectedMode; }
    static int GetSelectedGamepad() { return selectedGamepad; }
	static int GetSelectedMap() { return selectedMap; }
    static void RenderMainMenu();
    static void RenderPauseMenu();
    static void RenderSettings();
	static void RenderDroneCustomization();
    static void RenderPastSimulations();
    static void RenderHUD();
    static void RenderLoadingScreen(); // pantalla de c�rrega
    static void StartLoading();        // iniciar c�rrega
	static void InitDroneConfigs();
	static void applySettings(const SettingsData& s);
    static void guardarConfiguracio(const SettingsData& s);
	static SettingsData carregarConfiguracio();
	static bool showBatteryDeadOverlay;
	static bool isrenderfinal;

private:
    static void DoHeavyLoading();
	static Audio m_menuMusic;
};