#include "World.h"
#include "stb_image.h"
#include "Menu.h"
#include <GL/glew.h>
#include <iostream>
#include "ColliderLoader.h"

// Constants de llum (les mantenim)
const glm::vec3 DAY_DIRECTION = glm::normalize(glm::vec3(0.5f, -1.0f, 0.25f));
const glm::vec3 DAY_COLOR = glm::vec3(1.6f, 1.55f, 1.45f);
const float DAY_INTENSITY = 6.0f;

const glm::vec3 NIGHT_DIRECTION = glm::normalize(glm::vec3(-0.2f, -0.45f, 0.3f));
const glm::vec3 NIGHT_COLOR = glm::vec3(0.30f, 0.35f, 0.50f);
const float NIGHT_INTENSITY = 0.85f;



World::World()
    : m_dirLlum(Light::LIGHT_TYPE::DIRECTIONAL, DAY_COLOR, DAY_INTENSITY, glm::vec3(0.0f), DAY_DIRECTION)
{
    VAO = 0;
    VBO = 0;
    m_conectionLost = 4000.0f;
    m_lowSignal = 1000.0f;
	m_homePosition = glm::vec3(0.0f, 0.0f, 1.0f);
	m_homeRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    es_dia = true;
    m_map = nullptr;
    m_skybox = nullptr;
}

World::~World()
{
    clear();
    // Neteja buffers legacy si cal
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

void World::clear()
{
    // 1. Esborrar mapa base
    if (m_map != nullptr) {
        delete m_map;
        m_map = nullptr;
    }   

    // 2. Esborrar objectes est�tics
    for (auto* obj : m_staticObjects) {
        delete obj;
    }
    m_staticObjects.clear();

    // 3. Esborrar llums extres
    for (auto* light : m_lights) {
        delete light;
    }
    m_lights.clear();

    // 4. Esborrar Skybox
    if (m_skybox != nullptr) {
        delete m_skybox;
        m_skybox = nullptr;
    }

	if ( m_triggerShader ) delete m_triggerShader;
	if ( m_curveShader ) delete m_curveShader;
}

void World::reset()
{
	m_raceManager->reset();

	for ( Trigger* t : m_triggers ) {
		t->reset();
	}

}

void World::initWorld(MapID mapId)
{
    // Per defecte carreguem l'estadi
    loadLevel(mapId);
	// Inicialitzar shader per a la corba del circuit
	m_curveShader = new Shader("Shaders/curve.vert", "Shaders/curve.frag");
	m_triggerShader = new Shader("Shaders/trigger.vert", "Shaders/trigger.frag");
}

void World::loadLevel(MapID mapIndex)
{
    // Netejar el nivell actual abans de carregar el nou
    clear();

    switch (mapIndex)
    {
    case MapID::MAP_STADIUM:
        std::cout << "Loading Stadium Map..." << std::endl;
        loadStadium();
        break;

    case MapID::MAP_CITY:
        std::cout << "Loading City Map..." << std::endl;
        loadCity();
        break;

    default:
        std::cout << "Map index not found, loading default." << std::endl;
        loadStadium();
        break;
    }
}

void World::loadStadium()
{
	//CONFIGURACIÓ
	m_lowSignal = 20.0f;
	m_conectionLost = 40.0f;
	m_homePosition = glm::vec3(0.0f, 0.5f, 0.0f);
	m_homeRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	if (m_raceManager)
	{
		m_raceManager->setTotalLaps(1);
	}

	//SKYBOX
	std::vector<std::string> cares{
		"Skybox/skybox_default/right.jpg",
		"Skybox/skybox_default/left.jpg",
		"Skybox/skybox_default/top.jpg",
		"Skybox/skybox_default/bottom.jpg",
		"Skybox/skybox_default/front.jpg",
		"Skybox/skybox_default/back.jpg"
	};

	m_skybox = new Skybox();
	m_skybox->initSkybox(cares);

	//MAPA BASE
	m_map = new Model3D();
	if ( m_map->loadModel("./Models/Worlds/Stadium/stadium_v_4.gltf") ) {
		m_map->setupMesh();
		m_map->setScale(glm::vec3(1.0f));
		m_map->setRotation(glm::vec3(0.0f));
	}
	else {
		std::cerr << "Error loading Stadium map!" << std::endl;
	}

	//OBSTACLES
	Model3D* obstacles = new Model3D();
	if ( obstacles->loadModel("./Models/Worlds/Stadium/Obstacles/obstacles_v_1.gltf") ) {
		obstacles->setupMesh();
		obstacles->setScale(glm::vec3(1.0f));
		addObject(obstacles);


		// --- 5. TRIGGERS ---
		m_triggers.clear();

		for ( int i = 1; i <= 12; i++ )
		{
			std::ostringstream ss;
			ss << "./Models/Worlds/Stadium/Triggers/triggers_" << std::setw(3) << std::setfill('0') << i << ".gltf";

			std::string path = ss.str();

			Model3D* triggerModel = new Model3D();
			if ( triggerModel->loadModel(path) )
			{
				triggerModel->setupMesh();
				triggerModel->setScale(glm::vec3(1.0f));
				Trigger* t = new Trigger(i, triggerModel);
				t->initPhysics();

				m_triggers.push_back(t);
			}
			else
			{
				delete triggerModel;
				std::cerr << "Error loading trigger model: " << path << std::endl;
			}
		}

		// --- 6. CORBA DEL CIRCUIT ---
		m_curve = new Curve();
		if ( !m_curve->LoadFromGLTF("./Models/Worlds/Stadium/Track/ControlPoints.glb") ) {
			std::cerr << "Error loading track control points!" << std::endl;
		}
		else {
			m_trackPoints = m_curve->GenerateCurve(CurveType::CATMULL_ROM, 0.01f);
		}
	}
	else {
		delete obstacles;
		std::cerr << "Error loading obstacles!" << std::endl;
	}

	// --- 7. COLLIDERS DE FÍSICA ---
	m_colliderMap = new Model3D();
	if ( m_colliderMap->loadModel("./Models/Worlds/Stadium/Collider/stadium_collider_v1.glb") ) {
		m_colliderMap->setupMesh();
		m_colliderMap->setScale(glm::vec3(1.0f));
		m_colliderMap->setRotation(glm::vec3(0.0f));
	}
	else {
		std::cerr << "Error loading Stadium map!" << std::endl;
	}
}

void World::loadCity()
{
	//CONFIGURACIO
	m_lowSignal = 100.0f;
	m_conectionLost = 140.0f;
	m_homePosition = glm::vec3(0.0f, 1.0f, 1.0f);
	m_homeRotation = glm::vec3(0.0f, 0.0f, 0.0f);

	//SKYBOX
	std::vector<std::string> cares{
		"Skybox/skybox_city/right.png",
		"Skybox/skybox_city/left.png",
		"Skybox/skybox_city/top.png",
		"Skybox/skybox_city/bottom.png",
		"Skybox/skybox_city/front.png",
		"Skybox/skybox_city/back.png"
	};

	m_skybox = new Skybox();
	m_skybox->initSkybox(cares);

	//MAPA BASE
	m_map = new Model3D();
	if ( m_map->loadModel("./Models/Worlds/City_v2/scene.gltf") ) {
		m_map->setupMesh();
		m_map->setScale(glm::vec3(2.13f));
		m_map->setRotation(glm::vec3(0.0f));
	}
	else {
		std::cerr << "Error loading Stadium map!" << std::endl;
	}

	//OBSTACLES
	Model3D* obstacles = new Model3D();
	if ( obstacles->loadModel("./Models/Worlds/Stadium/obstacles_v1.gltf") ) {
		obstacles->setupMesh();
		obstacles->setScale(glm::vec3(1.0f));
		addObject(obstacles);
	}
	else {
		delete obstacles;
		std::cerr << "Error loading obstacles!" << std::endl;
	}
}



void World::draw(Shader& shader, Camera& camera)
{
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix();

    // 1. Dibuixar Skybox 
    if (m_skybox) {
        glDepthFunc(GL_LEQUAL);
        m_skybox->draw(view, projection);
        glDepthFunc(GL_LESS);
    }

    // 2. Configurar Shader per objectes
    shader.use();

    // Llums
    shader.setInt("NR_DIRECTIONAL_LIGHTS", 1);
	shader.setVec3("lightDir", m_dirLlum.getDirection());
	shader.setVec3("lightColor", m_dirLlum.getColor());
	shader.setFloat("lightIntensity", m_dirLlum.getIntensity());
	shader.setFloat("glowStrength", 3.0f);
	shader.setFloat("glowSize", 0.0f);

	for (Light* light : m_lights) {
		if (light) {
			light->applyToShader(shader);
		}
	}

    // C�mera
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    // 3. Dibuixar Mapa Base
    if (m_map) {
        m_map->draw(shader);
    }

    // 4. Dibuixar tots els obstacles/objectes
    for (Model3D* obj : m_staticObjects) {
        if (obj) obj->draw(shader);
    }

	// 5. Dibuixar corba del circuit
	if ( m_curve && m_curveShader ) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glEnable(GL_LINE_SMOOTH);

		m_curveShader->use();

		//Uniforms Bàsics
		m_curveShader->setMat4("view", view);
		m_curveShader->setMat4("projection", projection);
		m_curveShader->setMat4("model", glm::mat4(1.0f));

		// Uniforms de "Holograma"
		m_curveShader->setVec3("playerPos", camera.getPosition());

		// Paràmetres visuals
		m_curveShader->setFloat("visibleRange", 5.0f);
		m_curveShader->setFloat("fadeWidth", 10.0f);
		m_curveShader->setFloat("time", ( float ) glfwGetTime());

		m_curve->draw(*m_curveShader);

		glDisable(GL_BLEND);
		glDisable(GL_LINE_SMOOTH);
	}

	// 6. Dibuixar triggers
	if ( m_triggerShader ) {
		m_triggerShader->use();

		// 1. Configurar Uniforms Globals (Càmera i Temps)
		m_triggerShader->setMat4("view", camera.getViewMatrix());
		m_triggerShader->setMat4("projection", camera.getProjectionMatrix());
		m_triggerShader->setVec3("camPos", camera.getPosition());
		m_triggerShader->setFloat("time", ( float ) glfwGetTime());

		// 2. Activar Transparència Additiva (Tipus Làser/Holograma)
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDepthMask(GL_FALSE);

		for ( int i = 0; i < m_triggers.size(); i++ ) {
			Trigger* trigger = m_triggers[ i ];

			if ( trigger && trigger->getModel() ) {
				glm::vec3 color;

				// --- Lògica de Colors ---
				if ( m_raceManager && m_raceManager->getNextCheckpointID() == i ) {
					color = glm::vec3(0.0f, 1.0f, 0.0f); // VERD (Actiu)
				}
				else if ( trigger->getPassed() ) {
					color = glm::vec3(0.2f, 0.2f, 0.2f); // GRIS (Passat)
				}
				else {
					color = glm::vec3(1.0f, 0.0f, 0.0f); // VERMELL (Futur)
				}

				m_triggerShader->setVec3("color", color);

				glm::mat4 modelMatrix = trigger->getModel()->getModelMatrix();
				m_triggerShader->setMat4("model", modelMatrix);

				trigger->getModel()->draw(*m_triggerShader);
			}
		}

		// Restaurar estat
		glDepthMask(GL_TRUE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
	}
}

void World::addObject(Model3D* obj)
{
    m_staticObjects.push_back(obj);
}

void World::addLight(Light* light)
{
    m_lights.push_back(light);
}

void World::canviLlum() {
    es_dia = !es_dia;
    if (es_dia) {
        m_dirLlum.setDirection(DAY_DIRECTION);
        m_dirLlum.setColor(DAY_COLOR);
        m_dirLlum.setIntensity(DAY_INTENSITY);
    }
    else {
        m_dirLlum.setDirection(NIGHT_DIRECTION);
        m_dirLlum.setColor(NIGHT_COLOR);
        m_dirLlum.setIntensity(NIGHT_INTENSITY);
    }
}

void World::loadPhysicsColliders(PhysicsWorld& physicsWorld)
{
	std::cout << "[World] Carregant col·lisions físiques..." << std::endl;

	physicsWorld.clearStaticColliders();

	if ( m_map ) {
		std::vector<glm::vec3> vertices;
		std::vector<unsigned int> indices;

		m_map->getCollisionData(vertices, indices);

		btRigidBody* mapCollider = ColliderLoader::createFromMeshData(vertices, indices);
		if ( mapCollider ) {
			physicsWorld.addRigidBody(mapCollider);

			btVector3 aabbMin, aabbMax;
			mapCollider->getCollisionShape()->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
			std::cout << "[World DEBUG] AABB del collider:" << std::endl;
			std::cout << "  Min: (" << aabbMin.x() << ", " << aabbMin.y() << ", " << aabbMin.z() << ")" << std::endl;
			std::cout << "  Max: (" << aabbMax.x() << ", " << aabbMax.y() << ", " << aabbMax.z() << ")" << std::endl;

			if ( aabbMin.x() > aabbMax.x() ) {
				std::cout << "[World DEBUG] ERROR: Collider AABB invàlid (min.x > max.x)" << std::endl;
			}
		}
		else {
			std::cout << "[World DEBUG] ERROR: No s'ha pogut crear el collider del mapa" << std::endl;
		}
	}

	//Afegir colliders dels objectes estàtics
	int objectCount = 0;
	for ( auto& staticObj : m_staticObjects ) {
		std::vector<glm::vec3> vertices;
		std::vector<unsigned int> indices;

		staticObj ->getCollisionData(vertices, indices);

		// Transformar vèrtexs amb la matriu de cada objecte
		glm::mat4 modelMatrix = staticObj -> getModelMatrix();
		for ( auto& vertex : vertices ) {
			glm::vec4 transformed = modelMatrix * glm::vec4(vertex, 1.0f);
			vertex = glm::vec3(transformed);
		}

		btRigidBody* objCollider = ColliderLoader::createFromMeshData(vertices, indices);
		if ( objCollider ) {
			physicsWorld.addRigidBody(objCollider);
			objectCount++;
		}
	}

	for ( auto* trigger : m_triggers ) {
		// Assegura't que el trigger té la física inicialitzada
		if ( !trigger->getGhost() ) {
			trigger->initPhysics();
		}

		// Afegim el fantasma al món físic
		if ( trigger->getGhost() ) {
			physicsWorld.addGhostObject(trigger->getGhost());
		}
	}

	if ( m_raceManager ) delete m_raceManager;
	m_raceManager = new RaceManager();
	m_raceManager->init(m_triggers);

	std::cout << "[World] Triggers afegits a la física i RaceManager llest." << std::endl;
}