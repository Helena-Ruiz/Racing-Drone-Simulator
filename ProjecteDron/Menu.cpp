#include "Menu.h"
#include "GLFW/glfw3.h"
#include <cstring>
#include "Defines.h"
#include <chrono>
#include <thread>
#include "Application.h"
#include <experimental/filesystem>
#include "AudioManager.h"
#define NOMINMAX
#include <windows.h>

namespace fs = std::experimental::filesystem;

Texture* Menu::backgroundTex = nullptr;
const char* FONT_PATH = "Fonts/Aerial Faster - Regular.otf";
const float FONT_SIZE_BIG = 60.0f;
std::vector<Texture*> Menu::mapTextures;

const char* FONT_PATH_3 = "Fonts/FlerixRegular-6RAn6.ttf";
const float FONT_SIZE_SMALL = 20.0f;

const char* FONT_PATH_2 = "Fonts/Galantic-Regular.otf";

ImFont* g_TitleFont = nullptr;

bool Menu::fullscren = true;
bool Menu::showCredits = false;
bool Menu::showMapSelection = false;
bool Menu::simulationPaused = true;
bool Menu::showMainMenu = true;
bool Menu::showSettings = false;
bool Menu::showPastSimulations = false;
bool Menu::showDroneCustomization = false;
bool Menu::isLoading = false;
float Menu::loadingProgress = 0.0f;
float Menu::loadingStartTime = 0.0f;
int Menu::selectedGamepad = 0;
int Menu::selectedMap = 0;
int Menu::qualityPreset = 1;
int Menu::resolution_index = 0;
MODE_VOL Menu::selectedMode = MODE_VOL::DESARMED;
Application* Menu::m_app = nullptr;
float Menu::ConectionLost = 0.0f;
float Menu::LowSignal = 0.0f;
float Menu::volume = 0.5f;
int Menu::loadingState = 1;
bool Menu::isrenderfinal = false;
int Menu::selectedDroneModel = 0;

float Menu::currentMass = 0.0f;
float Menu::currentMaxThrust = 0.0f;
float Menu::currentTurnResponsiveness = 0.0f;
float Menu::currentDrag = 0.0f;
float Menu::currentMotorLag = 0.0f;

std::vector<Texture*> Menu::droneTextures;
std::vector<DroneConfig> Menu::droneConfigs;
std::vector<DroneConfig> Menu::defaultDroneConfigs;

void Menu::applySettings(const SettingsData& s)
{
	if ( !m_app ) return;

	m_app->ToggleFullscreen(s.m_isFullscreen);
	m_app->getmscene().getWorld().setMapActual(static_cast< MapID >(s.selectedMap));
	m_app->setController(static_cast<ControllerType>(s.selectedGamepad));
	m_app->getmscene().setMode(selectedMode);
	m_app->getmscene().getDron().setMode(static_cast<MODE_VOL>( s.selectedMode));
	// resolution?
	AudioManager::Get().SetMasterVolume(s.volume);
	// qualityPreset?

	Menu::selectedDroneModel = s.currentDroneModel;
	if ( Menu::selectedDroneModel >= 0 && Menu::selectedDroneModel < Menu::droneConfigs.size() )
	{
		DroneConfig& config = Menu::droneConfigs[ Menu::selectedDroneModel ];

		Menu::currentMass = config.mass;
		Menu::currentMaxThrust = config.maxThrust;
		Menu::currentTurnResponsiveness = config.turnResponsiveness;
		Menu::currentDrag = config.drag;
		Menu::currentMotorLag = config.motorLag;
	}

	m_app->getmscene().getDron().setPhysicsParameters(
		Menu::currentMass,
		Menu::currentMaxThrust,
		Menu::currentTurnResponsiveness,
		Menu::currentDrag,
		Menu::currentMotorLag
	);

	m_app->getmscene().getDron().setModelType(Menu::selectedDroneModel);
}

void Menu::guardarConfiguracio(const SettingsData& s) {
	json config;

	config[ "fullScreen" ] = s.m_isFullscreen;
	config[ "selectedMap" ] = s.selectedMap;
	config[ "selectedGamepad" ] = s.selectedGamepad;
	config[ "selectedMode" ] = s.selectedMode;
	config[ "resolutionIndex" ] = s.resolutionIndex;
	config[ "volume" ] = s.volume;
	config[ "qualityPreset" ] = s.qualityPreset;

	if ( selectedDroneModel >= 0 && selectedDroneModel < droneConfigs.size() ) {
		droneConfigs[ selectedDroneModel ].mass = currentMass;
		droneConfigs[ selectedDroneModel ].maxThrust = currentMaxThrust;
		droneConfigs[ selectedDroneModel ].turnResponsiveness = currentTurnResponsiveness;
		droneConfigs[ selectedDroneModel ].drag = currentDrag;
		droneConfigs[ selectedDroneModel ].motorLag = currentMotorLag;
	}

	config[ "currentDroneModel" ] = selectedDroneModel;

	json dronesArray;
	for ( const auto& d : droneConfigs ) {
		json dJson;
		dJson[ "mass" ] = d.mass;
		dJson[ "maxThrust" ] = d.maxThrust;
		dJson[ "turnResponsiveness" ] = d.turnResponsiveness;
		dJson[ "drag" ] = d.drag;
		dJson[ "motorLag" ] = d.motorLag;
		dronesArray.push_back(dJson);
	}
	config[ "drones" ] = dronesArray;

	std::ofstream("configuracioMenu.json") << config.dump(4);
}

SettingsData Menu::carregarConfiguracio() {
	SettingsData config{};

	if ( droneConfigs.empty() ) {
		InitDroneConfigs();
	}

	std::ifstream file("configuracioMenu.json");
	if ( !file.is_open() ) {
		config.m_isFullscreen = true;
		return config;
	}

	json configArxiu;
	file >> configArxiu;

	config.m_isFullscreen = configArxiu.value("fullScreen", true);
	config.selectedMap = configArxiu.value("selectedMap", 0);
	config.selectedGamepad = configArxiu.value("selectedGamepad", 0);
	config.selectedMode = configArxiu.value("selectedMode", 0);
	config.resolutionIndex = configArxiu.value("resolutionIndex", 0);
	config.volume = configArxiu.value("volume", 0.8f);
	config.qualityPreset = configArxiu.value("qualityPreset", 1);
	config.currentDroneModel = configArxiu.value("currentDroneModel", 0);
	
	int savedModel = configArxiu.value("currentDroneModel", 0);

	if ( configArxiu.contains("drones") ) {
		int i = 0;
		for ( const auto& dJson : configArxiu[ "drones" ] ) {
			if ( i < droneConfigs.size() ) {
				droneConfigs[ i ].mass = dJson.value("mass", defaultDroneConfigs[ i ].mass);
				droneConfigs[ i ].maxThrust = dJson.value("maxThrust", defaultDroneConfigs[ i ].maxThrust);
				droneConfigs[ i ].turnResponsiveness = dJson.value("turnResponsiveness", defaultDroneConfigs[ i ].turnResponsiveness);
				droneConfigs[ i ].drag = dJson.value("drag", defaultDroneConfigs[ i ].drag);
				droneConfigs[ i ].motorLag = dJson.value("motorLag", defaultDroneConfigs[ i ].motorLag);
				i++;
			}
		}
	}
	
	return config;
}

void Menu::DoHeavyLoading()
{
	if ( !m_app ) return;
	m_app->getmscene().initWorld(static_cast< MapID >( selectedMap ));
	m_app->getmscene().initDron();
	m_app->getmscene().initShadowsMap();
}

void Menu::InitDroneConfigs() {
	if ( !Menu::defaultDroneConfigs.empty() ) return;

	DroneConfig og = { 0.5f, 30.0f, 1.0f, 0.018f, 0.1f };
	DroneConfig freestyle = { 0.795f, 35.0f, 1.0f, 0.03f, 0.1f };
	DroneConfig aggressive = { 0.6f, 38.0f, 1.0f, 0.02f, 0.05f };

	Menu::defaultDroneConfigs = { og, freestyle, aggressive };
	Menu::droneConfigs = Menu::defaultDroneConfigs;

	if ( Menu::droneConfigs.size() > 0 ) {
		const auto& firstCfg = Menu::droneConfigs[ 0 ];
		Menu::currentMass = firstCfg.mass;
		Menu::currentMaxThrust = firstCfg.maxThrust;
		Menu::currentTurnResponsiveness = firstCfg.turnResponsiveness;
		Menu::currentDrag = firstCfg.drag;
		Menu::currentMotorLag = firstCfg.motorLag;
		Menu::selectedDroneModel = 0;
	}
}

void Menu::SwitchDroneModel(int newModelIndex) {
	if ( selectedDroneModel >= 0 && selectedDroneModel < droneConfigs.size() ) {
		droneConfigs[ selectedDroneModel ].mass = currentMass;
		droneConfigs[ selectedDroneModel ].maxThrust = currentMaxThrust;
		droneConfigs[ selectedDroneModel ].turnResponsiveness = currentTurnResponsiveness;
		droneConfigs[ selectedDroneModel ].drag = currentDrag;
		droneConfigs[ selectedDroneModel ].motorLag = currentMotorLag;
	}

	selectedDroneModel = newModelIndex;

	if ( newModelIndex >= 0 && newModelIndex < droneConfigs.size() ) {
		const auto& cfg = droneConfigs[ newModelIndex ];
		currentMass = cfg.mass;
		currentMaxThrust = cfg.maxThrust;
		currentTurnResponsiveness = cfg.turnResponsiveness;
		currentDrag = cfg.drag;
		currentMotorLag = cfg.motorLag;
	}
}

void Menu::ResetToDefaults() {
	if ( selectedDroneModel >= 0 && selectedDroneModel < defaultDroneConfigs.size() ) {
		droneConfigs[ selectedDroneModel ] = defaultDroneConfigs[ selectedDroneModel ];
		const auto& cfg = droneConfigs[ selectedDroneModel ];
		currentMass = cfg.mass;
		currentMaxThrust = cfg.maxThrust;
		currentTurnResponsiveness = cfg.turnResponsiveness;
		currentDrag = cfg.drag;
		currentMotorLag = cfg.motorLag;
	}
}

static void TextCentered(const char* text) {
	float window_width = ImGui::GetWindowSize().x;
	float text_width = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX(( window_width - text_width ) * 0.5f);
	ImGui::Text("%s", text);
}

void Menu::StartLoading() {
	isLoading = true;
	loadingProgress = 0.0f;
	loadingStartTime = glfwGetTime();
	showMainMenu = false;
	simulationPaused = true;
	loadingState = 0;
}

void Menu::RenderLoadingScreen()
{
	if ( !isLoading ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImVec2 display_size = io.DisplaySize;
	const ImVec2 loading_box_size(500, 250);

	if ( backgroundTex )
	{
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		draw_list->AddImage(bgID, ImVec2(0, 0), ImVec2(display_size.x, display_size.y), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
	}
	else
	{
		draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(display_size);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
	ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::SetNextWindowSize(loading_box_size);
	ImGui::SetNextWindowPos(ImVec2(
		display_size.x * 0.5f - loading_box_size.x * 0.5f,
		display_size.y * 0.5f - loading_box_size.y * 0.5f
	));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.18f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

	ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

	const char* statusText = "LOADING WORLD...";
	if ( loadingState == 1 ) statusText = "LOADING DRONE...";
	else if ( loadingState == 2 ) statusText = "LOADING SHADOWS AND COLISIONS...";
	else if ( loadingState == 3 ) statusText = "COMPLETED!";
	else if ( loadingState >= 4 ) statusText = "COMPLETED!";

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, 24.0f);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));

	float current_window_width = ImGui::GetContentRegionAvail().x + ImGui::GetStyle().WindowPadding.x * 2;
	float text_width = ImGui::CalcTextSize(statusText).x;

	ImGui::SetCursorPosX(( current_window_width - text_width ) * 0.5f);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
	ImGui::Text("%s", statusText);

	ImGui::PopStyleColor();
	if ( g_TitleFont ) ImGui::PopFont();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
	float bar_width = 400.0f;
	float bar_x = ( current_window_width - bar_width ) * 0.5f;

	ImGui::SetCursorPosX(bar_x);

	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.10f, 0.30f, 0.8f));

	ImGui::ProgressBar(loadingProgress, ImVec2(bar_width, 25), "");

	ImGui::PopStyleColor(2);

	ImGui::End();

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);

	if ( loadingState == 0 ) {
		loadingProgress = 0.05f;
		loadingState = 1;
		return;
	}
	if ( loadingState == 1 ) {
		if ( m_app ) m_app->getmscene().initWorld(static_cast< MapID >( selectedMap ));
		loadingProgress = 0.70f;
		loadingState = 2;
		return;
	}
	if ( loadingState == 2 ) {
		if ( m_app ) m_app->getmscene().initDron();
		loadingProgress = 0.80f;
		loadingState = 3;
		return;
	}
	if ( loadingState == 3 ) {
		if ( m_app ) m_app->getmscene().initShadowsMap();
		loadingProgress = 1.0f;
		loadingState = 4;
		return;
	}
	if ( loadingState == 4 ) {
		static float delay_timer = 0.0f;
		delay_timer += io.DeltaTime;

		if ( delay_timer >= 0.3f )
		{
			isLoading = false;
			simulationPaused = false;
			loadingState = 0;
			delay_timer = 0.0f;
		}
		return;
	}
}

void Menu::RenderMainMenu()
{
	if ( !showMainMenu ) return;

	static bool musicStarted = false;
	if ( !musicStarted ) {
		AudioManager::Get().PlayMusic("Audio/musica_menu.mp3");
		musicStarted = true;
	}

	if ( !backgroundTex )
	{
		backgroundTex = new Texture("Fons/FondoMenu.jpg", false);
	}

	ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
	GLuint m_ID;
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImVec2 display_size = io.DisplaySize;
	const ImVec2 window_size(850, 720);

	if ( backgroundTex )
	{
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		draw_list->AddImage(bgID, ImVec2(0, 0), ImVec2(display_size.x, display_size.y), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
	}
	else
	{
		draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f, display_size.y * 0.5f - window_size.y * 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

	ImGui::Begin("Main Menu Container", &showMainMenu, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 70);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH, FONT_SIZE_BIG);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.00f, 1.0f));
	TextCentered("DRONE FLIGHT SIM");
	ImGui::PopStyleColor();

	if ( g_TitleFont ) ImGui::PopFont();

	ImVec2 line_start = ImGui::GetCursorScreenPos();
	line_start.x += 80;
	line_start.y += 20;

	ImVec2 line_end(line_start.x + window_size.x - 160, line_start.y);

	ImGui::GetWindowDrawList()->AddLine(line_start, line_end, ImColor(0.30f, 0.60f, 0.90f, 1.0f), 3.0f);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 80);

	const ImVec2 button_size(350, 75);
	const float spacing_y = 40.0f;
	const float spacing_x = 30.0f;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 25));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::SetCursorPosX(80);
	if ( ImGui::Button("START FLIGHT", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		AudioManager::Get().StopMusic();
		AudioManager::Get().PlayMusic("Audio/musica_vol.mp3");
		m_app->setReplay(false);
		StartLoading();
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(80 + button_size.x + spacing_x);
	if ( ImGui::Button("SAVED REPLAYS", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showPastSimulations = true;
		showMainMenu = false;
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing_y);

	ImGui::SetCursorPosX(80);
	if ( ImGui::Button("SETTINGS", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showSettings = true;
		showMainMenu = false;
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(80 + button_size.x + spacing_x);
	if ( ImGui::Button("DRONE CUSTOMIZATION", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showMainMenu = false;
		showDroneCustomization = true;
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing_y);

	ImGui::SetCursorPosX(80);
	if ( ImGui::Button("CHOOSE MAP", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showMainMenu = false;
		showMapSelection = true;
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(80 + button_size.x + spacing_x);
	if ( ImGui::Button("CREDITS", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showCredits = true;
		showMainMenu = false;
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing_y + 10);

	ImGui::SetCursorPosX(( window_size.x - button_size.x ) * 0.5f);
	if ( ImGui::Button("EXIT", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		exit(0);
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);

	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 18);
	float text_width = ImGui::CalcTextSize("Version 1.0.0").x;
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f - text_width * 0.5f);
	ImGui::TextColored(ImVec4(0.85f, 0.95f, 1.00f, 1.0f), "Version 1.0.0");

	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void Menu::RenderPauseMenu()
{
	if ( !simulationPaused || showMainMenu || showMapSelection || showSettings || showCredits || showPastSimulations || showDroneCustomization 
		|| isrenderfinal) return;

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImVec2 display_size = io.DisplaySize;

	if ( backgroundTex )
	{
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		draw_list->AddImage(bgID, ImVec2(0, 0), display_size, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
	}
	else
	{
		draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	const ImVec2 window_size(500, 400);
	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f, display_size.y * 0.5f - window_size.y * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

	ImGui::Begin("Pause Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, 30.0f);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));
	TextCentered("SIMULATION PAUSED");
	ImGui::PopStyleColor();

	ImVec2 line_start = ImVec2(ImGui::GetCursorScreenPos().x + 30, ImGui::GetCursorScreenPos().y + 10);
	ImVec2 line_end = ImVec2(line_start.x + window_size.x - 100, line_start.y);
	ImGui::GetWindowDrawList()->AddLine(line_start, line_end, ImColor(0.30f, 0.60f, 0.90f, 1.0f), 3.0f);

	if ( g_TitleFont ) ImGui::PopFont();

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 60);

	const ImVec2 button_size(350, 75);
	const float spacing_y = 40.0f;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 25));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	ImGui::SetCursorPosX(( window_size.x - button_size.x ) * 0.5f);
	if ( ImGui::Button("RESUME", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		simulationPaused = false;
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing_y);

	ImGui::SetCursorPosX(( window_size.x - button_size.x ) * 0.5f);
	if ( ImGui::Button("MAIN MENU", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		AudioManager::Get().PlayMusic("Audio/musica_menu.mp3");
		simulationPaused = true;
		showMainMenu = true;
		ResetSimulation(m_app);
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(1);
}

void Menu::RenderSettings()
{
	if ( !showSettings ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImVec2 display_size = io.DisplaySize;
	const ImVec2 window_size(850, 720);

	if ( backgroundTex )
	{
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		draw_list->AddImage(bgID, ImVec2(0, 0), ImVec2(display_size.x, display_size.y), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
	}
	else
	{
		draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f, display_size.y * 0.5f - window_size.y * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 40));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::Begin("Settings Container", &showSettings, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, 30.0f);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));
	TextCentered("SETTINGS");
	ImGui::PopStyleColor();

	ImVec2 line_start = ImGui::GetCursorScreenPos();
	line_start.x += 80;
	line_start.y += 15;
	ImVec2 line_end(line_start.x + window_size.x - 160, line_start.y);

	ImGui::GetWindowDrawList()->AddLine(line_start, line_end, ImColor(0.30f, 0.60f, 0.90f, 1.0f), 3.0f);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);

	if ( g_TitleFont ) ImGui::PopFont();

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.10f, 0.30f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.95f, 0.75f, 0.20f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 15.0f));

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::SetCursorPosX(window_size.x * 0.5f - 300.0f);
	ImGui::BeginGroup();
	const float widgetWidth = 600.0f;

	ImGui::TextColored(ImVec4(0.30f, 0.60f, 0.90f, 1.0f), "GRAPHICS & DISPLAY");
	ImGui::Separator();
	ImGui::SetNextItemWidth(widgetWidth);

	char resolutionText[ 32 ];
	if ( m_app ) {
		snprintf(resolutionText, 32, "%d x %d", m_app->m_screen_width, m_app->m_screen_height);
	}
	else {
		snprintf(resolutionText, 32, "N/A");
	}

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Resolution : ");

	float resultLabelWidth = ImGui::CalcTextSize("Resolution : ").x + ImGui::GetStyle().ItemSpacing.x;
	ImGui::SameLine(resultLabelWidth);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.10f, 0.30f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, ImGui::GetStyle().FramePadding.y));

	ImGui::BeginGroup();
	ImGui::SetNextItemWidth(widgetWidth - resultLabelWidth);
	ImGui::TextColored(ImVec4(0.85f, 0.95f, 1.0f, 1.0f), "%s", resolutionText);
	ImGui::EndGroup();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	ImGui::SetNextItemWidth(widgetWidth);

	static int screenMode = fullscren;
	const char* screenModes[ ] = { "Windowed Mode", "Fullscreen" };

	if ( ImGui::Combo("Display Mode", &screenMode, screenModes, IM_ARRAYSIZE(screenModes)) )
	{
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
	}

	if ( screenMode == 1 )
		m_app->ToggleFullscreen(true);
	else
		m_app->ToggleFullscreen(false);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
	ImGui::TextColored(ImVec4(0.30f, 0.60f, 0.90f, 1.0f), "AUDIO");
	ImGui::Separator();

	static float tempVolume = Menu::volume;
	static bool volumeInitialized = false;

	if ( !volumeInitialized ) {
		tempVolume = Menu::volume;
		volumeInitialized = true;
	}

	ImGui::SetNextItemWidth(widgetWidth);
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::SliderFloat("Volume", &tempVolume, 0.0f, 1.0f, "%.2f");
	ImGui::PopStyleColor(2);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
	ImGui::TextColored(ImVec4(0.30f, 0.60f, 0.90f, 1.0f), "CONTROLS & GAMEPLAY");
	ImGui::Separator();

	const char* gamepads[ ] = { "PS4", "DRONE", "KEYBOARD" };
	ImGui::SetNextItemWidth(widgetWidth);
	if ( ImGui::Combo("Gamepad", &selectedGamepad, gamepads, IM_ARRAYSIZE(gamepads)) )
	{
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
	}

	ImGui::SetNextItemWidth(widgetWidth);
	int currentMode = static_cast< int >( selectedMode );
	const char* ModeVol[ ] = { "ACROBATIC", "STABILIZED", "DISARMED" };

	if ( ImGui::Combo("Flight Mode", &currentMode, ModeVol, IM_ARRAYSIZE(ModeVol)) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		selectedMode = static_cast< MODE_VOL >( currentMode );
	}

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
	ImGui::Separator();
	ImGui::EndGroup();

	const ImVec2 button_size(200, 50);
	const float button_area_width = button_size.x * 2 + 20;

	ImGui::SetCursorPosY(window_size.y - 55);
	ImGui::SetCursorPosX(( window_size.x - button_area_width ) * 0.5f);

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(6);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 15));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	if ( ImGui::Button("APPLY", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		SettingsData s;
		s.selectedMap = selectedMap;
		s.selectedGamepad = selectedGamepad;
		s.volume = tempVolume;
		s.qualityPreset = qualityPreset;
		s.resolutionIndex = resolution_index;
		s.selectedMode = static_cast< int >( selectedMode );
		s.m_isFullscreen = screenMode;
		guardarConfiguracio(s);
		applySettings(s);
	}

	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SameLine(0, 20.0f);

	if ( ImGui::Button("BACK", button_size) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		tempVolume = Menu::volume;
		volumeInitialized = false;
		showSettings = false;
		showMainMenu = true;
	}

	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(4);

	ImGui::End();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
}

void Menu::RenderDroneCustomization()
{
	if ( !showDroneCustomization ) return;
	if ( droneTextures.empty() ) LoadDroneTextures();
	if ( droneConfigs.empty() ) InitDroneConfigs();

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 display_size = io.DisplaySize;
	const ImVec2 window_size(950, 720);

	ImDrawList* bg_draw_list = ImGui::GetBackgroundDrawList();
	if ( backgroundTex ) {
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		bg_draw_list->AddImage(bgID, ImVec2(0, 0), display_size);
	}
	else {
		bg_draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f, display_size.y * 0.5f - window_size.y * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 30));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::Begin("Drone Customization", &showDroneCustomization, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, 35.0f);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));
	TextCentered("DRONE CUSTOMIZATION");
	ImGui::PopStyleColor();
	if ( g_TitleFont ) ImGui::PopFont();

	ImDrawList* window_draw_list = ImGui::GetWindowDrawList();
	ImVec2 line_start = ImGui::GetCursorScreenPos();
	line_start.x += 60; line_start.y += 5;
	ImVec2 line_end = ImVec2(line_start.x + window_size.x - 120, line_start.y);
	window_draw_list->AddLine(line_start, line_end, ImColor(0.30f, 0.60f, 0.90f, 1.0f), 3.0f);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::Columns(2, "CustomColumns", false);
	ImGui::SetColumnWidth(0, window_size.x * 0.50f);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
	ImGui::Text("SELECT CHASSIS MODEL");
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, 20));

	const char* droneNames[ ] = { "OG MODEL", "DJI FPV", "AGGRESSIVE MODEL" };
	ImVec2 cardSize(400, 110);
	float cardSpacing = -100.0f;

	for ( int i = 0; i < 3; i++ ) {
		ImGui::PushID(i);
		ImVec2 p = ImGui::GetCursorScreenPos();
		bool selected = ( selectedDroneModel == i );

		if ( ImGui::InvisibleButton("##CardHitbox", cardSize) ) {
			AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
			SwitchDroneModel(i);
		}

		bool hovered = ImGui::IsItemHovered();
		ImU32 bgColor = selected ? IM_COL32(60, 50, 90, 200) : ( hovered ? IM_COL32(60, 60, 100, 100) : IM_COL32(40, 30, 60, 150) );
		ImU32 borderColor = selected ? IM_COL32(255, 223, 0, 255) : ( hovered ? IM_COL32(150, 150, 255, 150) : IM_COL32(100, 80, 150, 100) );

		window_draw_list->AddRectFilled(p, ImVec2(p.x + cardSize.x, p.y + cardSize.y), bgColor, 8.0f);
		window_draw_list->AddRect(p, ImVec2(p.x + cardSize.x, p.y + cardSize.y), borderColor, 8.0f, 0, selected ? 3.0f : 1.0f);

		if ( i < droneTextures.size() && droneTextures[ i ] ) {
			ImTextureID texID = ( ImTextureID ) ( intptr_t ) droneTextures[ i ]->getID();
			window_draw_list->AddImage(texID, ImVec2(p.x + 10, p.y + 10), ImVec2(p.x + 100, p.y + 100));
		}

		const char* name = droneNames[ i ];
		ImVec2 textSize = ImGui::CalcTextSize(name);
		float textX = p.x + 120;
		float textY = p.y + ( cardSize.y - textSize.y ) * 0.5f;
		window_draw_list->AddText(ImVec2(textX, textY), selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 200, 255), name);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + cardSize.y + cardSpacing);
		ImGui::PopID();
	}

	ImGui::NextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
	ImGui::Text("PERFORMANCE TUNING");
	ImGui::SameLine();

	float availX = ImGui::GetContentRegionAvail().x;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 0.8f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	if ( ImGui::Button("RESET DEFAULT", ImVec2(150, 30)) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		ResetToDefaults();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.10f, 0.25f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.20f, 0.40f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.35f, 0.30f, 0.50f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.35f, 0.45f, 0.85f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 8));

	float sliderWidth = 380.0f;
	float verticalSpacing = 4.0f;

	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Mass (kg)");
	ImGui::SetNextItemWidth(sliderWidth);
	ImGui::SliderFloat("##Mass", &currentMass, 0.1f, 3.0f, "%.2f kg");
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip("Total mass of the drone.");
	ImGui::Dummy(ImVec2(0, verticalSpacing));

	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Maximum Thrust");
	ImGui::SetNextItemWidth(sliderWidth);
	ImGui::SliderFloat("##Thrust", &currentMaxThrust, 5.0f, 60.0f, "%.1f N");
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip("Total engine power at 100%%.");
	ImGui::Dummy(ImVec2(0, verticalSpacing));

	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Rotation responsiveness");
	ImGui::SetNextItemWidth(sliderWidth);
	ImGui::SliderFloat("##Resp", &currentTurnResponsiveness, 0.1f, 2.0f, "%.1f");
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip("Rotation speed and sensitivity.");
	ImGui::Dummy(ImVec2(0, verticalSpacing));

	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Air Resistance (Drag)");
	ImGui::SetNextItemWidth(sliderWidth);
	ImGui::SliderFloat("##Drag", &currentDrag, 0.0f, 2.0f, "%.2f");
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip("Aerodynamic air friction.");
	ImGui::Dummy(ImVec2(0, verticalSpacing));

	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Engine Lag (ms)");
	ImGui::SetNextItemWidth(sliderWidth);
	ImGui::SliderFloat("##Lag", &currentMotorLag, 0.0f, 1.0f, "%.2f s");
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip("Delay in engine response.");
	ImGui::Dummy(ImVec2(0, verticalSpacing));

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(5);

	ImGui::Columns(1);
	if ( g_TitleFont ) ImGui::PopFont();

	ImGui::SetCursorPosY(window_size.y - 90);
	const ImVec2 bottomBtnSize(220, 55);
	float groupWidth = bottomBtnSize.x * 2 + 40;
	ImGui::SetCursorPosX(( window_size.x - groupWidth ) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 15));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, 22.0f);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	if ( ImGui::Button("APPLY SETTINGS", bottomBtnSize) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		if ( m_app ) {
			droneConfigs[ selectedDroneModel ].mass = currentMass;
			droneConfigs[ selectedDroneModel ].maxThrust = currentMaxThrust;
			droneConfigs[ selectedDroneModel ].turnResponsiveness = currentTurnResponsiveness;
			droneConfigs[ selectedDroneModel ].drag = currentDrag;
			droneConfigs[ selectedDroneModel ].motorLag = currentMotorLag;

			Dron& d = m_app->getmscene().getDron();

			d.setModelType(selectedDroneModel);

			d.setPhysicsParameters(
				currentMass,
				currentMaxThrust,
				currentTurnResponsiveness,
				currentDrag,
				currentMotorLag
			);

			SettingsData s;
			s.selectedMap = selectedMap;
			s.selectedGamepad = selectedGamepad;
			s.selectedMode = ( int ) selectedMode;
			s.resolutionIndex = resolution_index;
			s.volume = volume;
			s.qualityPreset = qualityPreset;
			s.currentDroneModel = selectedDroneModel;
			guardarConfiguracio(s);
			applySettings(s);
		}
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 p = ImGui::GetItemRectMin();
		ImVec2 s = ImGui::GetItemRectSize();
		window_draw_list->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	ImGui::SameLine(0, 40);

	if ( ImGui::Button("BACK", bottomBtnSize) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showDroneCustomization = false;
		showMainMenu = true;
		SwitchDroneModel(selectedDroneModel);
	}
	if ( ImGui::IsItemHovered() ) {
		ImVec2 p = ImGui::GetItemRectMin();
		ImVec2 s = ImGui::GetItemRectSize();
		window_draw_list->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(4);

	ImGui::End();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(1);
}

void Menu::RenderPastSimulations()
{
	if ( !showPastSimulations || m_app->getReplay() ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImVec2 display_size = io.DisplaySize;
	const ImVec2 window_size(850, 720);

	if ( backgroundTex )
	{
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		draw_list->AddImage(bgID, ImVec2(0, 0), display_size, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
	}
	else
	{
		draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f, display_size.y * 0.5f - window_size.y * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 40));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::Begin("Past Simulations Container", &showPastSimulations, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, 30.0f);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));
	TextCentered("SAVED REPLAYS");
	ImGui::PopStyleColor();

	ImVec2 line_start = ImGui::GetCursorScreenPos();
	line_start.x += 80;
	line_start.y += 15;
	ImVec2 line_end(line_start.x + window_size.x - 160, line_start.y);

	ImGui::GetWindowDrawList()->AddLine(line_start, line_end, ImColor(0.30f, 0.60f, 0.90f, 1.0f), 3.0f);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);

	if ( g_TitleFont ) ImGui::PopFont();

	std::vector<std::string> simulations;

	for ( const auto& entry : fs::directory_iterator("Replays/") )
	{
		if ( fs::is_regular_file(entry.path()) )
		{
			simulations.push_back(entry.path().filename().string());
		}
	}

	const float listWidth = 600.0f;
	const ImVec2 selectableSize(listWidth, 45);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.10f, 0.30f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.15f, 0.35f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.95f, 0.75f, 0.20f, 0.9f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 8.0f));

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::SetCursorPosX(( window_size.x - listWidth ) * 0.5f);

	if ( simulations.empty() )
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 100);
		TextCentered("NO SAVED REPLAYS FOUND");
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
		TextCentered("Start a flight to record your first simulation.");
	}
	else
	{
		for ( size_t i = 0; i < simulations.size(); ++i )
		{
			std::string label = "Repeticio num " + std::to_string(i + 1) + " : " + simulations[ i ];

			if ( ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, selectableSize) )
			{
				AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
				m_app->getmscene().getDron().getreplay().loadReplay(simulations[ i ]);
				m_app->getmscene().getDron().setReplay(true);
				m_app->getmscene().getDron().getreplay().startReplay();
				m_app->setReplay(true);
				m_app->getmscene().getDron().stopReplay(true);
				StartLoading();
			}
			ImGui::SetCursorPosX(( window_size.x - listWidth ) * 0.5f);
		}
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(5);

	ImGui::SetCursorPosY(window_size.y - 100);

	const ImVec2 backButtonSize(350, 75);
	ImGui::SetCursorPosX(( window_size.x - backButtonSize.x ) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 25));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	if ( ImGui::Button("BACK", backButtonSize) )
	{
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showPastSimulations = false;
		showMainMenu = true;
	}

	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(4);

	ImGui::End();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
}

static void DrawTextShadow(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text)
{
	dl->AddText(font, size, ImVec2(pos.x + 1.5f, pos.y + 1.5f), IM_COL32(0, 0, 0, 200), text);
	dl->AddText(font, size, pos, col, text);
}

void Menu::RenderHUD()
{
	ImU32 c_tech_cyan = IM_COL32(0, 255, 255, 240);
	ImU32 c_tech_white = IM_COL32(220, 240, 255, 200);
	ImU32 c_tech_orange = IM_COL32(255, 140, 0, 255);
	ImU32 c_subtle_line = IM_COL32(255, 255, 255, 50);

	ConectionLost = m_app->getmscene().getWorld().getConectionLost();
	LowSignal = m_app->getmscene().getWorld().getLowSignal();

	if ( showMainMenu || simulationPaused || m_app->getReplay() || showDroneCustomization ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 s = io.DisplaySize;
	ImDrawList* dl = ImGui::GetForegroundDrawList();

	Dron* drone = getdorne();
	if ( !drone ) return;



	float alt = drone->getAltitude();
	float spd = drone->getSpeed();
	float pitch = drone->getPitch();
	float roll = drone->getRoll();
	float heading = drone->getYaw();

	while ( heading < 0.0f ) heading += 360.0f;
	while ( heading >= 360.0f ) heading -= 360.0f;

	float battery = drone->getBatteryLevel();

	if ( std::abs(drone->getPosition().x) > ConectionLost ||
		std::abs(drone->getPosition().y) > ConectionLost ||
		std::abs(drone->getPosition().z) > ConectionLost ) {
		RenderConnectionLostOverlay(); return;
	}
	if ( std::abs(drone->getPosition().x) > LowSignal ||
		std::abs(drone->getPosition().y) > LowSignal ||
		std::abs(drone->getPosition().z) > LowSignal ) {
		RenderLowSignalOverlay();
	}

	if ( battery < 0.1f ) {
		RenderLowBatteryOverlay();
		return;
	}

	if ( battery < 6.0f ) {
		RenderLowBattery2Overlay();
	}

	ImU32 c_white = IM_COL32(255, 255, 255, 230);
	ImU32 c_cyan = IM_COL32(0, 255, 240, 200);
	ImU32 c_red = IM_COL32(255, 50, 50, 255);
	ImU32 c_warn = IM_COL32(255, 200, 0, 255);

	ImFont* font = io.Fonts->Fonts[ 0 ];

	// Telemetry
	float bottomY = s.y - 80.0f;
	float centerGap = 300.0f;

	if ( drone->getMode() == MODE_VOL::DESARMED )
	{
		// Velocitat del parpelleig (vegades per segon aproximadament)
		float blinkSpeed = 2.0f;
		// Utilitzem fmod i el temps per crear un cicle on/off
		bool blinkOn = fmod(ImGui::GetTime() * blinkSpeed, 1.0f) < 0.5f;

		if ( blinkOn )
		{
			const char* warningText = "DRONE DISARMED";
			float warningFontSize = 60.0f; // Mida gran

			// Calculem la mida del text per poder centrar-lo
			ImVec2 textSize = font->CalcTextSizeA(warningFontSize, FLT_MAX, 0.0f, warningText);

			ImVec2 centerPos;
			centerPos.x = ( s.x - textSize.x ) * 0.5f;
			centerPos.y = ( s.y - textSize.y ) * 0.5f;

			// Dibuixem el text amb ombra perque es vegi bé sobre qualsevol fons
			DrawTextShadow(dl, font, warningFontSize, centerPos, c_red, warningText);
		}
	}

	{
		char valText[ 16 ]; snprintf(valText, 16, "%.1f", spd);
		char labelText[ ] = "M/S";

		float valW = font->CalcTextSizeA(40.0f, FLT_MAX, 0.0f, valText).x;
		ImVec2 pos(s.x * 0.5f - centerGap, bottomY);

		DrawTextShadow(dl, font, 40.0f, pos, c_white, valText);
		DrawTextShadow(dl, font, 18.0f, ImVec2(pos.x + valW + 5, pos.y + 18), c_cyan, labelText);

		float ratio = ( ( spd / 40.0f ) < 1.0f ) ? ( spd / 40.0f ) : 1.0f;
		dl->AddRectFilled(ImVec2(pos.x, pos.y + 45), ImVec2(pos.x + 100, pos.y + 47), IM_COL32(255, 255, 255, 50));
		dl->AddRectFilled(ImVec2(pos.x, pos.y + 45), ImVec2(pos.x + 100 * ratio, pos.y + 47), c_cyan);
	}

	{
		char valText[ 16 ]; snprintf(valText, 16, "%.1f", alt);
		char labelText[ ] = "M";

		float valW = font->CalcTextSizeA(40.0f, FLT_MAX, 0.0f, valText).x;
		ImVec2 pos(s.x * 0.5f + centerGap - 100, bottomY);

		DrawTextShadow(dl, font, 40.0f, pos, c_white, valText);
		DrawTextShadow(dl, font, 18.0f, ImVec2(pos.x + valW + 5, pos.y + 18), c_cyan, labelText);

		float ratio = ( ( ( alt / 100.0f ) < ( 1.0f ) ) ? ( alt / 100.0f ) : ( 1.0f ) );
		dl->AddRectFilled(ImVec2(pos.x, pos.y + 45), ImVec2(pos.x + 100, pos.y + 47), IM_COL32(255, 255, 255, 50));
		dl->AddRectFilled(ImVec2(pos.x, pos.y + 45), ImVec2(pos.x + 100 * ratio, pos.y + 47), c_warn);
	}

	{
		float dist = glm::length(drone->getPosition());
		char hText[ 32 ]; snprintf(hText, 32, "H: %.0fm", dist);
		ImVec2 tSz = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, hText);
		DrawTextShadow(dl, font, 20.0f, ImVec2(( s.x - tSz.x ) * 0.5f, s.y - 40), c_white, hText);
	}

	// Artificial Horizon
	float velY = s.y * 0.5f;
	float velX = 50;
	float pitchScale = 5.0f;
	float crossSize = 15.0f;

	float horizonLength = 300.0f;
	float pitchOffset = pitch * 0.0f;
	ImVec2 center(s.x * 0.5f, s.y * 0.5f);

	auto rotatePoint = [ & ] (const ImVec2& p, float angle) -> ImVec2 {
		float s = sin(angle);
		float c = cos(angle);
		float dx = p.x - center.x;
		float dy = p.y - center.y;
		return ImVec2(center.x + ( dx * c - dy * s ), center.y + ( dx * s + dy * c ));
		};

	float pitchOffset2 = pitch * 200.0f;

	if ( m_app->getmscene().getActiveCamera().getType() == Type::FPV ) {

		ImVec2 horizonCenterP1 = rotatePoint(ImVec2(center.x - horizonLength / 2, center.y + pitchOffset), roll);
		ImVec2 horizonCenterP2 = rotatePoint(ImVec2(center.x + horizonLength / 2, center.y + pitchOffset), roll);
		dl->AddLine(horizonCenterP1, horizonCenterP2, IM_COL32(255, 255, 255, 255), 3.0f);

		for ( int i = -4; i <= 4; i++ ) {
			float angleValue = i * 10.0f;
			if ( i == 0 ) continue;

			float markLength = ( abs(i) % 2 == 0 ) ? 60.0f : 40.0f;
			ImVec2 pLine1 = ImVec2(center.x - markLength / 2, center.y + pitchOffset2 - ( angleValue * pitchScale ));
			ImVec2 pLine2 = ImVec2(center.x + markLength / 2, center.y + pitchOffset2 - ( angleValue * pitchScale ));

			ImVec2 rLine1 = rotatePoint(pLine1, roll);
			ImVec2 rLine2 = rotatePoint(pLine2, roll);

			dl->AddLine(rLine1, rLine2, IM_COL32(255, 255, 255, 150), 1.0f);

			char pitchText[ 5 ];
			snprintf(pitchText, 5, "%d", ( int ) fabs(angleValue));

			ImVec2 textPosLeftNoRot = ImVec2(pLine1.x - 5 - ImGui::CalcTextSize(pitchText).x, pLine1.y - ImGui::CalcTextSize(pitchText).y * 0.5f);
			ImVec2 rTextPosLeft = rotatePoint(textPosLeftNoRot, roll);

			ImVec2 textPosRightNoRot = ImVec2(pLine2.x + 5, pLine1.y - ImGui::CalcTextSize(pitchText).y * 0.5f);
			ImVec2 rTextPosRight = rotatePoint(textPosRightNoRot, roll);


			dl->AddText(rTextPosLeft, IM_COL32(255, 255, 255, 200), pitchText);
			dl->AddText(rTextPosRight, IM_COL32(255, 255, 255, 200), pitchText);
		}

		dl->AddLine(ImVec2(center.x - crossSize, center.y), ImVec2(center.x - 5, center.y), IM_COL32(255, 255, 255, 255), 2.0f);
		dl->AddLine(ImVec2(center.x + 5, center.y), ImVec2(center.x + crossSize, center.y), IM_COL32(255, 255, 255, 255), 2.0f);

		dl->AddLine(ImVec2(center.x, center.y - 5), ImVec2(center.x, center.y + 5), IM_COL32(255, 255, 255, 255), 2.0f);

		dl->AddCircle(center, 3.0f, IM_COL32(255, 255, 255, 255), 4, 2.0f);
	}

	// Race Data
	auto* raceManager = m_app->getmscene().getWorld().getRaceManager();
	if ( raceManager )
	{
		float x = 30.0f;
		float y = 30.0f;

		DrawTextShadow(dl, font, 30.0f, ImVec2(x, y), c_white, FormatTime(raceManager->getCurrentTime()).c_str());

		y += 35.0f;
		ImU32 col_sub = IM_COL32(200, 200, 200, 255);

		char lapText[ 32 ]; snprintf(lapText, 32, "LAP: 1/3");
		DrawTextShadow(dl, font, 18.0f, ImVec2(x, y), col_sub, lapText);

		y += 20.0f;
		char cpText[ 32 ]; snprintf(cpText, 32, "CP: %d/%d", raceManager->getNextCheckpointID(), raceManager->getTotalCheckpoints());
		DrawTextShadow(dl, font, 18.0f, ImVec2(x, y), col_sub, cpText);
	}

	// --- FLIGHT MODE DISPLAY ---
	{
		float x = 30.0f;
		float y = 140.0f; // Positioned below Race Data

		const char* modeText = "UNKNOWN";
		ImU32 modeColor = c_white;

		MODE_VOL currentMode = drone->getMode(); // Assuming getMode() exists and returns MODE_VOL enum

		switch ( currentMode ) {
		case MODE_VOL::ACROBATIC:
			modeText = "ACRO";
			modeColor = c_red;
			break;
		case MODE_VOL::STABILIZED:
			modeText = "STAB";
			modeColor = c_tech_cyan;
			break;
		case MODE_VOL::DESARMED:
			modeText = "DISARMED";
			modeColor = c_warn;
			break;
		}

		DrawTextShadow(dl, font, 24.0f, ImVec2(x, y), modeColor, modeText);
	}

	// System Status
	{
		float x = s.x - 150.0f;
		float y = 30.0f;

		ImU32 batCol = ( battery > 20 ) ? c_white : c_red;
		char batText[ 32 ]; snprintf(batText, 32, "BAT %.0f%%", battery);
		DrawTextShadow(dl, font, 20.0f, ImVec2(x, y), batCol, batText);

		y += 20.0f;
		char voltText[ 32 ]; snprintf(voltText, 32, "%.1f V", 22.0f + ( battery / 100.0f ) * 3.2f);
		DrawTextShadow(dl, font, 16.0f, ImVec2(x, y), IM_COL32(180, 180, 180, 255), voltText);

		y += 20.0f;
		char fpsText[ 32 ]; snprintf(fpsText, 32, "FPS %d", ( int ) io.Framerate);
		DrawTextShadow(dl, font, 16.0f, ImVec2(x, y), IM_COL32(180, 180, 180, 255), fpsText);

		auto& replay = m_app->getmscene().getDron().getreplay();
		if ( replay.isRecordingbool() ) {
			y += 25.0f;
			if ( ( int ) ( ImGui::GetTime() * 2 ) % 2 == 0 ) {
				DrawTextShadow(dl, font, 18.0f, ImVec2(x, y), c_red, "REC");
			}
		}
	}

	// Compass
	{
		float tapeY = 45.0f;
		float tapeW = 450.0f;
		float centerX = s.x * 0.5f;
		float pxPerDeg = 4.5f;

		float headingDeg = -glm::degrees(drone->getYaw());
		headingDeg = fmod(headingDeg, 360.0f);
		if ( headingDeg < 0.0f ) headingDeg += 360.0f;

		dl->AddLine(ImVec2(centerX - tapeW / 2, tapeY), ImVec2(centerX + tapeW / 2, tapeY), c_subtle_line, 1.0f);
		dl->PushClipRect(ImVec2(centerX - tapeW / 2 - 20, 0), ImVec2(centerX + tapeW / 2 + 20, 150));

		float indSize = 8.0f;
		float indY = tapeY + 22.0f;
		dl->AddTriangleFilled(ImVec2(centerX, indY + 2), ImVec2(centerX - indSize, indY - indSize + 2), ImVec2(centerX + indSize, indY - indSize + 2), c_tech_orange);
		dl->AddLine(ImVec2(centerX, indY + 2), ImVec2(centerX, indY + 12), c_tech_orange, 2.0f);

		int range = ( int ) ( tapeW / pxPerDeg / 2 ) + 10;
		int startI = ( int ) floor(headingDeg) - range;
		int endI = ( int ) ceil(headingDeg) + range;

		for ( int i = startI; i <= endI; i++ ) {

			int displayDeg = i;
			while ( displayDeg < 0 ) displayDeg += 360;
			while ( displayDeg >= 360 ) displayDeg -= 360;

			if ( displayDeg % 15 == 0 ) {
				float diff = ( float ) i - headingDeg;
				float xPos = centerX + ( diff * pxPerDeg );

				bool isCardinal = ( displayDeg % 90 == 0 );

				float h = isCardinal ? 14.0f : 8.0f;
				float thick = isCardinal ? 2.0f : 1.5f;
				ImU32 col = isCardinal ? c_tech_cyan : c_tech_white;
				float textYOffset = isCardinal ? 28.0f : 22.0f;

				dl->AddLine(ImVec2(xPos, tapeY), ImVec2(xPos, tapeY + h), col, thick);
				if ( isCardinal ) {
					dl->AddCircleFilled(ImVec2(xPos, tapeY + h + 2), 2.0f, col);
				}

				if ( isCardinal ) {
					const char* txt = "N";
					if ( displayDeg == 90 ) txt = "E";
					else if ( displayDeg == 180 ) txt = "S";
					else if ( displayDeg == 270 ) txt = "W";

					ImVec2 tSz = font->CalcTextSizeA(22.0f, FLT_MAX, 0.0f, txt);
					DrawTextShadow(dl, font, 22.0f, ImVec2(xPos - tSz.x / 2, tapeY - textYOffset), col, txt);
				}
				else {
					char num[ 8 ]; snprintf(num, 8, "%d", displayDeg);
					ImVec2 tSz = font->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, num);
					DrawTextShadow(dl, font, 15.0f, ImVec2(xPos - tSz.x / 2, tapeY - textYOffset), c_tech_white, num);
				}
			}
			else if ( displayDeg % 5 == 0 ) {
				float diff = ( float ) i - headingDeg;
				float xPos = centerX + ( diff * pxPerDeg );
				dl->AddCircleFilled(ImVec2(xPos, tapeY + 2), 1.5f, c_subtle_line);
			}
		}
		dl->PopClipRect();
	}
}

void Menu::RenderLowSignalOverlay()
{
	static bool showLowSignal = true;
	static float glitchTimer = 1.0f;
	static float blinkTimer = 1.0f;

	if ( simulationPaused || showMainMenu || isLoading ) return;
	if ( !showLowSignal ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 screenSize = io.DisplaySize;
	ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
	ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
	float currentTime = glfwGetTime();

	glitchTimer += io.DeltaTime;

	if ( glitchTimer > 0.03f )
	{
		glitchTimer = 0.0f;
		int numGlitches = 8 + ( rand() % 8 );
		for ( int i = 0; i < numGlitches; ++i )
		{
			float w = ( rand() % 450 ) + 50.0f;
			float h = ( rand() % 25 ) + 3.0f;
			float x, y;
			if ( screenSize.x != 0 ) x = rand() % ( int ) screenSize.x; else x = 0;
			if ( screenSize.y != 0 ) y = rand() % ( int ) screenSize.y; else y = 0;
			ImU32 glitchColor = IM_COL32(255, 0, 200, 50 + ( rand() % 60 ));
			fgDrawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), glitchColor);

			if ( rand() % 10 == 0 )
			{
				float bigW = screenSize.x;
				float bigH = ( rand() % 80 ) + 20.0f;
				float bigY = rand() % ( int ) ( screenSize.y - bigH );
				ImU32 bigGlitchColor = IM_COL32(0, 255, 255, 10 + ( rand() % 40 ));
				bgDrawList->AddRectFilled(ImVec2(0, bigY), ImVec2(bigW, bigY + bigH), bigGlitchColor);
			}
		}
	}

	blinkTimer = fmod(currentTime, 1.0f);
	float blinkAlpha = ( blinkTimer < 0.5f ) ? 1.0f : 0.0f;

	if ( blinkAlpha > 0.0f )
	{
		const char* text = "LOW SIGNAL";
		ImFont* font = io.Fonts->Fonts[ 0 ];
		float fontSize = 80.0f;

		ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
		ImVec2 textPos = ImVec2(
			screenSize.x * 0.5f - textSize.x * 0.5f,
			screenSize.y * 0.5f - textSize.y * 0.5f
		);

		ImU32 finalColor = IM_COL32(255, 0, 0, ( int ) ( 255 * blinkAlpha ));
		fgDrawList->AddText(font, fontSize, textPos, finalColor, text);
	}
}

void Menu::RenderConnectionLostOverlay()
{
	static bool showConnectionLost = true;
	static float glitchTimer = 1.0f;
	static float blinkTimer = 1.0f;

	if ( simulationPaused || showMainMenu || isLoading ) return;
	if ( !showConnectionLost ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 screenSize = io.DisplaySize;
	ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
	ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
	float currentTime = glfwGetTime();


	bgDrawList->AddRectFilled(ImVec2(0, 0), screenSize, IM_COL32(0, 0, 0, 255));

	glitchTimer += io.DeltaTime;

	if ( glitchTimer > 0.02f )
	{
		glitchTimer = 0.0f;
		int numGlitches = 15 + ( rand() % 10 );
		for ( int i = 0; i < numGlitches; ++i )
		{
			float w = screenSize.x;
			float h = ( rand() % 40 ) + 5.0f;
			float x = 0;
			float y;
			if ( screenSize.y != 0 ) y = rand() % ( int ) screenSize.y; else y = 0;

			ImU32 glitchColor = IM_COL32(255, 0, 255, 100 + ( rand() % 100 ));
			fgDrawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), glitchColor);

			if ( rand() % 8 == 0 )
			{
				float bigW = ( rand() % 100 ) + 10.0f;
				float bigH = screenSize.y;
				float bigX = rand() % ( int ) ( screenSize.x - bigW );
				ImU32 bigGlitchColor = IM_COL32(0, 255, 255, 80 + ( rand() % 80 ));
				fgDrawList->AddRectFilled(ImVec2(bigX, 0), ImVec2(bigX + bigW, bigH), bigGlitchColor);
			}
		}
	}


	blinkTimer = fmod(currentTime, 1.0f);

	const char* text = "CONNECTION LOST";
	ImFont* font = io.Fonts->Fonts[ 0 ];
	float fontSize = 100.0f;
	float blinkAlpha = ( blinkTimer < 0.5f ) ? 1.0f : 0.0f;
	ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);

	ImVec2 textPos = ImVec2(
		screenSize.x * 0.5f - textSize.x * 0.5f,
		screenSize.y * 0.5f - textSize.y * 0.5f
	);

	if ( blinkAlpha > 0.0f )
	{
		ImFont* font = io.Fonts->Fonts[ 0 ];
		float fontSize = 100.0f;
		ImU32 finalColor = IM_COL32(255, 20, 20, ( int ) ( 255 * blinkAlpha ));
		fgDrawList->AddText(font, fontSize, textPos, finalColor, text);
	}

	ImVec2 buttonWindowPos(screenSize.x * 0.5f - 300.0f, textPos.y + textSize.y + 20.0f);
	ImGui::SetNextWindowPos(buttonWindowPos);
	ImGui::SetNextWindowSize(ImVec2(500, 150));

	ImGui::Begin("ReturnHomeButtonWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
	ImGui::End();

	ImGui::SetNextWindowPos(buttonWindowPos);
	ImGui::SetNextWindowSize(ImVec2(500, 150));
	ImGui::Begin("ReturnHomeAction", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::SetCursorPosX(( 500 - ImGui::CalcTextSize("Return Home").x - ImGui::GetStyle().FramePadding.x * 2 ) * 0.5f);

	ImGui::SetWindowFontScale(1.5f);

	if ( ImGui::Button("Return Home", ImVec2(200, 40)) )
	{
		m_app->getmscene().getWorld().reset();
		m_app->getmscene().getDron().reset(m_app->getmscene().getWorld().getHomePosition(), m_app->getmscene().getWorld().getHomeRotation());
	}

	ImGui::SetWindowFontScale(1.0f);
	ImGui::End();
}

void Menu::RenderReplay()
{
	if ( !m_app->getReplay() ) return;

	auto& replaySystem = m_app->getmscene().getDron().getreplay();
	float totalTime = replaySystem.getTotalTime();
	float currentTime = replaySystem.getReplayTime();
	bool isPaused = m_app->getmscene().getDron().getsopreplay();

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 screenSize = io.DisplaySize;

	ImVec4 col_bg = ImVec4(0.08f, 0.08f, 0.08f, 0.90f);
	ImVec4 col_accent = ImVec4(0.0f, 0.85f, 1.0f, 1.0f);
	ImVec4 col_accent_h = ImVec4(0.2f, 0.9f, 1.0f, 1.0f);
	ImVec4 col_text_dim = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	ImVec4 col_slider_bg = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	ImVec4 col_btn_bg = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);

	float panelWidth = 650.0f;
	float panelHeight = 100.0f;
	float margin = 20.0f;

	ImGui::SetNextWindowPos(ImVec2(( screenSize.x - panelWidth ) * 0.5f, screenSize.y - panelHeight - 40));
	ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, col_bg);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));

	ImGui::Begin("ReplayPlayer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);

	ImGui::SetCursorPosY(15.0f);
	ImGui::SetCursorPosX(margin);

	ImGui::PushStyleColor(ImGuiCol_Text, col_accent);
	ImGui::Text("%s", FormatTime(currentTime).c_str());
	ImGui::PopStyleColor();

	ImGui::SameLine();

	float timeTextWidth = 50.0f;
	float sliderWidth = panelWidth - ( margin * 2 ) - ( timeTextWidth * 2 ) - 20.0f;

	ImGui::PushStyleColor(ImGuiCol_FrameBg, col_slider_bg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, col_slider_bg);
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, col_accent);
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 100.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 14.0f);

	float seekTime = currentTime;
	ImGui::SetNextItemWidth(sliderWidth);

	if ( ImGui::SliderFloat("##Timeline", &seekTime, 0.0f, totalTime, "") )
	{
		replaySystem.setReplayTime(seekTime);
		if ( !isPaused ) m_app->getmscene().getDron().stopReplay(true);
	}

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(4);

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, col_text_dim);
	ImGui::Text("%s", FormatTime(totalTime).c_str());
	ImGui::PopStyleColor();

	ImGui::SetCursorPosY(50.0f);

	float bigBtnSize = 40.0f;
	float smlBtnSize = 30.0f;
	float spacing = 15.0f;
	float groupWidth = 50.0f + smlBtnSize + 40.0f + bigBtnSize + ( spacing * 4 );
	ImGui::SetCursorPosX(( panelWidth - groupWidth ) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, col_btn_bg);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

	if ( ImGui::Button("|<", ImVec2(smlBtnSize, smlBtnSize)) ) {
		replaySystem.setReplayTime(0.0f);
	}
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip("Reiniciar");
	ImGui::SameLine(0, spacing);

	float currentSpeed = replaySystem.getPlaybackSpeed();
	char speedText[ 16 ];
	snprintf(speedText, 16, "x%.1f", currentSpeed);

	if ( currentSpeed != 1.0f ) ImGui::PushStyleColor(ImGuiCol_Text, col_accent);
	else ImGui::PushStyleColor(ImGuiCol_Text, col_text_dim);

	if ( ImGui::Button(speedText, ImVec2(50, smlBtnSize)) ) {
		float newSpeed = ( currentSpeed == 1.0f ) ? 2.0f : ( currentSpeed == 2.0f ) ? 0.5f : 1.0f;
		replaySystem.setPlaybackSpeed(newSpeed);
	}
	ImGui::PopStyleColor();
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip("Velocitat");

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar();

	ImGui::SameLine(0, spacing);

	ImGui::PushStyleColor(ImGuiCol_Button, col_accent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col_accent_h);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);

	const char* playIcon = isPaused ? " > " : " || ";

	if ( ImGui::Button(playIcon, ImVec2(60, bigBtnSize)) ) {
		if ( isPaused ) m_app->getmscene().getDron().stopReplay(false);
		else m_app->getmscene().getDron().stopReplay(true);
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);
	ImGui::SameLine(0, spacing * 2.0f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));

	if ( ImGui::Button("EXIT", ImVec2(60, smlBtnSize)) ) {
		m_app->setReplay(false);
		m_app->getmscene().getDron().setReplay(false);
		m_app->getmscene().getDron().stopReplay(true);
		ResetSimulation(m_app);
	}
	ImGui::PopStyleColor(2);

	ImGui::End();

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

void Menu::RenderMapSelection()
{
	if ( !showMapSelection )
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImVec2 display_size = io.DisplaySize;
	const ImVec2 window_size(850, 720);

	if ( mapTextures.empty() )
	{
		Texture* temp;
		temp = new Texture("Foto_Mapes/Estadio.png", false);
		mapTextures.push_back(temp);
		temp = new Texture("Foto_Mapes/MapaCiutat.png", false);
		mapTextures.push_back(temp);
	}
	std::vector<std::string> mapNames = { "RACING DRONES STADIUM", "CITY" };

	if ( backgroundTex )
	{
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		draw_list->AddImage(bgID, ImVec2(0, 0), display_size, ImVec2(0, 0), ImVec2(1, 1),
			IM_COL32(255, 255, 255, 255));
	}
	else
	{
		draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f,
		display_size.y * 0.5f - window_size.y * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

	ImGui::Begin("Map Selection", &showMapSelection, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, 30.0f);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));
	TextCentered("CHOOSE MAP");
	ImGui::PopStyleColor();

	ImVec2 line_start = ImGui::GetCursorScreenPos();
	line_start.x += 50;
	line_start.y += 10;
	ImVec2 line_end = ImVec2(line_start.x + window_size.x - 100, line_start.y);
	ImGui::GetWindowDrawList()->AddLine(line_start, line_end, ImColor(0.30f, 0.60f, 0.90f, 1.0f), 3.0f);

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50);

	const float mapImageWidth = 180.0f;
	const float mapButtonWidth = 750.0f;
	const float mapButtonHeight = 150.0f;
	const float spacingY = 20.0f;

	ImGui::SetCursorPosX(( window_size.x - mapButtonWidth ) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.95f, 1.0f, 1.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	for ( size_t i = 0; i < mapTextures.size() && i < mapNames.size(); ++i )
	{
		ImGui::SetCursorPosX(( window_size.x - mapButtonWidth ) * 0.5f);

		char label_buf[ 32 ];
		sprintf_s(label_buf, sizeof(label_buf), "##MapBtn%zu", i);

		if ( ImGui::Button(label_buf, ImVec2(mapButtonWidth, mapButtonHeight)) )
		{
			AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
			selectedMap = i;
		}

		ImVec2 pMin = ImGui::GetItemRectMin();
		ImVec2 pMax = ImGui::GetItemRectMax();

		if ( selectedMap == i )
		{
			ImGui::GetWindowDrawList()->AddRect(pMin, pMax, IM_COL32(255, 223, 0, 255), 5.0f, 0, 3.0f);
		}
		else if ( ImGui::IsItemHovered() )
		{
			ImGui::GetWindowDrawList()->AddRectFilled(pMin, pMax, ImColor(0.25f, 0.5f, 0.85f, 0.2f));
		}

		GLuint texID = mapTextures[ i ]->getID();
		ImTextureID mapID = ( ImTextureID ) ( intptr_t ) texID;
		float imageRatio = mapButtonHeight / mapTextures[ i ]->getHeight();
		float scaledImageWidth = mapTextures[ i ]->getWidth() * imageRatio;

		float finalImageWidth = std::fmin(mapImageWidth, scaledImageWidth);

		ImVec2 image_pMin = ImVec2(pMin.x + 10, pMin.y + 10);
		ImVec2 image_pMax = ImVec2(image_pMin.x + finalImageWidth, pMax.y - 10);

		ImGui::GetWindowDrawList()->AddImage(mapID, image_pMin, image_pMax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));

		const std::string& mapName = mapNames[ i ];
		ImVec2 textSize = ImGui::CalcTextSize(mapName.c_str());

		float textX = image_pMax.x + 30;
		float textY = pMin.y + ( mapButtonHeight - textSize.y ) * 0.5f;

		ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY), IM_COL32(217, 242, 255, 255), mapName.c_str());

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacingY);
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar(3);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);

	const ImVec2 backButtonSize(350, 75);
	ImGui::SetCursorPosX(( window_size.x - backButtonSize.x ) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 25));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	if ( ImGui::Button("BACK", backButtonSize) )
	{
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showMapSelection = false;
		showMainMenu = true;
	}

	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(4);

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(1);
}

std::string Menu::FormatTime(float timeInSeconds)
{
	if ( timeInSeconds < 0.0f ) timeInSeconds = 0.0f;

	int totalSeconds = static_cast< int >(timeInSeconds);
	int minutes = totalSeconds / 60;
	int seconds = totalSeconds % 60;
	int milliseconds = static_cast< int >(( timeInSeconds - totalSeconds ) * 100);

	char buffer[ 16 ];
	snprintf(buffer, sizeof(buffer), "%02d:%02d.%02d", minutes, seconds, milliseconds);

	return std::string(buffer);
}

void Menu::LoadDroneTextures() {
	if ( !droneTextures.empty() ) return;

	droneTextures.push_back(new Texture("Fotos_Dron/og_drone.png", false));
	droneTextures.push_back(new Texture("Fotos_Dron/dji_drone.png", false));
	droneTextures.push_back(new Texture("Fotos_Dron/aggressive_drone.png", false));
}

void Menu::RenderCredits()
{
	if ( !showCredits ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImVec2 display_size = io.DisplaySize;
	const ImVec2 window_size(850, 720);
	const float credits_height = 1200.0f;

	if ( backgroundTex )
	{
		ImTextureID bgID = ( ImTextureID ) ( intptr_t ) backgroundTex->getID();
		draw_list->AddImage(bgID, ImVec2(0, 0), ImVec2(display_size.x, display_size.y), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
	}
	else
	{
		draw_list->AddRectFilledMultiColor(ImVec2(0, 0), display_size, ImColor(0.04f, 0.04f, 0.06f, 1.0f), ImColor(0.06f, 0.08f, 0.12f, 1.0f), ImColor(0.08f, 0.10f, 0.15f, 1.0f), ImColor(0.05f, 0.05f, 0.08f, 1.0f));
	}

	ImGui::SetNextWindowSize(window_size);
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f, display_size.y * 0.5f - window_size.y * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 40));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::Begin("Credits Container", &showCredits, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const float scrollSpeed = 40.0f;
	float time_offset = fmodf(ImGui::GetTime() * scrollSpeed, credits_height) + 115.0f;

	ImGui::BeginChild("CreditsScroll", ImVec2(window_size.x - 80, window_size.y - 200), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

	auto DrawCreditsContent = [ & ] (float offset)
		{
			ImGui::SetCursorPosY(offset);
			ImGui::PushFont(io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL));

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
			TextCentered("DEVELOPMENT TEAM");
			ImGui::PopStyleColor();
			TextCentered("Marti Bertrans");
			TextCentered("Marc Cantero");
			TextCentered("Bernat Domene");
			TextCentered("Helena Ruiz");
			TextCentered("Marti Serra");
			TextCentered("Aina Vidal");

			ImGui::NewLine(); ImGui::NewLine();

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
			TextCentered("SPECIAL THANKS");
			ImGui::PopStyleColor();
			TextCentered("Universitat autonoma de Barcelona");
			TextCentered("Escola d'enginyeria UAB");
			TextCentered("ImGui Library");
			TextCentered("Bullet Library");
			TextCentered("json Library");
			TextCentered("Miniaudio Library");
			TextCentered("tinygltf and tinyobj Libraries");
			TextCentered("quatern Library");
			TextCentered("stbimage Library");
			TextCentered("OpenGL Community");
			ImGui::NewLine(); ImGui::NewLine();

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.60f, 0.90f, 1.0f));
			TextCentered("VERSION");
			ImGui::PopStyleColor();
			TextCentered("DRONE FLIGHT SIM v1.0.0");
			ImGui::NewLine(); ImGui::NewLine();

			ImGui::Dummy(ImVec2(1.0f, 400.0f));

			ImGui::PopFont();
		};

	DrawCreditsContent(window_size.y - time_offset);
	DrawCreditsContent(window_size.y - time_offset + credits_height);

	ImGui::EndChild();

	ImGui::SetCursorPosY(window_size.y - 80);

	const ImVec2 backButtonSize(350, 75);
	ImGui::SetCursorPosX(( window_size.x - backButtonSize.x ) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.85f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.75f, 0.20f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.35f, 0.65f, 0.5f));

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 25));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	g_TitleFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_2, FONT_SIZE_SMALL);
	if ( g_TitleFont ) ImGui::PushFont(g_TitleFont);

	if ( ImGui::Button("BACK TO MENU", backButtonSize) )
	{
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		showCredits = false;
		showMainMenu = true;
	}

	if ( ImGui::IsItemHovered() ) {
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();
		ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.25f, 0.5f, 0.85f, 0.2f));
	}

	if ( g_TitleFont ) ImGui::PopFont();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(4);

	ImGui::End();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
}

void Menu::ResetSimulation(Application* app)
{
	simulationPaused = true;
	showMainMenu = true;
	showSettings = false;
	showPastSimulations = false;

	isLoading = false;
	loadingProgress = 0.0f;

	if ( app ) {
		app->getmscene().reset();
	}
}

void Menu::RenderRaceFinishedOverlay()
{
	if ( !isrenderfinal ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 s = io.DisplaySize;

	// Usamos BackgroundDrawList para que el fondo negro no "manche" los botones
	ImDrawList* dl_back = ImGui::GetBackgroundDrawList();
	ImFont* font = io.Fonts->Fonts[ 0 ];

	// 1. Fondo Oscuro
	dl_back->AddRectFilled(ImVec2(0, 0), s, IM_COL32(0, 0, 0, 220));

	auto* raceManager = m_app->getmscene().getWorld().getRaceManager();
	if ( !raceManager ) return;

	// --- 2. TÍTULO Y TIEMPO ---
	std::string finishTimeStr = FormatTime(raceManager->getCurrentTime());

	static float blink_timer = 0.0f;
	blink_timer += io.DeltaTime;
	float blink_value = ( std::sin(blink_timer * 3.0f * 6.283f) * 0.5f ) + 0.5f;
	int current_alpha = 150 + ( int ) ( 105 * blink_value );

	// Título (Race Finished)
	float titleFontSize = 90.0f;
	ImVec2 titleSz = font->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, "RACE FINISHED");
	ImVec2 titlePos = ImVec2(( s.x - titleSz.x ) * 0.5f, s.y * 0.5f - 220.0f);
	DrawTextShadow(dl_back, font, titleFontSize, titlePos, IM_COL32(0, 255, 255, current_alpha), "RACE FINISHED");

	// Tiempo Final
	float timeFontSize = 130.0f;
	ImVec2 timeSz = font->CalcTextSizeA(timeFontSize, FLT_MAX, 0.0f, finishTimeStr.c_str());
	ImVec2 timePos = ImVec2(( s.x - timeSz.x ) * 0.5f, s.y * 0.5f - 100.0f);
	DrawTextShadow(dl_back, font, timeFontSize, timePos, IM_COL32(255, 255, 255, 255), finishTimeStr.c_str());

	// Línea Divisoria Tecnológica
	float lineY = s.y * 0.5f + 20.0f;
	float lineW = 400.0f;
	dl_back->AddLine(ImVec2(s.x * 0.5f - lineW, lineY), ImVec2(s.x * 0.5f + lineW, lineY), IM_COL32(0, 255, 255, 180), 2.0f);

	// --- 3. VENTANA DE BOTONES ---
	float buttonW = 350.0f;
	float buttonH = 60.0f; // Un poco más altos para que quepa bien el texto grande

	ImGui::SetNextWindowPos(ImVec2(s.x * 0.5f, lineY + 50.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));

	ImGui::Begin("##RaceFinishButtons", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoScrollbar);

	// --- AQUÍ DEFINIMOS EL TAMAÑO DE LETRA PARA LOS BOTONES ---
	float buttonFontSize = 32.0f;
	ImGui::SetWindowFontScale(buttonFontSize / ImGui::GetFontSize());

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 20)); // Más espacio entre ellos

	// Colores Cyan Brillantes
	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 100, 110, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 255, 255, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 200, 200, 255));
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 255, 255, 255));
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));

	if ( ImGui::Button("KEEP FLYING", ImVec2(buttonW, buttonH)) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		isrenderfinal = false;
		simulationPaused = false;
	}

	if ( ImGui::Button("SAVE REPLAY", ImVec2(buttonW, buttonH)) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		m_app->getmscene().getDron().getreplay().saveReplay();
	}

	if ( ImGui::Button("RESTART RACE", ImVec2(buttonW, buttonH)) ) {
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		isrenderfinal = false;
		simulationPaused = false;
		m_app->getmscene().getDron().getreplay().setreplayzero();
		m_app->getmscene().setsaved(false);
		m_app->getmscene().getWorld().reset();
		m_app->getmscene().getDron().reset(m_app->getmscene().getWorld().getHomePosition(), m_app->getmscene().getWorld().getHomeRotation());
	}

	// Estilo para el botón de Salir (Rojo brillante)
	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(150, 40, 40, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 50, 50, 255));
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 100, 100, 255));

	if ( ImGui::Button("RETURN TO MENU", ImVec2(buttonW, buttonH)) ) {
		isrenderfinal = false;
		showMainMenu = true;
		AudioManager::Get().PlaySFX("Audio/buto_menu.mp3");
		AudioManager::Get().PlayMusic("Audio/musica_menu.mp3");
		ResetSimulation(m_app);
	}

	// Resetear escalas y pops
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor(8);
	ImGui::PopStyleVar(3);
	ImGui::End();
}

void Menu::RenderLowBatteryOverlay()
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 screenSize = io.DisplaySize;
	ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
	ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();

	// 1. FONS TOTALMENT NEGRE
	bgDrawList->AddRectFilled(ImVec2(0, 0), screenSize, IM_COL32(0, 0, 0, 255));

	// 2. TEXT D'ALERTA (Actualitzat a "BATTERY DEPLETED")
	const char* text = "BATTERY DEPLETED";
	ImFont* font = io.Fonts->Fonts[ 0 ];
	float fontSize = 80.0f;
	ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);

	ImVec2 textPos = ImVec2(
		( screenSize.x - textSize.x ) * 0.5f,
		( screenSize.y * 0.4f ) - ( textSize.y * 0.5f )
	);

	fgDrawList->AddText(font, fontSize, textPos, IM_COL32(255, 30, 30, 255), text);

	// 3. BOTÓ DE REINICIAR
	ImVec2 buttonSize(300, 60);
	ImVec2 buttonPos = ImVec2(
		( screenSize.x - buttonSize.x ) * 0.5f,
		( screenSize.y * 0.6f ) - ( buttonSize.y * 0.5f )
	);

	ImGui::SetNextWindowPos(buttonPos);
	ImGui::SetNextWindowSize(buttonSize);

	ImGui::Begin("BatteryRestartAction", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 40, 40, 200));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 60, 60, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(20, 20, 20, 255));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

	if ( ImGui::Button("RESTART SIMULATION", ImVec2(-1, -1)) )
	{
		if ( m_app ) {
			m_app->getmscene().getWorld().reset();
			m_app->getmscene().getDron().reset(
				m_app->getmscene().getWorld().getHomePosition(),
				m_app->getmscene().getWorld().getHomeRotation()
			);
			// Al reiniciar, l'audio es tornarà a activar al run() de l'Application
		}
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);
	ImGui::End();
}

void Menu::RenderLowBattery2Overlay()
{
	static float glitchTimer = 0.0f;
	static float blinkTimer = 0.0f;

	// No mostrem l'overlay si el joc està pausat o al menú
	if ( simulationPaused || showMainMenu || isLoading ) return;

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 screenSize = io.DisplaySize;
	ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
	ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
	float currentTime = glfwGetTime();

	// 1. LÒGICA DE GLITCH ELÈCTRIC (Vermells i Taronges)
	glitchTimer += io.DeltaTime;

	if ( glitchTimer > 0.04f ) // Una mica més lent que el de ràdio per semblar fallada elèctrica
	{
		glitchTimer = 0.0f;
		int numGlitches = 5 + ( rand() % 5 );
		for ( int i = 0; i < numGlitches; ++i )
		{
			float w = ( rand() % 300 ) + 100.0f;
			float h = ( rand() % 15 ) + 2.0f;
			float x = ( screenSize.x != 0 ) ? ( rand() % ( int ) screenSize.x ) : 0;
			float y = ( screenSize.y != 0 ) ? ( rand() % ( int ) screenSize.y ) : 0;

			// Color vermell/taronja amb poca opacitat (estil fallada d'energia)
			ImU32 glitchColor = IM_COL32(255, 50, 0, 40 + ( rand() % 50 ));
			fgDrawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), glitchColor);

			// Línies horitzontals de "flicker" negre (micro-apagades)
			if ( rand() % 5 == 0 )
			{
				float lineW = screenSize.x;
				float lineH = ( rand() % 5 ) + 1.0f;
				float lineY = ( screenSize.y != 0 ) ? ( rand() % ( int ) screenSize.y ) : 0;
				bgDrawList->AddRectFilled(ImVec2(0, lineY), ImVec2(lineW, lineY + lineH), IM_COL32(0, 0, 0, 100));
			}
		}
	}

	// 2. TEXT D'ALERTA PARPELLEJANT
	blinkTimer = fmod(currentTime, 0.6f); // Parpelleig més ràpid (urgent!)
	float blinkAlpha = ( blinkTimer < 0.3f ) ? 1.0f : 0.0f;

	if ( blinkAlpha > 0.0f )
	{
		const char* text = "LOW BATTERY - LAND NOW";
		ImFont* font = io.Fonts->Fonts[ 0 ];
		float fontSize = 70.0f;

		ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
		ImVec2 textPos = ImVec2(
			screenSize.x * 0.5f - textSize.x * 0.5f,
			screenSize.y * 0.3f - textSize.y * 0.5f // Una mica més amunt del centre
		);

		// Color taronja d'advertència
		ImU32 finalColor = IM_COL32(255, 140, 0, 255);
		fgDrawList->AddText(font, fontSize, textPos, finalColor, text);
	}
}