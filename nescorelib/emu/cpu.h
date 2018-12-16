#pragma once

#include "nescorelib_global.h"

// Qt includes
#include <QtGlobal>

// forward declarations
class NesEmulator;
class QDataStream;

class NESCORELIB_EXPORT Cpu
{
    union CpuRegister
    {
        quint16 v;
        struct {
            quint8 l;
            quint8 h;
        };
    };

public:
    explicit Cpu(NesEmulator &emu);

    quint8 getRegisterP() const;
    void setRegisterP(const quint8 value);
    quint8 getRegisterPb() const;

    void clock();

    void hardReset();
    void softReset();

    void interrupt();
    void branch(const bool condition);
    void push(const quint8 value);
    quint8 _pull();
    quint8 pull();

    void writeState(QDataStream &dataStream) const;
    void readState(QDataStream &dataStream);

    bool suspendNmi() const;
    bool suspendIrq() const;
    bool flagI() const;

    bool nmiPin() const;
    void setNmiPin(bool nmiPin);

    bool irqPin() const;
    void setIrqPin(bool irqPin);

private:
    // addressing modes
    void imp____();     void zpgX_r_();    void abs_rw_();
    void indX_r_();     void zpgX_w_();    void absX_r_();
    void indX_w_();     void zpgX_rw();    void absX_w_();
    void indX_rw();     void zpgY_r_();    void absX_rw();
    void indY_r_();     void zpgY_w_();    void absY_r_();
    void indY_w_();     void zpgY_rw();    void absY_w_();
    void indY_rw();     void imm____();    void absY_rw();
    void zpg_r__();     void imA____();
    void zpg_w__();     void abs_r__();
    void zpg_rw_();     void abs_w__();

    // instructions
    void adc__();    void clc__();    void lax__();    void sax__();
    void ahc__();    void cld__();    void lda__();    void sdc__();
    void alr__();    void clv__();    void ldx__();    void sec__();
    void anc__();    void cmp__();    void ldy__();    void sei__();
    void and__();    void cpx__();    void lsr_a();    void shx__();
    void arr__();    void cpy__();    void lsr_m();    void shy__();
    void axs__();    void cli__();    void nop__();    void slo__();
    void asl_m();    void dcp__();    void ora__();    void sre__();
    void asl_a();    void dec__();    void pha__();    void sta__();
    void bcc__();    void dey__();    void php__();    void stx__();
    void bcs__();    void dex__();    void pla__();    void sty__();
    void beq__();    void eor__();    void plp__();    void tax__();
    void bit__();    void inc__();    void rla__();    void tay__();
    void brk__();    void inx__();    void rol_a();    void tsx__();
    void bpl__();    void iny__();    void rol_m();    void txa__();
    void bne__();    void isc__();    void ror_a();    void txs__();
    void bmi__();    void jmp__();    void ror_m();    void tya__();
    void bvm__();    void jmp_i();    void rra__();    void xaa__();
    void bvs__();    void jsr__();    void rti__();    void xas__();
    void sed__();    void lar__();    void rts__();

    NesEmulator &m_emu;

    CpuRegister m_regPc {};
    CpuRegister m_regSp {};
    CpuRegister m_regEa {};

    quint8 m_regA {};
    quint8 m_regX {};
    quint8 m_regY {};

    //flags
    bool m_flagN {};
    bool m_flagV {};
    bool m_flagD {};
    bool m_flagI {};
    bool m_flagZ {};
    bool m_flagC {};

    quint8 m_m {};
    quint8 m_opcode {};

    bool m_irqPin {};
    bool m_nmiPin {};
    bool m_suspendNmi {};
    bool m_suspendIrq {};
};
