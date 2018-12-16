#include "gamepadinput.h"

GamepadInput::GamepadInput(int deviceId) :
    m_gamepad(deviceId)
{
}

void GamepadInput::update()
{
    m_data = 0;
    if(m_gamepad.buttonA()) m_data += 1;
    if(m_gamepad.buttonX()) m_data += 2;
    if(m_gamepad.buttonSelect()) m_data += 4;
    if(m_gamepad.buttonStart()) m_data += 8;
    if(m_gamepad.buttonUp()) m_data += 16;
    if(m_gamepad.buttonDown()) m_data += 32;
    if(m_gamepad.buttonLeft()) m_data += 64;
    if(m_gamepad.buttonRight()) m_data += 128;
}

quint8 GamepadInput::getData() const
{
    return m_data;
}
