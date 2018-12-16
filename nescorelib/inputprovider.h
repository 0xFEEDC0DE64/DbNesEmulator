#pragma once

#include <QtGlobal>

class InputProvider
{
public:
    virtual void update() = 0;
    virtual quint8 getData() const = 0;
};
