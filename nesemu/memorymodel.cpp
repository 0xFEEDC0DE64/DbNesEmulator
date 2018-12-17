#include "memorymodel.h"

// nescorelib includes
#include "nesemulator.h"

MemoryModel::MemoryModel(NesEmulator &emu, QObject *parent) :
    QAbstractTableModel(parent),
    m_emu(emu),
    m_wram(emu.memory().wram())
{
}

int MemoryModel::rowCount(const QModelIndex &parent) const
{
    return (END_ADDR - START_ADDR) / VALUES_PER_ROW;
}

int MemoryModel::columnCount(const QModelIndex &parent) const
{
    return VALUES_PER_ROW;
}

QVariant MemoryModel::data(const QModelIndex &index, int role) const
{
    const quint16 address = START_ADDR + (index.row() * VALUES_PER_ROW) + index.column();
    const auto arrIndex = Memory::wramAddressToIndex(address);

    switch(role)
    {
    case Qt::DisplayRole:
        return QString("%0").arg(m_wram[arrIndex], 2, 16, QLatin1Char('0'));
    }

    return QVariant();
}

QVariant MemoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
            return QString::number(section, 16);
        else
            return QString("0x%0").arg(START_ADDR + (section * VALUES_PER_ROW), 4, 16, QLatin1Char('0'));
    }

    return QVariant();
}

void MemoryModel::refresh()
{
    const auto &newWram = m_emu.memory().wram();

    for(quint16 i = START_ADDR; i < END_ADDR; i++)
    {
        const auto arrIndex = Memory::wramAddressToIndex(i);
        if(m_wram[arrIndex] != newWram[arrIndex])
        {
            QModelIndex index = createIndex((i-START_ADDR)/VALUES_PER_ROW, (i-START_ADDR)%VALUES_PER_ROW);
            Q_EMIT dataChanged(index, index, QVector<int> { Qt::DisplayRole });
            m_wram[arrIndex] = newWram[arrIndex];
        }
    }
}
