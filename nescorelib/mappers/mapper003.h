#pragma once

#include "nescorelib_global.h"
#include "boards/board.h"

class NESCORELIB_EXPORT Mapper003 : public Board
{
public:
    using Board::Board;

    QString name() const Q_DECL_OVERRIDE;
    quint8 mapper() const Q_DECL_OVERRIDE;

    void writePrg(quint16 address, quint8 value) Q_DECL_OVERRIDE;
};
