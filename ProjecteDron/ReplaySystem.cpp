#include "ReplaySystem.h"
#include <glm/gtx/norm.hpp>

std::string getDateTimeFilename()
{
	auto t = std::time(nullptr);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	std::stringstream ss;
	ss << "replay_"
		<< std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
		<< ".bin";
	return ss.str();
}

void ReplaySystem::saveReplay() 
{
	std::string filename = getDateTimeFilename();
	std::ofstream file("Replays/" + filename, std::ios::binary);
	if ( !file.is_open() ) return;

	size_t count = m_samples.size();

	file.write(reinterpret_cast< const char* >( &count ), sizeof(size_t));
	file.write(reinterpret_cast< const char* >( m_samples.data() ), count * sizeof(DronePosTime));
}

void ReplaySystem::loadReplay(const std::string& filename) 
{
	std::ifstream file("Replays/" + filename, std::ios::binary);

	if ( !file.is_open() ) return;

	size_t count;
	file.read(reinterpret_cast< char* >( &count ), sizeof(size_t));

	m_samples.resize(count);

	file.read(reinterpret_cast< char* >( m_samples.data() ), count * sizeof(DronePosTime));
}


float ReplaySystem::getTotalTime() 
{
    if (m_samples.empty()) return 0.0f;
    return m_samples.back().time;
}

void ReplaySystem::startRecording(float currentTime) 
{
    std::cout << "ReplaySystem: Empezando a grabar." << std::endl;
    m_samples.clear();
    m_recording = true;
    m_replay = false;
    m_startTime = currentTime;
    m_replayTime = 0.0f;
}

void ReplaySystem::stopRecording() 
{
    if (m_recording) {
        std::cout << "ReplaySystem: Grabación finalizada. Muestras: " << m_samples.size() << std::endl;
        m_recording = false;
    }
}

void ReplaySystem::startReplay() 
{
    if (!m_samples.empty()) {
        std::cout << "ReplaySystem: Iniciando repetición." << std::endl;
        m_replay = true;
        m_recording = false;
        m_replayTime = 0.0f;
    }
    else {
        std::cerr << "ReplaySystem: Error, no hay muestras para reproducir." << std::endl;
    }
}

void ReplaySystem::stopReplay() 
{
    std::cout << "ReplaySystem: Repetición detenida." << std::endl;
    m_replay = false;
    m_replayTime = 0.0f;
}

void ReplaySystem::record(const glm::vec3& pos, const glm::quat& rot, float currentTime)
{
    if (m_recording)
    {
        float relativeTime = currentTime - m_startTime;
        m_samples.push_back({ pos, rot, relativeTime });
    }
}

glm::vec3 ReplaySystem::catmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t)
{
    float t2 = t * t;
    float t3 = t * t * t;

    return 0.5f * ( (2.0f * p1) + (-p0 + p2) * t + 
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + 
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

glm::vec3 ReplaySystem::bezier(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t)
{
    float t2 = t * t;
    float t3 = t * t * t;
    float one_minus_t = 1.0f - t;
    float one_minus_t2 = one_minus_t * one_minus_t;
    float one_minus_t3 = one_minus_t * one_minus_t2;

    return one_minus_t3 * p0 +
        3.0f * t * one_minus_t2 * p1 +
        3.0f * t2 * one_minus_t * p2 +
        t3 * p3;
}

glm::vec3 ReplaySystem::getReplayPosition(float dt)
{
    if (m_samples.size() < 4) return m_samples.empty() ? glm::vec3(0.0f) : m_samples.front().position;

    m_replayTime += dt;

    float totalTime = getTotalTime();
    if (m_replayTime > totalTime) {
        m_replayTime = totalTime;
    }

    int i = 1;
    while (i < m_samples.size() - 2 && m_samples[i].time < m_replayTime) {
        i++;
    }


    i = glm::clamp(i, 1, (int)m_samples.size() - 3);

    float t0 = m_samples[i].time;
    float t1 = m_samples[i + 1].time;
    float segmentDuration = t1 - t0;

    float localT = 0.0f;
    if (segmentDuration > 0.0001f) {
        localT = (m_replayTime - t0) / segmentDuration;
    }
    localT = glm::clamp(localT, 0.0f, 1.0f);

    return bezier(m_samples[i - 1].position, m_samples[i].position,
        m_samples[i + 1].position, m_samples[i + 2].position, localT);
}

glm::quat ReplaySystem::getReplayRotation(float dt)
{
    if (m_samples.size() < 2) return m_samples.empty() ? glm::quat(1, 0, 0, 0) : m_samples.front().rotation;

    float totalTime = getTotalTime();
    if (m_replayTime > totalTime) {
        m_replayTime = totalTime;
    }

    int i = 0;
    while (i < m_samples.size() - 1 && m_samples[i].time < m_replayTime)
        i++;

    i = glm::clamp(i, 0, (int)m_samples.size() - 2);

    float t0 = m_samples[i].time;
    float t1 = m_samples[i + 1].time;
    float segmentDuration = t1 - t0;

    float localT = 0.0f;
    if (segmentDuration > 0.0001f) {
        localT = (m_replayTime - t0) / segmentDuration;
    }

    localT = glm::clamp(localT, 0.0f, 1.0f);
    return glm::slerp(m_samples[i].rotation, m_samples[i + 1].rotation, localT);
}