#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Camera.h"
#include "Dron.h"
#include "HUD.h"
#include "World.h" // Important per tenir acc�s a MapID
#include "Controller.h"
#include "Shader.h"
#include "PhysicsWorld.h"  //per gestionar collisions
class Application;

class Scene
{
public:
    Scene();
    Scene(float aspectRatio);
    ~Scene();

	void reset();
    void draw(Shader& shader);
    void update(float deltaTime, Controller& controller);

    void switchCamera(int index);
    void setCameraByAxis(float axisValue);
	void setModeByAxis(float axisValue);
	void setMode(MODE_VOL mode) { m_mode = mode; }
	// Inicialització dividida en mètodes específics
    void initWorld(MapID mapId);
    void initDron();
    void initShadowsMap();
	    
    Camera& getActiveCamera() { return *m_activeCamera; }
    Dron& getDron() { return m_dron; }
    World& getWorld() { return m_world; }
    void setAspectRatio(float aspect) { m_activeCamera->setAspectRatio(aspect); }
    void setwindow(GLFWwindow* win) { m_window = win; }
    void setApp(Application* app) { m_app = app; }
    void setreplay(bool modo) { m_replay = modo; }
    void initShadows();
	void setpaused(bool estat) { m_paused = estat; }
	void initPhysics();
	void setsaved(bool set) { saved = set; }

private:

    bool m_replay = getDron().getReplayActive();
    GLFWwindow* m_window;
    bool edit = false;
    
    // Objectes principals
    Dron m_dron;
    World m_world;
    HUD m_hud;
    
    MODE_VOL m_mode;
    bool m_prevCameraButtonState = false;;
    Application* m_app = nullptr;
    
	// Cameres
    Camera m_cameraFree;
    Camera m_cameraThirdPerson;
    Camera m_cameraFPV;
    Camera m_cameraEDIT;
    Camera* m_activeCamera;
    int m_currentCameraIndex = 0;

    // --- OMBRES ---
    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;
    static const unsigned int SHADOW_WIDTH = 2048;
    static const unsigned int SHADOW_HEIGHT = 2048;
    
    unsigned int staticDepthMapFBO = 0;
    unsigned int staticDepthMap = 0;
    
	bool m_paused = false;
    bool staticShadowsBaked = false; 

    Shader* simpleDepthShader;
    glm::mat4 m_lightSpaceMatrix;

    void drawSceneGeometry(Shader& shader);
    void bakeStaticShadows();

	PhysicsWorld m_physicsWorld;
	bool m_physicsInitialized;
	void cleanupPhysics();
	bool saved = false;
};