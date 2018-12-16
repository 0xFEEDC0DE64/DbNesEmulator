#pragma once

#include "nescorelib_global.h"

class NESCORELIB_EXPORT SoundHighPassFilter
{
public:
    SoundHighPassFilter(const double k);

    void reset();
    double doFiltering(const double sample);

private:
    const double m_k;
    double m_x;
    double m_y;
};
