#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/component_wise.hpp>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>

struct DronePosTime {
    glm::vec3 position;
    glm::quat rotation;
    float time;
};

class ReplaySystem {
public:

    bool isReplaying() const { return m_replay; }
    bool isRecordingbool() { return m_recording; }
   
	void setRecording(bool isRecording) { m_recording = isRecording; }
    void startRecording(float currentTime);
    void stopRecording();
    void startReplay();
    void stopReplay();
	void saveReplay();
	void loadReplay(const std::string& filename);
	void setReplayTime(float time) { m_replayTime = time; }
    void record(const glm::vec3& pos, const glm::quat& rot, float currentTime);
	void setPlaybackSpeed(float speed) { m_playbackSpeed = speed; }

	float getPlaybackSpeed() const { return m_playbackSpeed; }
    float getTotalTime();   
	float getReplayTime() const { return m_replayTime; }

	void setreplayzero() { m_samples.clear(); }

	glm::vec3 getReplayPosition(float dt);
    glm::quat getReplayRotation(float dt);
    glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t);
    glm::vec3 bezier(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t);

private:

    bool m_recording = false;
    std::vector<DronePosTime> m_samples;
    bool m_replay = false;
    float m_startTime = 0.0f;
    float m_replayTime = 0.0f;
	float m_playbackSpeed = 1.0f;
};