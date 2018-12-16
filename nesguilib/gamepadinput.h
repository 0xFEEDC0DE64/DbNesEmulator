#pragma once

#include "nesguilib_global.h"
#include "inputprovider.h"

// Qt includes
#include <QGamepad>

class NESGUILIB_EXPORT GamepadInput : public InputProvider
{
public:
    explicit GamepadInput(int deviceId = 0);

    void update();
    quint8 getData() const;

private:
    QGamepad m_gamepad;
    quint8 m_data {};
};
