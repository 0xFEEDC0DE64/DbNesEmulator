#include "mapper003.h"

QString Mapper003::name() const
{
    return QStringLiteral("CNROM");
}

quint8 Mapper003::mapper() const
{
    return 3;
}

void Mapper003::writePrg(quint16 address, quint8 value)
{
    // Bus conflicts !!
    switch8kChr(readPrg(address) & value);
}
