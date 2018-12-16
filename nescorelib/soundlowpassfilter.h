#pragma once

#include "nescorelib_global.h"

class NESCORELIB_EXPORT SoundLowPassFilter
{
public:
    SoundLowPassFilter(const double k);

    void reset();
    double doFiltering(const double sample);

private:
    const double m_k;
    double m_y;
    double m_x;
};
