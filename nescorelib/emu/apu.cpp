#include "apu.h"

// Qt includes
#include <QDataStream>

// system includes
#include <algorithm>

// local includes
#include "nesemulator.h"
#include "emusettings.h"

Apu::Apu(NesEmulator &emu) :
    QObject(&emu),
    m_emu(emu),
    m_dmc(*this),
    m_nos(*this),
    m_sq1(*this),
    m_sq2(*this),
    m_trl(*this),
    m_lowPassFilter(0.815686),   // 14 KHz
    m_highPassFilter1(0.999835), // 90 Hz
    m_highPassFilter2(0.996039)  // 442 Hz
{
}

void Apu::hardReset()
{
    m_regIoDb = 0;
    m_regIoAddr = 0;
    m_regAccessHappened = false;
    m_regAccessW = false;
    m_seqMode = false;
    m_oddCycle = false;
    m_cycleFt = 0;
    m_cycleE = 4;
    m_cycleF = 4;
    m_cycleL = 4;
    m_oddL = false;
    m_checkIrq = false;
    m_doEnv = false;
    m_doLength = false;

    m_sq1.apuSq1HardReset();
    m_sq2.apuSq2HardReset();
    m_nos.apuNosHardReset();
    m_dmc.apuDmcHardReset();
    m_trl.apuTrlHardReset();

    m_irqEnabled = true;
    m_irqFlag = false;

    m_timer = 0.;
    m_audioX = 0.;
    m_audioX1 = 0.;
    m_audioY = 0.;
    m_audioYClocks = 0.;

    m_highPassFilter1.reset();
    m_highPassFilter2.reset();
    m_lowPassFilter.reset();
}

void Apu::softReset()
{
    m_regIoDb = 0;
    m_regIoAddr = 0;
    m_regAccessHappened = false;
    m_regAccessW = false;
    m_seqMode = false;
    m_oddCycle = false;
    m_cycleFt = 0;
    m_cycleE = 4;
    m_cycleF = 4;
    m_cycleL = 4;
    m_oddL = false;
    m_checkIrq = false;
    m_doEnv = false;
    m_doLength = false;

    m_irqEnabled = true;
    m_irqFlag = false;

    m_sq1.apuSq1SoftReset();
    m_sq2.apuSq2SoftReset();
    m_trl.apuTrlSoftReset();
    m_nos.apuNosSoftReset();
    m_dmc.apuDmcSoftReset();
}

quint8 Apu::_ioRead(const quint16 address)
{
    static constexpr std::array<void (Apu::*)(), 0x20> apuRegReadFunc = []() {
        std::array<void (Apu::*)(), 0x20> apuRegReadFunc {};
        for(std::size_t i = 0; i < 0x20; i++)
            apuRegReadFunc[i] = &Apu::blankAccess;
        apuRegReadFunc[0x15] = &Apu::read4015;
        apuRegReadFunc[0x16] = &Apu::read4016;
        apuRegReadFunc[0x17] = &Apu::read4017;
        return apuRegReadFunc;
    }();

    if(address >= 0x4020)
        return m_emu.memory().board()->readEx(address);
    else
    {
        m_regIoAddr = address & 0x1F;
        m_regAccessHappened = true;
        m_regAccessW = false;
        (this->*apuRegReadFunc[m_regIoAddr])();
        return m_regIoDb;
    }
}

quint8 Apu::ioRead(const quint16 address)
{
    auto result = _ioRead(address);
    return result;
}

void Apu::ioWrite(const quint16 address, const quint8 value)
{
    if(address >= 0x4020)
        m_emu.memory().board()->writeEx(address, value);
    else
    {
        m_regIoDb = value;
        m_regIoAddr = address & 0x1F;
        m_regAccessHappened = true;
        m_regAccessW = true;
    }
}

void Apu::blankAccess()
{
}

void Apu::onRegister4014()
{
    if(!m_regAccessW)
        return;

    // oam dma
    m_emu.dma().setOamAddress(m_regIoDb << 8);
    m_emu.dma().assertOamDma();
}

void Apu::onRegister4015()
{
    if(!m_regAccessW)
    {
        // on reads, do the effects we know
        m_irqFlag = false;
        m_emu.interrupts().removeFlag(Interrupts::IRQ_APU);
    }
    else
    {
        // do a normal write
        m_sq1.apuSq1On4015();
        m_sq2.apuSq2On4015();
        m_nos.apuNosOn4015();
        m_trl.apuTrlOn4015();
        m_dmc.apuDmcOn4015();
    }
}

void Apu::onRegister4016()
{
    // Only writes accepted
    if(m_regAccessW)
    {
        if(m_inputStrobe && m_regIoDb == 0)
            m_emu.ports().updatePorts();

        m_inputStrobe = m_regIoDb;
    }
    else
        m_emu.ports().setPort0(m_emu.ports().port0() >> 1);
}

void Apu::onRegister4017()
{
    if(m_regAccessW)
    {
        m_seqMode = (m_regIoDb & 0x80) != 0;
        m_irqEnabled = (m_regIoDb & 0x40) == 0;

        // Reset counters
        m_cycleE = -1;
        m_cycleL = -1;
        m_cycleF = -1;
        m_oddL = false;
        // Clock immediately ?
        m_doLength = m_seqMode;
        m_doEnv = m_seqMode;
        // Reset irq
        m_checkIrq = false;

        if(!m_irqEnabled)
        {
            m_irqFlag = false;
            m_emu.interrupts().removeFlag(Interrupts::IRQ_APU);
        }
    }
    else
        m_emu.ports().setPort1(m_emu.ports().port1() >> 1);
}

void Apu::read4015()
{
    m_regIoDb = m_regIoDb & 0x20;

    // Channels enable
    m_sq1.apuSq1Read4015();
    m_sq2.apuSq2Read4015();
    m_nos.apuNosRead4015();
    m_trl.apuTrlRead4015();
    m_dmc.apuDmcRead4015();

    // IRQ
    if(m_irqFlag)
        m_regIoDb = (m_regIoDb & 0xBF) | 0x40;
    if(m_irqDeltaOccur)
        m_regIoDb = (m_regIoDb & 0x7F) | 0x80;
}

void Apu::read4016()
{
    m_regIoDb = m_emu.ports().port0() & 1;
}

void Apu::read4017()
{
    m_regIoDb = m_emu.ports().port1() & 1;
}

void Apu::clock()
{
    m_oddCycle = !m_oddCycle;

    if(m_doEnv)
        clockEnvelope();

    if(m_doLength)
        clockDuration();

    if(!m_oddCycle)
    {
        // IRQ
        m_cycleF++;
        if(m_cycleF >= m_freqF)
        {
            m_cycleF = -1;
            m_checkIrq = true;
            m_cycleFt = 3;
        }

        // Envelope
        m_cycleE++;
        if(m_cycleE >= m_freqE)
        {
            m_cycleE = -1;
            // Clock envelope and other units except when:
            // 1 the seq mode is set
            // 2 it is the time of irq check clock
            if(m_checkIrq)
            {
                if(!m_seqMode)
                {
                    // this is the 3rd step of mode 0, do a reset
                    m_doEnv = true;
                }
                else
                {
                    // the next step will be the 4th step of mode 1
                    // so, shorten the step then do a reset
                    m_cycleE = 4;
                }
            }
            else
                m_doEnv = true;
        }

        // Length
        m_cycleL++;
        if(m_cycleL >= m_freqL)
        {
            m_oddL = !m_oddL;

            m_cycleL = m_oddL ? -2 : -1;

            // Clock duration and sweep except when:
            // 1 the seq mode is set
            // 2 it is the time of irq check clock
            if(m_checkIrq && m_seqMode)
            {
                m_cycleL = 3730;// Next step will be after 7456 - 3730 = 3726 cycles, 2 cycles shorter than e freq
                m_oddL = true;
            }
            else
            {
                m_doLength = true;
            }
        }

        if(m_regAccessHappened)
        {
            m_regAccessHappened = false;
            switch(m_regIoAddr)
            {
            case 0: m_sq1.apuOnRegister4000(); break;
            case 1: m_sq1.apuOnRegister4001(); break;
            case 2: m_sq1.apuOnRegister4002(); break;
            case 3: m_sq1.apuOnRegister4003(); break;
            case 4: m_sq2.apuOnRegister4004(); break;
            case 5: m_sq2.apuOnRegister4005(); break;
            case 6: m_sq2.apuOnRegister4006(); break;
            case 7: m_sq2.apuOnRegister4007(); break;
            case 8: m_trl.apuOnRegister4008(); break;
            case 9: m_trl.apuOnRegister4009(); break;
            case 10: m_trl.apuOnRegister400A(); break;
            case 11: m_trl.apuOnRegister400B(); break;
            case 12: m_nos.apuOnRegister400C(); break;
            case 13: m_nos.apuOnRegister400D(); break;
            case 14: m_nos.apuOnRegister400E(); break;
            case 15: m_nos.apuOnRegister400F(); break;
            case 16: m_dmc.apuOnRegister4010(); break;
            case 17: m_dmc.apuOnRegister4011(); break;
            case 18: m_dmc.apuOnRegister4012(); break;
            case 19: m_dmc.apuOnRegister4013(); break;
            case 20: onRegister4014(); break;
            case 21: onRegister4015(); break;
            case 22: onRegister4016(); break;
            case 23: onRegister4017(); break;
            case 24:
            case 25:
            case 26:
            case 27:
            case 28:
            case 29:
            case 30:
            case 31: blankAccess(); break;
            }
        }

        m_sq1.apuSq1Clock();
        m_sq2.apuSq2Clock();
        m_nos.apuNosClock();

        if(m_emu.memory().board()->enableExternalSound())
            m_emu.memory().board()->onApuClock();

        //apuUpdatePlayback();
    }

    m_trl.apuTrlClock();
    m_dmc.apuDmcClock();

    if(m_emu.memory().board()->enableExternalSound())
        m_emu.memory().board()->onApuClockSingle();

    updatePlayback();

    if(m_checkIrq)
    {
        if(!m_seqMode)
            checkIrq();

        // This is stupid ... :(
        m_cycleFt--;
        if(m_cycleFt == 0)
            m_checkIrq = false;
    }
}

void Apu::clockDuration()
{
    m_sq1.apuSq1ClockLength();
    m_sq2.apuSq2ClockLength();
    m_nos.apuNosClockLength();
    m_trl.apuTrlClockLength();
    if(m_emu.memory().board()->enableExternalSound())
        m_emu.memory().board()->onApuClockDuration();
    m_doLength = false;
}

void Apu::clockEnvelope()
{
    m_sq1.apuSq1ClockEnvelope();
    m_sq2.apuSq2ClockEnvelope();
    m_nos.apuNosClockEnvelope();
    m_trl.apuTrlClockEnvelope();
    if(m_emu.memory().board()->enableExternalSound())
        m_emu.memory().board()->onApuClockEnvelope();
    m_doEnv = false;
}

void Apu::checkIrq()
{
    if(m_irqEnabled)
        m_irqFlag = true;
    if(m_irqFlag)
        m_emu.interrupts().addFlag(Interrupts::IRQ_APU);
}

void Apu::updatePlayback()
{
    static constexpr std::array<qreal, 32> audioPulseTable = []() constexpr {
        std::array<qreal, 32> audioPulseTable {};
        for(std::size_t i = 1; i < 32; i++)
            audioPulseTable[i] = 95.52 / (8128. / i + 100.);
        return audioPulseTable;
    }();

    static constexpr std::array<qreal, 204> audioTndTable = []() constexpr {
        std::array<qreal, 204> audioTndTable {};
        for(std::size_t i = 1; i < 204; i++)
            audioTndTable[i] = 163.67 / (24329. / i + 100.);
        return audioTndTable;
    }();

    // Collect the sample
    m_pulseOut = audioPulseTable[m_sq1.output() + m_sq2.output()];
    m_tndOut = audioTndTable[(3 * m_trl.output()) + (2 * m_nos.output()) + m_dmc.output()];

    m_audioX = m_pulseOut + m_tndOut;

    if(m_emu.memory().board()->enableExternalSound())
    {
        m_audioX += m_emu.memory().board()->apuGetSample();
        m_audioX /= 2;
    }

    m_audioX *= EmuSettings::Audio::internalAmplitude;

    // An implementation of the "Band-Limited Sound Synthesis" algorithm.
    // http://www.slack.net/~ant/bl-synth
    // Shay Green <hotpop.com@blargg> (swap to e-mail)
    // No sure about this, it just add the sample difference in each step.
    // Maybe here it is acting just like a low-pass filter ...
    if(m_audioX != m_audioX1)
    {
        if(m_audioX > m_audioX1)
            m_audioY += m_audioX - m_audioX1;
        else
            m_audioY += m_audioX1 - m_audioX;

        m_audioX = m_audioX1;
    }

    m_audioYClocks++;
    m_timer++;

    const auto audiotimerratio = m_freqL * 2. * 120. / m_sampleRate;

    if(m_timer >= audiotimerratio)
    {
        // Clock by output sample rate.
        m_timer -= audiotimerratio;

        // Get the sum of the samples
        m_audioY /= m_audioYClocks;

        // Do filtering, add 2 high-pass and one low-pass filters. See http://wiki.nesdev.com/w/index.php/APUMixer
        double audioDcY;
        audioDcY = m_highPassFilter2.doFiltering(m_audioY);// 442 Hz
        audioDcY = m_highPassFilter1.doFiltering(audioDcY);// 90 Hz
        audioDcY = m_lowPassFilter.doFiltering(audioDcY);// 14 KHz
        audioDcY = std::clamp(audioDcY, double(-EmuSettings::Audio::internalPeekLimit), double(EmuSettings::Audio::internalPeekLimit));

        m_samples.append(audioDcY);

        m_audioY = 0;
        m_audioYClocks = 0;
    }
}

void Apu::writeState(QDataStream &dataStream) const
{
    dataStream << m_regIoDb << m_regIoAddr << m_regAccessHappened << m_regAccessW << m_oddCycle << m_irqEnabled << m_irqFlag << m_irqDeltaOccur
               << m_seqMode << m_cycleF << m_cycleE << m_cycleL << m_oddL << m_cycleFt << m_checkIrq << m_doEnv << m_doLength << m_inputStrobe;

    m_sq1.apuSq1WriteState(dataStream);
    m_sq2.apuSq2WriteState(dataStream);
    m_nos.apuNosWriteState(dataStream);
    m_trl.apuTrlWriteState(dataStream);
    m_dmc.apuDmcWriteState(dataStream);
}

void Apu::readState(QDataStream &dataStream)
{
    dataStream >> m_regIoDb >> m_regIoAddr >> m_regAccessHappened >> m_regAccessW >> m_oddCycle >> m_irqEnabled >> m_irqFlag >> m_irqDeltaOccur
               >> m_seqMode >> m_cycleF >> m_cycleE >> m_cycleL >> m_oddL >> m_cycleFt >> m_checkIrq >> m_doEnv >> m_doLength >> m_inputStrobe;

    m_sq1.apuSq1ReadState(dataStream);
    m_sq2.apuSq2ReadState(dataStream);
    m_nos.apuNosReadState(dataStream);
    m_trl.apuTrlReadState(dataStream);
    m_dmc.apuDmcReadState(dataStream);
}

void Apu::flush()
{
    m_timer = 0;
    Q_EMIT samplesFinished(m_samples);
    m_samples.clear();
}

bool Apu::oddCycle() const
{
    return m_oddCycle;
}

NesEmulator &Apu::emu()
{
    return m_emu;
}

const NesEmulator &Apu::emu() const
{
    return m_emu;
}

ApuDmc &Apu::dmc()
{
    return m_dmc;
}

const ApuDmc &Apu::dmc() const
{
    return m_dmc;
}

ApuNos &Apu::nos()
{
    return m_nos;
}

const ApuNos &Apu::nos() const
{
    return m_nos;
}

ApuSq1 &Apu::sq1()
{
    return m_sq1;
}

const ApuSq1 &Apu::sq1() const
{
    return m_sq1;
}

ApuSq2 &Apu::sq2()
{
    return m_sq2;
}

const ApuSq2 &Apu::sq2() const
{
    return m_sq2;
}

ApuTrl &Apu::trl()
{
    return m_trl;
}

const ApuTrl &Apu::trl() const
{
    return m_trl;
}

void Apu::setIrqDeltaOccur(bool irqDeltaOccur)
{
    m_irqDeltaOccur = irqDeltaOccur;
}

bool Apu::regAccessW() const
{
    return m_regAccessW;
}

quint8 Apu::regIoDb() const
{
    return m_regIoDb;
}

void Apu::setRegIoDb(quint8 regIoDb)
{
    m_regIoDb = regIoDb;
}

bool Apu::regAccessHappened() const
{
    return m_regAccessHappened;
}

quint8 Apu::regIoAddr() const
{
    return m_regIoAddr;
}

qint32 Apu::sampleRate() const
{
    return m_sampleRate;
}

void Apu::setSampleRate(qint32 sampleRate)
{
    m_sampleRate = sampleRate;
}

const std::array<std::array<quint8, 8>, 4> Apu::m_sqDutyCycleSequences {
    std::array<quint8, 8> {  0, 1, 0, 0, 0, 0, 0, 0 }, // 12.5%
    std::array<quint8, 8> {  0, 1, 1, 0, 0, 0, 0, 0 }, // 25.0%
    std::array<quint8, 8> {  0, 1, 1, 1, 1, 0, 0, 0 }, // 50.0%
    std::array<quint8, 8> {  1, 0, 0, 1, 1, 1, 1, 1 }, // 75.0% (25.0% negated)
};

const std::array<quint8, 32> Apu::m_sqDurationTable {
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
    0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
};
