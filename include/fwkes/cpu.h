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

#pragma once

#include "util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CPU_FREQ 1786830
#define CPU_FREQ_U 1786830ULL

/*
 * Status flags.
 *
 * Each constant is a name for a bit in the P status register.
 */

#define CPU_FLAG_C (1u << 0) /* carry */
#define CPU_FLAG_Z (1u << 1) /* zero */
#define CPU_FLAG_I (1u << 2) /* interrupt disable */
#define CPU_FLAG_D (1u << 3) /* decimal mode (UNUSED) */
#define CPU_FLAG_B (1u << 4) /* break */
#define CPU_FLAG_U (1u << 5) /* unused */
#define CPU_FLAG_V (1u << 6) /* overflow */
#define CPU_FLAG_N (1u << 7) /* negative */

/*
 * Instruction opcodes.
 *
 * Instruction, that support different addressing modes,
 * have different opcodes.
 */

#define OPCODE_ADC_IMM 0x69
#define OPCODE_ADC_ABS 0x6d
#define OPCODE_ADC_ABSX 0x7d
#define OPCODE_ADC_ABSY 0x79
#define OPCODE_ADC_ZP 0x65
#define OPCODE_ADC_ZPX 0x75
#define OPCODE_ADC_INX 0x61
#define OPCODE_ADC_INY 0x71

#define OPCODE_AND_IMM 0x29
#define OPCODE_AND_ABS 0x2d
#define OPCODE_AND_ABSX 0x3d
#define OPCODE_AND_ABSY 0x39
#define OPCODE_AND_ZP 0x25
#define OPCODE_AND_ZPX 0x35
#define OPCODE_AND_INX 0x21
#define OPCODE_AND_INY 0x31

#define OPCODE_ASL_ACC 0x0a
#define OPCODE_ASL_ABS 0x0e
#define OPCODE_ASL_ABSX 0x1e
#define OPCODE_ASL_ZP 0x06
#define OPCODE_ASL_ZPX 0x16

#define OPCODE_BCC_REL 0x90

#define OPCODE_BCS_REL 0xb0

#define OPCODE_BEQ_REL 0xf0

#define OPCODE_BIT_ABS 0x2c

#define OPCODE_BIT_ZP 0x24

#define OPCODE_BMI_REL 0x30

#define OPCODE_BNE_REL 0xd0

#define OPCODE_BPL_REL 0x10

#define OPCODE_BRK_IMP 0x00

#define OPCODE_BVC_REL 0x50

#define OPCODE_BVS_REL 0x70

#define OPCODE_CLC_IMP 0x18
#define OPCODE_CLD_IMP 0xd8
#define OPCODE_CLI_IMP 0x58
#define OPCODE_CLV_IMP 0xb8

#define OPCODE_CMP_IMM 0xc9
#define OPCODE_CMP_ABS 0xcd
#define OPCODE_CMP_ABSX 0xdd
#define OPCODE_CMP_ABSY 0xd9
#define OPCODE_CMP_ZP 0xc5
#define OPCODE_CMP_ZPX 0xd5
#define OPCODE_CMP_INX 0xc1
#define OPCODE_CMP_INY 0xd1

#define OPCODE_CPX_IMM 0xe0
#define OPCODE_CPX_ABS 0xec
#define OPCODE_CPX_ZP 0xe4

#define OPCODE_CPY_IMM 0xc0
#define OPCODE_CPY_ABS 0xcc
#define OPCODE_CPY_ZP 0xc4

#define OPCODE_DEC_ABS 0xce
#define OPCODE_DEC_ABSX 0xde
#define OPCODE_DEC_ZP 0xc6
#define OPCODE_DEC_ZPX 0xd6

#define OPCODE_DEX_IMP 0xca

#define OPCODE_DEY_IMP 0x88

#define OPCODE_EOR_IMM 0x49
#define OPCODE_EOR_ABS 0x4d
#define OPCODE_EOR_ABSX 0x5d
#define OPCODE_EOR_ABSY 0x59
#define OPCODE_EOR_ZP 0x45
#define OPCODE_EOR_ZPX 0x55
#define OPCODE_EOR_INX 0x41
#define OPCODE_EOR_INY 0x51

#define OPCODE_INX_IMP 0xe8

#define OPCODE_INY_IMP 0xc8

#define OPCODE_INC_ABS 0xee
#define OPCODE_INC_ABSX 0xfe
#define OPCODE_INC_ZP 0xe6
#define OPCODE_INC_ZPX 0xf6

#define OPCODE_JMP_ABS 0x4c
#define OPCODE_JMP_IND 0x6c

#define OPCODE_JSR_ABS 0x20

#define OPCODE_LDA_IMM 0xa9
#define OPCODE_LDA_ABS 0xad
#define OPCODE_LDA_ABSX 0xbd
#define OPCODE_LDA_ABSY 0xb9
#define OPCODE_LDA_ZP 0xa5
#define OPCODE_LDA_ZPX 0xb5
#define OPCODE_LDA_INX 0xa1
#define OPCODE_LDA_INY 0xb1

#define OPCODE_LDX_IMM 0xa2
#define OPCODE_LDX_ABS 0xae
#define OPCODE_LDX_ABSY 0xbe
#define OPCODE_LDX_ZP 0xa6
#define OPCODE_LDX_ZPY 0xb6

#define OPCODE_LDY_IMM 0xa0
#define OPCODE_LDY_ABS 0xac
#define OPCODE_LDY_ABSX 0xbc
#define OPCODE_LDY_ZP 0xa4
#define OPCODE_LDY_ZPX 0xb4

#define OPCODE_LSR_ACC 0x4a
#define OPCODE_LSR_ABS 0x4e
#define OPCODE_LSR_ABSX 0x5e
#define OPCODE_LSR_ZP 0x46
#define OPCODE_LSR_ZPX 0x56

#define OPCODE_NOP_IMP 0xea
#define OPCODE_NOP_IMP_2 0x1a
#define OPCODE_NOP_IMP_3 0x3a
#define OPCODE_NOP_IMP_4 0x5a
#define OPCODE_NOP_IMP_5 0x7a
#define OPCODE_NOP_IMP_6 0xda
#define OPCODE_NOP_IMP_7 0xfa

#define OPCODE_ORA_IMM 0x09
#define OPCODE_ORA_ABS 0x0d
#define OPCODE_ORA_ABSX 0x1d
#define OPCODE_ORA_ABSY 0x19
#define OPCODE_ORA_ZP 0x05
#define OPCODE_ORA_ZPX 0x15
#define OPCODE_ORA_INX 0x01
#define OPCODE_ORA_INY 0x11

#define OPCODE_PHA_IMP 0x48
#define OPCODE_PHP_IMP 0x08
#define OPCODE_PLA_IMP 0x68
#define OPCODE_PLP_IMP 0x28

#define OPCODE_ROL_ACC 0x2a
#define OPCODE_ROL_ABS 0x2e
#define OPCODE_ROL_ABSX 0x3e
#define OPCODE_ROL_ZP 0x26
#define OPCODE_ROL_ZPX 0x36

#define OPCODE_ROR_ACC 0x6a
#define OPCODE_ROR_ABS 0x6e
#define OPCODE_ROR_ABSX 0x7e
#define OPCODE_ROR_ZP 0x66
#define OPCODE_ROR_ZPX 0x76

#define OPCODE_RTI_IMP 0x40

#define OPCODE_RTS_IMP 0x60

#define OPCODE_SBC_IMM 0xe9
#define OPCODE_SBC_ABS 0xed
#define OPCODE_SBC_ABSX 0xfd
#define OPCODE_SBC_ABSY 0xf9
#define OPCODE_SBC_ZP 0xe5
#define OPCODE_SBC_ZPX 0xf5
#define OPCODE_SBC_INX 0xe1
#define OPCODE_SBC_INY 0xf1

#define OPCODE_SEC_IMP 0x38

#define OPCODE_SED_IMP 0xf8

#define OPCODE_SEI_IMP 0x78

#define OPCODE_STA_ABS 0x8d
#define OPCODE_STA_ABSX 0x9d
#define OPCODE_STA_ABSY 0x99
#define OPCODE_STA_ZP 0x85
#define OPCODE_STA_ZPX 0x95
#define OPCODE_STA_INX 0x81
#define OPCODE_STA_INY 0x91

#define OPCODE_STX_ABS 0x8e
#define OPCODE_STX_ZP 0x86
#define OPCODE_STX_ZPY 0x96

#define OPCODE_STY_ABS 0x8c
#define OPCODE_STY_ZP 0x84
#define OPCODE_STY_ZPX 0x94

#define OPCODE_TAX_IMP 0xaa

#define OPCODE_TAY_IMP 0xa8

#define OPCODE_TSX_IMP 0xba

#define OPCODE_TXA_IMP 0x8a

#define OPCODE_TXS_IMP 0x9a

#define OPCODE_TYA_IMP 0x98

/*
 * UNOFFICIAL OPCODES
 */

#define OPCODE_ALR_IMM 0x4b

#define OPCODE_ANC_IMM 0x0b
#define OPCODE_ANC_IMM_2 0x2b

#define OPCODE_ANE_IMM 0x8b

#define OPCODE_ARR_IMM 0x6b

#define OPCODE_AXS_IMM 0xcb

#define OPCODE_LAX_IMM 0xab
#define OPCODE_LAX_ABS 0xaf
#define OPCODE_LAX_ABSY 0xbf
#define OPCODE_LAX_ZP 0xa7
#define OPCODE_LAX_ZPY 0xb7
#define OPCODE_LAX_INX 0xa3
#define OPCODE_LAX_INY 0xb3

#define OPCODE_SAX_ABS 0x8f
#define OPCODE_SAX_ZP 0x87
#define OPCODE_SAX_ZPY 0x97
#define OPCODE_SAX_INX 0x83

#define OPCODE_DCP_ABS 0xcf
#define OPCODE_DCP_ABSX 0xdf
#define OPCODE_DCP_ABSY 0xdb
#define OPCODE_DCP_ZP 0xc7
#define OPCODE_DCP_ZPX 0xd7
#define OPCODE_DCP_INX 0xc3
#define OPCODE_DCP_INY 0xd3

#define OPCODE_ISC_ABS 0xef
#define OPCODE_ISC_ABSX 0xff
#define OPCODE_ISC_ABSY 0xfb
#define OPCODE_ISC_ZP 0xe7
#define OPCODE_ISC_ZPX 0xf7
#define OPCODE_ISC_INX 0xe3
#define OPCODE_ISC_INY 0xf3

#define OPCODE_RLA_ABS 0x2f
#define OPCODE_RLA_ABSX 0x3f
#define OPCODE_RLA_ABSY 0x3b
#define OPCODE_RLA_ZP 0x27
#define OPCODE_RLA_ZPX 0x37
#define OPCODE_RLA_INX 0x23
#define OPCODE_RLA_INY 0x33

#define OPCODE_RRA_ABS 0x6f
#define OPCODE_RRA_ABSX 0x7f
#define OPCODE_RRA_ABSY 0x7b
#define OPCODE_RRA_ZP 0x67
#define OPCODE_RRA_ZPX 0x77
#define OPCODE_RRA_INX 0x63
#define OPCODE_RRA_INY 0x73

#define OPCODE_SLO_ABS 0x0f
#define OPCODE_SLO_ABSX 0x1f
#define OPCODE_SLO_ABSY 0x1b
#define OPCODE_SLO_ZP 0x07
#define OPCODE_SLO_ZPX 0x17
#define OPCODE_SLO_INX 0x03
#define OPCODE_SLO_INY 0x13

#define OPCODE_SRE_ABS 0x4f
#define OPCODE_SRE_ABSX 0x5f
#define OPCODE_SRE_ABSY 0x5b
#define OPCODE_SRE_ZP 0x47
#define OPCODE_SRE_ZPX 0x57
#define OPCODE_SRE_INX 0x43
#define OPCODE_SRE_INY 0x53

#define OPCODE_SHA_ABSY 0x9f
#define OPCODE_SHA_INY 0x93

#define OPCODE_SHX_ABSY 0x9e

#define OPCODE_SHY_ABSX 0x9c

#define OPCODE_TAS_ABSY 0x9b

#define OPCODE_USB_IMM 0xeb

#define OPCODE_LAS_ABSY 0xbb

#define OPCODE_SKB_IMM 0x80
#define OPCODE_SKB_IMM_2 0x82
#define OPCODE_SKB_IMM_3 0xc2
#define OPCODE_SKB_IMM_4 0xe2
#define OPCODE_SKB_IMM_5 0x89
#define OPCODE_SKB_ZP 0x04
#define OPCODE_SKB_ZP_2 0x44
#define OPCODE_SKB_ZP_3 0x64
#define OPCODE_SKB_ZPX 0x14
#define OPCODE_SKB_ZPX_2 0x34
#define OPCODE_SKB_ZPX_3 0x54
#define OPCODE_SKB_ZPX_4 0x74
#define OPCODE_SKB_ZPX_5 0xd4
#define OPCODE_SKB_ZPX_6 0xf4

#define OPCODE_IGN_ABS 0x0c
#define OPCODE_IGN_ABSX 0x1c
#define OPCODE_IGN_ABSX_2 0x3c
#define OPCODE_IGN_ABSX_3 0x5c
#define OPCODE_IGN_ABSX_4 0x7c
#define OPCODE_IGN_ABSX_5 0xdc
#define OPCODE_IGN_ABSX_6 0xfc

#define OPCODE_HLT_IMP 0x02
#define OPCODE_HLT_IMP_2 0x12
#define OPCODE_HLT_IMP_3 0x22
#define OPCODE_HLT_IMP_4 0x32
#define OPCODE_HLT_IMP_5 0x42
#define OPCODE_HLT_IMP_6 0x52
#define OPCODE_HLT_IMP_7 0x62
#define OPCODE_HLT_IMP_8 0x72
#define OPCODE_HLT_IMP_9 0x92
#define OPCODE_HLT_IMP_10 0xb2
#define OPCODE_HLT_IMP_11 0xd2
#define OPCODE_HLT_IMP_12 0xf2

#define U16_COMBINE(lsb, msb) ((uint16_t) ((msb) << 8) | (uint16_t) (lsb))

#define CPU_DISAS_BUF_SIZE 12

typedef enum IrqSource {
    IRQ_DMC = 1u << 0,
    IRQ_FRAME_CNT = 1u << 1,
    IRQ_MMC3 = 1u << 2
} IrqSource;

typedef struct Bus Bus;

typedef struct Cpu {
    Bus *bus;

    uint8_t A;
    uint8_t X;
    uint8_t Y;
    uint16_t PC;
    uint8_t S;
    uint8_t P;

    CycleCounter cycles;
    bool halt;
    bool update_i;
    bool new_i_value;
    bool halt_on_brk;
    bool nmi_pending;
    unsigned irq_mask;
} Cpu;

void cpu_init(Cpu *self, Bus *bus);
void cpu_reset(Cpu *self);
void cpu_run_until(Cpu *self, CycleCounter target_cycle);
void cpu_handle_nmi(Cpu *self);
void cpu_irq_pulldown(Cpu *self, IrqSource source, bool state);
size_t cpu_disassemble(const uint8_t *code, uint16_t pc, char *buf);

#ifdef __cplusplus
}
#endif
