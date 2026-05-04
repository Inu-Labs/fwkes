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

#include <fwkes/cpu.h>

#include <fwkes/bits.h>
#include <fwkes/bus.h>
#include <fwkes/util.h>

#include <assert.h>
#include <string.h>

#define SETFLAG(expr, flag)                                                    \
    self->P = (self->P & ~(flag)) | ((expr) ? (flag) : 0)

#define dummy_cycle() ++self->cycles
#define dummy_read(addr) read(self, addr)

#define SETC(expr) SETFLAG(expr, CPU_FLAG_C)
#define SETZ(expr) SETFLAG(expr, CPU_FLAG_Z)
#define SETI(expr) SETFLAG(expr, CPU_FLAG_I)
#define SETV(expr) SETFLAG(expr, CPU_FLAG_V)
#define SETN(expr) SETFLAG(expr, CPU_FLAG_N)

#define TAKE_BRANCH(expr)                                                      \
    if (expr) {                                                                \
        ++self->cycles;                                                        \
                                                                               \
        uint16_t offset = bus_read(self->bus, op_addr);                        \
        if (offset & 0x80)                                                     \
            offset |= 0xff00;                                                  \
        uint16_t eff_addr = self->PC + offset;                                 \
        bool page_crossed = (self->PC & 0xff00) != (eff_addr & 0xff00);        \
                                                                               \
        if (page_crossed) {                                                    \
            ++self->cycles;                                                    \
        }                                                                      \
                                                                               \
        self->PC = eff_addr;                                                   \
    }

typedef enum AddrMode {
    MODE_IMP,
    MODE_IND,
    MODE_INDX,
    MODE_INDX_RMW,
    MODE_INDY,
    MODE_INDY_RMW,
    MODE_INDY_W,
    MODE_ZP,
    MODE_ZP_RMW,
    MODE_ZPX,
    MODE_ZPX_RMW,
    MODE_ZPY,
    MODE_ZPY_RMW,
    MODE_IMM,
    MODE_ACC,
    MODE_ABS,
    MODE_ABS_RMW,
    MODE_ABSX,
    MODE_ABSX_RMW,
    MODE_ABSX_W,
    MODE_ABSY,
    MODE_ABSY_RMW,
    MODE_ABSY_W,
    MODE_REL
} AddrMode;

/* clang-format off */

static const uint8_t g_mode_table[256] = {
/* 0          1         2          3              4            5            6              7              8         9         A         B          C            D            E              F */
/* 0 */  MODE_IMP,  MODE_INDX, MODE_IMP,  MODE_INDX_RMW, MODE_ZP,     MODE_ZP,     MODE_ZP_RMW,   MODE_ZP_RMW,   MODE_IMP, MODE_IMM, MODE_ACC, MODE_IMM,  MODE_ABS,    MODE_ABS,    MODE_ABS_RMW,  MODE_ABS_RMW,
/* 1 */  MODE_REL,  MODE_INDY, MODE_IMP,  MODE_INDY_RMW, MODE_ZPX,    MODE_ZPX,    MODE_ZPX_RMW,  MODE_ZPX_RMW,  MODE_IMP, MODE_ABSY,MODE_IMP, MODE_ABSY_W, MODE_ABSX,   MODE_ABSX,   MODE_ABSX_RMW, MODE_ABSX_RMW,
/* 2 */  MODE_ABS,  MODE_INDX, MODE_IMP,  MODE_INDX_RMW, MODE_ZP,     MODE_ZP,     MODE_ZP_RMW,   MODE_ZP_RMW,   MODE_IMP, MODE_IMM, MODE_ACC, MODE_IMM,  MODE_ABS,    MODE_ABS,    MODE_ABS_RMW,  MODE_ABS_RMW,
/* 3 */  MODE_REL,  MODE_INDY, MODE_IMP,  MODE_INDY_RMW, MODE_ZPX,    MODE_ZPX,    MODE_ZPX_RMW,  MODE_ZPX_RMW,  MODE_IMP, MODE_ABSY,MODE_IMP, MODE_ABSY_RMW,MODE_ABSX,   MODE_ABSX,   MODE_ABSX_RMW, MODE_ABSX_RMW,
/* 4 */  MODE_IMP,  MODE_INDX, MODE_IMP,  MODE_INDX_RMW, MODE_ZP,     MODE_ZP,     MODE_ZP_RMW,   MODE_ZP_RMW,   MODE_IMP, MODE_IMM, MODE_ACC, MODE_IMM,  MODE_ABS,    MODE_ABS,    MODE_ABS_RMW,  MODE_ABS_RMW,
/* 5 */  MODE_REL,  MODE_INDY, MODE_IMP,  MODE_INDY_RMW, MODE_ZPX,    MODE_ZPX,    MODE_ZPX_RMW,  MODE_ZPX_RMW,  MODE_IMP, MODE_ABSY,MODE_IMP, MODE_ABSY_W, MODE_ABSX,   MODE_ABSX,   MODE_ABSX_RMW, MODE_ABSX_RMW,
/* 6 */  MODE_IMP,  MODE_INDX, MODE_IMP,  MODE_INDX_RMW, MODE_ZP,     MODE_ZP,     MODE_ZP_RMW,   MODE_ZP_RMW,   MODE_IMP, MODE_IMM, MODE_ACC, MODE_IMM,  MODE_IND,    MODE_ABS,    MODE_ABS_RMW,  MODE_ABS_RMW,
/* 7 */  MODE_REL,  MODE_INDY, MODE_IMP,  MODE_INDY_RMW, MODE_ZPX,    MODE_ZPX,    MODE_ZPX_RMW,  MODE_ZPX_RMW,  MODE_IMP, MODE_ABSY,MODE_IMP, MODE_ABSY_RMW,MODE_ABSX,   MODE_ABSX,   MODE_ABSX_RMW, MODE_ABSX_RMW,
/* 8 */  MODE_IMM,  MODE_INDX, MODE_IMM,  MODE_INDX,     MODE_ZP,     MODE_ZP,     MODE_ZP,       MODE_ZP_RMW,   MODE_IMP, MODE_IMM, MODE_IMP, MODE_IMM,  MODE_ABS,    MODE_ABS,    MODE_ABS,      MODE_ABS,
/* 9 */  MODE_REL,  MODE_INDY_W,MODE_IMP, MODE_INDY_W,   MODE_ZPX,    MODE_ZPX,    MODE_ZPY,      MODE_ZPY,      MODE_IMP, MODE_ABSY_W,MODE_IMP,MODE_ABSY_W, MODE_ABSX,   MODE_ABSX_W, MODE_ABSY_W,   MODE_ABSY_W,
/* A */  MODE_IMM,  MODE_INDX, MODE_IMM,  MODE_INDX,     MODE_ZP,     MODE_ZP,     MODE_ZP,       MODE_ZP,       MODE_IMP, MODE_IMM, MODE_IMP, MODE_IMM,  MODE_ABS,    MODE_ABS,    MODE_ABS,      MODE_ABS,
/* B */  MODE_REL,  MODE_INDY, MODE_IMP,  MODE_INDY,     MODE_ZPX,    MODE_ZPX,    MODE_ZPY,      MODE_ZPY,      MODE_IMP, MODE_ABSY,MODE_IMP, MODE_ABSY, MODE_ABSX,   MODE_ABSX,   MODE_ABSY,     MODE_ABSY,
/* C */  MODE_IMM,  MODE_INDX, MODE_IMM,  MODE_INDX_RMW, MODE_ZP,     MODE_ZP,     MODE_ZP_RMW,   MODE_ZP_RMW,   MODE_IMP, MODE_IMM, MODE_IMP, MODE_IMM,  MODE_ABS,    MODE_ABS,    MODE_ABS_RMW,  MODE_ABS_RMW,
/* D */  MODE_REL,  MODE_INDY, MODE_IMP,  MODE_INDY_RMW, MODE_ZPX,    MODE_ZPX,    MODE_ZPX_RMW,  MODE_ZPX_RMW,  MODE_IMP, MODE_ABSY,MODE_IMP, MODE_ABSY_RMW,MODE_ABSX,   MODE_ABSX,   MODE_ABSX_RMW, MODE_ABSX_RMW,
/* E */  MODE_IMM,  MODE_INDX, MODE_IMM,  MODE_INDX_RMW, MODE_ZP,     MODE_ZP,     MODE_ZP_RMW,   MODE_ZP_RMW,   MODE_IMP, MODE_IMM, MODE_IMP, MODE_IMM,  MODE_ABS,    MODE_ABS,    MODE_ABS_RMW,  MODE_ABS_RMW,
/* F */  MODE_REL,  MODE_INDY, MODE_IMP,  MODE_INDY_RMW, MODE_ZPX,    MODE_ZPX,    MODE_ZPX_RMW,  MODE_ZPX_RMW,  MODE_IMP, MODE_ABSY,MODE_IMP, MODE_ABSY_RMW,MODE_ABSX,   MODE_ABSX,   MODE_ABSX_RMW, MODE_ABSX_RMW
};

/* clang-format on */

FORCE_INLINE void write(Cpu *self, uint16_t address, uint8_t data) {
    ++self->cycles;
    bus_write(self->bus, address, data);
}

FORCE_INLINE uint8_t read(Cpu *self, uint16_t address) {
    ++self->cycles;
    return bus_read(self->bus, address);
}

FORCE_INLINE void push_stack(Cpu *self, uint8_t value) {
    write(self, 0x0100 + self->S, value);
    --self->S;
}

FORCE_INLINE uint8_t pop_stack(Cpu *self) {
    ++self->S;
    ++self->cycles;

    return read(self, 0x0100 + self->S);
}

size_t disas_acc(char *buf, const char *instr_name) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s A", instr_name);

    return 1;
}

size_t disas_imp(char *buf, const char *instr_name) {
    strcpy(buf, instr_name);

    return 1;
}

size_t disas_imm(char *buf, const char *instr_name, uint8_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s #$%02x", instr_name, op);

    return 2;
}

size_t disas_zp(char *buf, const char *instr_name, uint8_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s $%02x", instr_name, op);

    return 2;
}

size_t disas_zpx(char *buf, const char *instr_name, uint8_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s $%02x,X", instr_name, op);

    return 2;
}

size_t disas_zpy(char *buf, const char *instr_name, uint8_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s $%02x,Y", instr_name, op);

    return 2;
}

size_t disas_abs(char *buf, const char *instr_name, uint16_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s $%04x", instr_name, op);

    return 3;
}

size_t disas_ind(char *buf, const char *instr_name, uint16_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s ($%04x)", instr_name, op);

    return 3;
}

size_t disas_absx(char *buf, const char *instr_name, uint16_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s $%04x,X", instr_name, op);

    return 3;
}

size_t disas_absy(char *buf, const char *instr_name, uint16_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s $%04x,Y", instr_name, op);

    return 3;
}

size_t disas_inx(char *buf, const char *instr_name, uint8_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s ($%02x,X)", instr_name, op);

    return 2;
}

size_t disas_iny(char *buf, const char *instr_name, uint8_t op) {
    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s ($%02x),Y", instr_name, op);

    return 2;
}

size_t disas_rel(char *buf, const char *instr_name, uint8_t op, uint16_t pc) {
    uint16_t offset = op;

    if (offset & 0x80) {
        offset |= 0xff00;
    }

    uint16_t eff_addr = pc + offset + 2;

    snprintf(buf, CPU_DISAS_BUF_SIZE, "%s $%04x", instr_name, eff_addr);

    return 2;
}

void cpu_init(Cpu *self, Bus *bus) {
    memset(self, 0, sizeof(*self));

    self->bus = bus;
    self->S = 0xfd;
    self->P = 0x00 | CPU_FLAG_I | CPU_FLAG_U;

    /* According to the datasheet, init sequence takes 6 cycles.
     * The extra +2 cycles takes setting the PC register. */
    self->cycles = 8;
}

void cpu_reset(Cpu *self) {
    self->S -= 3;
    self->P |= CPU_FLAG_I;
    self->cycles = 8;
}

FORCE_INLINE uint16_t fetch_word(Cpu *self) {
    uint16_t lsb = read(self, self->PC++);
    uint16_t msb = read(self, self->PC++);

    return U16_COMBINE(lsb, msb);
}

static uint16_t fetch_op_addr(Cpu *self, AddrMode mode) {
    uint16_t eff_addr = 0;

    switch (mode) {
    case MODE_IMP:
    case MODE_ACC:
        dummy_read(self->PC);

        break;
    case MODE_IMM:
        eff_addr = self->PC++;

        break;
    case MODE_REL:
        ++self->cycles;
        eff_addr = self->PC++;

        break;
    case MODE_ZP:
        eff_addr = read(self, self->PC++);

        break;
    case MODE_ZP_RMW: {
        eff_addr = read(self, self->PC++);

        /* 6502 writes original value back to memory. */
        uint8_t original_value = bus_read(self->bus, eff_addr);
        write(self, eff_addr, original_value);

        break;
    }
    case MODE_ZPX:
        eff_addr = (read(self, self->PC++) + self->X) & 0xff;
        dummy_read(eff_addr);

        break;
    case MODE_ZPX_RMW: {
        eff_addr = (read(self, self->PC++) + self->X) & 0xff;

        /* 6502 writes original value back to memory. */
        uint8_t original_value = read(self, eff_addr);
        write(self, eff_addr, original_value);

        break;
    }
    case MODE_ZPY:
        eff_addr = (read(self, self->PC++) + self->Y) & 0xff;
        dummy_read(eff_addr);

        break;
    case MODE_ZPY_RMW: {
        eff_addr = (read(self, self->PC++) + self->Y) & 0xff;

        /* 6502 writes original value back to memory. */
        uint8_t original_value = read(self, eff_addr);
        write(self, eff_addr, original_value);

        break;
    }
    case MODE_ABS:
        eff_addr = fetch_word(self);

        break;
    case MODE_ABS_RMW: {
        eff_addr = fetch_word(self);

        /* 6502 writes original value back to memory. */
        uint8_t original_value = bus_read(self->bus, eff_addr);
        write(self, eff_addr, original_value);

        break;
    }
    case MODE_ABSX: {
        uint16_t addr = fetch_word(self);
        eff_addr = addr + self->X;
        bool page_crossed = (addr & 0xff00) != (eff_addr & 0xff00);

        if (page_crossed) {
            ++self->cycles;
        }

        break;
    }
    case MODE_ABSX_RMW: {
        uint16_t addr = fetch_word(self);
        eff_addr = addr + self->X;
        bool page_crossed = (addr & 0xff00) != (eff_addr & 0xff00);

        /* 6502 writes original value back to memory. */
        uint8_t original_value = read(self, eff_addr);
        write(self, eff_addr, original_value);

        if (page_crossed) {
            ++self->cycles;
        }

        break;
    }
    case MODE_ABSX_W: {
        uint16_t base = fetch_word(self);
        eff_addr = base + self->X;
        dummy_read(eff_addr);

        break;
    }
    case MODE_ABSY: {
        uint16_t addr = fetch_word(self);
        eff_addr = addr + self->Y;
        bool page_crossed = (addr & 0xff00) != (eff_addr & 0xff00);

        if (page_crossed) {
            ++self->cycles;
        }

        break;
    }
    case MODE_ABSY_RMW: {
        uint16_t addr = fetch_word(self);
        eff_addr = addr + self->Y;
        bool page_crossed = (addr & 0xff00) != (eff_addr & 0xff00);

        /* 6502 writes original value back to memory. */
        uint8_t original_value = read(self, eff_addr);
        write(self, eff_addr, original_value);

        if (page_crossed) {
            ++self->cycles;
        }

        break;
    }
    case MODE_ABSY_W: {
        uint16_t base = fetch_word(self);
        eff_addr = base + self->Y;
        dummy_read(eff_addr);

        break;
    }
    case MODE_IND: {
        uint16_t low_addr = read(self, self->PC++);
        uint16_t high_addr = read(self, self->PC++);
        uint16_t addr = U16_COMBINE(low_addr, high_addr);

        uint16_t lsb = read(self, addr);
        uint16_t msb =
            read(self, (low_addr == 0xff) ? (addr & 0xff00) : (addr + 1));
        eff_addr = U16_COMBINE(lsb, msb);

        break;
    }
    case MODE_INDX: {
        uint16_t addr = (read(self, self->PC++) + self->X) & 0xff;
        dummy_read(addr);
        uint16_t lsb = read(self, addr);
        uint16_t msb = read(self, (addr + 1) & 0xff);
        eff_addr = U16_COMBINE(lsb, msb);

        break;
    }
    case MODE_INDX_RMW: {
        uint16_t addr = (read(self, self->PC++) + self->X) & 0xff;
        dummy_read(addr);
        uint16_t lsb = read(self, addr);
        uint16_t msb = read(self, (addr + 1) & 0xff);
        eff_addr = U16_COMBINE(lsb, msb);

        /* 6502 writes original value back to memory. */
        uint8_t original_value = read(self, eff_addr);
        write(self, eff_addr, original_value);

        break;
    }
    case MODE_INDY: {
        uint16_t addr = read(self, self->PC++);
        uint16_t lsb = read(self, addr);
        uint16_t msb = read(self, (addr + 1) & 0xff);
        uint16_t base = U16_COMBINE(lsb, msb);
        eff_addr = base + self->Y;

        bool page_crossed = (base & 0xff00) != (eff_addr & 0xff00);

        if (page_crossed) {
            dummy_read((base & 0xff00) | (eff_addr & 0x00ff));
        }

        break;
    }
    case MODE_INDY_RMW: {
        uint16_t ptr = read(self, self->PC++);
        uint16_t lsb = read(self, ptr);
        uint16_t msb = read(self, (ptr + 1) & 0xff);
        uint16_t base = U16_COMBINE(lsb, msb);
        eff_addr = base + self->Y;

        uint8_t original_value = read(self, eff_addr);
        write(self, eff_addr, original_value);

        bool page_crossed = (base & 0xff00) != (eff_addr & 0xff00);

        if (page_crossed) {
            dummy_read((base & 0xff00) | (eff_addr & 0x00ff));
        }

        break;
    }
    case MODE_INDY_W: {
        uint16_t addr = read(self, self->PC++);
        uint16_t lsb = read(self, addr);
        uint16_t msb = read(self, (addr + 1) & 0xff);
        uint16_t base = U16_COMBINE(lsb, msb);
        eff_addr = base + self->Y;
        dummy_read(eff_addr);

        break;
    }
    }

    return eff_addr;
}

static bool check_interrupts(Cpu *self) {
    if (self->nmi_pending) {
        cpu_handle_nmi(self);
        self->nmi_pending = false;
        return true;
    }

    if (self->irq_mask != 0 && !(self->P & CPU_FLAG_I)) {
        self->cycles += 7;

        push_stack(self, (self->PC >> 8) & 0xFF);
        push_stack(self, self->PC & 0xFF);
        push_stack(self, (self->P & ~0x10) | 0x20);

        self->P |= CPU_FLAG_I;

        uint8_t low = read(self, 0xFFFE);
        uint8_t high = read(self, 0xFFFF);
        self->PC = (uint16_t) (high << 8) | low;

        return true;
    }

    if (self->update_i) {
        SETI(self->new_i_value);
        self->update_i = false;
    }

    return false;
}

void NOTFLASH_FN(cpu_run_until)(Cpu *self, CycleCounter target_cycle) {
    /* clang-format off */
    static const void *dispatch_table[256] = {
        /* 0x00 */ &&op_brk_imp,    &&op_ora_inx,    &&op_hlt_imp,    &&op_slo_inx,    &&op_skb_zp,     &&op_ora_zp,     &&op_asl_zp,     &&op_slo_zp,
        /* 0x08 */ &&op_php_imp,    &&op_ora_imm,    &&op_asl_acc,    &&op_anc_imm,    &&op_ign_abs,    &&op_ora_abs,    &&op_asl_abs,    &&op_slo_abs,
        /* 0x10 */ &&op_bpl_rel,    &&op_ora_iny,    &&op_hlt_imp_2,  &&op_slo_iny,    &&op_skb_zpx,    &&op_ora_zpx,    &&op_asl_zpx,    &&op_slo_zpx,
        /* 0x18 */ &&op_clc_imp,    &&op_ora_absy,   &&op_nop_imp_2,  &&op_slo_absy,   &&op_ign_absx,   &&op_ora_absx,   &&op_asl_absx,   &&op_slo_absx,
        /* 0x20 */ &&op_jsr_abs,    &&op_and_inx,    &&op_hlt_imp_3,  &&op_rla_inx,    &&op_bit_zp,     &&op_and_zp,     &&op_rol_zp,     &&op_rla_zp,
        /* 0x28 */ &&op_plp_imp,    &&op_and_imm,    &&op_rol_acc,    &&op_anc_imm_2,  &&op_bit_abs,    &&op_and_abs,    &&op_rol_abs,    &&op_rla_abs,
        /* 0x30 */ &&op_bmi_rel,    &&op_and_iny,    &&op_hlt_imp_4,  &&op_rla_iny,    &&op_skb_zpx_2,  &&op_and_zpx,    &&op_rol_zpx,    &&op_rla_zpx,
        /* 0x38 */ &&op_sec_imp,    &&op_and_absy,   &&op_nop_imp_3,  &&op_rla_absy,   &&op_ign_absx_2, &&op_and_absx,   &&op_rol_absx,   &&op_rla_absx,
        /* 0x40 */ &&op_rti_imp,    &&op_eor_inx,    &&op_hlt_imp_5,  &&op_sre_inx,    &&op_skb_zp_2,   &&op_eor_zp,     &&op_lsr_zp,     &&op_sre_zp,
        /* 0x48 */ &&op_pha_imp,    &&op_eor_imm,    &&op_lsr_acc,    &&op_alr_imm,    &&op_jmp_abs,    &&op_eor_abs,    &&op_lsr_abs,    &&op_sre_abs,
        /* 0x50 */ &&op_bvc_rel,    &&op_eor_iny,    &&op_hlt_imp_6,  &&op_sre_iny,    &&op_skb_zpx_3,  &&op_eor_zpx,    &&op_lsr_zpx,    &&op_sre_zpx,
        /* 0x58 */ &&op_cli_imp,    &&op_eor_absy,   &&op_nop_imp_4,  &&op_sre_absy,   &&op_ign_absx_3, &&op_eor_absx,   &&op_lsr_absx,   &&op_sre_absx,
        /* 0x60 */ &&op_rts_imp,    &&op_adc_inx,    &&op_hlt_imp_7,  &&op_rra_inx,    &&op_skb_zp_3,   &&op_adc_zp,     &&op_ror_zp,     &&op_rra_zp,
        /* 0x68 */ &&op_pla_imp,    &&op_adc_imm,    &&op_ror_acc,    &&op_arr_imm,    &&op_jmp_ind,    &&op_adc_abs,    &&op_ror_abs,    &&op_rra_abs,
        /* 0x70 */ &&op_bvs_rel,    &&op_adc_iny,    &&op_hlt_imp_8,  &&op_rra_iny,    &&op_skb_zpx_4,  &&op_adc_zpx,    &&op_ror_zpx,    &&op_rra_zpx,
        /* 0x78 */ &&op_sei_imp,    &&op_adc_absy,   &&op_nop_imp_5,  &&op_rra_absy,   &&op_ign_absx_4, &&op_adc_absx,   &&op_ror_absx,   &&op_rra_absx,
        /* 0x80 */ &&op_skb_imm,    &&op_sta_inx,    &&op_skb_imm_2,  &&op_sax_inx,    &&op_sty_zp,     &&op_sta_zp,     &&op_stx_zp,     &&op_sax_zp,
        /* 0x88 */ &&op_dey_imp,    &&op_skb_imm_3,  &&op_txa_imp,    &&op_ane_imm,    &&op_sty_abs,    &&op_sta_abs,    &&op_stx_abs,    &&op_sax_abs,
        /* 0x90 */ &&op_bcc_rel,    &&op_sta_iny,    &&op_hlt_imp_9,  &&op_sha_iny,    &&op_sty_zpx,    &&op_sta_zpx,    &&op_stx_zpy,    &&op_sax_zpy,
        /* 0x98 */ &&op_tya_imp,    &&op_sta_absy,   &&op_txs_imp,    &&op_tas_absy,   &&op_shy_absx,   &&op_sta_absx,   &&op_shx_absy,   &&op_sha_absy,
        /* 0xa0 */ &&op_ldy_imm,    &&op_lda_inx,    &&op_ldx_imm,    &&op_lax_inx,    &&op_ldy_zp,     &&op_lda_zp,     &&op_ldx_zp,     &&op_lax_zp,
        /* 0xa8 */ &&op_tay_imp,    &&op_lda_imm,    &&op_tax_imp,    &&op_lax_imm,    &&op_ldy_abs,    &&op_lda_abs,    &&op_ldx_abs,    &&op_lax_abs,
        /* 0xb0 */ &&op_bcs_rel,    &&op_lda_iny,    &&op_hlt_imp_10, &&op_lax_iny,    &&op_ldy_zpx,    &&op_lda_zpx,    &&op_ldx_zpy,    &&op_lax_zpy,
        /* 0xb8 */ &&op_clv_imp,    &&op_lda_absy,   &&op_tsx_imp,    &&op_las_absy,   &&op_ldy_absx,   &&op_lda_absx,   &&op_ldx_absy,   &&op_lax_absy,
        /* 0xc0 */ &&op_cpy_imm,    &&op_cmp_inx,    &&op_skb_imm_4,  &&op_dcp_inx,    &&op_cpy_zp,     &&op_cmp_zp,     &&op_dec_zp,     &&op_dcp_zp,
        /* 0xc8 */ &&op_iny_imp,    &&op_cmp_imm,    &&op_dex_imp,    &&op_axs_imm,    &&op_cpy_abs,    &&op_cmp_abs,    &&op_dec_abs,    &&op_dcp_abs,
        /* 0xd0 */ &&op_bne_rel,    &&op_cmp_iny,    &&op_hlt_imp_11, &&op_dcp_iny,    &&op_skb_zpx_5,  &&op_cmp_zpx,    &&op_dec_zpx,    &&op_dcp_zpx,
        /* 0xd8 */ &&op_cld_imp,    &&op_cmp_absy,   &&op_nop_imp_6,  &&op_dcp_absy,   &&op_ign_absx_5, &&op_cmp_absx,   &&op_dec_absx,   &&op_dcp_absx,
        /* 0xe0 */ &&op_cpx_imm,    &&op_sbc_inx,    &&op_skb_imm_5,  &&op_isc_inx,    &&op_cpx_zp,     &&op_sbc_zp,     &&op_inc_zp,     &&op_isc_zp,
        /* 0xe8 */ &&op_inx_imp,    &&op_sbc_imm,    &&op_nop_imp,    &&op_usb_imm,    &&op_cpx_abs,    &&op_sbc_abs,    &&op_inc_abs,    &&op_isc_abs,
        /* 0xf0 */ &&op_beq_rel,    &&op_sbc_iny,    &&op_hlt_imp_12, &&op_isc_iny,    &&op_skb_zpx_6,  &&op_sbc_zpx,    &&op_inc_zpx,    &&op_isc_zpx,
        /* 0xf8 */ &&op_sed_imp,    &&op_sbc_absy,   &&op_nop_imp_7,  &&op_isc_absy,   &&op_ign_absx_6, &&op_sbc_absx,   &&op_inc_absx,   &&op_isc_absx,
    };
    /* clang-format on */

#define DISPATCH()                                                             \
    do {                                                                       \
        if (self->cycles >= target_cycle || self->bus->ppu.frame_done)         \
            return;                                                            \
        if (check_interrupts(self))                                            \
            return;                                                            \
        opcode = read(self, self->PC++);                                       \
        op_addr = fetch_op_addr(self, g_mode_table[opcode]);                   \
        goto *dispatch_table[opcode];                                          \
    } while (0)

    uint8_t opcode = read(self, self->PC++);
    uint16_t op_addr = fetch_op_addr(self, g_mode_table[opcode]);
    goto *dispatch_table[opcode];

op_adc_imm:
op_adc_abs:
op_adc_absx:
op_adc_absy:
op_adc_zp:
op_adc_zpx:
op_adc_inx:
op_adc_iny: {
    uint8_t data = read(self, op_addr);
    uint8_t res = 0;

    uint8_t curr_carry = BIT_CHECK(self->P, CPU_FLAG_C);
    uint8_t new_carry = 0;

#ifdef __clang__
    res = __builtin_addcb(self->A, data, curr_carry, &new_carry);
#else
    uint8_t temp_res;
    uint8_t c1 = __builtin_add_overflow(self->A, data, &temp_res);
    uint8_t c2 = __builtin_add_overflow(temp_res, curr_carry, &res);
    new_carry = c1 | c2;
#endif

    SETV(~(self->A ^ data) & (self->A ^ res) & 0x80);
    SETC(new_carry);
    SETZ(res == 0x00);
    SETN(res & 0x80);

    self->A = res;

    DISPATCH();
}
op_and_imm:
op_and_abs:
op_and_absx:
op_and_absy:
op_and_zp:
op_and_zpx:
op_and_inx:
op_and_iny: {
    uint8_t data = read(self, op_addr);
    self->A &= data;

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_asl_acc: {
    SETC(self->A & 0x80);

    self->A <<= 1;

    SETN(self->A & 0x80);
    SETZ(self->A == 0);

    DISPATCH();
}
op_asl_abs:
op_asl_absx:
op_asl_zp:
op_asl_zpx: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);

    SETC(data & 0x80);

    data <<= 1;
    write(self, op_addr, data);

    SETN(data & 0x80);
    SETZ(data == 0);

    DISPATCH();
}
op_bcc_rel: {
    TAKE_BRANCH(!(self->P & CPU_FLAG_C));

    DISPATCH();
}
op_bcs_rel: {
    TAKE_BRANCH(self->P & CPU_FLAG_C);

    DISPATCH();
}
op_beq_rel: {
    TAKE_BRANCH(self->P & CPU_FLAG_Z);

    DISPATCH();
}
op_bit_abs:
op_bit_zp: {
    uint8_t mask = read(self, op_addr);
    uint8_t res = self->A & mask;

    SETZ(res == 0);
    SETN(mask & 0x80);
    SETV(mask & 0x40);

    DISPATCH();
}
op_bmi_rel: {
    TAKE_BRANCH(self->P & CPU_FLAG_N);

    DISPATCH();
}
op_bne_rel: {
    TAKE_BRANCH(!(self->P & CPU_FLAG_Z));

    DISPATCH();
}
op_bpl_rel: {
    TAKE_BRANCH(!(self->P & CPU_FLAG_N));

    DISPATCH();
}
op_brk_imp: {
    if (self->halt_on_brk) {
        self->halt = true;

        DISPATCH();
    }

    uint16_t pc = ++self->PC;
    push_stack(self, (pc & 0xff00) >> 8);
    push_stack(self, pc & 0x00ff);

    uint8_t flags = self->P | CPU_FLAG_B;
    push_stack(self, flags);
    self->P |= CPU_FLAG_I;

    uint8_t pc_lsb = read(self, 0xfffe);
    uint8_t pc_msb = read(self, 0xffff);

    self->PC = U16_COMBINE(pc_lsb, pc_msb);

    DISPATCH();
}
op_bvc_rel: {
    TAKE_BRANCH(!(self->P & CPU_FLAG_V));

    DISPATCH();
}
op_bvs_rel: {
    TAKE_BRANCH(self->P & CPU_FLAG_V);

    DISPATCH();
}
op_clc_imp:
    BIT_CLEAR(self->P, CPU_FLAG_C);

    DISPATCH();
op_cld_imp:
    BIT_CLEAR(self->P, CPU_FLAG_D);

    DISPATCH();
op_cli_imp:
    self->update_i = true;
    self->new_i_value = false;

    DISPATCH();
op_clv_imp:
    BIT_CLEAR(self->P, CPU_FLAG_V);

    DISPATCH();
op_cmp_imm:
op_cmp_abs:
op_cmp_absx:
op_cmp_absy:
op_cmp_zp:
op_cmp_zpx:
op_cmp_inx:
op_cmp_iny: {
    uint8_t data = read(self, op_addr);

    SETZ(self->A == data);
    SETN((self->A - data) & 0x80);
    SETC(data <= self->A);

    DISPATCH();
}
op_cpx_imm:
op_cpx_abs:
op_cpx_zp: {
    uint8_t data = read(self, op_addr);
    uint8_t res = self->X - data;

    SETZ(res == 0x00);
    SETN(res & 0x80);
    SETC(data <= self->X);

    DISPATCH();
}
op_cpy_imm:
op_cpy_abs:
op_cpy_zp: {
    uint8_t data = read(self, op_addr);
    uint8_t res = self->Y - data;

    SETZ(res == 0x00);
    SETN(res & 0x80);
    SETC(data <= self->Y);

    DISPATCH();
}
op_dec_abs:
op_dec_absx:
op_dec_zp:
op_dec_zpx: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    uint8_t res = data - 1;

    SETZ(res == 0x00);
    SETN(res & 0x80);

    write(self, op_addr, res);

    DISPATCH();
}
op_dex_imp:
    --self->X;

    SETZ(self->X == 0x00);
    SETN(self->X & 0x80);

    DISPATCH();
op_dey_imp:
    --self->Y;

    SETZ(self->Y == 0x00);
    SETN(self->Y & 0x80);

    DISPATCH();
op_eor_imm:
op_eor_abs:
op_eor_absx:
op_eor_absy:
op_eor_zp:
op_eor_zpx:
op_eor_inx:
op_eor_iny: {
    uint8_t data = read(self, op_addr);
    self->A ^= data;

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_inc_abs:
op_inc_absx:
op_inc_zp:
op_inc_zpx: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    uint8_t res = data + 1;

    SETZ(res == 0x00);
    SETN(res & 0x80);

    write(self, op_addr, res);

    DISPATCH();
}
op_inx_imp:
    ++self->X;

    SETZ(self->X == 0x00);
    SETN(self->X & 0x80);

    DISPATCH();
op_iny_imp:
    ++self->Y;

    SETZ(self->Y == 0x00);
    SETN(self->Y & 0x80);

    DISPATCH();
op_jmp_abs:
op_jmp_ind:
    self->PC = op_addr;

    DISPATCH();
op_jsr_abs: {
    --self->PC;
    push_stack(self, (self->PC & 0xff00) >> 8);
    push_stack(self, self->PC & 0xff);

    self->PC = op_addr;

    dummy_cycle();

    DISPATCH();
}
op_lda_imm:
op_lda_abs:
op_lda_absx:
op_lda_absy:
op_lda_zp:
op_lda_zpx:
op_lda_inx:
op_lda_iny: {
    self->A = read(self, op_addr);

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_ldx_imm:
op_ldx_abs:
op_ldx_absy:
op_ldx_zp:
op_ldx_zpy: {
    self->X = read(self, op_addr);

    SETZ(self->X == 0);
    SETN(self->X & 0x80);

    DISPATCH();
}
op_ldy_imm:
op_ldy_abs:
op_ldy_absx:
op_ldy_zp:
op_ldy_zpx: {
    self->Y = read(self, op_addr);

    SETN(self->Y & 0x80);
    SETZ(self->Y == 0);

    DISPATCH();
}
op_lsr_acc: {
    SETC(self->A & 0x1);

    self->A >>= 1;

    SETZ(self->A == 0x00);
    BIT_CLEAR(self->P, CPU_FLAG_N);

    DISPATCH();
}
op_lsr_abs:
op_lsr_absx:
op_lsr_zp:
op_lsr_zpx: {
    uint8_t data = read(self, op_addr);

    SETC(data & 0x1);

    data >>= 1;

    SETZ(data == 0x00);
    BIT_CLEAR(self->P, CPU_FLAG_N);

    write(self, op_addr, data);

    DISPATCH();
}
op_nop_imp:
op_nop_imp_2:
op_nop_imp_3:
op_nop_imp_4:
op_nop_imp_5:
op_nop_imp_6:
op_nop_imp_7:
    DISPATCH();
op_skb_imm:
op_skb_imm_2:
op_skb_imm_3:
op_skb_imm_4:
op_skb_imm_5:
    dummy_read(op_addr);

    DISPATCH();
op_skb_zp:
op_skb_zp_2:
op_skb_zp_3:
op_skb_zpx:
op_skb_zpx_2:
op_skb_zpx_3:
op_skb_zpx_4:
op_skb_zpx_5:
op_skb_zpx_6:
op_ign_abs:
op_ign_absx:
op_ign_absx_2:
op_ign_absx_3:
op_ign_absx_4:
op_ign_absx_5:
op_ign_absx_6:
    dummy_read(op_addr);
    DISPATCH();
op_ora_imm:
op_ora_abs:
op_ora_absx:
op_ora_absy:
op_ora_zp:
op_ora_zpx:
op_ora_inx:
op_ora_iny: {
    uint8_t data = read(self, op_addr);
    self->A |= data;

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_pha_imp:
    push_stack(self, self->A);

    DISPATCH();
op_php_imp:
    push_stack(self, self->P | CPU_FLAG_B | CPU_FLAG_U);

    DISPATCH();
op_pla_imp:
    self->A = pop_stack(self);

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
op_plp_imp: {
    uint8_t flags = pop_stack(self);

    flags = BIT_COPY(flags, self->P, CPU_FLAG_U);
    flags = BIT_COPY(flags, self->P, CPU_FLAG_B);
    uint8_t new_i = flags & CPU_FLAG_I;
    flags = BIT_COPY(flags, self->P, CPU_FLAG_I);

    self->update_i = true;
    self->new_i_value = new_i;

    self->P = flags;

    DISPATCH();
}
op_rol_acc: {
    uint8_t new_carry = self->A & 0x80;

    self->A <<= 1;

    BIT_SET(self->A, self->P & CPU_FLAG_C);

    SETC(new_carry);
    SETZ(self->A == 0x00);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_rol_abs:
op_rol_absx:
op_rol_zp:
op_rol_zpx: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    uint8_t new_carry = data & 0x80;

    data <<= 1;

    BIT_SET(data, self->P & CPU_FLAG_C);

    SETC(new_carry);
    SETZ(data == 0x00);
    SETN(data & 0x80);

    write(self, op_addr, data);

    DISPATCH();
}
op_ror_acc: {
    uint8_t new_carry = self->A & 0x01;

    self->A >>= 1;

    BIT_SET(self->A, (self->P & CPU_FLAG_C) << 7);

    SETC(new_carry);
    SETZ(self->A == 0x00);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_ror_abs:
op_ror_absx:
op_ror_zp:
op_ror_zpx: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    uint8_t new_carry = data & 0x01;

    data >>= 1;

    BIT_SET(data, (self->P & CPU_FLAG_C) << 7);

    SETC(new_carry);
    SETZ(data == 0x00);
    SETN(data & 0x80);

    write(self, op_addr, data);

    DISPATCH();
}
op_rti_imp: {
    uint8_t flags = pop_stack(self);

    flags = BIT_COPY(flags, self->P, CPU_FLAG_U);
    flags = BIT_COPY(flags, self->P, CPU_FLAG_B);

    self->P = flags;

    uint8_t pc_lsb = pop_stack(self);
    uint8_t pc_msb = pop_stack(self);

    self->PC = U16_COMBINE(pc_lsb, pc_msb);
    self->cycles -= 2;

    DISPATCH();
}
op_rts_imp: {
    uint8_t pc_lsb = pop_stack(self);
    uint8_t pc_msb = pop_stack(self);

    self->PC = U16_COMBINE(pc_lsb, pc_msb) + 1;

    DISPATCH();
}
op_sbc_imm:
op_sbc_abs:
op_sbc_absx:
op_sbc_absy:
op_sbc_zp:
op_sbc_zpx:
op_sbc_inx:
op_sbc_iny:
op_usb_imm: {
    uint8_t data = read(self, op_addr);
    uint8_t res = 0;

    uint8_t curr_carry = !BIT_CHECK(self->P, CPU_FLAG_C);
    uint8_t new_carry;

#ifdef __clang__
    res = __builtin_subcb(self->A, data, curr_carry, &new_carry);
#else
    uint8_t temp_res;
    uint8_t c1 = __builtin_sub_overflow(self->A, data, &temp_res);
    uint8_t c2 = __builtin_sub_overflow(temp_res, curr_carry, &res);
    new_carry = c1 | c2;
#endif

    SETV((res ^ self->A) & (res ^ ~data) & 0x80);
    SETC(!new_carry);
    SETZ(res == 0x00);
    SETN(res & 0x80);

    self->A = res;

    DISPATCH();
}
op_sec_imp:
    BIT_SET(self->P, CPU_FLAG_C);

    DISPATCH();
op_sed_imp:
    BIT_SET(self->P, CPU_FLAG_D);

    DISPATCH();
op_sei_imp:
    BIT_SET(self->P, CPU_FLAG_I);

    self->update_i = true;
    self->new_i_value = true;

    DISPATCH();
op_sta_absx:
op_sta_absy:
    // read(cpu, op_addr); /* dummy read */
op_sta_abs:
op_sta_zp:
op_sta_zpx:
op_sta_inx:
op_sta_iny: {
    write(self, op_addr, self->A);

    DISPATCH();
}
op_stx_abs:
op_stx_zp:
op_stx_zpy:
    write(self, op_addr, self->X);

    DISPATCH();
op_sty_abs:
op_sty_zp:
op_sty_zpx:
    write(self, op_addr, self->Y);

    DISPATCH();
op_tax_imp:
    self->X = self->A;

    SETN(self->X & 0x80);
    SETZ(self->X == 0x00);

    DISPATCH();
op_tay_imp:
    self->Y = self->A;

    SETN(self->Y & 0x80);
    SETZ(self->Y == 0x00);

    DISPATCH();
op_tsx_imp:
    self->X = self->S;

    SETN(self->X & 0x80);
    SETZ(self->X == 0x00);

    DISPATCH();
op_txa_imp:
    self->A = self->X;

    SETN(self->A & 0x80);
    SETZ(self->A == 0x00);

    DISPATCH();
op_txs_imp:
    self->S = self->X;

    DISPATCH();
op_tya_imp:
    self->A = self->Y;

    SETN(self->A & 0x80);
    SETZ(self->A == 0x00);

    DISPATCH();
op_alr_imm: {
    uint8_t data = read(self, op_addr);

    self->A &= data;
    SETC(self->A & 0x1);

    self->A >>= 1;

    SETZ(self->A == 0x00);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_anc_imm:
op_anc_imm_2: {
    uint8_t data = read(self, op_addr);
    self->A &= data;

    SETZ(self->A == 0);
    SETN(self->A & 0x80);
    SETC(self->A & 0x80);

    DISPATCH();
}
op_arr_imm: {
    uint8_t data = read(self, op_addr);

    self->A &= data;
    self->A >>= 1;

    BIT_SET(self->A, (self->P & CPU_FLAG_C) << 7);

    SETC(self->A & 0x40);
    SETV((self->A & 0x40) ^ ((self->A & 0x20) << 1));
    SETZ(self->A == 0x00);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_axs_imm: {
    uint8_t data = read(self, op_addr);

    self->X &= self->A;

    SETZ(self->X == data);
    SETN((self->X - data) & 0x80);
    SETC(data <= self->X);

    self->X -= data;

    DISPATCH();
}
op_lax_imm:
    /*
     * 'NMOS 6510 Unintended Opcodes' by groepaz/solution states, that this
     * instruction performs A,X = (A | MAGIC_CONST) & imm, but it fails
     * instr_test-v5. I looked into source code of a popular emulator
     * Mesen2, which passes this test, and it just performs A,X = imm.
     *
     * I guess it do be like that...
     */

    self->A = read(self, op_addr);
    self->X = self->A;

    SETN(self->X & 0x80);
    SETZ(self->X == 0x00);

    DISPATCH();
op_lax_iny:
    self->A = read(self, op_addr);
    self->X = self->A;

    SETN(self->A & 0x80);
    SETZ(self->A == 0x00);

    DISPATCH();
op_lax_zp:
op_lax_zpy:
op_lax_inx:
op_lax_abs:
op_lax_absy:
    self->A = read(self, op_addr);
    self->X = self->A;

    SETN(self->A & 0x80);
    SETZ(self->A == 0x00);

    DISPATCH();
op_sax_zp:
op_sax_zpy:
op_sax_inx:
op_sax_abs:
    write(self, op_addr, self->A & self->X);

    DISPATCH();
op_dcp_zp:
op_dcp_zpx:
op_dcp_inx:
op_dcp_iny:
op_dcp_abs:
op_dcp_absx:
op_dcp_absy: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    uint8_t res = data - 1;

    SETZ(res == self->A);
    SETN((self->A - res) & 0x80);
    SETC(res <= self->A);

    write(self, op_addr, res);

    DISPATCH();
}
op_isc_zp:
op_isc_zpx:
op_isc_inx:
op_isc_iny:
op_isc_abs:
op_isc_absx:
op_isc_absy: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    data += 1;

    write(self, op_addr, data);

    uint8_t res = 0;
    uint8_t curr_carry = !BIT_CHECK(self->P, CPU_FLAG_C);
    uint8_t new_carry;

#ifdef __clang__
    res = __builtin_subcb(self->A, data, curr_carry, &new_carry);
#else
    uint8_t temp_res;
    uint8_t c1 = __builtin_sub_overflow(self->A, data, &temp_res);
    uint8_t c2 = __builtin_sub_overflow(temp_res, curr_carry, &res);
    new_carry = c1 | c2;
#endif

    SETV((res ^ self->A) & (res ^ ~data) & 0x80);
    SETC(!new_carry);
    SETZ(res == 0x00);
    SETN(res & 0x80);

    self->A = res;

    DISPATCH();
}
op_rla_zp:
op_rla_zpx:
op_rla_inx:
op_rla_iny:
op_rla_abs:
op_rla_absx:
op_rla_absy: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    uint8_t new_carry = data & 0x80;

    data <<= 1;
    BIT_SET(data, self->P & CPU_FLAG_C);

    SETC(new_carry);

    write(self, op_addr, data);

    self->A &= data;

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_rra_zp:
op_rra_zpx:
op_rra_inx:
op_rra_iny:
op_rra_abs:
op_rra_absx:
op_rra_absy: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);
    uint8_t new_carry = data & 0x01;

    data >>= 1;
    BIT_SET(data, (self->P & CPU_FLAG_C) << 7);
    write(self, op_addr, data);

    uint8_t res = 0;
    uint8_t curr_carry = new_carry;
    new_carry = 0;

#ifdef __clang__
    res = __builtin_addcb(self->A, data, curr_carry, &new_carry);
#else
    uint8_t temp_res;
    uint8_t c1 = __builtin_add_overflow(self->A, data, &temp_res);
    uint8_t c2 = __builtin_add_overflow(temp_res, curr_carry, &res);
    new_carry = c1 | c2;
#endif

    SETV(~(self->A ^ data) & (self->A ^ res) & 0x80);
    SETC(new_carry);
    SETZ(res == 0x00);
    SETN(res & 0x80);

    self->A = res;

    DISPATCH();
}
op_slo_zp:
op_slo_zpx:
op_slo_inx:
op_slo_iny:
op_slo_abs:
op_slo_absx:
op_slo_absy: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);

    SETC(data & 0x80);

    data <<= 1;
    write(self, op_addr, data);

    self->A |= data;

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_sre_zp:
op_sre_zpx:
op_sre_inx:
op_sre_iny:
op_sre_abs:
op_sre_absx:
op_sre_absy: {
    // dummy_cycle();
    uint8_t data = read(self, op_addr);

    SETC(data & 0x1);

    data >>= 1;
    write(self, op_addr, data);

    self->A ^= data;

    SETZ(self->A == 0);
    SETN(self->A & 0x80);

    DISPATCH();
}
op_sha_iny:
op_sha_absy:
    /* TODO: implement later (unstable opcode) */

    DISPATCH();
op_shx_absy:
    /* TODO: implement later (unstable opcode) */

    DISPATCH();
op_shy_absx:
    /* TODO: implement later (unstable opcode) */

    DISPATCH();
op_tas_absy:
    /* TODO: implement later (unstable opcode) */

    DISPATCH();
op_las_absy: {
    uint8_t data = read(self, op_addr);
    uint8_t res = data & self->S;

    self->A = res;
    self->X = res;
    self->S = res;

    SETZ(res == 0x00);
    SETN(res & 0x80);

    DISPATCH();
}
op_hlt_imp:
op_hlt_imp_2:
op_hlt_imp_3:
op_hlt_imp_4:
op_hlt_imp_5:
op_hlt_imp_6:
op_hlt_imp_7:
op_hlt_imp_8:
op_hlt_imp_9:
op_hlt_imp_10:
op_hlt_imp_11:
op_hlt_imp_12:
    self->halt = true;

    DISPATCH();
op_ane_imm:
    DISPATCH();
}

void NOTFLASH_FN(cpu_handle_nmi)(Cpu *self) {
    self->nmi_pending = false;
    self->cycles += 2;

    push_stack(self, (self->PC & 0xff00) >> 8);
    push_stack(self, self->PC & 0xff);
    push_stack(self, self->P & ~CPU_FLAG_B);

    self->P |= CPU_FLAG_I;

    uint8_t pc_lsb = read(self, 0xfffa);
    uint8_t pc_msb = read(self, 0xfffb);
    self->PC = (uint16_t) ((pc_msb << 8) | pc_lsb);
}

void cpu_irq_pulldown(Cpu *self, IrqSource source, bool state) {
    if (state) {
        self->irq_mask |= source;
    } else {
        self->irq_mask &= ~source;
    }
}

size_t cpu_disassemble(const uint8_t *code, uint16_t pc, char *buf) {
    uint8_t opc0 = code[0];
    uint8_t opc1 = code[1];
    uint8_t opc2 = code[2];
    size_t instr_size = 0;

    switch (opc0) {
    case OPCODE_ADC_IMM:
        instr_size = disas_imm(buf, "adc", opc1);

        break;
    case OPCODE_ADC_ABS:
        instr_size = disas_abs(buf, "adc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ADC_ABSX:
        instr_size = disas_absx(buf, "adc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ADC_ABSY:
        instr_size = disas_absy(buf, "adc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ADC_ZP:
        instr_size = disas_zp(buf, "adc", opc1);

        break;
    case OPCODE_ADC_ZPX:
        instr_size = disas_zpx(buf, "adc", opc1);

        break;
    case OPCODE_ADC_INX:
        instr_size = disas_inx(buf, "adc", opc1);

        break;
    case OPCODE_ADC_INY:
        instr_size = disas_iny(buf, "adc", opc1);

        break;
    case OPCODE_AND_IMM:
        instr_size = disas_imm(buf, "and", opc1);

        break;
    case OPCODE_AND_ABS:
        instr_size = disas_abs(buf, "and", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_AND_ABSX:
        instr_size = disas_absx(buf, "and", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_AND_ABSY:
        instr_size = disas_absy(buf, "and", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_AND_ZP:
        instr_size = disas_zp(buf, "and", opc1);

        break;
    case OPCODE_AND_ZPX:
        instr_size = disas_zpx(buf, "and", opc1);

        break;
    case OPCODE_AND_INX:
        instr_size = disas_inx(buf, "and", opc1);

        break;
    case OPCODE_AND_INY:
        instr_size = disas_iny(buf, "and", opc1);

        break;
    case OPCODE_ASL_ACC:
        instr_size = disas_acc(buf, "asl");

        break;
    case OPCODE_ASL_ABS:
        instr_size = disas_abs(buf, "asl", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ASL_ABSX:

        instr_size = disas_absx(buf, "asl", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ASL_ZP:
        instr_size = disas_zp(buf, "asl", opc1);

        break;
    case OPCODE_ASL_ZPX:
        instr_size = disas_zpx(buf, "asl", opc1);

        break;
    case OPCODE_BCC_REL:
        instr_size = disas_rel(buf, "bcc", opc1, pc);

        break;
    case OPCODE_BCS_REL:
        instr_size = disas_rel(buf, "bcs", opc1, pc);

        break;
    case OPCODE_BEQ_REL:
        instr_size = disas_rel(buf, "beq", opc1, pc);

        break;
    case OPCODE_BIT_ABS:
        instr_size = disas_abs(buf, "bit", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_BIT_ZP:
        instr_size = disas_zp(buf, "bit", opc1);

        break;
    case OPCODE_BMI_REL:
        instr_size = disas_rel(buf, "bmi", opc1, pc);

        break;
    case OPCODE_BNE_REL:
        instr_size = disas_rel(buf, "bne", opc1, pc);

        break;
    case OPCODE_BPL_REL:
        instr_size = disas_rel(buf, "bpl", opc1, pc);

        break;
    case OPCODE_BRK_IMP:
        instr_size = disas_imp(buf, "brk");

        break;
    case OPCODE_BVC_REL:
        instr_size = disas_rel(buf, "bvc", opc1, pc);

        break;
    case OPCODE_BVS_REL:
        instr_size = disas_rel(buf, "bvs", opc1, pc);

        break;
    case OPCODE_CLC_IMP:
        instr_size = disas_imp(buf, "clc");

        break;
    case OPCODE_CLD_IMP:
        instr_size = disas_imp(buf, "cld");

        break;
    case OPCODE_CLI_IMP:
        instr_size = disas_imp(buf, "cli");

        break;
    case OPCODE_CLV_IMP:
        instr_size = disas_imp(buf, "clv");

        break;
    case OPCODE_CMP_IMM:
        instr_size = disas_imm(buf, "cmp", opc1);

        break;
    case OPCODE_CMP_ABS:
        instr_size = disas_abs(buf, "cmp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_CMP_ABSX:

        instr_size = disas_absx(buf, "cmp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_CMP_ABSY:

        instr_size = disas_absy(buf, "cmp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_CMP_ZP:
        instr_size = disas_zp(buf, "cmp", opc1);

        break;
    case OPCODE_CMP_ZPX:
        instr_size = disas_zpx(buf, "cmp", opc1);

        break;
    case OPCODE_CMP_INX:
        instr_size = disas_inx(buf, "cmp", opc1);

        break;
    case OPCODE_CMP_INY:
        instr_size = disas_iny(buf, "cmp", opc1);

        break;
    case OPCODE_CPX_IMM:
        instr_size = disas_imm(buf, "cpx", opc1);

        break;
    case OPCODE_CPX_ABS:
        instr_size = disas_abs(buf, "cpx", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_CPX_ZP:
        instr_size = disas_zp(buf, "cpx", opc1);

        break;
    case OPCODE_CPY_IMM:
        instr_size = disas_imm(buf, "cpy", opc1);

        break;
    case OPCODE_CPY_ABS:
        instr_size = disas_abs(buf, "cpy", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_CPY_ZP:
        instr_size = disas_zp(buf, "cpy", opc1);

        break;
    case OPCODE_DEC_ABS:
        instr_size = disas_abs(buf, "dec", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_DEC_ABSX:

        instr_size = disas_absx(buf, "dec", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_DEC_ZP:
        instr_size = disas_zp(buf, "dec", opc1);

        break;
    case OPCODE_DEC_ZPX:
        instr_size = disas_zpx(buf, "dec", opc1);

        break;
    case OPCODE_DEX_IMP:
        instr_size = disas_imp(buf, "dex");

        break;
    case OPCODE_DEY_IMP:
        instr_size = disas_imp(buf, "dey");

        break;
    case OPCODE_EOR_IMM:
        instr_size = disas_imm(buf, "eor", opc1);

        break;
    case OPCODE_EOR_ABS:
        instr_size = disas_abs(buf, "eor", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_EOR_ABSX:

        instr_size = disas_absx(buf, "eor", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_EOR_ABSY:

        instr_size = disas_absy(buf, "eor", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_EOR_ZP:
        instr_size = disas_zp(buf, "eor", opc1);

        break;
    case OPCODE_EOR_ZPX:
        instr_size = disas_zpx(buf, "eor", opc1);

        break;
    case OPCODE_EOR_INX:
        instr_size = disas_inx(buf, "eor", opc1);

        break;
    case OPCODE_EOR_INY:
        instr_size = disas_iny(buf, "eor", opc1);

        break;
    case OPCODE_INX_IMP:
        instr_size = disas_imp(buf, "inx");

        break;
    case OPCODE_INY_IMP:
        instr_size = disas_imp(buf, "iny");

        break;
    case OPCODE_INC_ABS:
        instr_size = disas_abs(buf, "inc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_INC_ABSX:

        instr_size = disas_absx(buf, "inc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_INC_ZP:
        instr_size = disas_zp(buf, "inc", opc1);

        break;
    case OPCODE_INC_ZPX:
        instr_size = disas_zpx(buf, "inc", opc1);

        break;
    case OPCODE_JMP_ABS:
        instr_size = disas_abs(buf, "jmp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_JMP_IND:
        instr_size = disas_ind(buf, "jmp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_JSR_ABS:
        instr_size = disas_abs(buf, "jsr", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDA_IMM:
        instr_size = disas_imm(buf, "lda", opc1);

        break;
    case OPCODE_LDA_ABS:
        instr_size = disas_abs(buf, "lda", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDA_ABSX:

        instr_size = disas_absx(buf, "lda", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDA_ABSY:

        instr_size = disas_absy(buf, "lda", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDA_ZP:
        instr_size = disas_zp(buf, "lda", opc1);

        break;
    case OPCODE_LDA_ZPX:
        instr_size = disas_zpx(buf, "lda", opc1);

        break;
    case OPCODE_LDA_INX:
        instr_size = disas_inx(buf, "lda", opc1);

        break;
    case OPCODE_LDA_INY:
        instr_size = disas_iny(buf, "lda", opc1);

        break;
    case OPCODE_LDX_IMM:
        instr_size = disas_imm(buf, "ldx", opc1);

        break;
    case OPCODE_LDX_ABS:
        instr_size = disas_abs(buf, "ldx", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDX_ABSY:

        instr_size = disas_absy(buf, "ldx", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDX_ZP:
        instr_size = disas_zp(buf, "ldx", opc1);

        break;
    case OPCODE_LDX_ZPY:
        instr_size = disas_zpy(buf, "ldx", opc1);

        break;
    case OPCODE_LDY_IMM:
        instr_size = disas_imm(buf, "ldy", opc1);

        break;
    case OPCODE_LDY_ABS:
        instr_size = disas_abs(buf, "ldy", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDY_ABSX:

        instr_size = disas_absx(buf, "ldy", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LDY_ZP:
        instr_size = disas_zp(buf, "ldy", opc1);

        break;
    case OPCODE_LDY_ZPX:
        instr_size = disas_zpx(buf, "ldy", opc1);

        break;
    case OPCODE_LSR_ACC:
        instr_size = disas_acc(buf, "lsr");

        break;
    case OPCODE_LSR_ABS:
        instr_size = disas_abs(buf, "lsr", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LSR_ABSX:
        instr_size = disas_absx(buf, "lsr", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LSR_ZP:
        instr_size = disas_zp(buf, "lsr", opc1);

        break;
    case OPCODE_LSR_ZPX:
        instr_size = disas_zpx(buf, "lsr", opc1);

        break;
    case OPCODE_NOP_IMP:
    case OPCODE_NOP_IMP_2:
    case OPCODE_NOP_IMP_3:
    case OPCODE_NOP_IMP_4:
    case OPCODE_NOP_IMP_5:
    case OPCODE_NOP_IMP_6:
    case OPCODE_NOP_IMP_7:
        instr_size = disas_imp(buf, "nop");

        break;
    case OPCODE_ORA_IMM:
        instr_size = disas_imm(buf, "ora", opc1);

        break;
    case OPCODE_ORA_ABS:
        instr_size = disas_abs(buf, "ora", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ORA_ABSX:
        instr_size = disas_absx(buf, "ora", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ORA_ABSY:
        instr_size = disas_absy(buf, "ora", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ORA_ZP:
        instr_size = disas_zp(buf, "ora", opc1);

        break;
    case OPCODE_ORA_ZPX:
        instr_size = disas_zpx(buf, "ora", opc1);

        break;
    case OPCODE_ORA_INX:
        instr_size = disas_inx(buf, "ora", opc1);

        break;
    case OPCODE_ORA_INY:
        instr_size = disas_iny(buf, "ora", opc1);

        break;
    case OPCODE_PHA_IMP:
        instr_size = disas_imp(buf, "pha");

        break;
    case OPCODE_PHP_IMP:
        instr_size = disas_imp(buf, "php");

        break;
    case OPCODE_PLA_IMP:
        instr_size = disas_imp(buf, "pla");

        break;
    case OPCODE_PLP_IMP:
        instr_size = disas_imp(buf, "plp");

        break;
    case OPCODE_ROL_ACC:
        instr_size = disas_acc(buf, "rol");

        break;
    case OPCODE_ROL_ABS:
        instr_size = disas_abs(buf, "rol", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ROL_ABSX:
        instr_size = disas_absx(buf, "rol", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ROL_ZP:
        instr_size = disas_zp(buf, "rol", opc1);

        break;
    case OPCODE_ROL_ZPX:
        instr_size = disas_zpx(buf, "rol", opc1);

        break;
    case OPCODE_ROR_ACC:
        instr_size = disas_acc(buf, "ror");

        break;
    case OPCODE_ROR_ABS:
        instr_size = disas_abs(buf, "ror", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ROR_ABSX:
        instr_size = disas_absx(buf, "ror", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ROR_ZP:
        instr_size = disas_zp(buf, "ror", opc1);

        break;
    case OPCODE_ROR_ZPX:
        instr_size = disas_zpx(buf, "ror", opc1);

        break;
    case OPCODE_RTI_IMP:
        instr_size = disas_imp(buf, "rti");

        break;
    case OPCODE_RTS_IMP:
        instr_size = disas_imp(buf, "rts");

        break;
    case OPCODE_SBC_IMM:
        instr_size = disas_imm(buf, "sbc", opc1);

        break;
    case OPCODE_SBC_ABS:
        instr_size = disas_abs(buf, "sbc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SBC_ABSX:

        instr_size = disas_absx(buf, "sbc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SBC_ABSY:

        instr_size = disas_absy(buf, "sbc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SBC_ZP:
        instr_size = disas_zp(buf, "sbc", opc1);

        break;
    case OPCODE_SBC_ZPX:
        instr_size = disas_zpx(buf, "sbc", opc1);

        break;
    case OPCODE_SBC_INX:
        instr_size = disas_inx(buf, "sbc", opc1);

        break;
    case OPCODE_SBC_INY:
        instr_size = disas_iny(buf, "sbc", opc1);

        break;
    case OPCODE_SEC_IMP:
        instr_size = disas_imp(buf, "sec");

        break;
    case OPCODE_SED_IMP:
        instr_size = disas_imp(buf, "sed");

        break;
    case OPCODE_SEI_IMP:
        instr_size = disas_imp(buf, "sei");

        break;
    case OPCODE_STA_ABS:
        instr_size = disas_abs(buf, "sta", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_STA_ABSX:
        instr_size = disas_absx(buf, "sta", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_STA_ABSY:
        instr_size = disas_absy(buf, "sta", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_STA_ZP:
        instr_size = disas_zp(buf, "sta", opc1);

        break;
    case OPCODE_STA_ZPX:
        instr_size = disas_zpx(buf, "sta", opc1);

        break;
    case OPCODE_STA_INX:
        instr_size = disas_inx(buf, "sta", opc1);

        break;
    case OPCODE_STA_INY:
        instr_size = disas_iny(buf, "sta", opc1);

        break;
    case OPCODE_STX_ABS:
        instr_size = disas_abs(buf, "stx", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_STX_ZP:
        instr_size = disas_zp(buf, "stx", opc1);

        break;
    case OPCODE_STX_ZPY:
        instr_size = disas_zpy(buf, "stx", opc1);

        break;
    case OPCODE_STY_ABS:
        instr_size = disas_abs(buf, "sty", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_STY_ZP:
        instr_size = disas_zp(buf, "sty", opc1);

        break;
    case OPCODE_STY_ZPX:
        instr_size = disas_zpx(buf, "sty", opc1);

        break;
    case OPCODE_TAX_IMP:
        instr_size = disas_imp(buf, "tax");

        break;
    case OPCODE_TAY_IMP:
        instr_size = disas_imp(buf, "tay");

        break;
    case OPCODE_TSX_IMP:
        instr_size = disas_imp(buf, "tsx");

        break;
    case OPCODE_TXA_IMP:
        instr_size = disas_imp(buf, "txa");

        break;
    case OPCODE_TXS_IMP:
        instr_size = disas_imp(buf, "txs");

        break;
    case OPCODE_TYA_IMP:
        instr_size = disas_imp(buf, "tya");

        break;
    case OPCODE_ALR_IMM:
        instr_size = disas_imm(buf, "alr", opc1);

        break;
    case OPCODE_ANC_IMM:
    case OPCODE_ANC_IMM_2:
        instr_size = disas_imm(buf, "anc", opc1);

        break;
    case OPCODE_ANE_IMM:
        instr_size = disas_imm(buf, "ane", opc1);

        break;
    case OPCODE_ARR_IMM:
        instr_size = disas_imm(buf, "arr", opc1);

        break;
    case OPCODE_AXS_IMM:
        instr_size = disas_imm(buf, "axs", opc1);

        break;
    case OPCODE_LAX_IMM:
        instr_size = disas_imm(buf, "lax", opc1);

        break;
    case OPCODE_LAX_ABS:
        instr_size = disas_abs(buf, "lax", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LAX_ABSY:
        instr_size = disas_absy(buf, "lax", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_LAX_ZP:
        instr_size = disas_zp(buf, "lax", opc1);

        break;
    case OPCODE_LAX_ZPY:
        instr_size = disas_zpy(buf, "lax", opc1);

        break;
    case OPCODE_LAX_INX:
        instr_size = disas_inx(buf, "lax", opc1);

        break;
    case OPCODE_LAX_INY:
        instr_size = disas_iny(buf, "lax", opc1);

        break;
    case OPCODE_SAX_ABS:
        instr_size = disas_abs(buf, "sax", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SAX_ZP:
        instr_size = disas_zp(buf, "sax", opc1);

        break;
    case OPCODE_SAX_ZPY:
        instr_size = disas_zpy(buf, "sax", opc1);

        break;
    case OPCODE_SAX_INX:
        instr_size = disas_inx(buf, "sax", opc1);

        break;
    case OPCODE_DCP_ABS:
        instr_size = disas_abs(buf, "dcp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_DCP_ABSX:
        instr_size = disas_absx(buf, "dcp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_DCP_ABSY:
        instr_size = disas_absy(buf, "dcp", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_DCP_ZP:
        instr_size = disas_zp(buf, "dcp", opc1);

        break;
    case OPCODE_DCP_ZPX:
        instr_size = disas_zpy(buf, "dcp", opc1);

        break;
    case OPCODE_DCP_INX:
        instr_size = disas_inx(buf, "dcp", opc1);

        break;
    case OPCODE_DCP_INY:
        instr_size = disas_iny(buf, "dcp", opc1);

        break;
    case OPCODE_ISC_ABS:
        instr_size = disas_abs(buf, "isc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ISC_ABSX:
        instr_size = disas_absx(buf, "isc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ISC_ABSY:
        instr_size = disas_absy(buf, "isc", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_ISC_ZP:
        instr_size = disas_zp(buf, "isc", opc1);

        break;
    case OPCODE_ISC_ZPX:
        instr_size = disas_zpy(buf, "isc", opc1);

        break;
    case OPCODE_ISC_INX:
        instr_size = disas_inx(buf, "isc", opc1);

        break;
    case OPCODE_ISC_INY:
        instr_size = disas_iny(buf, "isc", opc1);

        break;
    case OPCODE_RLA_ABS:
        instr_size = disas_abs(buf, "rla", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_RLA_ABSX:
        instr_size = disas_absx(buf, "rla", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_RLA_ABSY:
        instr_size = disas_absy(buf, "rla", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_RLA_ZP:
        instr_size = disas_zp(buf, "rla", opc1);

        break;
    case OPCODE_RLA_ZPX:
        instr_size = disas_zpy(buf, "rla", opc1);

        break;
    case OPCODE_RLA_INX:
        instr_size = disas_inx(buf, "rla", opc1);

        break;
    case OPCODE_RLA_INY:
        instr_size = disas_iny(buf, "rla", opc1);

        break;
    case OPCODE_RRA_ABS:
        instr_size = disas_abs(buf, "rra", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_RRA_ABSX:
        instr_size = disas_absx(buf, "rra", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_RRA_ABSY:
        instr_size = disas_absy(buf, "rra", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_RRA_ZP:
        instr_size = disas_zp(buf, "rra", opc1);

        break;
    case OPCODE_RRA_ZPX:
        instr_size = disas_zpy(buf, "rra", opc1);

        break;
    case OPCODE_RRA_INX:
        instr_size = disas_inx(buf, "rra", opc1);

        break;
    case OPCODE_RRA_INY:
        instr_size = disas_iny(buf, "rra", opc1);

        break;
    case OPCODE_SLO_ABS:
        instr_size = disas_abs(buf, "slo", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SLO_ABSX:
        instr_size = disas_absx(buf, "slo", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SLO_ABSY:
        instr_size = disas_absy(buf, "slo", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SLO_ZP:
        instr_size = disas_zp(buf, "slo", opc1);

        break;
    case OPCODE_SLO_ZPX:
        instr_size = disas_zpy(buf, "slo", opc1);

        break;
    case OPCODE_SLO_INX:
        instr_size = disas_inx(buf, "slo", opc1);

        break;
    case OPCODE_SLO_INY:
        instr_size = disas_iny(buf, "slo", opc1);

        break;
    case OPCODE_SRE_ABS:
        instr_size = disas_abs(buf, "sre", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SRE_ABSX:
        instr_size = disas_absx(buf, "sre", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SRE_ABSY:
        instr_size = disas_absy(buf, "sre", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SRE_ZP:
        instr_size = disas_zp(buf, "sre", opc1);

        break;
    case OPCODE_SRE_ZPX:
        instr_size = disas_zpy(buf, "sre", opc1);

        break;
    case OPCODE_SRE_INX:
        instr_size = disas_inx(buf, "sre", opc1);

        break;
    case OPCODE_SRE_INY:
        instr_size = disas_iny(buf, "sre", opc1);

        break;
    case OPCODE_SHA_ABSY:
        instr_size = disas_absy(buf, "sha", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SHA_INY:
        instr_size = disas_iny(buf, "sha", opc1);

        break;
    case OPCODE_SHX_ABSY:
        instr_size = disas_absy(buf, "shx", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SHY_ABSX:
        instr_size = disas_absx(buf, "shy", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_TAS_ABSY:
        instr_size = disas_absy(buf, "tas", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_USB_IMM:
        instr_size = disas_imm(buf, "usb", opc1);

        break;
    case OPCODE_LAS_ABSY:
        instr_size = disas_absy(buf, "las", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_SKB_IMM:
    case OPCODE_SKB_IMM_2:
    case OPCODE_SKB_IMM_3:
    case OPCODE_SKB_IMM_4:
    case OPCODE_SKB_IMM_5:
        instr_size = disas_imm(buf, "skb", opc1);

        break;
    case OPCODE_SKB_ZP:
    case OPCODE_SKB_ZP_2:
    case OPCODE_SKB_ZP_3:
        instr_size = disas_zp(buf, "skb", opc1);

        break;
    case OPCODE_SKB_ZPX:
    case OPCODE_SKB_ZPX_2:
    case OPCODE_SKB_ZPX_3:
    case OPCODE_SKB_ZPX_4:
    case OPCODE_SKB_ZPX_5:
    case OPCODE_SKB_ZPX_6:
        instr_size = disas_zpx(buf, "skb", opc1);

        break;
    case OPCODE_IGN_ABS:
        instr_size = disas_abs(buf, "ign", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_IGN_ABSX:
    case OPCODE_IGN_ABSX_2:
    case OPCODE_IGN_ABSX_3:
    case OPCODE_IGN_ABSX_4:
    case OPCODE_IGN_ABSX_5:
    case OPCODE_IGN_ABSX_6:
        instr_size = disas_absx(buf, "ign", U16_COMBINE(opc1, opc2));

        break;
    case OPCODE_HLT_IMP:
    case OPCODE_HLT_IMP_2:
    case OPCODE_HLT_IMP_3:
    case OPCODE_HLT_IMP_4:
    case OPCODE_HLT_IMP_5:
    case OPCODE_HLT_IMP_6:
    case OPCODE_HLT_IMP_7:
    case OPCODE_HLT_IMP_8:
    case OPCODE_HLT_IMP_9:
    case OPCODE_HLT_IMP_10:
    case OPCODE_HLT_IMP_11:
    case OPCODE_HLT_IMP_12:
        instr_size = disas_imp(buf, "hlt");

        break;
    default:
        strcpy(buf, "<unknown>");
        instr_size = 1;

        break;
    }

    return instr_size;
}

#ifdef __cplusplus
}
#endif
