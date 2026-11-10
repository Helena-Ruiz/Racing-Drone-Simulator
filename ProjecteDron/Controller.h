#pragma once
#include <string>
#include "Defines.h"

class Controller
{
protected:
	ControllerType m_type;
public:
    virtual ~Controller() = default;
    virtual void update() = 0;
	virtual void setMode(MODE_VOL mode) {}
    virtual float getAxis(const std::string& axisName) const = 0;
    virtual bool getButton(const std::string& buttonName) const = 0;
	ControllerType getType() const { return m_type; }
	void SetMode(MODE_VOL mode) { m_mode = mode; }

	void setType(ControllerType type) { m_type = type; }

	bool keyboardConnected = false;
    MODE_VOL m_mode;
};