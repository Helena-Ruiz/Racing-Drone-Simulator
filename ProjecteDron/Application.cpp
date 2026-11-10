#define GLM_ENABLE_EXPERIMENTAL
#include "Application.h"
#include "Menu.h"
#include "ImGuiManager.h"
#include "AudioManager.h"

Application::Application()
	: m_window(nullptr),
	gamepad(0),
	Mode(0),
	m_windowedPosX(0),
	m_windowedPosY(0),
	m_ModoReplay(false)
{
	m_screen_width = 1920;
	m_screen_height = 1080;

	initOpenGL();

	m_scene = Scene(static_cast< float >( m_screen_width ) / m_screen_height);
	m_scene.setApp(this);
	Menu::m_app = this;
	KeyboardController keyboardController;
	GamepadController gamepadController(GLFW_JOYSTICK_1);

	SettingsData cfg = Menu::carregarConfiguracio();
	
	Menu::selectedMap = cfg.selectedMap;
	Menu::selectedGamepad = cfg.selectedGamepad;
	Menu::selectedMode = static_cast< MODE_VOL >( cfg.selectedMode );
	Menu::volume = cfg.volume;
	Menu::qualityPreset = cfg.qualityPreset;
	Menu::resolution_index = cfg.resolutionIndex;
	Menu::fullscren = cfg.m_isFullscreen;
	Menu::applySettings(cfg);

	AudioManager::Get().Init();
	AudioManager::Get().SetMasterVolume(Menu::volume);

	if ( Menu::selectedGamepad == 0 ) setController(ControllerType::CONTROLLER_PS4);
	else if ( Menu::selectedGamepad == 1 ) setController(ControllerType::CONTROLLER_DRONE);
	else setController(ControllerType::KEYBOARD);
	gamepad = Menu::selectedGamepad;
}

Application::~Application()
{
	if ( m_window )
		glfwDestroyWindow(m_window);

	AudioManager::Get().Cleanup();
	glfwTerminate();
}

void Application::ToggleFullscreen(bool fs, bool init)
{
	if ( !init && fs == m_isFullscreen ) return;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if ( fs )
	{
		glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
		glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

		glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);
		glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

		m_screen_width = mode->width;
		m_screen_height = mode->height;
		m_isFullscreen = true;
	}
	else
	{
		int posX = ( mode->width - m_windowedWidth ) / 2;
		int posY = ( mode->height - m_windowedHeight ) / 2;

		glfwSetWindowMonitor(m_window, nullptr, posX, posY, m_windowedWidth, m_windowedHeight, 0);
		glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);

		m_screen_width = m_windowedWidth;
		m_screen_height = m_windowedHeight;
		m_isFullscreen = false;
	}

	m_scene.setAspectRatio(static_cast< float >( m_screen_width ) / static_cast< float >( m_screen_height ));
}

void Application::setScreenSize(int width, int height)
{
	m_screen_width = width;
	m_screen_height = height;
	m_scene.setAspectRatio(( float ) width / ( float ) height);
}

void Application::setController(ControllerType type)
{
	if ( m_controller ) {
		delete m_controller;
		m_controller = nullptr;
	}

	switch (type)
	{
	case ControllerType::CONTROLLER_PS4:
		m_controller = new GamepadController(GLFW_JOYSTICK_1);
		break;
	case ControllerType::CONTROLLER_DRONE:
		m_controller = new GamepadController(GLFW_JOYSTICK_1);
		break;
	case ControllerType::KEYBOARD:
		m_controller = new KeyboardController();
		break;
	default:
		std::cout << "[WARNING] Tipus de controlador desconegut. Usant Teclat." << std::endl;
		m_controller = new KeyboardController();
		m_controller->setType(ControllerType::KEYBOARD);
		break;
		break;
	}

	m_controller->setType(type);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->setScreenSize(width, height);
    }
}

void Application::initOpenGL()
{
	if ( !glfwInit() )
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	m_windowedWidth = static_cast< int >( mode->width * 0.9999 );
	m_windowedHeight = static_cast< int >( mode->height * 0.94 );
	m_windowedPosX = ( mode->width - m_windowedWidth ) / 2;
	m_windowedPosY = ( mode->height - m_windowedHeight ) / 2;

	m_screen_width = mode->width;
	m_screen_height = mode->height;

	m_window = glfwCreateWindow(m_screen_width, m_screen_height, "Simulador de Dron", NULL, NULL);

	ToggleFullscreen(m_isFullscreen, true);

	if ( !m_window )
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(m_window);

	if ( glewInit() != GLEW_OK )
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return;
	}

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
}

void Application::run() {
	glEnable(GL_DEPTH_TEST);
	float lastFrame = 0.0f;

	bool escKeyPressed = false;
	float escCooldown = 0.0f;

	Shader shader("Shaders/basic.vert", "Shaders/basic.frag");
	Menu::m_app = this;
	m_scene = Scene(static_cast< float >( m_screen_width ) / m_screen_height);
	m_scene.setApp(this);
	m_scene.setMode(Menu::selectedMode);
	ImGuiManager imguiManager(m_window);

	m_scene.setwindow(m_window);

	while ( !glfwWindowShouldClose(m_window) )
	{
		float currentFrame = glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		imguiManager.BeginFrame();

		// Input Handling
		bool currentEscPressed = glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

		if ( currentEscPressed && !escKeyPressed && escCooldown <= 0.0f && !Menu::showMainMenu && !Menu::showCredits 
			&& !Menu::showDroneCustomization && !Menu::showMapSelection && !Menu::showPastSimulations && !Menu::showSettings ) 
		{
			Menu::simulationPaused = !Menu::simulationPaused;
			escCooldown = 0.5f;
		}

		escKeyPressed = currentEscPressed;
		if ( escCooldown > 0.0f ) {
			escCooldown -= deltaTime;
		}

		// Update Scene
		if ( !Menu::simulationPaused && !Menu::showMainMenu && !Menu::isLoading ) {
			MODE_VOL currentMode = m_scene.getDron().getMode();

			if ( m_controller != nullptr) {
				m_controller->setMode(currentMode);
				m_controller->update();
				m_scene.update(deltaTime, *m_controller);
				m_replay = m_scene.getDron().getreplay();
			}
		}
		else
		{
			AudioManager::Get().UpdateDroneEngine(0,0, true, false);
		}

		// Rendering
		glClearColor(0.68f, 0.85f, 0.90f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if ( !Menu::showMainMenu && !Menu::simulationPaused && !Menu::isLoading ) {
			m_scene.draw(shader);
		}

		// UI Rendering
		if ( Menu::isLoading ) {
			Menu::RenderLoadingScreen();
		}
		else {
			Menu::RenderRaceFinishedOverlay();
			Menu::RenderMainMenu();
			Menu::RenderPauseMenu();
			Menu::RenderSettings();
			Menu::RenderDroneCustomization();
			Menu::RenderPastSimulations();
			Menu::RenderHUD();
			Menu::RenderReplay();
			Menu::RenderMapSelection();
			Menu::RenderCredits();
		}

		imguiManager.EndFrame();

		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}
}

int main() {
	Application app;
	app.run();
	return 0;
}