#pragma once

#include "nescorelib_global.h"
#include "boards/board.h"

class NESCORELIB_EXPORT Mapper000 : public Board
{
public:
    using Board::Board;

    QString name() const Q_DECL_OVERRIDE;
    quint8 mapper() const Q_DECL_OVERRIDE;
};
