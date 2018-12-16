#include "mapper002.h"

QString Mapper002::name() const
{
    return QStringLiteral("UxROM");
}

quint8 Mapper002::mapper() const
{
    return 2;
}

void Mapper002::hardReset()
{
    Board::hardReset();
    switch16kPrg(prgRom16KbMask(), PRGArea::AreaC000);
}

void Mapper002::writePrg(quint16 address, quint8 value)
{
    Q_UNUSED(address)
    switch16kPrg(value, PRGArea::Area8000);
}
