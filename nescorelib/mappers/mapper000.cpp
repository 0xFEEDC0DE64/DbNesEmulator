#include "mapper000.h"

QString Mapper000::name() const
{
    return QStringLiteral("NROM");
}

quint8 Mapper000::mapper() const
{
    return 0;
}
