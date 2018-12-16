#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QVector>
#include <QByteArray>

// system includes
#include <optional>
#include <array>

// local includes
#include "enums/mirroring.h"

struct NESCORELIB_EXPORT Rom
{
    int prgCount;
    int chrCount;
    Mirroring mirroring;
    bool hasBattery;
    bool hasTrainer;
    int mapperNumber;
    bool isVsUnisystem;
    bool isPlaychoice10;

    QVector<std::array<quint8, 0x1000> > prg;
    QVector<std::array<quint8, 0x400> > chr;
    std::array<quint8, 512> trainer;

    static Rom fromFile(const QString &path);
};
