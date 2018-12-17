#pragma once

// Qt includes
#include <QAbstractTableModel>

// system includes
#include <array>

// forward declarations
class NesEmulator;

class MemoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MemoryModel(NesEmulator &emu, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void refresh();

private:
    static constexpr quint16 START_ADDR = 0x0000;
    static constexpr quint16 END_ADDR = 0x2000;
    static constexpr quint16 VALUES_PER_ROW = 16;

    NesEmulator &m_emu;
    std::array<quint8, 0x0800> m_wram;
};
