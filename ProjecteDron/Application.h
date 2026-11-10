#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Scene.h"
#include "Controller.h"
#include "KeyboardController.h"
#include "GamepadController.h"
#include "ReplaySystem.h"
#include "Audio.h"

class Application
{
public:
    Application();
    ~Application();    

	bool getReplay() { return m_ModoReplay; }
	
	void ToggleFullscreen(bool fs, bool init = false);
    void run();
    void setScreenSize(int width, int height);
    void setReplay(bool replay) { m_ModoReplay = replay; }
	void setController(ControllerType type);
    
	Scene& getmscene() { return m_scene; }
    GLFWwindow* getWindow() const { return m_window; }
    ReplaySystem m_replay;
    Audio& getAudio() { return m_audio; }
	Audio& getSFXAudio() { return m_sfx_audio; }

	int m_screen_width;
	int m_screen_height;
	int m_windowedPosX;
	int m_windowedPosY;
	int m_windowedWidth;
	int m_windowedHeight;

private:
	bool m_isFullscreen;
    GLFWwindow* m_window;
    Scene m_scene;
    Controller* m_controller;
    int gamepad;
    int Mode;
    bool m_ModoReplay;
    void initOpenGL();
    Audio m_audio;
	Audio m_sfx_audio; 
};