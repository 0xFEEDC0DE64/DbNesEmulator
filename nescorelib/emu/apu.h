#pragma once

#include "nescorelib_global.h"
#include <QObject>

// Qt includes
#include <QtGlobal>
#include <QVector>

// system includes
#include <array>

// local includes
#include "apudmc.h"
#include "apunos.h"
#include "apusq1.h"
#include "apusq2.h"
#include "aputrl.h"
#include "soundlowpassfilter.h"
#include "soundhighpassfilter.h"

// forward declarations
class QDataStream;
class NesEmulator;

class NESCORELIB_EXPORT Apu : public QObject
{
    Q_OBJECT

public:
    explicit Apu(NesEmulator &emu);

    void hardReset();
    void softReset();

    quint8 _ioRead(const quint16 address);
    quint8 ioRead(const quint16 address);
    void ioWrite(const quint16 address, const quint8 value);

    // io
    void blankAccess();
    void onRegister4014();
    void onRegister4015();
    void onRegister4016();
    void onRegister4017();
    void read4015();
    void read4016();
    void read4017();

    void clock();
    void clockDuration();
    void clockEnvelope();
    void checkIrq();
    void updatePlayback();

    void writeState(QDataStream &dataStream) const;
    void readState(QDataStream &dataStream);

    void flush();

    bool oddCycle() const;

    NesEmulator &emu();
    const NesEmulator &emu() const;

    ApuDmc &dmc();
    const ApuDmc &dmc() const;
    ApuNos &nos();
    const ApuNos &nos() const;
    ApuSq1 &sq1();
    const ApuSq1 &sq1() const;
    ApuSq2 &sq2();
    const ApuSq2 &sq2() const;
    ApuTrl &trl();
    const ApuTrl &trl() const;

    void setIrqDeltaOccur(bool irqDeltaOccur);

    bool regAccessW() const;

    quint8 regIoDb() const;
    void setRegIoDb(quint8 regIoDb);

    bool regAccessHappened() const;

    quint8 regIoAddr() const;

    qint32 sampleRate() const;
    void setSampleRate(qint32 sampleRate);

    static const std::array<std::array<quint8, 8>, 4> m_sqDutyCycleSequences;
    static const std::array<quint8, 32> m_sqDurationTable;

Q_SIGNALS:
    void samplesFinished(const QVector<qint32> &samples);

private:
    NesEmulator &m_emu;

    ApuDmc m_dmc;
    ApuNos m_nos;
    ApuSq1 m_sq1;
    ApuSq2 m_sq2;
    ApuTrl m_trl;

    //DATA REG
    quint8 m_regIoDb {}; //The data bus
    quint8 m_regIoAddr {}; //The address bus
    bool m_regAccessHappened {}; // Triggers when cpu accesses apu bus.
    bool m_regAccessW {}; //True= write access, False= Read access.

    bool m_oddCycle {};
    bool m_irqEnabled {};
    bool m_irqFlag {};
    bool m_irqDeltaOccur {};

    bool m_seqMode {};
    static constexpr qint32 m_freqF = 14914; //IRQ clock frequency
    static constexpr qint32 m_freqL = 7456; //Length counter clock
    static constexpr qint32 m_freqE = 3728; //Envelope counter clock
    qint32 m_cycleF {};
    qint32 m_cycleFt {};
    qint32 m_cycleE {};
    qint32 m_cycleL {};
    bool m_oddL {};
    bool m_checkIrq {};
    bool m_doEnv {};
    bool m_doLength {};

    //DAC
    double m_pulseOut {};
    double m_tndOut {};

    //Output values
    double m_audioX {};
    double m_audioX1 {};
    double m_audioY {};
    double m_audioYClocks {};
    static constexpr double m_audioDcR = 0.995;
    double m_timer {};

    SoundLowPassFilter m_lowPassFilter;
    SoundHighPassFilter m_highPassFilter1;
    SoundHighPassFilter m_highPassFilter2;

    bool m_inputStrobe {};

    QVector<qint32> m_samples;
    qint32 m_sampleRate { 44100 };
};
