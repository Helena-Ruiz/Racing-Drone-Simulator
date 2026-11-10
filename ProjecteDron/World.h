#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <limits>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Camera.h"
#include "Model3D.h" 
#include "Light.h"
#include "Skybox.h"
#include "Curve.h"
#include "PhysicsWorld.h"
#include "ColliderLoader.h"
#include "Trigger.h"
#include "RaceManager.h"

class World
{
public:
    World();
    ~World();
    void initWorld(MapID mapId);

    // Render
    void draw(Shader& shader, Camera& camera);

    // Getters / Setters
    float getConectionLost() const { return m_conectionLost; }
    float getLowSignal() const { return m_lowSignal; }
	RaceManager* getRaceManager() { return m_raceManager; }
    MapID getMapaActual() const { return mapaActual; }
	glm::vec3 getHomePosition() const { return m_homePosition; }
	glm::vec3 getHomeRotation() const { return m_homeRotation; }

	void setMapActual(MapID mapa) { mapaActual = mapa; }

    void canviLlum();

    void addObject(Model3D* obj);
    void addLight(Light* light);
	void reset();

	//Metodes per física
	void loadPhysicsColliders(PhysicsWorld& physicsWorld);

private:

	glm::vec3 m_homePosition;
	glm::vec3 m_homeRotation;

    MapID mapaActual;

    // --- Neteja ---
    void clear();

    // --- C�rrega de Mapes ---
    void loadLevel(MapID mapIndex);
    void loadStadium();
    void loadCity();

    // --- Mapa ---
    Model3D* m_map = nullptr;
	Model3D* m_colliderMap = nullptr; // mapa simplificat per col·lisions
    std::vector<Model3D*> m_staticObjects;

	// --- Race Manager ---
	RaceManager* m_raceManager = nullptr;

	// --- Triggers ---
	std::vector<Trigger*> m_triggers;
	Shader* m_triggerShader = nullptr;

	// --- Llums ---
    std::vector<Light*> m_lights;

	// -- LLUMS SOL --
    Light m_dirLlum;
    bool es_dia;

    // --- Skybox ---
    Skybox* m_skybox = nullptr;

	// --- Corba del circuit ---
	Curve* m_curve = nullptr;
	Shader* m_curveShader = nullptr;
	std::vector<glm::vec3> m_trackPoints;

	// --- Paràmetres de connexió ---
    float m_conectionLost;
    float m_lowSignal;

	// --- Legacy buffers (si cal) ---
    unsigned int VAO, VBO;

	// --- Physics bodies ---
	std::vector<btRigidBody*> m_physicsBodies;
};