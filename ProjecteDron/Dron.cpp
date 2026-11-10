#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include "Dron.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "PhysicsWorld.h"
#include <algorithm>

Dron::Dron() :
    m_altitudeHold(0.0f),
    m_position(0.0f), 
    m_velocity(0.0f),
    m_acceleration(0.0f),
	m_currentThrottle(0.0f),
    m_pitch(0.0f), 
    m_yaw(0.0f),
    m_roll(0.0f),
    m_flightTime(0.0f),
    m_mass(0.500f),
    m_altitude(0.0f),
    m_batteryLevel(100.0f),
    m_modeVol(MODE_VOL::DESARMED),
    m_loaded(false),
    m_propellerLoaded(false),
    m_propellerAngle(0.0f)

{

    // Inicialitzar quaternion com a identitat
    m_orientacio = { 1.0, 0.0, 0.0, 0.0 };
    m_angularVelocity = glm::vec3(0.0f); // rad/s
    m_inercia = glm::vec3(0.08f, 0.08f, 0.12f);
}

// Rotació d'un vector segons quaternion
static glm::vec3 RotateVecByQuat(const GL_Quat& q, const glm::vec3& v)
{
    glm::vec3 qv((float)q.x, (float)q.y, (float)q.z);
    glm::vec3 t = 2.0f * glm::cross(qv, v);
    return v + (float)q.w * t + glm::cross(qv, t);
}

void Dron::update(float deltaTime, Controller& controller, MODE_VOL mode, PhysicsWorld& physicsWorld)
{

	// 1. RESET flag de col·lisió cada frame
	m_isColliding = false; 
	m_collisionNormal = glm::vec3(0.0f);

	if ( !m_ReplayActive )
	{
		if ( m_modeVol == MODE_VOL::DESARMED && mode != MODE_VOL::DESARMED )
		{
			AudioManager::Get().PlaySFX("Audio/startup-drone.mp3");
		}

		m_modeVol = mode;
		const float MAX_PROPELLER_ROTATION_SPEED = 360.0f * 10.0f;
		float inputThrottle2 = ( controller.getAxis("Throttle") ) + 1.15f;
		const float constantSpeedFactor = inputThrottle2;
		if (m_modeVol != MODE_VOL::DESARMED)
			m_propellerAngle += constantSpeedFactor * MAX_PROPELLER_ROTATION_SPEED * deltaTime;

		if ( m_modeVol == MODE_VOL::ACROBATIC )
		{
			// 1. Llegim controls (van de -1 a 1)
			float inputThrottle = controller.getAxis("Throttle");
			float inputYaw = controller.getAxis("Yaw");
			float inputPitch = controller.getAxis("Pitch");
			float inputRoll = controller.getAxis("Roll");
			//m_pitch = inputPitch;
			//m_roll = inputRoll;

			// 2. Normalitzar throttle de [-1,1] a [0,1] i aplicar lag dels motors
			inputThrottle = ( inputThrottle + 1.0f ) / 2.0f;
			m_currentThrottle += ( inputThrottle - m_currentThrottle ) * deltaTime / MOTOR_LAG;
			audio_Throttle = m_currentThrottle;

			// 3. Rotació ACROBATIC
			glm::vec3 torque(
				inputPitch * ACRO_TORQUE,
				inputYaw * ACRO_TORQUE_YAW,
				inputRoll * ACRO_TORQUE
			);

			glm::vec3 angAccel = torque / m_inercia;
			m_angularVelocity += angAccel * deltaTime;
			m_angularVelocity *= expf(-ANGULAR_DAMP * deltaTime); // damping
			m_angularVelocity = glm::clamp(m_angularVelocity, -glm::vec3(MAX_ANGULAR_SPEED), glm::vec3(MAX_ANGULAR_SPEED));

			// 4. Integració quaternion
			glm::quat q(( float ) m_orientacio.w, ( float ) m_orientacio.x, ( float ) m_orientacio.y, ( float ) m_orientacio.z);
			glm::quat omega_q(0.0f, m_angularVelocity.x, m_angularVelocity.y, m_angularVelocity.z);
			glm::quat qdot = 0.5f * ( q * omega_q );
			q += qdot * deltaTime;
			q = glm::normalize(q);
			m_orientacio = { q.w, q.x, q.y, q.z };
			// Euler per HUD (radians) a partir del quaternion real
			m_pitch = asinf(2.0f * ( q.w * q.x - q.z * q.y ));
			m_roll = atan2f(
				2.0f * ( q.w * q.z + q.x * q.y ),
				1.0f - 2.0f * ( q.x * q.x + q.z * q.z )
			);
			m_yaw = atan2f(
				2.0f * ( q.w * q.y + q.x * q.z ),
				1.0f - 2.0f * ( q.y * q.y + q.x * q.x )
			);

			// 5. Calcul força i acceleració
			glm::quat glmq(q.w, q.x, q.y, q.z);
			glm::vec3 up = glmq * glm::vec3(0, 1, 0);

			const float THROTTLE_EXPO = 0.8f; // Ajusta entre 0.0 (lineal) i 1.0 (corbat)
			float t = m_currentThrottle;      // Valors entre 0.0 i 1.0

			// Fórmula Betaflight simplificada: Input * (1-expo) + Input^3 * expo
			float curvedThrottle = t * ( 1.0f - THROTTLE_EXPO ) + ( t * t * t ) * THROTTLE_EXPO;

			glm::vec3 thrustForce = up * ( curvedThrottle * MAX_THRUST );

			glm::vec3 gravityForce = glm::vec3(0, GRAVETAT * m_mass, 0);
			glm::vec3 totalForce = thrustForce + gravityForce;

			// 6. Drag aerodinàmic quadràtic
			glm::vec3 dragForce(0.0f);
			float speed = glm::length(m_velocity);
			if ( speed > 0.001f ) {
				glm::vec3 dragDir = -glm::normalize(m_velocity);
				dragForce = dragDir * ( DRAG_LINEAR * speed + DRAG_QUADRATIC * speed * speed );
			}

			m_acceleration = ( totalForce + dragForce ) / m_mass;
			m_velocity += m_acceleration * deltaTime;

			// 9. Altura i temps de vol
			m_altitude = m_position.y;
			m_flightTime += deltaTime;

			m_yaw = atan2f(
				2.0f * ( q.w * q.y + q.x * q.z ),
				1.0f - 2.0f * ( q.y * q.y + q.x * q.x )
			);
		}

		else if ( m_modeVol == MODE_VOL::STABILIZED )
		{
			// 1. Lectura de sticks
			float inputThrottle = controller.getAxis("Throttle"); // -1..1
			float inputYaw = controller.getAxis("Yaw");      // -1..1
			float inputPitch = controller.getAxis("Pitch");    // -1..1
			float inputRoll = controller.getAxis("Roll");     // -1..1
			m_pitch = inputPitch;
			m_roll = inputRoll;
			
			float throttleInput = ( inputThrottle + 1.0f ) * 0.5f;

			float attitudeLoad =
				fabs(inputPitch) * 0.5f +
				fabs(inputRoll) * 0.5f;

			attitudeLoad = glm::clamp(attitudeLoad, 0.0f, 1.0f);

			float targetEngineLoad = throttleInput + attitudeLoad * 0.6f;

			targetEngineLoad = glm::clamp(targetEngineLoad, 0.0f, 1.0f);

			m_currentThrottle += ( targetEngineLoad - m_currentThrottle ) * deltaTime / MOTOR_LAG;
			audio_Throttle = m_currentThrottle;

			// --- Deadzone ---
			const float DEADZONE = 0.05f;
			if ( fabs(inputPitch) < DEADZONE ) inputPitch = 0.0f;
			if ( fabs(inputRoll) < DEADZONE ) inputRoll = 0.0f;
			if ( fabs(inputYaw) < DEADZONE ) inputYaw = 0.0f;
			if ( fabs(inputThrottle) < DEADZONE ) inputThrottle = 0.0f;

			// --- 2. Extracció correcta d'Euler sense saltos ---
			glm::quat q(m_orientacio.w, m_orientacio.x, m_orientacio.y, m_orientacio.z);

			// Pitch
			float pitch = asinf(2.0f * ( q.w * q.x - q.z * q.y ));

			// Roll
			float roll = atan2f(
				2.0f * ( q.w * q.z + q.x * q.y ),
				1.0f - 2.0f * ( q.x * q.x + q.z * q.z )
			);

			// Yaw 
			float yaw = atan2f(
				2.0f * ( q.w * q.y + q.x * q.z ),
				1.0f - 2.0f * ( q.y * q.y + q.x * q.x )
			);

			// --- 3. Nivelado automático ---
			const float MAX_PITCH_ROLL = glm::radians(20.0f);
			const float STABILIZE_SPEED = 8.0f;
			const float YAW_SPEED = 8.0f;

			float targetPitch = inputPitch * MAX_PITCH_ROLL;
			float targetRoll = inputRoll * MAX_PITCH_ROLL;

			float pitchError = targetPitch - pitch;
			float rollError = targetRoll - roll;

			float angAccelPitch = pitchError * STABILIZE_SPEED;
			float angAccelRoll = rollError * STABILIZE_SPEED;

			m_angularVelocity.x = glm::clamp(angAccelPitch, -MAX_ANGULAR_SPEED, MAX_ANGULAR_SPEED);
			m_angularVelocity.z = glm::clamp(angAccelRoll, -MAX_ANGULAR_SPEED, MAX_ANGULAR_SPEED);

			// Yaw manual
			m_angularVelocity.y = inputYaw * ACRO_TORQUE_YAW;

			// --- 4. Integración quaternion ---
			glm::quat omega_q(0.0f, m_angularVelocity.x, m_angularVelocity.y, m_angularVelocity.z);
			glm::quat qdot = 0.5f * ( q * omega_q );
			q += qdot * deltaTime;
			q = glm::normalize(q);
			m_orientacio = { q.w, q.x, q.y, q.z };

			// --- 5. Throttle ---
			const float ALTITUDE_HOLD_SPEED = YAW_SPEED;
			float verticalInput = inputThrottle;
			float targetVerticalSpeed = verticalInput * ALTITUDE_HOLD_SPEED;

			glm::vec3 up = q * glm::vec3(0, 1, 0);
			float thrustBase = m_mass * -GRAVETAT;
			glm::vec3 thrustForce = up * ( thrustBase + targetVerticalSpeed * m_mass );

			// --- 6. Drag aerodinámico ---
			glm::vec3 dragForce(0.0f);
			float speed = glm::length(m_velocity);
			if ( speed > 0.001f ) {
				glm::vec3 dragDir = -glm::normalize(m_velocity);
				dragForce = dragDir * ( DRAG_LINEAR * speed + DRAG_QUADRATIC * speed * speed );
			}

			// --- 7. Actualización física ---
			glm::vec3 gravityForce = glm::vec3(0, GRAVETAT * m_mass, 0);
			glm::vec3 totalForce = thrustForce + gravityForce + dragForce;

			m_acceleration = totalForce / m_mass;
			m_velocity += m_acceleration * deltaTime;

			// Limitar velocidad horizontal
			float MAX_HORIZ_SPEED = 5.0f;
			glm::vec3 horizontalVel = glm::vec3(m_velocity.x, 0, m_velocity.z);
			if ( glm::length(horizontalVel) > MAX_HORIZ_SPEED ) {
				horizontalVel = glm::normalize(horizontalVel) * MAX_HORIZ_SPEED;
				m_velocity.x = horizontalVel.x;
				m_velocity.z = horizontalVel.z;
			}


			m_altitude = m_position.y;
			m_flightTime += deltaTime;

			m_pitch = asinf(2.0f * ( q.w * q.x - q.z * q.y ));
			// Roll
			m_roll = atan2f(
				2.0f * ( q.w * q.z + q.x * q.y ),
				1.0f - 2.0f * ( q.x * q.x + q.z * q.z )
			);
			m_yaw = yaw;
		}

		else if ( m_modeVol == MODE_VOL::DESARMED )
		{
			m_currentThrottle = 0.0f;
			m_angularVelocity = glm::vec3(0.0f);

			glm::vec3 gravityForce = glm::vec3(0, GRAVETAT * m_mass, 0);
			m_acceleration = gravityForce / m_mass;

			m_velocity += m_acceleration * deltaTime;

			if ( m_position.y <= 0.0f ) {
				m_position.y = 0.0f;
				m_velocity = glm::vec3(0.0f);
				m_acceleration = glm::vec3(0.0f);
			}

			m_altitude = m_position.y;
		}

		if ( m_modeVol != MODE_VOL::DESARMED && !m_ReplayActive )
		{
			// 1. FACTOR DE TEMPS DE PROVA (FA QUE VAGI MOLT MÉS RÀPID)
			// Augmentem la taxa base per 10 per veure el desgast en ~30 segons (a màxim ús).
			// MAX_FLIGHT_TIME_SECONDS = 30.0f
			const float MAX_FLIGHT_TIME_SECONDS = 30.0f;
			const float BASE_CONSUMPTION_RATE_PER_SEC = 10.0f / MAX_FLIGHT_TIME_SECONDS; // 3.33% / segon

			// 2. CONSUM PER POTÈNCIA (Base: Throttle)
			// La potència consumida és proporcional al cub del Throttle (més realista).
			float powerInputConsumption = m_currentThrottle * m_currentThrottle * m_currentThrottle;

			// 3. CONSUM PER VELOCITAT (Resistència de l'aire)
			// El drag aerodinàmic consumeix potència (resistencia proporcional a la velocitat al quadrat).
			float speed = glm::length(m_velocity);
			// Normalitzem la velocitat màxima esperada (p. ex., 15 m/s) per obtenir un factor entre 0 i 1.
			const float MAX_SPEED_EXPECTED = 15.0f;
			float speedFactor = ( speed / MAX_SPEED_EXPECTED );
			float speedConsumption = speedFactor * speedFactor; // Consum quadrat

			// 4. CONSUM PER MANIOBRES (Velocitat angular/Esforç)
			// Les rotacions ràpides consumeixen més.
			float angSpeed = glm::length(m_angularVelocity);
			const float MAX_ANGULAR_SPEED_EXPECTED = 20.0f; // ~1145 deg/s (Valor alt)
			float angularConsumption = ( angSpeed / MAX_ANGULAR_SPEED_EXPECTED ) * 0.15f; // Factor més petit (15% del total)

			// 5. CÀLCUL TOTAL
			// Utilitzem el màxim entre el consum de Throttle i el factor de Velocitat,
			// i afegim el factor angular (maniobres).
			float combinedFactor = ( std::max ) ( powerInputConsumption, speedConsumption ) + angularConsumption;

			// Apliquem la taxa base al factor combinat
			float consumption = ( BASE_CONSUMPTION_RATE_PER_SEC * combinedFactor ) * deltaTime;

			// Reduir el nivell de bateria i assegurar que no baixa de zero
			m_batteryLevel = ( std::max ) ( 0.0f, m_batteryLevel - consumption );
		}

		glm::vec3 displacement = m_velocity * deltaTime;
		glm::vec3 currentPos = m_position;
		glm::vec3 targetPos = currentPos + displacement;

		// Variables per recollir dades del xoc
		glm::vec3 hitNormal;
		float hitFraction;
		glm::quat currentRot(m_orientacio.w, m_orientacio.x, m_orientacio.y, m_orientacio.z);

		// Fem el SWEEP TEST
		if ( physicsWorld.checkSweep(currentPos, targetPos, currentRot, hitNormal, hitFraction) )
		{
			// --- HEM XOCAT! ---
			m_isColliding = true; // Per si vols posar sons o efectes visualment
			m_collisionNormal = hitNormal;

			// A. Movem el dron fins just abans del mur
			// Restem un petit epsilon (0.01f) per assegurar que no ens quedem "dins" la paret
			float safeFraction = ( std::max ) ( 0.0f, hitFraction - 0.001f );
			m_position = glm::mix(currentPos, targetPos, safeFraction);

			// B. RESPOSTA FÍSICA (Rebot i Fricció)
			// Això substitueix la funció onCollision antiga

			glm::vec3 N = glm::normalize(hitNormal);

			// Descomposem la velocitat
			float vn = glm::dot(m_velocity, N); // Velocitat cap a la paret

			if ( vn < 0.0f ) { // Només rebotem si anem cap a la paret
				glm::vec3 vNormal = vn * N;
				glm::vec3 vTangent = m_velocity - vNormal;

				const float restitution = 0.25f; // Quant rebota (0 = enganxat, 1 = goma)
				const float friction = 0.15f;    // Fricció de paret

				// Nova velocitat: invertim la normal i frenem la tangent
				m_velocity = ( -vNormal * restitution ) + ( vTangent * ( 1.0f - friction ) );

				// Opcional: Si la velocitat és molt baixa després del xoc, la matem per evitar vibracions
				if ( glm::length(m_velocity) < 0.2f ) m_velocity = glm::vec3(0.0f);
			}
		}
		else
		{
			// --- CAMÍ LLIURE ---
			m_position = targetPos;
		}

		m_replay.record(m_position, glm::quat(m_orientacio.w,
			m_orientacio.x, m_orientacio.y, m_orientacio.z),
			( float ) glfwGetTime());
	}
	else // (Replay Mode)
	{
		if ( !m_stop_replay )
		{
			float speedFactor = m_replay.getPlaybackSpeed();
			float replayDeltaTime = deltaTime * speedFactor;

			const float MAX_PROPELLER_ROTATION_SPEED = 360.0f * 10.0f;
			// També afectem la velocitat de les hèlices perquè quedi visualment coherent
			m_propellerAngle += MAX_PROPELLER_ROTATION_SPEED * deltaTime * speedFactor;

			m_position = m_replay.getReplayPosition(replayDeltaTime);
			glm::quat q = m_replay.getReplayRotation(replayDeltaTime);
			m_orientacio = { q.w, q.x, q.y, q.z };

			if ( m_replay.getReplayTime() >= m_replay.getTotalTime() )
			{
				m_replay.setReplayTime(0.0f);
			}
			m_replayTime = m_replay.getReplayTime();
		}
	}
}

void Dron::reset()
{
    m_position = glm::vec3(0.0f);
    m_velocity = glm::vec3(0.0f);
    m_acceleration = glm::vec3(0.0f);
    m_currentThrottle = 0.0f;
    m_pitch = 0.0f;
    m_yaw = 0.0f;
    m_roll = 0.0f;
    m_flightTime = 0.0f;
    m_altitude = 0.0f;
    m_batteryLevel = 100.0f;
    m_modeVol = MODE_VOL::DESARMED;
    m_orientacio = { 1.0, 0.0, 0.0, 0.0 };
	m_angularVelocity = glm::vec3(0.0f);
}

void Dron::reset(glm::vec3 position, glm::vec3 rotation)
{
	m_position = position;
	m_velocity = glm::vec3(0.0f);
	m_acceleration = glm::vec3(0.0f);
	m_currentThrottle = 0.0f;
	m_pitch = 0.0f;
	m_yaw = 0.0f;
	m_roll = 0.0f;
	m_flightTime = 0.0f;
	m_altitude = position.y;
	m_batteryLevel = 100.0f;
	m_modeVol = MODE_VOL::DESARMED;
	glm::quat q = glm::quat(glm::radians(rotation));
	m_orientacio.x = q.x;
	m_orientacio.y = q.y;
	m_orientacio.z = q.z;
	m_orientacio.w = q.w;
	m_angularVelocity = glm::vec3(0.0f);
}

glm::vec3 Dron::getForward() const {
    return glm::normalize(RotateVecByQuat(m_orientacio, glm::vec3(0.0f, 0.0f, -1.0f)));
}

void Dron::draw(Shader& shader)
{
    carregarModel();
    carregarPropellers();
    shader.use();
    m_model.setPosition(m_position);
    glm::quat glmq((float)m_orientacio.w, (float)m_orientacio.x, (float)m_orientacio.y, (float)m_orientacio.z);
    m_model.setRotation(glm::degrees(glm::eulerAngles(glmq)));
    m_model.draw(shader);

    if (m_propellerLoaded) {
        glm::quat droneQ((float)m_orientacio.w, (float)m_orientacio.x,
            (float)m_orientacio.y, (float)m_orientacio.z);

        for (int i = 0; i < 4; ++i)
        {
            glm::vec3 rotatedPos = droneQ * m_propellerPos[i];

            glm::quat propellerSpin =
                glm::angleAxis(glm::radians(m_propellerAngle), glm::vec3(0, 1, 0));

            glm::quat finalRotation = droneQ * propellerSpin;

            m_propellerModel.setPosition(m_position + rotatedPos);
            m_propellerModel.setRotation(glm::degrees(glm::eulerAngles(finalRotation)));
            m_propellerModel.setScale(glm::vec3(1));
            m_propellerModel.draw(shader);
        }
    }
}

void Dron::carregarModel()
{
	if ( m_loaded ) return;

	m_loaded = false;

	std::string modelPath;
	std::string propellerPath;

	switch ( m_currentModelIndex ) {
	case 0: // OG MODEL
		modelPath = "Models/Dron/og_drone/drone_base/drone_base.obj";
		propellerPath = "Models/Dron/og_drone/helix/helix.obj";
		m_propellerPos[ 0 ] = glm::vec3(-0.089816f, 0.015052f, -0.071208f);
		m_propellerPos[ 1 ] = glm::vec3(0.089816f, 0.015052f, -0.071208f);
		m_propellerPos[ 2 ] = glm::vec3(0.105738f, 0.015217f, 0.062211f);
		m_propellerPos[ 3 ] = glm::vec3(-0.105738f, 0.015217f, 0.062211f);
		break;
	case 1: // DJI FPV
		modelPath = "Models/Dron/dji_drone/scene.gltf";
		propellerPath = "Models/Dron/dji_drone/helix.gltf"; 
		m_propellerPos[ 0 ] = glm::vec3(0.104769f, 0.072159f, 0.065601f);
		m_propellerPos[ 1 ] = glm::vec3(-0.105794f, 0.072159f, 0.065601f);
		m_propellerPos[ 2 ] = glm::vec3(-0.093762f, 0.096711f, -0.073782f);
		m_propellerPos[ 3 ] = glm::vec3(0.093762f, 0.096711f, -0.073782f);
		break;
	case 2: // AGGRESSIVE
		modelPath = "Models/Dron/agressive_drone/fpv_drone.gltf";
		propellerPath = "Models/Dron/agressive_drone/helix.gltf"; 
		m_propellerPos[ 0 ] = glm::vec3(-0.089816f, 0.015052f, -0.071208f);
		m_propellerPos[ 1 ] = glm::vec3(0.089816f, 0.015052f, -0.071208f);
		m_propellerPos[ 2 ] = glm::vec3(0.105738f, 0.015217f, 0.062211f);
		m_propellerPos[ 3 ] = glm::vec3(-0.105738f, 0.015217f, 0.062211f);
		break;
	default:
		modelPath = "Models/Dron/og_drone/drone_base/drone_base.obj";
		propellerPath = "Models/Dron/og_drone/helix/helix.obj";
		m_propellerPos[ 0 ] = glm::vec3(-0.089816f, 0.015052f, -0.071208f);
		m_propellerPos[ 1 ] = glm::vec3(0.089816f, 0.015052f, -0.071208f);
		m_propellerPos[ 2 ] = glm::vec3(0.105738f, 0.015217f, 0.062211f);
		m_propellerPos[ 3 ] = glm::vec3(-0.105738f, 0.015217f, 0.062211f);
		break;
	}

	if ( m_model.loadModel(modelPath) ) {
		m_model.setupMesh();
		m_loaded = true;
		std::cout << "Model de dron canviat a index: " << m_currentModelIndex << std::endl;
	}
	else {
		std::cerr << "Error carregant model dron: " << modelPath << std::endl;
	}

	if ( m_propellerModel.loadModel(propellerPath) ) {
		m_propellerModel.setupMesh();
		m_propellerLoaded = true;
	}
}

void Dron::setModelType(int modelIndex)
{
	if ( m_currentModelIndex != modelIndex )
	{
		m_currentModelIndex = modelIndex;
		m_loaded = false;
	}
}
void Dron::setPhysicsParameters(float mass, float maxThrust, float responsiveness, float drag, float lag)
{
	m_mass = mass;
	m_maxThrust = maxThrust;
	m_turnResponsiveness = responsiveness;
	m_dragCoef = drag;
	m_motorLag = lag;
}

void Dron::carregarPropellers()
{
	const char* PROPELLER_MODEL_PATH = "./Models/Dron/og_drone/helix/helix.obj";

	if ( !m_propellerLoaded && m_propellerModel.loadModel(PROPELLER_MODEL_PATH) )
	{
		m_propellerModel.setupMesh();
		m_propellerLoaded = true;
		std::cout << "Hèlix carregada correctament!" << std::endl;
	}
	else if ( !m_propellerLoaded )
	{
		std::cerr << "No s'ha pogut carregar l'hèlix." << std::endl;
	}
}

glm::mat4 Dron::getRotationMatrix() const
{
    double matd[16];
    QuatToMatrix(m_orientacio, matd);
    glm::mat4 rot;
    for (int i = 0; i < 16; ++i) rot[i / 4][i % 4] = (float)matd[i];
    return rot;
}

void Dron::onCollision(const glm::vec3& normal, float penetrationDepth)
{
	m_isColliding = true;
	glm::vec3 N = glm::normalize(normal);

	// Paràmetres físics
	const float restitution = 0.15f;
	const float friction = 0.2f;
	const float penetrationCorrection = 0.3f;

	// --- 1. Correcció suau de penetració ---
	if ( penetrationDepth > 0.0f ) {
		m_position -= N * penetrationDepth * penetrationCorrection;
	}

	// --- 2. Separar velocitat normal i tangencial ---
	glm::vec3 v = m_velocity;

	float vn = glm::dot(v, -N);          // component normal correcta

	glm::vec3 vNormal = vn * ( -N );       // direcció normal correcta
	glm::vec3 vTangent = v - vNormal;    // component tangencial

	if ( vn < 0.0f ) {
		vNormal = -vNormal * restitution;   // invertir + reduir
	}
	else {
		vNormal = glm::vec3(0.0f);          // ja ens allunyem
	}

	// --- 4. Fricció (deslliscament) ---
	vTangent *= ( 1.0f - friction );

	// --- 5. Nova velocitat ---
	m_velocity = vNormal + vTangent;

	if ( glm::dot(-N, glm::vec3(0, 1, 0)) > 0.9f && m_velocity.y < 0.0f ) {
		m_velocity.y = 0.0f;
	}
}