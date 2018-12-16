#include "soundhighpassfilter.h"

SoundHighPassFilter::SoundHighPassFilter(const double k) :
    m_k(k)
{
    reset();
}

void SoundHighPassFilter::reset()
{
    m_x = 0.;
    m_y = 0.;
}

double SoundHighPassFilter::doFiltering(const double sample)
{
    const auto filtered = (m_y * m_k) + (sample - m_x);
    m_x = sample;
    m_y = filtered;
    return filtered;
}
