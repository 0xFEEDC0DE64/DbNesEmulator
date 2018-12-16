#include "cpu.h"

// Qt includes
#include <QDataStream>

// local includes
#include "nesemulator.h"

Cpu::Cpu(NesEmulator &emu) :
    m_emu(emu)
{
}

quint8 Cpu::getRegisterP() const
{
    return (m_flagN ? 0x80 : 0) |
           (m_flagV ? 0x40 : 0) |
           (m_flagD ? 0x08 : 0) |
           (m_flagI ? 0x04 : 0) |
           (m_flagZ ? 0x02 : 0) |
           (m_flagC ? 0x01 : 0) | 0x20;
}

void Cpu::setRegisterP(const quint8 value)
{
    m_flagN = value & 0x80;
    m_flagV = value & 0x40;
    m_flagD = value & 0x08;
    m_flagI = value & 0x04;
    m_flagZ = value & 0x02;
    m_flagC = value & 0x01;
}

quint8 Cpu::getRegisterPb() const
{
    return (m_flagN ? 0x80 : 0) |
           (m_flagV ? 0x40 : 0) |
           (m_flagD ? 0x08 : 0) |
           (m_flagI ? 0x04 : 0) |
           (m_flagZ ? 0x02 : 0) |
           (m_flagC ? 0x01 : 0) | 0x30;
}

void Cpu::clock()
{
    static constexpr std::array<void (Cpu::*)(), 256> cpuAddressings {
    //           0x0,           0x1,           0x2,           0x3,           0x4,           0x5,           0x6,           0x7
    //           0x8,           0x9,           0xA,           0xB,           0xC,           0xD,           0xE,           0xF
    /*0x0*/&Cpu::imp____, &Cpu::indX_r_, &Cpu::imA____, &Cpu::indX_w_, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_rw_, &Cpu::zpg_w__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::abs_r__, &Cpu::abs_r__, &Cpu::abs_rw_, &Cpu::abs_w__, // 0x0
    /*0x1*/&Cpu::imp____, &Cpu::indY_r_, &Cpu::imp____, &Cpu::indY_w_, &Cpu::zpgX_r_, &Cpu::zpgX_r_, &Cpu::zpgX_rw, &Cpu::zpgX_w_,
           &Cpu::imA____, &Cpu::absY_r_, &Cpu::imA____, &Cpu::absY_w_, &Cpu::absX_r_, &Cpu::absX_r_, &Cpu::absX_rw, &Cpu::absX_w_, // 0x1
    /*0x2*/&Cpu::imp____, &Cpu::indX_r_, &Cpu::imA____, &Cpu::indX_w_, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_rw_, &Cpu::zpg_w__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::abs_r__, &Cpu::abs_r__, &Cpu::abs_rw_, &Cpu::abs_w__, // 0x2
    /*0x3*/&Cpu::imp____, &Cpu::indY_r_, &Cpu::imp____, &Cpu::indY_w_, &Cpu::zpgX_r_, &Cpu::zpgX_r_, &Cpu::zpgX_rw, &Cpu::zpgX_w_,
           &Cpu::imA____, &Cpu::absY_r_, &Cpu::imA____, &Cpu::absY_w_, &Cpu::absX_r_, &Cpu::absX_r_, &Cpu::absX_rw, &Cpu::absX_w_, // 0x3
    /*0x4*/&Cpu::imA____, &Cpu::indX_r_, &Cpu::imA____, &Cpu::indX_w_, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_rw_, &Cpu::zpg_w__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::abs_w__, &Cpu::abs_r__, &Cpu::abs_rw_, &Cpu::abs_w__, // 0x4
    /*0x5*/&Cpu::imp____, &Cpu::indY_r_, &Cpu::imp____, &Cpu::indY_w_, &Cpu::zpgX_r_, &Cpu::zpgX_r_, &Cpu::zpgX_rw, &Cpu::zpgX_w_,
           &Cpu::imA____, &Cpu::absY_r_, &Cpu::imA____, &Cpu::absY_w_, &Cpu::absX_r_, &Cpu::absX_r_, &Cpu::absX_rw, &Cpu::absX_w_, // 0x5
    /*0x6*/&Cpu::imA____, &Cpu::indX_r_, &Cpu::imA____, &Cpu::indX_w_, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_rw_, &Cpu::zpg_w__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::imp____, &Cpu::abs_r__, &Cpu::abs_rw_, &Cpu::abs_w__, // 0x6
    /*0x7*/&Cpu::imp____, &Cpu::indY_r_, &Cpu::imp____, &Cpu::indY_w_, &Cpu::zpgX_r_, &Cpu::zpgX_r_, &Cpu::zpgX_rw, &Cpu::zpgX_w_,
           &Cpu::imA____, &Cpu::absY_r_, &Cpu::imA____, &Cpu::absY_w_, &Cpu::absX_r_, &Cpu::absX_r_, &Cpu::absX_rw, &Cpu::absX_w_, // 0x7
    /*0x8*/&Cpu::imm____, &Cpu::indX_w_, &Cpu::imm____, &Cpu::indX_w_, &Cpu::zpg_w__, &Cpu::zpg_w__, &Cpu::zpg_w__, &Cpu::zpg_w__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::abs_w__, &Cpu::abs_w__, &Cpu::abs_w__, &Cpu::abs_w__, // 0x8
    /*0x9*/&Cpu::imp____, &Cpu::indY_w_, &Cpu::imp____, &Cpu::indY_w_, &Cpu::zpgX_w_, &Cpu::zpgX_w_, &Cpu::zpgY_w_, &Cpu::zpgY_w_,
           &Cpu::imA____, &Cpu::absY_w_, &Cpu::imA____, &Cpu::absY_w_, &Cpu::abs_w__, &Cpu::absX_w_, &Cpu::abs_w__, &Cpu::absY_w_, // 0x9
    /*0xA*/&Cpu::imm____, &Cpu::indX_r_, &Cpu::imm____, &Cpu::indX_r_, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_r__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::abs_r__, &Cpu::abs_r__, &Cpu::abs_r__, &Cpu::abs_r__, // 0xA
    /*0xB*/&Cpu::imp____, &Cpu::indY_r_, &Cpu::imp____, &Cpu::indY_r_, &Cpu::zpgX_r_, &Cpu::zpgX_r_, &Cpu::zpgY_r_, &Cpu::zpgY_r_,
           &Cpu::imA____, &Cpu::absY_r_, &Cpu::imA____, &Cpu::absY_r_, &Cpu::absX_r_, &Cpu::absX_r_, &Cpu::absY_r_, &Cpu::absY_r_, // 0xB
    /*0xC*/&Cpu::imm____, &Cpu::indX_r_, &Cpu::imm____, &Cpu::indX_r_, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_rw_, &Cpu::zpg_r__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::abs_r__, &Cpu::abs_r__, &Cpu::abs_rw_, &Cpu::abs_r__, // 0xC
    /*0xD*/&Cpu::imp____, &Cpu::indY_r_, &Cpu::imp____, &Cpu::indY_rw, &Cpu::zpgX_r_, &Cpu::zpgX_r_, &Cpu::zpgX_rw, &Cpu::zpgX_rw,
           &Cpu::imA____, &Cpu::absY_r_, &Cpu::imA____, &Cpu::absY_rw, &Cpu::absX_r_, &Cpu::absX_r_, &Cpu::absX_rw, &Cpu::absX_rw, // 0xD
    /*0xE*/&Cpu::imm____, &Cpu::indX_r_, &Cpu::imm____, &Cpu::indX_w_, &Cpu::zpg_r__, &Cpu::zpg_r__, &Cpu::zpg_rw_, &Cpu::zpg_w__,
           &Cpu::imA____, &Cpu::imm____, &Cpu::imA____, &Cpu::imm____, &Cpu::abs_r__, &Cpu::abs_r__, &Cpu::abs_rw_, &Cpu::abs_w__, // 0xE
    /*0xF*/&Cpu::imp____, &Cpu::indY_r_, &Cpu::imp____, &Cpu::indY_w_, &Cpu::zpgX_r_, &Cpu::zpgX_r_, &Cpu::zpgX_rw, &Cpu::zpgX_w_,
           &Cpu::imA____, &Cpu::absY_r_, &Cpu::imA____, &Cpu::absY_w_, &Cpu::absX_r_, &Cpu::absX_r_, &Cpu::absX_rw, &Cpu::absX_w_, // 0xF
    };

    static constexpr std::array<void (Cpu::*)(), 256> cpuInstructions {
    //           0x0,         0x1,         0x2,         0x3,         0x4,         0x5,         0x6,         0x7
    //           0x8,         0x9,         0xA,         0xB,         0xC,         0xD,         0xE,         0xF
    /*0x0*/&Cpu::brk__, &Cpu::ora__, &Cpu::nop__, &Cpu::slo__, &Cpu::nop__, &Cpu::ora__, &Cpu::asl_m, &Cpu::slo__,
           &Cpu::php__, &Cpu::ora__, &Cpu::asl_a, &Cpu::anc__, &Cpu::nop__, &Cpu::ora__, &Cpu::asl_m, &Cpu::slo__, // 0x0
    /*0x1*/&Cpu::bpl__, &Cpu::ora__, &Cpu::nop__, &Cpu::slo__, &Cpu::nop__, &Cpu::ora__, &Cpu::asl_m, &Cpu::slo__,
           &Cpu::clc__, &Cpu::ora__, &Cpu::nop__, &Cpu::slo__, &Cpu::nop__, &Cpu::ora__, &Cpu::asl_m, &Cpu::slo__, // 0x1
    /*0x2*/&Cpu::jsr__, &Cpu::and__, &Cpu::nop__, &Cpu::rla__, &Cpu::bit__, &Cpu::and__, &Cpu::rol_m, &Cpu::rla__,
           &Cpu::plp__, &Cpu::and__, &Cpu::rol_a, &Cpu::anc__, &Cpu::bit__, &Cpu::and__, &Cpu::rol_m, &Cpu::rla__, // 0x2
    /*0x3*/&Cpu::bmi__, &Cpu::and__, &Cpu::nop__, &Cpu::rla__, &Cpu::nop__, &Cpu::and__, &Cpu::rol_m, &Cpu::rla__,
           &Cpu::sec__, &Cpu::and__, &Cpu::nop__, &Cpu::rla__, &Cpu::nop__, &Cpu::and__, &Cpu::rol_m, &Cpu::rla__, // 0x3
    /*0x4*/&Cpu::rti__, &Cpu::eor__, &Cpu::nop__, &Cpu::sre__, &Cpu::nop__, &Cpu::eor__, &Cpu::lsr_m, &Cpu::sre__,
           &Cpu::pha__, &Cpu::eor__, &Cpu::lsr_a, &Cpu::alr__, &Cpu::jmp__, &Cpu::eor__, &Cpu::lsr_m, &Cpu::sre__, // 0x4
    /*0x5*/&Cpu::bvm__, &Cpu::eor__, &Cpu::nop__, &Cpu::sre__, &Cpu::nop__, &Cpu::eor__, &Cpu::lsr_m, &Cpu::sre__,
           &Cpu::cli__, &Cpu::eor__, &Cpu::nop__, &Cpu::sre__, &Cpu::nop__, &Cpu::eor__, &Cpu::lsr_m, &Cpu::sre__, // 0x5
    /*0x6*/&Cpu::rts__, &Cpu::adc__, &Cpu::nop__, &Cpu::rra__, &Cpu::nop__, &Cpu::adc__, &Cpu::ror_m, &Cpu::rra__,
           &Cpu::pla__, &Cpu::adc__, &Cpu::ror_a, &Cpu::arr__, &Cpu::jmp_i, &Cpu::adc__, &Cpu::ror_m, &Cpu::rra__, // 0x6
    /*0x7*/&Cpu::bvs__, &Cpu::adc__, &Cpu::nop__, &Cpu::rra__, &Cpu::nop__, &Cpu::adc__, &Cpu::ror_m, &Cpu::rra__,
           &Cpu::sei__, &Cpu::adc__, &Cpu::nop__, &Cpu::rra__, &Cpu::nop__, &Cpu::adc__, &Cpu::ror_m, &Cpu::rra__, // 0x7
    /*0x8*/&Cpu::nop__, &Cpu::sta__, &Cpu::nop__, &Cpu::sax__, &Cpu::sty__, &Cpu::sta__, &Cpu::stx__, &Cpu::sax__,
           &Cpu::dey__, &Cpu::nop__, &Cpu::txa__, &Cpu::xaa__, &Cpu::sty__, &Cpu::sta__, &Cpu::stx__, &Cpu::sax__, // 0x8
    /*0x9*/&Cpu::bcc__, &Cpu::sta__, &Cpu::nop__, &Cpu::ahc__, &Cpu::sty__, &Cpu::sta__, &Cpu::stx__, &Cpu::sax__,
           &Cpu::tya__, &Cpu::sta__, &Cpu::txs__, &Cpu::xas__, &Cpu::shy__, &Cpu::sta__, &Cpu::shx__, &Cpu::ahc__, // 0x9
    /*0xA*/&Cpu::ldy__, &Cpu::lda__, &Cpu::ldx__, &Cpu::lax__, &Cpu::ldy__, &Cpu::lda__, &Cpu::ldx__, &Cpu::lax__,
           &Cpu::tay__, &Cpu::lda__, &Cpu::tax__, &Cpu::lax__, &Cpu::ldy__, &Cpu::lda__, &Cpu::ldx__, &Cpu::lax__, // 0xA
    /*0xB*/&Cpu::bcs__, &Cpu::lda__, &Cpu::nop__, &Cpu::lax__, &Cpu::ldy__, &Cpu::lda__, &Cpu::ldx__, &Cpu::lax__,
           &Cpu::clv__, &Cpu::lda__, &Cpu::tsx__, &Cpu::lar__, &Cpu::ldy__, &Cpu::lda__, &Cpu::ldx__, &Cpu::lax__, // 0xB
    /*0xC*/&Cpu::cpy__, &Cpu::cmp__, &Cpu::nop__, &Cpu::dcp__, &Cpu::cpy__, &Cpu::cmp__, &Cpu::dec__, &Cpu::dcp__,
           &Cpu::iny__, &Cpu::cmp__, &Cpu::dex__, &Cpu::axs__, &Cpu::cpy__, &Cpu::cmp__, &Cpu::dec__, &Cpu::dcp__, // 0xC
    /*0xD*/&Cpu::bne__, &Cpu::cmp__, &Cpu::nop__, &Cpu::dcp__, &Cpu::nop__, &Cpu::cmp__, &Cpu::dec__, &Cpu::dcp__,
           &Cpu::cld__, &Cpu::cmp__, &Cpu::nop__, &Cpu::dcp__, &Cpu::nop__, &Cpu::cmp__, &Cpu::dec__, &Cpu::dcp__, // 0xD
    /*0xE*/&Cpu::cpx__, &Cpu::sdc__, &Cpu::nop__, &Cpu::isc__, &Cpu::cpx__, &Cpu::sdc__, &Cpu::inc__, &Cpu::isc__,
           &Cpu::inx__, &Cpu::sdc__, &Cpu::nop__, &Cpu::sdc__, &Cpu::cpx__, &Cpu::sdc__, &Cpu::inc__, &Cpu::isc__, // 0xE
    /*0xF*/&Cpu::beq__, &Cpu::sdc__, &Cpu::nop__, &Cpu::isc__, &Cpu::nop__, &Cpu::sdc__, &Cpu::inc__, &Cpu::isc__,
           &Cpu::sed__, &Cpu::sdc__, &Cpu::nop__, &Cpu::isc__, &Cpu::nop__, &Cpu::sdc__, &Cpu::inc__, &Cpu::isc__, // 0xF
    };

    m_opcode = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;

    (this->*cpuAddressings[m_opcode])();
    (this->*cpuInstructions[m_opcode])();

    //handle interrupts
    if(m_irqPin || m_nmiPin)
    {
        m_emu.memory().read(m_regPc.v);
        m_emu.memory().read(m_regPc.v);
        interrupt();
    }
}

void Cpu::hardReset()
{
    m_regA = 0;
    m_regX = 0;
    m_regY = 0;

    m_regSp.v = 0x01FD;

    m_regPc.l = m_emu.memory().board()->readPrg(0xFFFC);
    m_regPc.h = m_emu.memory().board()->readPrg(0xFFFD);

    setRegisterP(0);
    m_flagI = true;
    m_regEa.v = 0;
    m_opcode = 0;

    //interrupts
    m_irqPin = false;
    m_nmiPin = false;
    m_suspendNmi = false;
    m_suspendIrq = false;
    m_emu.interrupts().setFlags(0);
}

void Cpu::softReset()
{
    m_flagI = true;
    m_regSp.v -= 3;

    m_regPc.l = m_emu.memory().board()->readPrg(0xFFFC);
    m_regPc.h = m_emu.memory().board()->readPrg(0xFFFD);
}

void Cpu::interrupt()
{
    push(m_regPc.h);
    push(m_regPc.l);

    push(m_opcode == 0 ? getRegisterPb() : getRegisterP());

    // pins are detected during φ2 of previous cycle (before push about 2 ppu
    // cycles)
    const auto temp = m_emu.interrupts().vector();

    // THEORY:
    // Once the vector requested, the interrupts are suspended and cleared
    // by setting the I flag and clearing the nmi detect flag. Also, the nmi
    // detection get suspended for 2 cycles while pulling PC, irq still can
    // be detected but will not be taken since I is set.
    m_suspendNmi = true;
    m_flagI = true;
    m_nmiPin = false;

    m_regPc.l = m_emu.memory().read(temp);
    m_regPc.h = m_emu.memory().read(temp + 1);

    m_suspendNmi = false;
}

void Cpu::branch(const bool condition)
{
    const auto temp = m_emu.memory().read(m_regPc.v++);

    if (condition) {
        m_suspendIrq = true;
        m_emu.memory().read(m_regPc.v);
        m_regPc.l += temp;
        m_suspendIrq = false;
        if (temp >= 0x80) {
            if (m_regPc.l >= temp) {
                m_emu.memory().read(m_regPc.v);
                m_regPc.h--;
            }
        } else {
            if (m_regPc.l < temp) {
                m_emu.memory().read(m_regPc.v);
                m_regPc.h++;
            }
        }
    }
}

void Cpu::push(const quint8 value)
{
    m_emu.memory().write(m_regSp.v--, value);
}

quint8 Cpu::_pull()
{
    return m_emu.memory().read(++m_regSp.v);
}

quint8 Cpu::pull()
{
    auto result = _pull();
    return result;
}

void Cpu::imp____()
{
    // No addressing mode ...
}

void Cpu::indX_r_()
{
    CpuRegister temp;
    temp.h = 0;// the zero page boundary crossing is not handled.
    temp.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// CLock 1
    m_emu.memory().read(temp.v);// Clock 2
    temp.l += m_regX;

    m_regEa.l = m_emu.memory().read(temp.v);// Clock 3
    temp.l++;

    m_regEa.h = m_emu.memory().read(temp.v);// Clock 4

    m_m = m_emu.memory().read(m_regEa.v);
}

void Cpu::indX_w_()
{
    CpuRegister temp;
    temp.h = 0;// the zero page boundary crossing is not handled.
    temp.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// CLock 1
    m_emu.memory().read(temp.v);// Clock 2
    temp.l += m_regX;

    m_regEa.l = m_emu.memory().read(temp.v);// Clock 3
    temp.l++;

    m_regEa.h = m_emu.memory().read(temp.v);// Clock 4
}

void Cpu::indX_rw()
{
    CpuRegister temp;
    temp.h = 0;// the zero page boundary crossing is not handled.
    temp.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// CLock 1
    m_emu.memory().read(temp.v);// Clock 2
    temp.l += m_regX;

    m_regEa.l = m_emu.memory().read(temp.v);// Clock 3
    temp.l++;

    m_regEa.h = m_emu.memory().read(temp.v);// Clock 4

    m_m = m_emu.memory().read(m_regEa.v);
}

void Cpu::indY_r_()
{
    CpuRegister temp;
    temp.h = 0;// the zero page boundary crossing is not handled.
    temp.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// CLock 1
    m_regEa.l = m_emu.memory().read(temp.v);// Clock 3
    temp.l++;// Clock 2
    m_regEa.h = m_emu.memory().read(temp.v);// Clock 4

    m_regEa.l += m_regY;

    m_m = m_emu.memory().read(m_regEa.v);
    if (m_regEa.l < m_regY)
    {
        m_regEa.h++;
        m_m = m_emu.memory().read(m_regEa.v);
    }
}

void Cpu::indY_w_()
{
    CpuRegister temp;
    temp.h = 0;// the zero page boundary crossing is not handled.
    temp.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// CLock 1

    m_regEa.l = m_emu.memory().read(temp.v);
    temp.l++;// Clock 2

    m_regEa.h = m_emu.memory().read(temp.v);// Clock 2
    m_regEa.l += m_regY;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
    if (m_regEa.l < m_regY)
        m_regEa.h++;
}

void Cpu::indY_rw()
{
    CpuRegister temp;
    temp.h = 0;// the zero page boundary crossing is not handled.
    temp.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// CLock 1
    m_regEa.l = m_emu.memory().read(temp.v);
    temp.l++;// Clock 2
    m_regEa.h = m_emu.memory().read(temp.v);// Clock 2

    m_regEa.l += m_regY;

    m_emu.memory().read(m_regEa.v);// Clock 3
    if (m_regEa.l < m_regY)
        m_regEa.h++;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::zpg_r__()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::zpg_w__()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
}

void Cpu::zpg_rw_()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::zpgX_r_()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_emu.memory().read(m_regEa.v);// Clock 2
    m_regEa.l += m_regX;
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::zpgX_w_()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_emu.memory().read(m_regEa.v);// Clock 2
    m_regEa.l += m_regX;
}

void Cpu::zpgX_rw()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_emu.memory().read(m_regEa.v);// Clock 2
    m_regEa.l += m_regX;
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::zpgY_r_()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_emu.memory().read(m_regEa.v);// Clock 2
    m_regEa.l += m_regY;
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::zpgY_w_()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_emu.memory().read(m_regEa.v);// Clock 2
    m_regEa.l += m_regY;
}

void Cpu::zpgY_rw()
{
    m_regEa.h = 0;
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_emu.memory().read(m_regEa.v);// Clock 2
    m_regEa.l += m_regY;
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::imm____()
{
    m_m = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
}

void Cpu::imA____()
{
    m_emu.memory().read(m_regPc.v);
}

void Cpu::abs_r__()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::abs_w__()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2
}

void Cpu::abs_rw_()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2
    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
}

void Cpu::absX_r_()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2

    m_regEa.l += m_regX;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 3
    if (m_regEa.l < m_regX)
    {
        m_regEa.h++;
        m_m = m_emu.memory().read(m_regEa.v);// Clock 4
    }
}

void Cpu::absX_w_()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2

    m_regEa.l += m_regX;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 4
    if (m_regEa.l < m_regX)
        m_regEa.h++;
}

void Cpu::absX_rw()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2

    m_regEa.l += m_regX;

    m_emu.memory().read(m_regEa.v);// Clock 3
    if (m_regEa.l < m_regX)
        m_regEa.h++;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 4
}

void Cpu::absY_r_()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2

    m_regEa.l += m_regY;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 4
    if (m_regEa.l < m_regY)
    {
        m_regEa.h++;
        m_m = m_emu.memory().read(m_regEa.v);// Clock 4
    }
}

void Cpu::absY_w_()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2

    m_regEa.l += m_regY;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 4
    if (m_regEa.l < m_regY)
        m_regEa.h++;
}

void Cpu::absY_rw()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 1
    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;// Clock 2

    m_regEa.l += m_regY;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 4
    if (m_regEa.l < m_regY)
        m_regEa.h++;

    m_m = m_emu.memory().read(m_regEa.v);// Clock 4
}

void Cpu::adc__()
{
    const qint32 temp = m_regA + m_m + (m_flagC ? 1 : 0);

    m_flagV = (temp ^ m_regA) & (temp ^ m_m) & 0x80;
    m_flagN = temp & 0x80;
    m_flagZ = (temp & 0xFF) == 0;
    m_flagC = temp >> 0x8;

    m_regA = temp;
}

void Cpu::ahc__()
{
    m_emu.memory().write(m_regEa.v, m_regA & m_regX & 7);
}

void Cpu::alr__()
{
    m_regA &= m_m;

    m_flagC = m_regA & 0x01;

    m_regA >>= 1;

    m_flagN = m_regA & 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::anc__()
{
    m_regA &= m_m;
    m_flagN = m_regA & 0x80;
    m_flagZ = m_regA == 0;
    m_flagC = m_regA & 0x80;
}

void Cpu::and__()
{
    m_regA &= m_m;
    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = (m_regA == 0);
}

void Cpu::arr__()
{
    m_regA = ((m_m & m_regA) >> 1) | (m_flagC ? 0x80 : 0x00);

    m_flagZ = (m_regA & 0xFF) == 0;
    m_flagN = m_regA & 0x80;
    m_flagC = m_regA & 0x40;
    m_flagV = (m_regA << 1 ^ m_regA) & 0x40;
}

void Cpu::axs__()
{
    const qint32 temp = (m_regA & m_regX) - m_m;

    m_flagN = temp & 0x80;
    m_flagZ = (temp & 0xFF) == 0;
    m_flagC = ~temp >> 8;

    m_regX = temp;
}

void Cpu::asl_m()
{
    m_flagC = (m_m & 0x80) == 0x80;
    m_emu.memory().write(m_regEa.v, m_m);

    m_m = (m_m << 1) & 0xFE;

    m_emu.memory().write(m_regEa.v, m_m);

    m_flagN = (m_m & 0x80) == 0x80;
    m_flagZ = m_m == 0;
}

void Cpu::asl_a()
{
    m_flagC = (m_regA & 0x80) == 0x80;

    m_regA = (m_regA << 1) & 0xFE;

    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::bcc__()
{
    branch(!m_flagC);
}

void Cpu::bcs__()
{
    branch(m_flagC);
}

void Cpu::beq__()
{
    branch(m_flagZ);
}

void Cpu::bit__()
{
    m_flagN = m_m & 0x80;
    m_flagV = m_m & 0x40;
    m_flagZ = (m_m & m_regA) == 0;
}

void Cpu::brk__()
{
    m_emu.memory().read(m_regPc.v);
    m_regPc.v++;
    interrupt();
}

void Cpu::bpl__()
{
    branch(!m_flagN);
}

void Cpu::bne__()
{
    branch(!m_flagZ);
}

void Cpu::bmi__()
{
    branch(m_flagN);
}

void Cpu::bvm__()
{
    branch(!m_flagV);
}

void Cpu::bvs__()
{
    branch(m_flagV);
}

void Cpu::sed__()
{
    m_flagD = true;
}

void Cpu::clc__()
{
    m_flagC = false;
}

void Cpu::cld__()
{
    m_flagD = false;
}

void Cpu::clv__()
{
    m_flagV = false;
}

void Cpu::cmp__()
{
    const qint32 temp = m_regA - m_m;
    m_flagN = (temp & 0x80) == 0x80;
    m_flagC = m_regA >= m_m;
    m_flagZ = temp == 0;
}

void Cpu::cpx__()
{
    const qint32 temp = m_regX - m_m;
    m_flagN = (temp & 0x80) == 0x80;
    m_flagC = m_regX >= m_m;
    m_flagZ = temp == 0;
}

void Cpu::cpy__()
{
    const qint32 temp = m_regY - m_m;
    m_flagN = (temp & 0x80) == 0x80;
    m_flagC = m_regY >= m_m;
    m_flagZ = temp == 0;
}

void Cpu::cli__()
{
    m_flagI = false;
}

void Cpu::dcp__()
{
    m_emu.memory().write(m_regEa.v, m_m--);
    m_emu.memory().write(m_regEa.v, m_m);

    const qint32 temp = m_regA - m_m;

    m_flagN = temp & 0x80;
    m_flagZ = temp == 0;
    m_flagC = ~temp >> 8;
}

void Cpu::dec__()
{
    m_emu.memory().write(m_regEa.v, m_m--);
    m_emu.memory().write(m_regEa.v, m_m);
    m_flagN = (m_m & 0x80) == 0x80;
    m_flagZ = m_m == 0;
}

void Cpu::dey__()
{
    m_flagZ = --m_regY == 0;
    m_flagN = (m_regY & 0x80) == 0x80;
}

void Cpu::dex__()
{
    m_flagZ = --m_regX == 0;
    m_flagN = (m_regX & 0x80) == 0x80;
}

void Cpu::eor__()
{
    m_regA ^= m_m;
    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::inc__()
{
    m_emu.memory().write(m_regEa.v, m_m++);
    m_emu.memory().write(m_regEa.v, m_m);
    m_flagN = (m_m & 0x80) == 0x80;
    m_flagZ = m_m == 0;
}

void Cpu::inx__()
{
    m_flagZ = ++m_regX == 0;
    m_flagN = (m_regX & 0x80) == 0x80;
}

void Cpu::iny__()
{
    m_flagN = (++m_regY & 0x80) == 0x80;
    m_flagZ = m_regY == 0;
}

void Cpu::isc__()
{
    quint8 temp0 = m_emu.memory().read(m_regEa.v);

    m_emu.memory().write(m_regEa.v, temp0);
    temp0++;
    m_emu.memory().write(m_regEa.v, temp0);

    const qint32 temp1 = temp0 ^ 0xFF;
    const qint32 temp2 = (m_regA + temp1 + (m_flagC ? 1 : 0));

    m_flagN = temp2 & 0x80;
    m_flagV = (temp2 ^ m_regA) & (temp2 ^ temp1) & 0x80;
    m_flagZ = (temp2 & 0xFF) == 0;
    m_flagC = temp2 >> 0x8;
    m_regA = temp2;
}

void Cpu::jmp__()
{
    m_regPc.v = m_regEa.v;
}

void Cpu::jmp_i()
{
    // Fetch pointer
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;
    m_regEa.h = m_emu.memory().read(m_regPc.v);

    m_regPc.l = m_emu.memory().read(m_regEa.v);
    m_regEa.l++; // only increment the low byte, causing the "JMP ($nnnn)" bug
    m_regPc.h = m_emu.memory().read(m_regEa.v);
}

void Cpu::jsr__()
{
    m_regEa.l = m_emu.memory().read(m_regPc.v);
    m_regPc.v++;

    // Store EAL at SP, see http://users.telenet.be/kim1-6502/6502/proman.html (see the JSR part)
    m_emu.memory().write(m_regSp.v, m_regEa.l);

    push(m_regPc.h);
    push(m_regPc.l);

    m_regEa.h = m_emu.memory().read(m_regPc.v);
    m_regPc.v = m_regEa.v;
}

void Cpu::lar__()
{
    m_regSp.l &= m_m;
    m_regA = m_regSp.l;
    m_regX = m_regSp.l;

    m_flagN = m_regSp.l & 0x80;
    m_flagZ = (m_regSp.l & 0xFF) == 0;
}

void Cpu::lax__()
{
    m_regX = m_regA = m_m;

    m_flagN = m_regX & 0x80;
    m_flagZ = (m_regX & 0xFF) == 0;
}

void Cpu::lda__()
{
    m_regA = m_m;
    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::ldx__()
{
    m_regX = m_m;
    m_flagN = (m_regX & 0x80) == 0x80;
    m_flagZ = m_regX == 0;
}

void Cpu::ldy__()
{
    m_regY = m_m;
    m_flagN = (m_regY & 0x80) == 0x80;
    m_flagZ = m_regY == 0;
}

void Cpu::lsr_a()
{
    m_flagC = m_regA & 1;
    m_regA >>= 1;
    m_flagZ = m_regA == 0;
    m_flagN = m_regA & 0x80;
}

void Cpu::lsr_m()
{
    m_flagC = m_m & 1;
    m_emu.memory().write(m_regEa.v, m_m);
    m_m >>= 1;

    m_emu.memory().write(m_regEa.v, m_m);
    m_flagZ = m_m == 0;
    m_flagN = m_m & 0x80;
}

void Cpu::nop__()
{
    // Do nothing.
}

void Cpu::ora__()
{
    m_regA |= m_m;
    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::pha__()
{
    push(m_regA);
}

void Cpu::php__()
{
    push(getRegisterPb());
}

void Cpu::pla__()
{
    m_emu.memory().read(m_regSp.v);
    m_regA = pull();
    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::plp__()
{
    m_emu.memory().read(m_regSp.v);
    setRegisterP(pull());
}

void Cpu::rla__()
{
    const quint8 temp0 = m_emu.memory().read(m_regEa.v);

    m_emu.memory().write(m_regEa.v, temp0);

    const quint8 temp1 = (temp0 << 1) | (m_flagC ? 0x01 : 0x00);

    m_emu.memory().write(m_regEa.v, temp1);

    m_flagN = temp1 & 0x80;
    m_flagZ = (temp1 & 0xFF) == 0;
    m_flagC = temp0 & 0x80;

    m_regA &= temp1;
    m_flagN = m_regA & 0x80;
    m_flagZ = (m_regA & 0xFF) == 0;
}

void Cpu::rol_a()
{
    const quint8 temp = (m_regA << 1) | (m_flagC ? 0x01 : 0x00);

    m_flagN = temp & 0x80;
    m_flagZ = (temp & 0xFF) == 0;
    m_flagC = m_regA & 0x80;

    m_regA = temp;
}

void Cpu::rol_m()
{
    m_emu.memory().write(m_regEa.v, m_m);

    const quint8 temp = (m_m << 1) | (m_flagC ? 0x01 : 0x00);

    m_emu.memory().write(m_regEa.v, temp);
    m_flagN = temp & 0x80;
    m_flagZ = (temp & 0xFF) == 0;
    m_flagC = m_m & 0x80;
}

void Cpu::ror_a()
{
    const quint8 temp = (m_regA >> 1) | (m_flagC ? 0x80 : 0x00);

    m_flagN = temp & 0x80;
    m_flagZ = (temp & 0xFF) == 0;
    m_flagC = m_regA & 0x01;

    m_regA = temp;
}

void Cpu::ror_m()
{
    m_emu.memory().write(m_regEa.v, m_m);

    const quint8 temp = (m_m >> 1) | (m_flagC ? 0x80 : 0x00);
    m_emu.memory().write(m_regEa.v, temp);

    m_flagN = temp & 0x80;
    m_flagZ = (temp & 0xFF) == 0;
    m_flagC = m_m & 0x01;
}

void Cpu::rra__()
{
    const quint8 cpu_byte_temp = m_emu.memory().read(m_regEa.v);

    m_emu.memory().write(m_regEa.v, cpu_byte_temp);

    const quint8 cpu_dummy = (cpu_byte_temp >> 1) | (m_flagC ? 0x80 : 0x00);

    m_emu.memory().write(m_regEa.v, cpu_dummy);

    m_flagN = cpu_dummy & 0x80;
    m_flagZ = (cpu_dummy & 0xFF) == 0;
    m_flagC = cpu_byte_temp & 0x01;

    int cpu_int_temp = (m_regA + cpu_dummy + (m_flagC ? 1 : 0));

    m_flagN = cpu_int_temp & 0x80;
    m_flagV = (cpu_int_temp ^ m_regA) & (cpu_int_temp ^ cpu_dummy) & 0x80;
    m_flagZ = (cpu_int_temp & 0xFF) == 0;
    m_flagC = cpu_int_temp >> 0x8;
    m_regA = cpu_int_temp;
}

void Cpu::rti__()
{
    m_emu.memory().read(m_regSp.v);

    setRegisterP(pull());

    m_regPc.l = pull();
    m_regPc.h = pull();
}

void Cpu::rts__()
{
    m_emu.memory().read(m_regSp.v);
    m_regPc.l = pull();
    m_regPc.h = pull();

    m_regPc.v++;

    m_emu.memory().read(m_regPc.v);
}

void Cpu::sax__()
{
    m_emu.memory().write(m_regEa.v, m_regX & m_regA);
}

void Cpu::sdc__()
{
    m_m ^= 0xFF;

    const qint32 temp = (m_regA + m_m + (m_flagC ? 1 : 0));

    m_flagN = temp & 0x80;
    m_flagV = (temp ^ m_regA) & (temp ^ m_m) & 0x80;
    m_flagZ = (temp & 0xFF) == 0;
    m_flagC = temp >> 0x8;
    m_regA = temp;
}

void Cpu::sec__()
{
    m_flagC = true;
}

void Cpu::sei__()
{
    m_flagI = true;
}

void Cpu::shx__()
{
    const quint8 cpu_byte_temp = m_regX & (m_regEa.h + 1);

    m_emu.memory().read(m_regEa.v);
    m_regEa.l += m_regY;

    if (m_regEa.l < m_regY)
        m_regEa.h = cpu_byte_temp;

    m_emu.memory().write(m_regEa.v, cpu_byte_temp);
}

void Cpu::shy__()
{
    const quint8 temp = m_regY & (m_regEa.h + 1);

    m_emu.memory().read(m_regEa.v);
    m_regEa.l += m_regX;

    if (m_regEa.l < m_regX)
        m_regEa.h = temp;

    m_emu.memory().write(m_regEa.v, temp);
}

void Cpu::slo__()
{
    quint8 cpu_byte_temp = m_emu.memory().read(m_regEa.v);

    m_flagC = cpu_byte_temp & 0x80;

    m_emu.memory().write(m_regEa.v, cpu_byte_temp);

    cpu_byte_temp <<= 1;

    m_emu.memory().write(m_regEa.v, cpu_byte_temp);

    m_flagN = cpu_byte_temp & 0x80;
    m_flagZ = (cpu_byte_temp & 0xFF) == 0;

    m_regA |= cpu_byte_temp;
    m_flagN = m_regA & 0x80;
    m_flagZ = (m_regA & 0xFF) == 0;
}

void Cpu::sre__()
{
    quint8 cpu_byte_temp = m_emu.memory().read(m_regEa.v);

    m_flagC = cpu_byte_temp & 0x01;

    m_emu.memory().write(m_regEa.v, cpu_byte_temp);

    cpu_byte_temp >>= 1;

    m_emu.memory().write(m_regEa.v, cpu_byte_temp);

    m_flagN = cpu_byte_temp & 0x80;
    m_flagZ = (cpu_byte_temp & 0xFF) == 0;

    m_regA ^= cpu_byte_temp;
    m_flagN = m_regA & 0x80;
    m_flagZ = (m_regA & 0xFF) == 0;
}

void Cpu::sta__()
{
    m_emu.memory().write(m_regEa.v, m_regA);
}

void Cpu::stx__()
{
    m_emu.memory().write(m_regEa.v, m_regX);
}

void Cpu::sty__()
{
    m_emu.memory().write(m_regEa.v, m_regY);
}

void Cpu::tax__()
{
    m_regX = m_regA;
    m_flagN = (m_regX & 0x80) == 0x80;
    m_flagZ = m_regX == 0;
}

void Cpu::tay__()
{
    m_regY = m_regA;
    m_flagN = (m_regY & 0x80) == 0x80;
    m_flagZ = m_regY == 0;
}

void Cpu::tsx__()
{
    m_regX = m_regSp.l;
    m_flagN = m_regX & 0x80;
    m_flagZ = m_regX == 0;
}

void Cpu::txa__()
{
    m_regA = m_regX;
    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::txs__()
{
    m_regSp.l = m_regX;
}

void Cpu::tya__()
{
    m_regA = m_regY;
    m_flagN = (m_regA & 0x80) == 0x80;
    m_flagZ = m_regA == 0;
}

void Cpu::xaa__()
{
    m_regA = m_regX & m_m;
    m_flagN = m_regA & 0x80;
    m_flagZ = (m_regA & 0xFF) == 0;
}

void Cpu::xas__()
{
    m_regSp.l = m_regA & m_regX /*& ((dummyVal >> 8) + 1) */;
    m_emu.memory().write(m_regEa.v, m_regSp.l);
}

void Cpu::writeState(QDataStream &dataStream) const
{
    dataStream << m_regPc.v << m_regSp.v << m_regEa.v << m_regA << m_regX << m_regY
               << m_flagN << m_flagV << m_flagD << m_flagI << m_flagZ << m_flagC
               << m_m << m_opcode << m_irqPin << m_nmiPin << m_suspendNmi << m_suspendIrq;
}

void Cpu::readState(QDataStream &dataStream)
{
    dataStream >> m_regPc.v >> m_regSp.v >> m_regEa.v >> m_regA >> m_regX >> m_regY
               >> m_flagN >> m_flagV >> m_flagD >> m_flagI >> m_flagZ >> m_flagC
            >> m_m >> m_opcode >> m_irqPin >> m_nmiPin >> m_suspendNmi >> m_suspendIrq;
}

bool Cpu::suspendNmi() const
{
    return m_suspendNmi;
}

bool Cpu::suspendIrq() const
{
    return m_suspendIrq;
}

bool Cpu::flagI() const
{
    return m_flagI;
}

bool Cpu::nmiPin() const
{
    return m_nmiPin;
}

void Cpu::setNmiPin(bool nmiPin)
{
    m_nmiPin = nmiPin;
}

bool Cpu::irqPin() const
{
    return m_irqPin;
}

void Cpu::setIrqPin(bool irqPin)
{
    m_irqPin = irqPin;
}
