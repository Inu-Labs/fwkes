/*
 * Copyright (C) 2025-present InuLabs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file cpu.h
 * @brief NES 6502 CPU emulation – registers, opcodes, and public API.
 *
 * The NES uses a Ricoh 2A03 processor, which is almost identical to the
 * MOS Technology 6502.  The main differences from a standard 6502 are:
 *   - The decimal mode flag (D) exists in the status register but has no
 *     effect – BCD arithmetic is not implemented in the 2A03.
 *   - The chip includes the APU (audio hardware) alongside the CPU core.
 *
 * ## Registers
 * The 6502 has six registers:
 *   - **A**  – Accumulator: most arithmetic and logic results go here.
 *   - **X, Y** – Index registers: mainly used for address offsets in loops.
 *   - **PC** – Program Counter: address of the next instruction to execute.
 *   - **S**  – Stack Pointer: offset into page $01 (the hardware stack).
 *   - **P**  – Processor Status: eight flag bits (see CPU_FLAG_* below).
 *
 * ## Addressing modes
 * Each opcode suffix encodes the addressing mode used:
 *   - **IMM**  – Immediate: the operand byte follows the opcode directly.
 *   - **ABS**  – Absolute: a full 16-bit address follows the opcode.
 *   - **ABSX/ABSY** – Absolute + X/Y: address is offset by the X or Y register.
 *   - **ZP**   – Zero Page: an 8-bit address in the $00-$FF range (faster).
 *   - **ZPX/ZPY** – Zero Page + X/Y: zero-page address offset by X or Y.
 *   - **INX**  – Indexed Indirect: the zero-page pointer is offset by X first.
 *   - **INY**  – Indirect Indexed: the 16-bit address read from zero page is
 *                offset by Y after the fact.
 *   - **IND**  – Indirect: the operand is a pointer to the real address (JMP only).
 *   - **REL**  – Relative: a signed 8-bit offset from the next instruction (branches).
 *   - **IMP**  – Implied: no operand needed (the target is fixed by the instruction).
 *   - **ACC**  – Accumulator: the instruction operates directly on A.
 */

#pragma once

#include "util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Clock frequency
 * ========================================================================= */

/** NTSC CPU clock frequency in Hz (~1.79 MHz). */
#define CPU_FREQ 1786830

/** Same value as an unsigned long long literal, for use in 64-bit arithmetic. */
#define CPU_FREQ_U 1786830ULL

/* =========================================================================
 * Processor status register (P) flags
 *
 * Each constant selects one bit in the P register.
 * ========================================================================= */

/** @defgroup cpu_flags CPU status register (P) flag bits
 *  @{
 */

/** Bit 0 – Carry flag.
 *  Set when an addition overflows 8 bits, or when a subtraction/shift
 *  produces a borrow/shifted-out bit. */
#define CPU_FLAG_C (1u << 0)

/** Bit 1 – Zero flag.  Set when the result of the last operation is zero. */
#define CPU_FLAG_Z (1u << 1)

/** Bit 2 – Interrupt Disable flag.
 *  When set, hardware IRQ signals are ignored.  NMIs are never masked. */
#define CPU_FLAG_I (1u << 2)

/** Bit 3 – Decimal mode flag.
 *  Present in the register but **has no effect** on the 2A03 – BCD arithmetic
 *  is not implemented. */
#define CPU_FLAG_D (1u << 3)

/** Bit 4 – Break flag.
 *  Pushed onto the stack during a BRK instruction to distinguish a software
 *  break from a hardware IRQ (both vector through $FFFE/$FFFF). */
#define CPU_FLAG_B (1u << 4)

/** Bit 5 – Unused.  Always reads as 1 on real hardware. */
#define CPU_FLAG_U (1u << 5)

/** Bit 6 – Overflow flag.
 *  Set when a signed arithmetic result overflows (e.g. positive + positive
 *  gives a negative result). */
#define CPU_FLAG_V (1u << 6)

/** Bit 7 – Negative flag.  Set when the result has bit 7 set (i.e. is
 *  negative in two's-complement terms). */
#define CPU_FLAG_N (1u << 7)

/** @} */

/* =========================================================================
 * Official instruction opcodes
 *
 * The suffix after the mnemonic encodes the addressing mode (see file-level
 * documentation for the full list of mode abbreviations).
 * ========================================================================= */

/** @defgroup opcodes_official Official 6502 opcodes
 *  @{
 */

/** ADC – Add with Carry.  A = A + operand + C. */
#define OPCODE_ADC_IMM  0x69
#define OPCODE_ADC_ABS  0x6d
#define OPCODE_ADC_ABSX 0x7d
#define OPCODE_ADC_ABSY 0x79
#define OPCODE_ADC_ZP   0x65
#define OPCODE_ADC_ZPX  0x75
#define OPCODE_ADC_INX  0x61
#define OPCODE_ADC_INY  0x71

/** AND – Bitwise AND with Accumulator.  A = A & operand. */
#define OPCODE_AND_IMM  0x29
#define OPCODE_AND_ABS  0x2d
#define OPCODE_AND_ABSX 0x3d
#define OPCODE_AND_ABSY 0x39
#define OPCODE_AND_ZP   0x25
#define OPCODE_AND_ZPX  0x35
#define OPCODE_AND_INX  0x21
#define OPCODE_AND_INY  0x31

/** ASL – Arithmetic Shift Left.  Shifts all bits one position left; bit 0 = 0, old bit 7 → C. */
#define OPCODE_ASL_ACC  0x0a
#define OPCODE_ASL_ABS  0x0e
#define OPCODE_ASL_ABSX 0x1e
#define OPCODE_ASL_ZP   0x06
#define OPCODE_ASL_ZPX  0x16

/** BCC – Branch if Carry Clear.  Jump if C == 0. */
#define OPCODE_BCC_REL  0x90

/** BCS – Branch if Carry Set.  Jump if C == 1. */
#define OPCODE_BCS_REL  0xb0

/** BEQ – Branch if Equal (Zero Set).  Jump if Z == 1. */
#define OPCODE_BEQ_REL  0xf0

/** BIT – Bit Test.  Sets Z = (A & operand) == 0, N = operand[7], V = operand[6]. */
#define OPCODE_BIT_ABS  0x2c
#define OPCODE_BIT_ZP   0x24

/** BMI – Branch if Minus (Negative Set).  Jump if N == 1. */
#define OPCODE_BMI_REL  0x30

/** BNE – Branch if Not Equal (Zero Clear).  Jump if Z == 0. */
#define OPCODE_BNE_REL  0xd0

/** BPL – Branch if Plus (Negative Clear).  Jump if N == 0. */
#define OPCODE_BPL_REL  0x10

/** BRK – Force Interrupt.  Pushes PC+2 and P onto the stack, then jumps via $FFFE/$FFFF. */
#define OPCODE_BRK_IMP  0x00

/** BVC – Branch if Overflow Clear.  Jump if V == 0. */
#define OPCODE_BVC_REL  0x50

/** BVS – Branch if Overflow Set.  Jump if V == 1. */
#define OPCODE_BVS_REL  0x70

/** CLC – Clear Carry flag. */
#define OPCODE_CLC_IMP  0x18
/** CLD – Clear Decimal flag (no effect on 2A03). */
#define OPCODE_CLD_IMP  0xd8
/** CLI – Clear Interrupt Disable flag (enables IRQs). */
#define OPCODE_CLI_IMP  0x58
/** CLV – Clear Overflow flag. */
#define OPCODE_CLV_IMP  0xb8

/** CMP – Compare Accumulator.  Sets flags as if A - operand, without storing the result. */
#define OPCODE_CMP_IMM  0xc9
#define OPCODE_CMP_ABS  0xcd
#define OPCODE_CMP_ABSX 0xdd
#define OPCODE_CMP_ABSY 0xd9
#define OPCODE_CMP_ZP   0xc5
#define OPCODE_CMP_ZPX  0xd5
#define OPCODE_CMP_INX  0xc1
#define OPCODE_CMP_INY  0xd1

/** CPX – Compare X register.  Sets flags as if X - operand. */
#define OPCODE_CPX_IMM  0xe0
#define OPCODE_CPX_ABS  0xec
#define OPCODE_CPX_ZP   0xe4

/** CPY – Compare Y register.  Sets flags as if Y - operand. */
#define OPCODE_CPY_IMM  0xc0
#define OPCODE_CPY_ABS  0xcc
#define OPCODE_CPY_ZP   0xc4

/** DEC – Decrement memory by 1. */
#define OPCODE_DEC_ABS  0xce
#define OPCODE_DEC_ABSX 0xde
#define OPCODE_DEC_ZP   0xc6
#define OPCODE_DEC_ZPX  0xd6

/** DEX – Decrement X register by 1. */
#define OPCODE_DEX_IMP  0xca

/** DEY – Decrement Y register by 1. */
#define OPCODE_DEY_IMP  0x88

/** EOR – Exclusive OR with Accumulator.  A = A ^ operand. */
#define OPCODE_EOR_IMM  0x49
#define OPCODE_EOR_ABS  0x4d
#define OPCODE_EOR_ABSX 0x5d
#define OPCODE_EOR_ABSY 0x59
#define OPCODE_EOR_ZP   0x45
#define OPCODE_EOR_ZPX  0x55
#define OPCODE_EOR_INX  0x41
#define OPCODE_EOR_INY  0x51

/** INX – Increment X register by 1. */
#define OPCODE_INX_IMP  0xe8

/** INY – Increment Y register by 1. */
#define OPCODE_INY_IMP  0xc8

/** INC – Increment memory by 1. */
#define OPCODE_INC_ABS  0xee
#define OPCODE_INC_ABSX 0xfe
#define OPCODE_INC_ZP   0xe6
#define OPCODE_INC_ZPX  0xf6

/** JMP – Jump to address.  ABS = direct jump; IND = jump through a pointer.
 *  Note: the 6502 has a bug with IND when the pointer is at the end of a page
 *  ($xxFF) – the high byte wraps to $xx00 instead of $xx+1 00. */
#define OPCODE_JMP_ABS  0x4c
#define OPCODE_JMP_IND  0x6c

/** JSR – Jump to Subroutine.  Pushes PC-1 onto the stack, then jumps. */
#define OPCODE_JSR_ABS  0x20

/** LDA – Load Accumulator. */
#define OPCODE_LDA_IMM  0xa9
#define OPCODE_LDA_ABS  0xad
#define OPCODE_LDA_ABSX 0xbd
#define OPCODE_LDA_ABSY 0xb9
#define OPCODE_LDA_ZP   0xa5
#define OPCODE_LDA_ZPX  0xb5
#define OPCODE_LDA_INX  0xa1
#define OPCODE_LDA_INY  0xb1

/** LDX – Load X register. */
#define OPCODE_LDX_IMM  0xa2
#define OPCODE_LDX_ABS  0xae
#define OPCODE_LDX_ABSY 0xbe
#define OPCODE_LDX_ZP   0xa6
#define OPCODE_LDX_ZPY  0xb6

/** LDY – Load Y register. */
#define OPCODE_LDY_IMM  0xa0
#define OPCODE_LDY_ABS  0xac
#define OPCODE_LDY_ABSX 0xbc
#define OPCODE_LDY_ZP   0xa4
#define OPCODE_LDY_ZPX  0xb4

/** LSR – Logical Shift Right.  Shifts all bits one position right; bit 7 = 0, old bit 0 → C. */
#define OPCODE_LSR_ACC  0x4a
#define OPCODE_LSR_ABS  0x4e
#define OPCODE_LSR_ABSX 0x5e
#define OPCODE_LSR_ZP   0x46
#define OPCODE_LSR_ZPX  0x56

/** NOP – No Operation.  Does nothing for 2 cycles.
 *  The official opcode is $EA; the rest are unofficial aliases (see below). */
#define OPCODE_NOP_IMP   0xea
#define OPCODE_NOP_IMP_2 0x1a
#define OPCODE_NOP_IMP_3 0x3a
#define OPCODE_NOP_IMP_4 0x5a
#define OPCODE_NOP_IMP_5 0x7a
#define OPCODE_NOP_IMP_6 0xda
#define OPCODE_NOP_IMP_7 0xfa

/** ORA – Bitwise OR with Accumulator.  A = A | operand. */
#define OPCODE_ORA_IMM  0x09
#define OPCODE_ORA_ABS  0x0d
#define OPCODE_ORA_ABSX 0x1d
#define OPCODE_ORA_ABSY 0x19
#define OPCODE_ORA_ZP   0x05
#define OPCODE_ORA_ZPX  0x15
#define OPCODE_ORA_INX  0x01
#define OPCODE_ORA_INY  0x11

/** PHA – Push Accumulator onto stack. */
#define OPCODE_PHA_IMP  0x48
/** PHP – Push Processor status (P) onto stack. */
#define OPCODE_PHP_IMP  0x08
/** PLA – Pull (pop) Accumulator from stack. */
#define OPCODE_PLA_IMP  0x68
/** PLP – Pull (pop) Processor status from stack. */
#define OPCODE_PLP_IMP  0x28

/** ROL – Rotate Left.  Shifts left through the carry bit: old C → bit 0, old bit 7 → C. */
#define OPCODE_ROL_ACC  0x2a
#define OPCODE_ROL_ABS  0x2e
#define OPCODE_ROL_ABSX 0x3e
#define OPCODE_ROL_ZP   0x26
#define OPCODE_ROL_ZPX  0x36

/** ROR – Rotate Right.  Shifts right through the carry bit: old C → bit 7, old bit 0 → C. */
#define OPCODE_ROR_ACC  0x6a
#define OPCODE_ROR_ABS  0x6e
#define OPCODE_ROR_ABSX 0x7e
#define OPCODE_ROR_ZP   0x66
#define OPCODE_ROR_ZPX  0x76

/** RTI – Return from Interrupt.  Pulls P then PC from the stack. */
#define OPCODE_RTI_IMP  0x40

/** RTS – Return from Subroutine.  Pulls PC from the stack and adds 1. */
#define OPCODE_RTS_IMP  0x60

/** SBC – Subtract with Carry (borrow).  A = A - operand - (1 - C). */
#define OPCODE_SBC_IMM  0xe9
#define OPCODE_SBC_ABS  0xed
#define OPCODE_SBC_ABSX 0xfd
#define OPCODE_SBC_ABSY 0xf9
#define OPCODE_SBC_ZP   0xe5
#define OPCODE_SBC_ZPX  0xf5
#define OPCODE_SBC_INX  0xe1
#define OPCODE_SBC_INY  0xf1

/** SEC – Set Carry flag. */
#define OPCODE_SEC_IMP  0x38
/** SED – Set Decimal flag (no effect on 2A03). */
#define OPCODE_SED_IMP  0xf8
/** SEI – Set Interrupt Disable flag (disables IRQs). */
#define OPCODE_SEI_IMP  0x78

/** STA – Store Accumulator to memory. */
#define OPCODE_STA_ABS  0x8d
#define OPCODE_STA_ABSX 0x9d
#define OPCODE_STA_ABSY 0x99
#define OPCODE_STA_ZP   0x85
#define OPCODE_STA_ZPX  0x95
#define OPCODE_STA_INX  0x81
#define OPCODE_STA_INY  0x91

/** STX – Store X register to memory. */
#define OPCODE_STX_ABS  0x8e
#define OPCODE_STX_ZP   0x86
#define OPCODE_STX_ZPY  0x96

/** STY – Store Y register to memory. */
#define OPCODE_STY_ABS  0x8c
#define OPCODE_STY_ZP   0x84
#define OPCODE_STY_ZPX  0x94

/** TAX – Transfer Accumulator to X.  X = A. */
#define OPCODE_TAX_IMP  0xaa
/** TAY – Transfer Accumulator to Y.  Y = A. */
#define OPCODE_TAY_IMP  0xa8
/** TSX – Transfer Stack pointer to X.  X = S. */
#define OPCODE_TSX_IMP  0xba
/** TXA – Transfer X to Accumulator.  A = X. */
#define OPCODE_TXA_IMP  0x8a
/** TXS – Transfer X to Stack pointer.  S = X.  Does not update any flags. */
#define OPCODE_TXS_IMP  0x9a
/** TYA – Transfer Y to Accumulator.  A = Y. */
#define OPCODE_TYA_IMP  0x98

/** @} */ /* end opcodes_official */

/* =========================================================================
 * Unofficial (undocumented) opcodes
 *
 * These opcodes are not part of the official 6502 specification but have
 * predictable behaviour on real silicon due to how the decode logic works.
 * Several NES games rely on them, so a cycle-accurate emulator must support
 * them.
 * ========================================================================= */

/** @defgroup opcodes_unofficial Unofficial / undocumented 6502 opcodes
 *  @{
 */

/** ALR (ASR) – AND immediate, then LSR the result.  A = (A & imm) >> 1. */
#define OPCODE_ALR_IMM   0x4b

/** ANC – AND immediate, then copy bit 7 of the result into the carry flag. */
#define OPCODE_ANC_IMM   0x0b
#define OPCODE_ANC_IMM_2 0x2b

/** ANE (XAA) – Highly unstable.  Roughly: A = (A | const) & X & imm.
 *  Behaviour depends on the specific chip revision; avoid relying on it. */
#define OPCODE_ANE_IMM   0x8b

/** ARR – AND immediate, then ROR the result, with special carry/overflow behaviour. */
#define OPCODE_ARR_IMM   0x6b

/** AXS (SBX) – X = (A & X) - imm, sets flags like CMP. */
#define OPCODE_AXS_IMM   0xcb

/** LAX – Load both A and X with the same value (LDA + LDX combined). */
#define OPCODE_LAX_IMM  0xab
#define OPCODE_LAX_ABS  0xaf
#define OPCODE_LAX_ABSY 0xbf
#define OPCODE_LAX_ZP   0xa7
#define OPCODE_LAX_ZPY  0xb7
#define OPCODE_LAX_INX  0xa3
#define OPCODE_LAX_INY  0xb3

/** SAX – Store A & X to memory (does not affect flags). */
#define OPCODE_SAX_ABS  0x8f
#define OPCODE_SAX_ZP   0x87
#define OPCODE_SAX_ZPY  0x97
#define OPCODE_SAX_INX  0x83

/** DCP (DCM) – DEC then CMP: decrements memory, then compares result with A. */
#define OPCODE_DCP_ABS  0xcf
#define OPCODE_DCP_ABSX 0xdf
#define OPCODE_DCP_ABSY 0xdb
#define OPCODE_DCP_ZP   0xc7
#define OPCODE_DCP_ZPX  0xd7
#define OPCODE_DCP_INX  0xc3
#define OPCODE_DCP_INY  0xd3

/** ISC (ISB) – INC then SBC: increments memory, then subtracts result from A. */
#define OPCODE_ISC_ABS  0xef
#define OPCODE_ISC_ABSX 0xff
#define OPCODE_ISC_ABSY 0xfb
#define OPCODE_ISC_ZP   0xe7
#define OPCODE_ISC_ZPX  0xf7
#define OPCODE_ISC_INX  0xe3
#define OPCODE_ISC_INY  0xf3

/** RLA – ROL then AND: rotates memory left, then ANDs result with A. */
#define OPCODE_RLA_ABS  0x2f
#define OPCODE_RLA_ABSX 0x3f
#define OPCODE_RLA_ABSY 0x3b
#define OPCODE_RLA_ZP   0x27
#define OPCODE_RLA_ZPX  0x37
#define OPCODE_RLA_INX  0x23
#define OPCODE_RLA_INY  0x33

/** RRA – ROR then ADC: rotates memory right, then adds result to A. */
#define OPCODE_RRA_ABS  0x6f
#define OPCODE_RRA_ABSX 0x7f
#define OPCODE_RRA_ABSY 0x7b
#define OPCODE_RRA_ZP   0x67
#define OPCODE_RRA_ZPX  0x77
#define OPCODE_RRA_INX  0x63
#define OPCODE_RRA_INY  0x73

/** SLO (ASO) – ASL then ORA: shifts memory left, then ORs result into A. */
#define OPCODE_SLO_ABS  0x0f
#define OPCODE_SLO_ABSX 0x1f
#define OPCODE_SLO_ABSY 0x1b
#define OPCODE_SLO_ZP   0x07
#define OPCODE_SLO_ZPX  0x17
#define OPCODE_SLO_INX  0x03
#define OPCODE_SLO_INY  0x13

/** SRE (LSE) – LSR then EOR: shifts memory right, then XORs result into A. */
#define OPCODE_SRE_ABS  0x4f
#define OPCODE_SRE_ABSX 0x5f
#define OPCODE_SRE_ABSY 0x5b
#define OPCODE_SRE_ZP   0x47
#define OPCODE_SRE_ZPX  0x57
#define OPCODE_SRE_INX  0x43
#define OPCODE_SRE_INY  0x53

/** SHA (AHX/AXA) – Unstable.  Stores A & X & (high byte of address + 1) to memory. */
#define OPCODE_SHA_ABSY 0x9f
#define OPCODE_SHA_INY  0x93

/** SHX (A11/SXA/XAS) – Unstable.  Stores X & (high byte of address + 1) to memory. */
#define OPCODE_SHX_ABSY 0x9e

/** SHY (A11/SYA/SAY) – Unstable.  Stores Y & (high byte of address + 1) to memory. */
#define OPCODE_SHY_ABSX 0x9c

/** TAS (XAS/SHS) – Unstable.  S = A & X; stores A & X & (high byte + 1) to memory. */
#define OPCODE_TAS_ABSY 0x9b

/** USB (USBC) – Same as SBC immediate but using the unofficial slot ($EB). */
#define OPCODE_USB_IMM  0xeb

/** LAS (LAR) – Loads A, X, and S all with the value (memory & S). */
#define OPCODE_LAS_ABSY 0xbb

/** SKB – Skip Byte.  Reads and discards the operand byte; effectively a 2-byte NOP.
 *  Multiple opcodes map to this behaviour. */
#define OPCODE_SKB_IMM   0x80
#define OPCODE_SKB_IMM_2 0x82
#define OPCODE_SKB_IMM_3 0xc2
#define OPCODE_SKB_IMM_4 0xe2
#define OPCODE_SKB_IMM_5 0x89
#define OPCODE_SKB_ZP    0x04
#define OPCODE_SKB_ZP_2  0x44
#define OPCODE_SKB_ZP_3  0x64
#define OPCODE_SKB_ZPX   0x14
#define OPCODE_SKB_ZPX_2 0x34
#define OPCODE_SKB_ZPX_3 0x54
#define OPCODE_SKB_ZPX_4 0x74
#define OPCODE_SKB_ZPX_5 0xd4
#define OPCODE_SKB_ZPX_6 0xf4

/** IGN – Ignore (read and discard an absolute-addressed value); a 3-byte NOP.
 *  Still performs the memory read, which can matter for memory-mapped hardware. */
#define OPCODE_IGN_ABS    0x0c
#define OPCODE_IGN_ABSX   0x1c
#define OPCODE_IGN_ABSX_2 0x3c
#define OPCODE_IGN_ABSX_3 0x5c
#define OPCODE_IGN_ABSX_4 0x7c
#define OPCODE_IGN_ABSX_5 0xdc
#define OPCODE_IGN_ABSX_6 0xfc

/** HLT (KIL/JAM) – Halts the CPU.
 *  Executing one of these opcodes freezes the real 6502 until the system is
 *  reset; a reset or power-cycle is the only recovery.  Twelve different
 *  opcodes all trigger this behaviour. */
#define OPCODE_HLT_IMP    0x02
#define OPCODE_HLT_IMP_2  0x12
#define OPCODE_HLT_IMP_3  0x22
#define OPCODE_HLT_IMP_4  0x32
#define OPCODE_HLT_IMP_5  0x42
#define OPCODE_HLT_IMP_6  0x52
#define OPCODE_HLT_IMP_7  0x62
#define OPCODE_HLT_IMP_8  0x72
#define OPCODE_HLT_IMP_9  0x92
#define OPCODE_HLT_IMP_10 0xb2
#define OPCODE_HLT_IMP_11 0xd2
#define OPCODE_HLT_IMP_12 0xf2

/** @} */ /* end opcodes_unofficial */

/* =========================================================================
 * Utility macros
 * ========================================================================= */

/**
 * @brief Combine a low byte and a high byte into a 16-bit value.
 *
 * The 6502 stores 16-bit addresses in little-endian order (low byte first),
 * so this macro is used frequently when reading addresses from memory.
 *
 * @param lsb The least-significant (low) byte.
 * @param msb The most-significant (high) byte.
 */
#define U16_COMBINE(lsb, msb) ((uint16_t) ((msb) << 8) | (uint16_t) (lsb))

/** Size of the buffer required by @ref cpu_disassemble (in bytes). */
#define CPU_DISAS_BUF_SIZE 12

/* =========================================================================
 * Types
 * ========================================================================= */

/**
 * @brief Source of a hardware IRQ signal.
 *
 * The 6502 has a single IRQ line, but multiple devices can pull it low at
 * the same time.  This enum is used as a bitmask so the CPU can track which
 * sources are currently asserting the line and only release the interrupt
 * when all of them have de-asserted.
 */
typedef enum IrqSource {
    IRQ_DMC       = 1u << 0, /**< APU DMC (delta modulation channel) sample DMA finished. */
    IRQ_FRAME_CNT = 1u << 1, /**< APU frame counter IRQ (generated at ~240 Hz). */
    IRQ_MMC3      = 1u << 2, /**< MMC3 mapper scanline counter reached zero. */
} IrqSource;

typedef struct Bus Bus;

/**
 * @brief Complete state of the emulated 6502 CPU.
 *
 * One instance of this struct represents one CPU.  All fields are updated
 * by @ref cpu_run_until as instructions are executed.
 */
typedef struct Cpu {
    Bus *bus; /**< The system bus used for all memory reads and writes. */

    /* -----------------------------------------------------------------
     * Architectural registers
     * ----------------------------------------------------------------- */

    uint8_t  A;  /**< Accumulator. */
    uint8_t  X;  /**< X index register. */
    uint8_t  Y;  /**< Y index register. */
    uint16_t PC; /**< Program Counter – address of the next instruction. */
    uint8_t  S;  /**< Stack Pointer – offset within page $01 ($0100-$01FF). */
    uint8_t  P;  /**< Processor Status – bitfield of CPU_FLAG_* values. */

    /* -----------------------------------------------------------------
     * Emulation-internal state
     * ----------------------------------------------------------------- */

    CycleCounter cycles;       /**< Total CPU cycles elapsed since reset. */
    bool halt;                 /**< When true the CPU is frozen (HLT opcode was executed). */

    bool update_i;             /**< Deferred interrupt-disable update pending.
                                 *   The 6502 applies CLI/SEI one instruction late; this flag
                                 *   signals that @ref new_i_value should be written to P on
                                 *   the next instruction boundary. */
    bool new_i_value;          /**< The value to write to the I flag when @ref update_i fires. */

    bool halt_on_brk;          /**< Debug option: stop emulation when a BRK instruction is hit. */
    bool nmi_pending;          /**< An NMI edge has been detected and is waiting to be serviced. */
    unsigned irq_mask;         /**< Bitmask of @ref IrqSource values currently asserting the IRQ line.
                                 *   An IRQ is active as long as any bit is set and I flag is clear. */
} Cpu;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initialise a Cpu instance and link it to the system bus.
 *
 * Must be called once before any other cpu_* function.  Does not perform a
 * CPU reset – call @ref cpu_reset after this to put the CPU into a defined
 * starting state.
 *
 * @param self  The Cpu struct to initialise.
 * @param bus   The system bus to use for all memory accesses.
 */
void cpu_init(Cpu *self, Bus *bus);

/**
 * @brief Reset the CPU to its power-on / reset-vector state.
 *
 * Reads the reset vector from $FFFC/$FFFD and loads it into PC.  Sets the I
 * flag, clears most other state, and sets S to $FD (as real hardware does
 * after reset).
 *
 * @param self  The Cpu struct to reset.
 */
void cpu_reset(Cpu *self);

/**
 * @brief Execute instructions until the cycle counter reaches @p target_cycle.
 *
 * This is the main execution entry point.  The CPU runs instructions one by
 * one, updating @ref Cpu::cycles after each one, until the total cycle count
 * is >= @p target_cycle.  Pending NMIs and IRQs are checked between
 * instructions.
 *
 * @param self          The Cpu struct to advance.
 * @param target_cycle  The cycle count to run up to.
 */
void cpu_run_until(Cpu *self, CycleCounter target_cycle);

/**
 * @brief Service a pending Non-Maskable Interrupt (NMI).
 *
 * Pushes PC and P onto the stack, then jumps to the NMI vector at
 * $FFFA/$FFFB.  Unlike IRQs, NMIs cannot be masked by the I flag.
 * Normally called internally by @ref cpu_run_until, but exposed here for
 * testing and debugging.
 *
 * @param self  The Cpu struct to interrupt.
 */
void cpu_handle_nmi(Cpu *self);

/**
 * @brief Assert or release one IRQ source.
 *
 * Updates the IRQ line bitmask (@ref Cpu::irq_mask).  When @p state is true
 * the source is added to the mask (line pulled low); when false it is removed
 * (line released).  An interrupt is triggered on the next instruction
 * boundary if any source remains asserted and the I flag is clear.
 *
 * @param self    The Cpu struct.
 * @param source  Which device is changing the IRQ line.
 * @param state   true = asserting IRQ, false = releasing IRQ.
 */
void cpu_irq_pulldown(Cpu *self, IrqSource source, bool state);

/**
 * @brief Disassemble one instruction into a human-readable string.
 *
 * Reads up to 3 bytes starting at @p code, decodes the opcode, and writes
 * a short mnemonic string (e.g. "LDA #$42" or "JMP $C000") into @p buf.
 * Useful for debug logging and step-trace output.
 *
 * @param code  Pointer to the raw instruction bytes in memory.
 * @param pc    The address of the instruction (used for branch target display).
 * @param buf   Output buffer of at least @ref CPU_DISAS_BUF_SIZE bytes.
 * @return      Number of bytes the instruction occupies (1, 2, or 3).
 */
size_t cpu_disassemble(const uint8_t *code, uint16_t pc, char *buf);

#ifdef __cplusplus
}
#endif
