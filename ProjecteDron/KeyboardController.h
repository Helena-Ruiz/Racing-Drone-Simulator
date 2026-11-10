#pragma once
#include "Controller.h"
#include "Defines.h"

class KeyboardController : public Controller
{
public:
    KeyboardController();
    void update() override;
    float getAxis(const std::string& axisName) const override;
    bool getButton(const std::string& buttonName) const override;
	void setMode(MODE_VOL mode) { m_mode = mode; }
private:
    float m_axisValues[4];
    bool m_buttonStates[4];
    bool m_prevButtonStates[4];
	MODE_VOL m_mode = MODE_VOL::ACROBATIC;
};