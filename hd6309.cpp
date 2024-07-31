//
//
//	hd6309.cpp
//
//	(C) R.P.Bellis
//

#include "hd6309.h"
#include <memory>
#include <cstdio>

hd6309::hd6309() : a(acc.byte.a), b(acc.byte.b), e(acc.byte.e), f(acc.byte.f), d(acc.word.d), w(acc.word.w), q(acc.q)
{
}

hd6309::~hd6309()
{
}

void hd6309::reset()
{
	USim::reset();

	pc = read_word(0xfffe);
	cycles = 0;
	dp = 0x00;		/* Direct page register = 0x00 */
	d = 0x0000;
	x = 0x0000;
	y = 0x0000;
	cc.all = 0x00;		/* Clear all flags */
	cc.bit.i = 1;		/* IRQ disabled */
	cc.bit.f = 1;		/* FIRQ disabled */
	waiting_sync = false;	/* not in SYNC */
	waiting_cwai = false;	/* not in CWAI */
	nmi_previous = true;	/* no NMI present */
}

void hd6309::tick()
{
	// handle the attached devices
	USim::tick();

	// get interrupt pin states
	bool c_nmi = NMI;
	bool c_firq = FIRQ;
	bool c_irq = IRQ;

	// check for NMI falling edge
	bool nmi_triggered = !c_nmi && nmi_previous;
	nmi_previous = c_nmi;

	if (waiting_sync) {
		// if NMI or IRQ or FIRQ asserts (flags don't matter)
		if (nmi_triggered || !c_firq || !c_irq) {
			waiting_sync = false;
		} else {
			return;
		}
	}

	// look for external interrupts
	if (nmi_triggered) {
		do_nmi();
	} else if (!c_firq && !cc.bit.f) {
		do_firq();
	} else if (!c_irq && !cc.bit.i) {
		do_irq();
	} else if (waiting_cwai) {
		return;
	}

	// if we got here, then CWAI is no longer in effect
	waiting_cwai = false;

	// remember current instruction address
	insn_pc = pc;

	// hook
	pre_exec();

	// fetch the next instruction
	fetch_instruction();

	// and process it
	execute_instruction();

	// hook
	post_exec();

	// deduct a cycle to account for the one added in USim::tick
	--cycles;
}

void hd6309::do_nmi()
{
	if (!waiting_cwai) {
		cc.bit.e = 1;
		help_psh(0xff, s, u);
	}
	cc.bit.f = cc.bit.i = 1;
	pc = read_word(0xfffc);
}

void hd6309::do_firq()
{
	if (!waiting_cwai) {
		cc.bit.e = 0;
		help_psh(0x81, s, u);
	}
	cc.bit.f = cc.bit.i = 1;
	pc = read_word(0xfff6);
}

void hd6309::do_irq()
{
	if (!waiting_cwai) {
		cc.bit.e = 1;
		help_psh(0xff, s, u);
	}
	cc.bit.f = cc.bit.i = 1;
	pc = read_word(0xfff8);
}

void hd6309::fetch_instruction()
{
	ir = fetch();

	// look for two-byte instructions
	if (ir == 0x10 || ir == 0x11) {
		ir <<= 8;
		ir |= fetch();
	}

	// Decode addressing mode
	switch (ir & 0xf0) {
		case 0x00: case 0x90: case 0xd0:
			mode = direct; break;
		case 0x20:
			mode = relative; break;
		case 0x30: case 0x40: case 0x50:
			if (ir < 0x34) {
				mode = indexed;
			} else if (ir < 0x38 || ir == 0x3c) {
				mode = immediate;
			} else {
				mode = inherent;
			}
			break;
		case 0x60: case 0xa0: case 0xe0:
			mode = indexed; break;
		case 0x70: case 0xb0: case 0xf0:
			mode = extended; break;
		case 0x80: case 0xc0:
			if (ir == 0x8d) {
				mode = relative;
			} else {
				mode = immediate;
			}
			break;
		case 0x10:
			switch (ir & 0x0f) {
				case 0x02: case 0x03: case 0x09: case 0x0d:
					mode = inherent; break;
				case 0x06: case 0x07:
					mode = relative; break;
				case 0x0a: case 0x0c: case 0x0e: case 0x0f:
					mode = immediate; break;
			}
			break;
	}
}

void hd6309::execute_instruction()
{
	switch (ir) {
		case 0x3a:
			abx(); break;
		case 0x89: case 0x99: case 0xa9: case 0xb9:
			adca(); break;
		case 0xc9: case 0xd9: case 0xe9: case 0xf9:
			adcb(); break;
		case 0x8b: case 0x9b: case 0xab: case 0xbb:
			adda(); break;
		case 0xcb: case 0xdb: case 0xeb: case 0xfb:
			addb(); break;
		case 0x118b: case 0x119b: case 0x11ab: case 0x11bb:
			adde(); break;
		case 0x11cb: case 0x11db: case 0x11eb: case 0x11fb:
			addf(); break;
		case 0xc3: case 0xd3: case 0xe3: case 0xf3:
			addd(); break;
		case 0x108b: case 0x109b: case 0x10ab: case 0x10bb:
			addw(); break;
		case 0x84: case 0x94: case 0xa4: case 0xb4:
			anda(); break;
		case 0xc4: case 0xd4: case 0xe4: case 0xf4:
			andb(); break;
		case 0x1c:
			andcc(); break;
		case 0x47:
			asra(); break;
		case 0x57:
			asrb(); break;
		case 0x07: case 0x67: case 0x77:
			asr(); break;
		case 0x24:
			bcc(); break;
		case 0x25:
			bcs(); break;
		case 0x27:
			beq(); break;
		case 0x2c:
			bge(); break;
		case 0x2e:
			bgt(); break;
		case 0x22:
			bhi(); break;
		case 0x85: case 0x95: case 0xa5: case 0xb5:
			bita(); break;
		case 0xc5: case 0xd5: case 0xe5: case 0xf5:
			bitb(); break;
		case 0x2f:
			ble(); break;
		case 0x23:
			bls(); break;
		case 0x2d:
			blt(); break;
		case 0x2b:
			bmi(); break;
		case 0x26:
			bne(); break;
		case 0x2a:
			bpl(); break;
		case 0x20:
			bra(); break;
		case 0x16:
			lbra(); break;
		case 0x21:
			brn(); break;
		case 0x8d:
			bsr(); break;
		case 0x17:
			lbsr(); break;
		case 0x28:
			bvc(); break;
		case 0x29:
			bvs(); break;
		case 0x4e: case 0x4f:
			// 0x4e undocumented
			clra(); break;
		case 0x5e: case 0x5f:
			// 0x5e undocumented
			clrb(); break;
        case 0x114f:
            clre(); break;
        case 0x115f:
            clrf(); break;
        case 0x104f:
            clrd(); break;
        case 0x105f:
            clrw(); break;
		case 0x0f: case 0x6f: case 0x7f:
			clr(); break;
		case 0x81: case 0x91: case 0xa1: case 0xb1:
			cmpa(); break;
		case 0xc1: case 0xd1: case 0xe1: case 0xf1:
			cmpb(); break;
		case 0x1181: case 0x1191: case 0x11a1: case 0x11b1:
			cmpe(); break;
		case 0x11c1: case 0x11d1: case 0x11e1: case 0x11f1:
			cmpf(); break;
		case 0x1083: case 0x1093: case 0x10a3: case 0x10b3:
			cmpd(); break;
		case 0x1081: case 0x1091: case 0x10a1: case 0x10b1:
			cmpw(); break;
		case 0x118c: case 0x119c: case 0x11ac: case 0x11bc:
			cmps(); break;
		case 0x8c: case 0x9c: case 0xac: case 0xbc:
			cmpx(); break;
		case 0x1183: case 0x1193: case 0x11a3: case 0x11b3:
			cmpu(); break;
		case 0x108c: case 0x109c: case 0x10ac: case 0x10bc:
			cmpy(); break;
		case 0x42: case 0x43: case 0x1042:
			// 0x42 / 0x1042 undocumented
			coma(); break;
		case 0x52: case 0x53:
			// 0x52 undocumented
			comb(); break;
		case 0x03: case 0x62: case 0x63: case 0x73:
			// 0x62 undocumented
			com(); break;
		case 0x3c:
			cwai(); break;
		case 0x19:
			daa(); break;
		case 0x4a: case 0x4b:
			// 0x4b undocumented
			deca(); break;
		case 0x5a: case 0x5b:
			// 0x5b undocumented
			decb(); break;
		case 0x0a: case 0x0b:
		case 0x6a: case 0x6b:
		case 0x7a: case 0x7b:
			// 0x0b, 0x6b, 0x7b undocumented
			dec(); break;
		case 0x88: case 0x98: case 0xa8: case 0xb8:
			eora(); break;
		case 0xc8: case 0xd8: case 0xe8: case 0xf8:
			eorb(); break;
		case 0x1e:
			exg(); break;
		case 0x4c:
			inca(); break;
		case 0x5c:
			incb(); break;
		case 0x0c: case 0x6c: case 0x7c:
			inc(); break;
		case 0x0e: case 0x6e: case 0x7e:
			jmp(); break;
		case 0x9d: case 0xad: case 0xbd:
			jsr(); break;
		case 0x86: case 0x96: case 0xa6: case 0xb6:
			lda(); break;
		case 0xc6: case 0xd6: case 0xe6: case 0xf6:
			ldb(); break;
		case 0x1186: case 0x1196: case 0x11a6: case 0x11b6:
			lde(); break;
		case 0x11c6: case 0x11d6: case 0x11e6: case 0x11f6:
			ldf(); break;
		case 0xcc: case 0xdc: case 0xec: case 0xfc:
			ldd(); break;
		case 0x1086: case 0x1096: case 0x10a6: case 0x10b6:
			ldw(); break;
		case 0x10ce: case 0x10de: case 0x10ee: case 0x10fe:
			lds(); break;
		case 0xce: case 0xde: case 0xee: case 0xfe:
			ldu(); break;
		case 0x8e: case 0x9e: case 0xae: case 0xbe:
			ldx(); break;
		case 0x108e: case 0x109e: case 0x10ae: case 0x10be:
			ldy(); break;
		case 0x32:
			leas(); break;
		case 0x33:
			leau(); break;
		case 0x30:
			leax(); break;
		case 0x31:
			leay(); break;
		case 0x48:
			lsla(); break;
		case 0x58:
			lslb(); break;
		case 0x08: case 0x68: case 0x78:
			lsl(); break;
		case 0x44: case 0x45:
			// 0x45 undocumented
			lsra(); break;
		case 0x54: case 0x55:
			// 0x55 undocumented
			lsrb(); break;
		case 0x04: case 0x05:
		case 0x64: case 0x65:
		case 0x74: case 0x75:
			// 0x05, 0x65, 0x75 undocumented
			lsr(); break;
		case 0x3d:
			mul(); break;
		case 0x40: case 0x41:
			// 0x41 undocumented
			nega(); break;
		case 0x50: case 0x51:
			// 0x51 undocumented
			negb(); break;
		case 0x00: case 0x01:
		case 0x60: case 0x61:
		case 0x70: case 0x71:
			// 0x01, 0x61, 0x71 undocumented
			neg(); break;
		case 0x12:
			nop(); break;
		case 0x8a: case 0x9a: case 0xaa: case 0xba:
			ora(); break;
		case 0xca: case 0xda: case 0xea: case 0xfa:
			orb(); break;
		case 0x1a:
			orcc(); break;
		case 0x34:
			pshs(); break;
		case 0x36:
			pshu(); break;
		case 0x35:
			puls(); break;
		case 0x37:
			pulu(); break;
		case 0x49:
			rola(); break;	
		case 0x59:
			rolb(); break;
		case 0x09: case 0x69: case 0x79:
			rol(); break;
		case 0x46:
			rora(); break;	
		case 0x56:
			rorb(); break;
		case 0x06: case 0x66: case 0x76:
			ror(); break;
		case 0x3b:
			rti(); break;
		case 0x39:
			rts(); break;
		case 0x82: case 0x92: case 0xa2: case 0xb2: 
			sbca(); break;
		case 0xc2: case 0xd2: case 0xe2: case 0xf2: 
			sbcb(); break;
		case 0x1d:
			sex(); break;
		case 0x97: case 0xa7: case 0xb7:
			sta(); break;
		case 0xd7: case 0xe7: case 0xf7:
			stb(); break;
		case 0x1197: case 0x11a7: case 0x11b7:
			ste(); break;
		case 0x11d7: case 0x11e7: case 0x11f7:
			stf(); break;
		case 0xdd: case 0xed: case 0xfd:
			std(); break;
		case 0x1097: case 0x10a7: case 0x10b7:
			stw(); break;
		case 0x10df: case 0x10ef: case 0x10ff:
			sts(); break;
		case 0xdf: case 0xef: case 0xff:
			stu(); break;
		case 0x9f: case 0xaf: case 0xbf:
			stx(); break;
		case 0x109f: case 0x10af: case 0x10bf:
			sty(); break;
		case 0x80: case 0x90: case 0xa0: case 0xb0:
			suba(); break;
		case 0xc0: case 0xd0: case 0xe0: case 0xf0:
			subb(); break;
		case 0x1180: case 0x1190: case 0x11a0: case 0x11b0:
			sube(); break;
		case 0x11c0: case 0x11d0: case 0x11e0: case 0x11f0:
			subf(); break;
		case 0x83: case 0x93: case 0xa3: case 0xb3:
			subd(); break;
		case 0x1080: case 0x1090: case 0x10a0: case 0x10b0:
			subw(); break;
		case 0x3f:
			swi(); break;
		case 0x103f:
			swi2(); break;
		case 0x113f:
			swi3(); break;
		case 0x13:
			sync(); break;
		case 0x1f:
			tfr(); break;
		case 0x4d:
			tsta(); break;
		case 0x5d:
			tstb(); break;
		case 0x114d:
			tste(); break;
		case 0x115d:
			tstf(); break;
		case 0x104d:
			tstd(); break;
		case 0x105d:
			tstw(); break;
		case 0x0d: case 0x6d: case 0x7d:
			tst(); break;
		case 0x1024:
			lbcc(); break;
		case 0x1025:
			lbcs(); break;
		case 0x1027:
			lbeq(); break;
		case 0x102c:
			lbge(); break;
		case 0x102e:
			lbgt(); break;
		case 0x1022:
			lbhi(); break;
		case 0x102f:
			lble(); break;
		case 0x1023:
			lbls(); break;
		case 0x102d:
			lblt(); break;
		case 0x102b:
			lbmi(); break;
		case 0x1026:
			lbne(); break;
		case 0x102a:
			lbpl(); break;
		case 0x1021:
			lbrn(); break;
		case 0x1028:
			lbvc(); break;
		case 0x1029:
			lbvs(); break;
		default:
            nop();  // Treat invalid instruction as NOP
			// invalid("invalid instruction");
	}
}

void hd6309::print_regs()
{
	char flags[] = "EFHINZVC";
	for (uint8_t i = 0, mask = 0x80; mask; ++i, mask >>= 1) {
		if ((cc.all & mask) == 0) {
			flags[i] = '-';
		}
	}
	fprintf(stderr, "PC:%04X CC:%s S:%04X U:%04X A:%02X B:%02X X:%04X Y:%04X DP:%02X\r\n",
		pc, flags, s, u, a, b, x, y, dp);
}

void hd6309::pre_exec()
{
	if (!m_trace) return;

	print_regs();
}

void hd6309::post_exec()
{
	if (!m_trace) return;

	fprintf(stderr, "/ %04X: [%2d] %-8s%s\r\n", insn_pc, cycles, insn, disasm_operand().c_str());
}

// used for EXG and TFR instructions
Word& hd6309::wordrefreg(int r)
{
	static Word no_return = 0;

	switch (r) {
		case  0: return d;
		case  1: return x;
		case  2: return y;
		case  3: return u;
		case  4: return s;
		case  5: return pc;
	}

	invalid("invalid word register selector");
	return no_return;
}

Byte& hd6309::byterefreg(int r)
{
	static Byte no_return = 0;

	switch (r) {
		case  8: return a;
		case  9: return b;
		case 10: return cc.all;
		case 11: return dp;
	}

	invalid("invalid byte register selector");
	return no_return;
}

// decodes the postbyte for most indexed modes
Word& hd6309::ix_refreg(Byte post)
{
	static Word no_return = 0;

	post = (post >> 5) & 0x03;
	switch (post) {
		case 0: return x;
		case 1: return y;
		case 2: return u;
		case 3: return s;
	}

	invalid("invalid register reference");
	return no_return;
}

Byte hd6309::fetch_operand()
{
	switch (mode) {
		case immediate:
			return operand = extend8(fetch());
		case relative: {
			Byte r = fetch();
			operand = pc + extend8(r);
			return r;
		}
		default:
			return read(fetch_effective_address());
	}
}

Word hd6309::fetch_word_operand()
{
	switch (mode) {
		case immediate:
			return operand = fetch_word();
		case relative: {
			Word r = fetch_word();
			operand = pc + r;
			return r;
		}
		default:
			return read_word(fetch_effective_address());
	}
}

Word hd6309::fetch_effective_address()
{
	switch (mode) {
		case extended:
			++cycles;
			return operand = fetch_word();
		case direct:
			++cycles;
			operand = fetch();
			return ((Word)dp << 8) | operand;
		case indexed: {
			post = fetch();

			do_predecrement();
			Word addr = fetch_indexed_operand();
			do_postincrement();

			// handle indirect indexed mode
			if (btst(post, 4) && btst(post, 7)) {
				++cycles;
				addr = read_word(addr);
			}
			return addr;
		}
		default:
			invalid("invalid addressing mode");
			return 0;
	}
}

Word hd6309::fetch_indexed_operand()
{
	if ((post & 0x80) == 0x00) {		// ,R + 5 bit offset
		cycles += 2;
		return ix_refreg(post) + extend5(post & 0x1f);
	}

	switch (post & 0x1f) {
		case 0x00:			// ,R+
			cycles += 3;
			return ix_refreg(post);
		case 0x01: case 0x11:		// ,R++
			cycles += 4;
			return ix_refreg(post);
		case 0x02:			// ,-R
			cycles += 3;
			return ix_refreg(post);
		case 0x03: case 0x13:		// ,--R
			cycles += 4;
			return ix_refreg(post);
		case 0x04: case 0x14:		// ,R + 0
			cycles += 1;
			return ix_refreg(post);
			break;
		case 0x05: case 0x15:		// ,R + B
			cycles += 2;
			return extend8(b) + ix_refreg(post);
		case 0x06: case 0x16:		// ,R + A
			cycles += 2;
			return extend8(a) + ix_refreg(post);
		case 0x08: case 0x18:		// ,R + 8 bit
			cycles += 1;
			operand = extend8(fetch());
			return ix_refreg(post) + operand;
		case 0x09: case 0x19:		// ,R + 16 bit
			cycles += 3;
			operand = fetch_word();
			return ix_refreg(post) + operand;
		case 0x0b: case 0x1b:		// ,R + D
			cycles += 5;
			return d + ix_refreg(post);
		case 0x0c: case 0x1c:		// ,PC + 8
			cycles += 1;
			operand = extend8(fetch());
			return pc + operand;
		case 0x0d: case 0x1d:		// ,PC + 16
			cycles += 3;
			operand = fetch_word();
			return pc + operand;
		case 0x1f:			// [,Address]
			cycles += 1;
			operand = fetch_word();
			return operand;
		default:
			invalid("invalid indexed addressing postbyte");
			return 0;
	}
}

void hd6309::do_postincrement()
{
	switch (post & 0x9f) {
		case 0x80:
			ix_refreg(post) += 1;
			break;
		case 0x90:
			invalid("invalid post-increment operation");
			break;
		case 0x81: case 0x91:
			ix_refreg(post) += 2;
			break;
	}
}

void hd6309::do_predecrement()
{
	switch (post & 0x9f) {
		case 0x82:
			ix_refreg(post) -= 1;
			break;
		case 0x92:
			invalid("invalid pre-decrement operation");
			break;
		case 0x83: case 0x93:
			ix_refreg(post) -= 2;
			break;
	}
}

//---------------------------------------------------------------------
//
// disassembly support
//
//---------------------------------------------------------------------

static std::string disasm_reglist(Byte w, const char *other_sr)
{
        static const char* regs[]  = {
                "CC", "A", "B", "DP", "X", "Y", "", "PC"
        };

        std::string r;

        for (int n = 0; (n < 8) && w; ++n, w >>= 1) {
                if (w & 1) {
                        r += (n == 6) ? other_sr : regs[n];
                        if (w & 0xfe) {
                                r += ",";
                        }
                }
        }

        return r;
}

static std::string disasm_regpair(Byte w)
{
	static const char* regnames[] = {
		"D", "X", "Y", "U", "S", "PC", "", "",
		"A", "B", "CC", "DP", "", "", "", ""
	};

	int r1 = (w & 0xf0) >> 4;
	int r2 = (w & 0x0f) >> 0;

	return std::string(regnames[r1]) + "," + std::string(regnames[r2]);
}

template<typename ... Args>
static std::string fmt(const std::string& format, Args ... args)
{
	int size = ::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
	if (size <= 0) {
		return "string formatting error";
	}

	std::unique_ptr<char[]> buf(new char[size]);
	::snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size - 1 );
}

std::string hd6309::disasm_indexed()
{
	static const char regs[] = "XYUS";
	const char reg = regs[(post >> 5) & 0x03];

	if (!btst(post, 7)) {			// ,R + 5 bit offset
		return fmt("%d,%c", (int16_t)extend5(post & 0x1f), reg);
	}

	switch (post & 0x1f) {
		case 0x00:			// ,R+
			return fmt(",%c+", reg);
		case 0x01: case 0x11:		// ,R++
			return fmt(",%c++", reg);
		case 0x02:			// ,-R
			return fmt(",-%c", reg);
		case 0x03: case 0x13:		// ,--R
			return fmt(",--%c", reg);
		case 0x04: case 0x14:		// ,R + 0
			return fmt(",%c", reg);
		case 0x05: case 0x15:		// ,R + B
			return fmt("B,%c", reg);
		case 0x06: case 0x16:		// ,R + A
			return fmt("A,%c", reg);
		case 0x08: case 0x18:		// ,R + offset
		case 0x09: case 0x19:
			return fmt("%d,%c", (int16_t)operand, reg);
		case 0x0b: case 0x1b:		// ,R + D
			return fmt("D,%c", reg);
		case 0x0c: case 0x1c:		// ,PCR + offset
		case 0x0d: case 0x1d:
			return fmt("%d,PCR", (int16_t)operand, reg);
		case 0x1f:			// ,Address
			return fmt(",$%04hx", (int16_t)operand);
		default:
			invalid("indirect addressing postbyte");
			return "";
	}
}

std::string hd6309::disasm_operand()
{
	// special cases for PSHx / PULx / EXG / TFR
	switch (ir) {
		case 0x34: case 0x36:	// PSHS / PULS
			return disasm_reglist(operand, "U");
		case 0x35: case 0x37:	// PSHU / PULU
			return disasm_reglist(operand, "S");
		case 0x1e: case 0x1f:	// EXG / TFR
			return disasm_regpair(operand);
	}

	switch (mode) {
		case inherent:
			return "";
		case immediate:
			return fmt("#$%02X", operand);
		case relative:
			return fmt("$%04X", operand);
		case direct:
			return fmt("<$%02X", operand);
		case extended:
			return fmt("$%04X", operand);
		case indexed: {
			auto r = disasm_indexed();
			if (btst(post, 4) && btst(post, 7)) {
				r = "[" + r + "]";
			}
			return r;
		}
	}

	return "";
}
