#include "soundlowpassfilter.h"

SoundLowPassFilter::SoundLowPassFilter(const double k) :
    m_k(k)
{
    reset();
}

void SoundLowPassFilter::reset()
{
    m_x = 0.;
    m_y = 0.;
}

double SoundLowPassFilter::doFiltering(const double sample)
{
    const auto filtered = (sample - m_y) * m_k;
    m_x = sample;
    m_y = filtered;
    return filtered;
}
