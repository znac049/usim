//
//	hd6309.h
//
//	Class definition for Motorola MC6809 microprocessor
//
//	(C) R.P.Bellis 1993
//

#pragma once

#include <string>
#include "wiring.h"
#include "usim.h"
#include "bits.h"

#ifndef USIM_MACHDEP_H
#include "machdep.h"
#endif

class hd6309 : virtual public USimMotorola {

protected: // Processor addressing modes

	enum {
				immediate,
				direct,
				indexed,
				extended,
				inherent,
				relative
	} mode;

protected:	// Processor registers

	Word			u, s;		// Stack pointers
	Word			x, y;		// Index registers
	Byte			dp;		// Direct Page register
	union {
		DWord			q;
		struct {
#ifdef MACH_BYTE_ORDER_MSB_FIRST
			Word		d;
			Word		w;
			
#else
			Word		w;
			Word		d;
#endif

		} word;
		struct {
#ifdef MACH_BYTE_ORDER_MSB_FIRST
			Byte		a;	// Accumulator a
			Byte		b;	// Accumulator b
			Byte		e;
			Byte		f;
#else
			Byte		f;
			Byte		e;
			Byte		b;	// Accumulator b
			Byte		a;	// Accumulator a
#endif
		} byte;
	} acc;
	Byte&			a;
	Byte&			b;
	Byte&			e;
	Byte&			f;
	Word&			d;
	Word&			w;
	DWord&			q;
	union {
		Byte			all;	// Condition code register
		struct {
#ifdef MACH_BITFIELDS_LSB_FIRST
			Byte		c : 1;	// Carry
			Byte		v : 1;	// Overflow
			Byte		z : 1;	// Zero
			Byte		n : 1;	// Negative
			Byte		i : 1;	// IRQ disable
			Byte		h : 1;	// Half carry
			Byte		f : 1;	// FIRQ disable
			Byte		e : 1;	// Entire
#else
			Byte		e : 1;	// Entire
			Byte		f : 1;	// FIRQ disable
			Byte		h : 1;	// Half carry
			Byte		i : 1;	// IRQ disable
			Byte		n : 1;	// Negative
			Byte		z : 1;	// Zero
			Byte		v : 1;	// Overflow
			Byte		c : 1;	// Carry
#endif
		} bit;
	} cc;

    union {
        Byte            all;
        struct {
#ifdef MACH_BITFIELDS_LSB_FIRST
			Byte		nm : 1;         // Native mode
			Byte		fm : 1;         // FIRQ mode
			Byte		notused : 4;    // Unused
            Byte        ii : 1;         // Illegal instruction
            Byte        div0 : 1;       // Divide by 0
#else
            Byte        div0 : 1;
            Byte        ii : 1;
			Byte		notused : 4;
			Byte		nm : 1;
			Byte		fm : 1;
#endif
        } bit;
    } md;

private:	// internal processor state
	bool			waiting_sync;
	bool			waiting_cwai;
	bool			nmi_previous;

private:	// instruction and operand fetch and decode
	Word&			ix_refreg(Byte);

	void			fetch_instruction();
	Byte			fetch_operand();
	Word			fetch_word_operand();
	Word			fetch_effective_address();
	Word			fetch_indexed_operand();
	void			execute_instruction();

	void			do_predecrement();
	void			do_postincrement();

private:	// instruction implementations
	void			abx();
	void			adca(), adcb();
	void			adda(), addb(), adde(), addf(), addd(), addw();
	void			anda(), andb(), andcc(), andd();
	void			asra(), asrb(), asr();
	void			bcc(), lbcc();
	void			bcs(), lbcs();
	void			beq(), lbeq();
	void			bge(), lbge();
	void			bgt(), lbgt();
	void			bhi(), lbhi();
	void			bita(), bitb();
    void            bitmd();
	void			ble(), lble();
	void			bls(), lbls();
	void			blt(), lblt();
	void			bmi(), lbmi();
	void			bne(), lbne();
	void			bpl(), lbpl();
	void			bra(), lbra();
	void			brn(), lbrn();
	void			bsr(), lbsr();
	void			bvc(), lbvc();
	void			bvs(), lbvs();
	void			clra(), clrb(), clre(), clrf(), clrd(), clrw(), clr();
	void			cmpa(), cmpb(), cmpe(), cmpf();
	void			cmpd(), cmpw(), cmpx(), cmpy(), cmpu(), cmps();
	void			coma(), comb(), come(), comf(), comd(), comw(), com();
	void			cwai();
	void			daa();
	void			deca(), decb(), dece(), decf(), decd(), decw(), dec();
	void			eora(), eorb(), eord();
	void			exg();
	void			inca(), incb(), ince(), incf(), incd(), incw(), inc();
	void			jmp();
	void			jsr();
	void			lda(), ldb(), lde(), ldf();
	void			ldd(), ldw(), ldx(), ldy(), lds(), ldu();
    void            ldmd();
	void			leax(), leay(), leas(), leau(); 
	void			lsla(), lslb(), lsl();
	void			lsra(), lsrb(), lsr();
	void			mul();
	void			nega(), negb(), neg();
	void			nop();
	void			ora(), orb(), orcc(), ord();
	void			pshs(), pshu();
	void			puls(), pulu();
	void			rola(), rolb(), rol();
	void			rora(), rorb(), ror();
	void			rti(), rts();
	void			sbca(), sbcb();
	void			sex();
	void			sta(), stb(), ste(), stf();
	void			std(), stw(), stx(), sty(), sts(), stu();
	void			suba(), subb(), sube(), subf();
	void			subd(), subw();
	void			swi(), swi2(), swi3();
	void			sync();
	void			tfr();
	void			tsta(), tstb(), tste(), tstf(), tstd(), tstw(), tst();

protected:	// helper functions
	void			help_adc(Byte&);
	void			help_add(Byte&);
	void			help_and(Byte&);
    void            help_and(Word&);
	void			help_asr(Byte&);
	void			help_bit(Byte);
	void			help_clr(Byte&);
    void            help_clr(Word&);
	void			help_cmp(Byte);
	void			help_cmp(Word);
	void			help_com(Byte&);
    void            help_com(Word&);
	void			help_dec(Byte&);
    void            help_dec(Word&);
	void			help_eor(Byte&);
	void			help_eor(Word&);
	void			help_inc(Byte&);
    void            help_inc(Word&);
	void			help_ld(Byte&);
	void			help_ld(Word&);
	void			help_lsr(Byte&);
	void			help_lsl(Byte&);
	void			help_neg(Byte&);
	void			help_or(Byte&);
	void			help_or(Word&);
	void			help_psh(Byte, Word&, Word&);
	void			help_pul(Byte, Word&, Word&);
	void			help_ror(Byte&);
	void			help_rol(Byte&);
	void			help_sbc(Byte&);
	void			help_st(Byte);
	void			help_st(Word);
	void			help_sub(Byte&);
	void			help_sub(Word&);
	void			help_tst(Byte);
    void            help_tst(Word);

protected:	// overloadable functions (e.g. for breakpoints)
	virtual void		do_br(const char *, bool);
	virtual void		do_lbr(const char *, bool);

	virtual void		do_psh(Word& sp, Byte);
	virtual void		do_psh(Word& sp, Word);
	virtual void		do_pul(Word& sp, Byte&);
	virtual void		do_pul(Word& sp, Word&);

	virtual void		do_nmi();
	virtual void		do_firq();
	virtual void		do_irq();

	virtual void		pre_exec();
	virtual void		post_exec();

protected: 	// instruction tracing
	Word			insn_pc;
	const char*		insn;
	Byte			post;
	Word			operand;

	std::string		disasm_operand();
	std::string		disasm_indexed();

public:		// external signal pins
	InputPin		IRQ, FIRQ, NMI;

public:
					hd6309();		// public constructor
	virtual			~hd6309();		// public destructor

	virtual void	reset();		// CPU reset
	virtual void	tick();

	virtual void	print_regs();

	Byte&			byterefreg(int);
	Word&			wordrefreg(int);

};

inline void hd6309::do_br(const char *mnemonic, bool test)
{
	(void)mnemonic;
	Word offset = extend8(fetch_operand());
	if (test) pc += offset;
	++cycles;
}

inline void hd6309::do_lbr(const char *mnemonic, bool test)
{
	(void)mnemonic;
	Word offset = fetch_word_operand();
	if (test) {
		pc += offset;
		++cycles;
	}
	++cycles;
}

inline void hd6309::do_psh(Word& sp, Byte val)
{
	write(--sp, val);
}

inline void hd6309::do_psh(Word& sp, Word val)
{
	write(--sp, (Byte)val);
	write(--sp, (Byte)(val >> 8));
}

inline void hd6309::do_pul(Word& sp, Byte& val)
{
	val = read(sp++);
}

inline void hd6309::do_pul(Word& sp, Word& val)
{
	val  = read(sp++) << 8;
	val |= read(sp++);
}
