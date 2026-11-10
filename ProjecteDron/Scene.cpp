#define GLM_ENABLE_EXPERIMENTAL
#include "Scene.h"
#include "Application.h"
#include "AudioManager.h"
#include "Menu.h"

Scene::Scene()
{
	m_window = nullptr;
	simpleDepthShader = nullptr;
	m_mode = MODE_VOL::ACROBATIC;
	m_world = World();
	m_dron = Dron();
	m_hud = HUD();
	m_cameraFree = Camera(16.0f / 9.0f, Type::FREE);
	m_cameraEDIT = Camera(16.0f / 9.0f, Type::EDIT);
	m_cameraThirdPerson = Camera(16.0f / 9.0f, Type::THIRD_PERSON);
	m_cameraFPV = Camera(16.0f / 9.0f, Type::FPV);
	m_physicsInitialized =  false;
	
	if (edit)
	{
		m_activeCamera = &m_cameraEDIT;
	}
	else{
		m_activeCamera = &m_cameraFree;
	}
	m_prevCameraButtonState = false;
}

Scene::Scene(float aspectRatio)
{
	m_window = nullptr;
	simpleDepthShader = nullptr;
	m_mode = MODE_VOL::ACROBATIC;
	m_world = World();
	m_dron = Dron();
	m_hud = HUD();
	m_cameraFree = Camera(aspectRatio, Type::FREE);
	m_cameraEDIT = Camera(aspectRatio, Type::EDIT);
	m_cameraThirdPerson = Camera(aspectRatio, Type::THIRD_PERSON);
	m_cameraFPV = Camera(aspectRatio, Type::FPV);
	if (edit)
	{
		m_activeCamera = &m_cameraEDIT;
	}
	else {
		m_activeCamera = &m_cameraFree;
	}
	m_prevCameraButtonState = false;
}

Scene::~Scene()
{
	// Esborrar shader
	if (simpleDepthShader != nullptr) {
		delete simpleDepthShader;
		simpleDepthShader = nullptr;
	}

	// Esborrar buffers d'ombres dinàmiques
	if (depthMapFBO != 0) {
		glDeleteFramebuffers(1, &depthMapFBO);
		glDeleteTextures(1, &depthMap);
		depthMapFBO = 0;
	}

	// Esborrar buffers d'ombres estàtiques
	if (staticDepthMapFBO != 0) {
		glDeleteFramebuffers(1, &staticDepthMapFBO);
		glDeleteTextures(1, &staticDepthMap);
		staticDepthMapFBO = 0;
	}

	AudioManager::Get().UpdateDroneEngine(0.0f, 0.0f, true, false);

	cleanupPhysics();
}

// Inicialització de la física amb Bullet
void Scene::initPhysics() {
	std::cout << "[Scene] Inicialitzant física..." << std::endl;

	m_physicsWorld.init();

	// Posició inicial del dron
	glm::vec3 startPos = m_dron.getPosition();
	startPos.y = 10.0f;  // 10 unitats amunt

	std::cout << "[Scene] Posició inicial del drone: "
		<< startPos.x << ", " << startPos.y << ", " << startPos.z << std::endl;

	m_physicsWorld.createDroneBody(startPos);

	std::cout << "[TEST] Creant terra de prova..." << std::endl;

	// Crear una caixa gran com a terra
	btBoxShape* groundShape = new btBoxShape(btVector3(100, 1, 100));
	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0, 0, 0));  // Terra a y=0

	btDefaultMotionState* groundMotion = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo groundInfo(0.0f, groundMotion, groundShape);
	btRigidBody* groundBody = new btRigidBody(groundInfo);

	groundBody->setCollisionFlags(groundBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
	groundBody->setActivationState(DISABLE_DEACTIVATION);

	m_physicsWorld.addRigidBody(groundBody);

	std::cout << "[TEST] Creant paret de prova..." << std::endl;
	btBoxShape* wallShape = new btBoxShape(btVector3(1, 10, 20));
	btTransform wallTransform;
	wallTransform.setIdentity();
	wallTransform.setOrigin(btVector3(20, 5, 0));  // Paret a x=20

	btDefaultMotionState* wallMotion = new btDefaultMotionState(wallTransform);
	btRigidBody::btRigidBodyConstructionInfo wallInfo(0.0f, wallMotion, wallShape);
	btRigidBody* wallBody = new btRigidBody(wallInfo);

	wallBody->setCollisionFlags(wallBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
	wallBody->setActivationState(DISABLE_DEACTIVATION);

	m_physicsWorld.addRigidBody(wallBody);

	std::cout << "[TEST] Terra i paret de prova creades" << std::endl;

	// Ara carreguem els colliders del món real
	m_world.loadPhysicsColliders(m_physicsWorld);

	m_physicsInitialized = true;
	std::cout << "[Scene] Física inicialitzada correctament" << std::endl;
}

void Scene::switchCamera(int index)
{
	switch (index)
	{
	case 0: m_activeCamera = &m_cameraFree; break;
	case 1: m_activeCamera = &m_cameraThirdPerson; break;
	case 2: m_activeCamera = &m_cameraFPV; break;
	default: break;
	}
}

void Scene::reset()
{
	m_dron.reset();
	m_activeCamera = &m_cameraThirdPerson;
}

void Scene::initWorld(MapID map)
{
	m_world.initWorld(map);
}

void Scene::initDron()
{
	m_dron.reset(m_world.getHomePosition(), m_world.getHomeRotation());
	m_dron.carregarModel();
	m_dron.carregarPropellers();
}

void Scene::initShadowsMap()
{
	staticShadowsBaked = false;
	m_activeCamera = &m_cameraThirdPerson;
	initShadows();
	initPhysics();
	AudioManager::Get().InitDroneSound();
}

void Scene::initShadows()
{
	// NETEJA PREVENTIVA 
	if (depthMapFBO != 0) {
		glDeleteFramebuffers(1, &depthMapFBO);
		glDeleteTextures(1, &depthMap);
	}
	if (staticDepthMapFBO != 0) {
		glDeleteFramebuffers(1, &staticDepthMapFBO);
		glDeleteTextures(1, &staticDepthMap);
	}
	if (simpleDepthShader != nullptr) {
		delete simpleDepthShader;
	}

	// 1. Carregar el shader d'ombres
	simpleDepthShader = new Shader("Shaders/shadow_depth.vert", "Shaders/shadow_depth.frag");

	// Definim el color de la vora 
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	// 2. CONFIGURACIÓ MAPA DINÀMIC (DRON)
	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	// Reservem espai (SHADOW_WIDTH hauria de ser 4096 per bona qualitat)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	//PARÀMETRES CLAU PER A SUAVITZAT
	// Canviem a LINEAR per permetre interpolació
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Aquestes dues línies activen el "Shadow Sampler" al shader 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// Evitem que l'ombra es repeteixi fora de la caixa
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Adjuntem al Framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 3. CONFIGURACIÓ MAPA ESTÀTIC (WORLD)
	glGenFramebuffers(1, &staticDepthMapFBO);

	glGenTextures(1, &staticDepthMap);
	glBindTexture(GL_TEXTURE_2D, staticDepthMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// --- PARÀMETRES DE SUAVITZAT ---
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, staticDepthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, staticDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Scene::bakeStaticShadows()
{
	// 1. Configurar la matriu de llum 
	glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.5f));
	glm::vec3 lightPos = glm::vec3(0.0f) - (lightDir * 30.0f);
	float boxSize = 20.0f;
	glm::mat4 lightProjection = glm::ortho(-boxSize, boxSize, -boxSize, boxSize, 1.0f, 100.0f);
	glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	m_lightSpaceMatrix = lightProjection * lightView; 

	simpleDepthShader->use();
	simpleDepthShader->setMat4("lightSpaceMatrix", m_lightSpaceMatrix);

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, staticDepthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_FRONT);
	m_world.draw(*simpleDepthShader, *m_activeCamera);
	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	staticShadowsBaked = true; 
}

void Scene::setCameraByAxis(float axisValue)
{
	const float deadzone = 0.3f;	

	if (axisValue < -deadzone) 
	{
		m_activeCamera = &m_cameraFree; 
		m_currentCameraIndex = 0; 
	}
	else if (axisValue > deadzone) 
	{
		m_activeCamera = &m_cameraFPV; 
		m_currentCameraIndex = 2; 
	}
	else 
	{
		m_activeCamera = &m_cameraThirdPerson; 
		m_currentCameraIndex = 1; 
	}
}

void Scene::setModeByAxis(float axisValue)
{
	const float deadzone = 0.3f;

	if ( axisValue < -deadzone )
	{
		if ( m_mode != MODE_VOL::ACROBATIC ) {
			m_mode = MODE_VOL::ACROBATIC;
		}
	}
	else if ( axisValue > deadzone )
	{
		if ( m_mode != MODE_VOL::DESARMED ) {
			m_mode = MODE_VOL::DESARMED;
		}
	}
	else
	{
		if ( m_mode != MODE_VOL::STABILIZED ) {
			m_mode = MODE_VOL::STABILIZED;
		}
	}
}

void Scene::drawSceneGeometry(Shader& shader)
{
	m_dron.draw(shader);
	m_world.draw(shader, *m_activeCamera);
}

void Scene::draw(Shader& pbrShader)
{
	// Si és el primer cop o la llum ha canviat, calculem el mapa estàtic
	if (!staticShadowsBaked) {
		bakeStaticShadows();
	}

	//PREPARAR EL MAPA D'OMBRES ACTUAL

	glBindFramebuffer(GL_READ_FRAMEBUFFER, staticDepthMapFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, depthMapFBO);
	glBlitFramebuffer(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT,
		0, 0, SHADOW_WIDTH, SHADOW_HEIGHT,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	//Dibuixar el DRON al mapa actual
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	simpleDepthShader->use();
	simpleDepthShader->setMat4("lightSpaceMatrix", m_lightSpaceMatrix); 

	glCullFace(GL_FRONT);
	m_dron.draw(*simpleDepthShader);
	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//RENDERITZAT NORMAL
	int scrWidth, scrHeight;
	glfwGetFramebufferSize(m_window, &scrWidth, &scrHeight);
	glViewport(0, 0, scrWidth, scrHeight);

	pbrShader.use();

	pbrShader.setMat4("view", m_activeCamera->getViewMatrix());
	pbrShader.setMat4("projection", m_activeCamera->getProjectionMatrix());
	pbrShader.setVec3("camPos", m_activeCamera->getPosition());

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.5f));
	pbrShader.setVec3("lightDir", lightDir);

	pbrShader.setVec3("lightColor", glm::vec3(1.5f, 1.5f, 1.5f));
	pbrShader.setFloat("lightIntensity", 5.0f);
	pbrShader.setFloat("emissiveIntensity", 10.0f);
	pbrShader.setInt("debugMode", currentDebugMode);

	//OMBRES
	pbrShader.setMat4("lightSpaceMatrix", m_lightSpaceMatrix); // Usem la guardada

	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, depthMap); // Passem el mapa combinat (Món copiat + Dron nou)
	pbrShader.setInt("shadowMap", 10);

	drawSceneGeometry(pbrShader);
}

void Scene::update(float deltaTime, Controller& controller)
{
	
	if ( !m_dron.getreplay().isRecordingbool() && m_dron.getreplay().getTotalTime() == 0.0f && 
		m_world.getRaceManager()->getState() == RaceState::IDLE)
	{
		m_dron.getreplay().startRecording(( float ) glfwGetTime());
	}
	if ( m_world.getRaceManager()->getState() == RaceState::FINISHED && saved == false)
	{
		Menu::isrenderfinal = true;
		Menu::simulationPaused = true;
		m_dron.getreplay().stopRecording();
		saved = true;
	}

	// 1. INPUT I FÍSICA DEL DRON (Moviment + Sweep Test)
	// Això mou el dron, calcula col·lisions amb parets i actualitza la posició final.
	m_activeCamera->setreplay(m_dron.getReplayActive());
	m_cameraThirdPerson.setdeltatime(deltaTime);

	// --- Actualitzar el so del motor segons la velocitat del dron ---
	float normalized = m_dron.getaudioThrottle();
	m_paused = Menu::showMainMenu || Menu::simulationPaused;

	m_dron.update(deltaTime, controller, m_mode, m_physicsWorld);
	
	m_physicsWorld.syncDroneTransform(
		m_dron.getPosition(),
		m_dron.getRotationMatrix()
	);

	m_physicsWorld.stepSimulation(deltaTime);

	// 3. Comprovar col·lisions
	float penetrationDepth = 0.0f;
	glm::vec3 collisionNormal;
	if ( m_physicsWorld.droneHit(collisionNormal, penetrationDepth) ) {
		m_dron.onCollision(collisionNormal, penetrationDepth);
	}

	btRigidBody* droneBody = m_physicsWorld.getDroneRigidBody();

	if ( m_world.getRaceManager() ) {
		m_world.getRaceManager()->update(deltaTime, droneBody);

		if ( m_world.getRaceManager()->getState() == RaceState::RACING ) {
			// std::cout << "Temps: " << m_world.getRaceManager()->getCurrentTime() << std::endl;
		}
	}
	
	bool mode;

	if ( m_mode == MODE_VOL::STABILIZED )
	{
		mode = true;
	}
	else
	{
		mode = false;
	}

	if ( !m_dron.getReplayActive() )
	{
		AudioManager::Get().UpdateDroneEngine(normalized, m_dron.getSpeed(), m_paused, mode);
	}

	// CONTROLS DE MANDO
	if (controller.getType() == ControllerType::KEYBOARD)
	{
		if ( controller.getButton("CanviCamara") )
		{
			m_currentCameraIndex = ( m_currentCameraIndex + 1 ) % 3;
			switchCamera(m_currentCameraIndex);
			std::cout << "Càmera canviada a: " << m_currentCameraIndex << std::endl;
		}
		if ( controller.getButton("CanviLlum") )
		{
			m_world.canviLlum();
		}
		if ( controller.getButton("debug") )
		{
			currentDebugMode = ( currentDebugMode + 1 ) % 8;
		}
		if ( controller.getButton("SwitchMode") )
		{
			int nextMode = ( static_cast< int >( m_mode ) + 1 ) % 3;
			m_mode = static_cast< MODE_VOL >( nextMode );
			std::cout << "Mode: " << nextMode << std::endl;
		}
	}
	else if ( controller.getType() == ControllerType::CONTROLLER_DRONE )
	{
		float axisValue = controller.getAxis("CameraSwitch");
		setCameraByAxis(axisValue);

		float axisMode = controller.getAxis("SwitchMode");
		setModeByAxis(axisMode);

		if ( controller.getButton("debug") ) {
			currentDebugMode = ( currentDebugMode + 1 ) % 8;
		}
	}
	else if ( controller.getType() == ControllerType::CONTROLLER_PS4 )
	{
		bool camButton = controller.getButton("CameraSwitch");
		if ( camButton && !m_prevCameraButtonState )
		{
			m_currentCameraIndex = ( m_currentCameraIndex + 1 ) % 3;
			switchCamera(m_currentCameraIndex);
		}
		m_prevCameraButtonState = camButton;
	}

	// 5. ACTUALITZAR CÀMERA (Sempre al final, perquè segueixi la posició nova)
	if ( m_activeCamera->getType() == Type::EDIT )
	{
		m_activeCamera->updateFreeCamera(deltaTime, m_window);
	}
	else
	{
		glm::mat4 droneRot = m_dron.getRotationMatrix();
		m_activeCamera->updateFollow(m_dron.getPosition(), droneRot);
	}
}

void Scene::cleanupPhysics() {
	// El destructor de PhysicsWorld ja neteja tot
	m_physicsInitialized = false;
}
