#pragma once
#include "Controller.h"

class GamepadController : public Controller
{
public:
    GamepadController(int gamepadID);
    void update() override;
    float getAxis(const std::string& axisName) const override;
    bool getButton(const std::string& buttonName) const override;
    bool isConnected() const;

private:
    int m_gamepadID;
    float m_axisValues[8];
    bool m_buttonStates[4];
	bool m_prevButtonStates[4];
};
