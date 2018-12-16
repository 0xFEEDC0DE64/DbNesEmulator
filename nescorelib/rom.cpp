#include "rom.h"

// Qt includes
#include <QFile>
#include <QFileInfo>

// system includes
#include <stdexcept>
#include <array>

Rom Rom::fromFile(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(QString("cannot open file %0 because %1").arg(file.fileName(), file.errorString()).toStdString());

    std::array<quint8, 16> header {};

    if(file.read(reinterpret_cast<char*>(&header[0]), 16) != 16)
        throw std::runtime_error("rom is not long enough");

    if(header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A)
        throw std::runtime_error("wrong header");

    Rom rom;
    rom.prgCount = header[4];
    rom.chrCount = header[5];
    rom.mirroring = Mirroring(0);
    switch (header[6] & 0x09)
    {
    case 0: rom.mirroring = Mirroring::Horizontal; break;
    case 1: rom.mirroring = Mirroring::Vertical; break;
    case 8:
    case 9: rom.mirroring = Mirroring::Full; break;
    }
    rom.hasBattery = header[6] & 0x0F;
    rom.hasTrainer = header[6] & 0x04;

    auto temp0 = header[6] >> 4;
    auto temp1 = header[7] & 0x0F;
    auto temp2 = header[7] & 0xF0;

    rom.mapperNumber = temp0;
    if(temp1 == 0)
        rom.mapperNumber |= temp2;

    rom.isVsUnisystem = header[7] & 0x01;
    rom.isPlaychoice10 = header[7] & 0x02;
    if(rom.hasTrainer)
    {
        if(file.read(reinterpret_cast<char*>(&rom.trainer[0]), 512) != 512)
            throw std::runtime_error("rom is not long enough fortrainer");
    }

    rom.prg.resize(rom.prgCount * 4);

    for(int i = 0; i < rom.prg.size(); i++)
    {
        if(file.read(reinterpret_cast<char*>(&rom.prg[i][0]), 0x1000) != 0x1000)
            throw std::runtime_error("rom is not long enough forprg");
    }

    rom.chr.resize(rom.chrCount * 8);
    for(int i = 0; i < rom.chr.size(); i++)
        if(file.read(reinterpret_cast<char*>(&rom.chr[i][0]), 0x400) != 0x400)
            throw std::runtime_error("rom is not long enough forchr");

    return rom;
}
