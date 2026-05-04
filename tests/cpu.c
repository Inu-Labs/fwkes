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

#include <unity.h>

#include <fwkes/bits.h>
#include <fwkes/bus.h>
#include <fwkes/cpu.h>

#include <string.h>

static Cpu g_cpu;
static Bus g_bus;
static uint8_t g_memory[MEMORY_SIZE];

#define RESET_VECTOR_LB 0xfffc
#define RESET_VECTOR_HB 0xfffd
#define PROG_ADDR 0x8000

void setUp() {
    memset(&g_cpu, 0, sizeof(g_cpu));
    memset(&g_bus, 0, sizeof(g_bus));
    memset(&g_memory, 0, sizeof(g_memory));

    g_memory[RESET_VECTOR_LB] = PROG_ADDR & 0xff;
    g_memory[RESET_VECTOR_HB] = (uint8_t) (PROG_ADDR >> 8);

    g_bus.memory = g_memory;
    g_cpu.bus = &g_bus;

    cpu_init(&g_cpu, &g_bus);
}

void tearDown() {}

void test_reset_vector(void) {
    /* reset vector is set in setUp() */

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR, g_cpu.PC);
}

void test_fetch_byte(void) {
    uint16_t data = cpu_fetch_byte(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT8(PROG_ADDR, data);
}

void test_fetch_word(void) {
    g_memory[PROG_ADDR] = 0x7f;
    g_memory[PROG_ADDR + 1] = 0x94;

    uint16_t data = cpu_fetch_word(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x947f, data);
}

void test_fetch_abs(void) {
    g_memory[PROG_ADDR] = 0x7f;
    g_memory[PROG_ADDR + 1] = 0x94;

    uint16_t data = cpu_fetch_abs(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x947f, data);
}

void test_fetch_zp(void) {
    g_memory[PROG_ADDR] = 0x7f;

    uint16_t data = cpu_fetch_zp(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x7f, data);
}

void test_fetch_abs_x(void) {
    g_cpu.X = 129;
    g_memory[PROG_ADDR] = 0x7f;
    g_memory[PROG_ADDR + 1] = 0x94;

    uint16_t data = cpu_fetch_abs_x(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x9500, data);
}

void test_fetch_abs_y(void) {
    g_cpu.Y = 129;
    g_memory[PROG_ADDR] = 0x7f;
    g_memory[PROG_ADDR + 1] = 0x94;

    uint16_t data = cpu_fetch_abs_y(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x9500, data);
}

void test_fetch_abs_x_p(void) {
    g_cpu.X = 129;
    g_memory[PROG_ADDR] = 0x7f;
    g_memory[PROG_ADDR + 1] = 0x94;

    uint16_t data = cpu_fetch_abs_x_p(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(9, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT16(0x9500, data);
}

void test_fetch_abs_y_p(void) {
    g_cpu.Y = 129;
    g_memory[PROG_ADDR] = 0x7f;
    g_memory[PROG_ADDR + 1] = 0x94;
    g_memory[0x9500] = 115;

    uint16_t data = cpu_fetch_abs_y_p(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(9, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX16(0x9500, data);
}

void test_fetch_indirect_abs(void) {
    g_memory[PROG_ADDR] = 0x01;
    g_memory[PROG_ADDR + 1] = 0x02;
    g_memory[0x01] = 0xfc;
    g_memory[0x02] = 0xba;

    uint16_t data = cpu_fetch_indirect_abs(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0xbafc, data);
}

void test_fetch_zp_x(void) {
    g_cpu.X = 5;
    g_memory[PROG_ADDR] = 0x0f;

    uint16_t data = cpu_fetch_zp_x(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x14, data);
}

void test_fetch_zp_y(void) {
    g_cpu.Y = 5;
    g_memory[PROG_ADDR] = 0x0f;

    uint16_t data = cpu_fetch_zp_y(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x14, data);
}

void test_fetch_indexed_indirect(void) {
    g_cpu.X = 4;
    g_memory[PROG_ADDR] = 0x0020;
    g_memory[PROG_ADDR + 1] = 0x0021;
    g_memory[0x0024] = 0x10;
    g_memory[0x0025] = 0x80;

    uint16_t data = cpu_fetch_indexed_indirect(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x8010, data);
}

void test_fetch_indexed_indirect_overflow(void) {
    g_cpu.X = 4;
    g_memory[PROG_ADDR] = 0x00ff;
    g_memory[PROG_ADDR + 1] = 0x0004;
    g_memory[0x0003] = 0x10;
    g_memory[0x0004] = 0x80;

    uint16_t data = cpu_fetch_indexed_indirect(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x8010, data);
}

void test_fetch_indirect_indexed(void) {
    g_cpu.Y = 4;
    g_memory[PROG_ADDR] = 0x0020;
    g_memory[PROG_ADDR + 1] = 0x0021;
    g_memory[0x0020] = 0x30;
    g_memory[0x0021] = 0x40;

    uint16_t data = cpu_fetch_indirect_indexed(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_HEX16(0x4034, data);
}

void test_fetch_indirect_indexed_p(void) {
    g_cpu.Y = 0xd0;
    g_memory[PROG_ADDR] = 0x20;
    g_memory[PROG_ADDR + 1] = 0x21;
    g_memory[0x0020] = 0x30;
    g_memory[0x0021] = 0x40;

    uint16_t data = cpu_fetch_indirect_indexed_p(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(9, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX16(0x4100, data);
}

void test_fetch_relative_positive(void) {
    g_memory[PROG_ADDR] = 0x40;

    int8_t offset = (int8_t) bus_read(g_cpu.bus, cpu_fetch_relative(&g_cpu));

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(8, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_INT8(0x40, offset);
}

void test_fetch_relative_negative(void) {
    g_memory[PROG_ADDR] = 0x88;

    int8_t offset = (int8_t) bus_read(g_cpu.bus, cpu_fetch_relative(&g_cpu));

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(8, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_INT8(-120, offset);
}

void test_adc(void) {
    g_cpu.A = 5;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 115;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(120, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_adc_carry(void) {
    g_cpu.A = 255;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 6;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(5, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_adc_carry_overflow(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_C);

    g_cpu.A = 5;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 255;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(5, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_adc_zero(void) {
    g_cpu.A = 250;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 6;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(0, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C | CPU_FLAG_Z, g_cpu.P
    );
}

void test_adc_neg_positive(void) {
    g_cpu.A = 120;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 8;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(128, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_V | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P
    );
}

void test_adc_neg_reset(void) {
    g_cpu.A = 255;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 1;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(0, g_cpu.A);
    TEST_ASSERT_FALSE(BIT_CHECK(g_cpu.P, CPU_FLAG_N));
}

void test_adc_signed_overflow_positive(void) {
    g_cpu.A = 120;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 8;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(128, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_V | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P
    );
}

void test_adc_signed_overflow_negative(void) {
    g_cpu.A = 128;
    g_memory[PROG_ADDR] = OPCODE_ADC_IMM;
    g_memory[PROG_ADDR + 1] = 128;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(0, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_V | CPU_FLAG_U | CPU_FLAG_Z | CPU_FLAG_C, g_cpu.P
    );
}

void test_and(void) {
    g_cpu.A = 0xae;
    g_memory[PROG_ADDR] = OPCODE_AND_IMM;
    g_memory[PROG_ADDR + 1] = 0xdd;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x8c, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_and_zero(void) {
    g_cpu.A = 0x0f;
    g_memory[PROG_ADDR] = OPCODE_AND_IMM;
    g_memory[PROG_ADDR + 1] = 0xf0;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_and_neg_set(void) {
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_AND_IMM;
    g_memory[PROG_ADDR + 1] = 0x80;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_and_neg_reset(void) {
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_AND_IMM;
    g_memory[PROG_ADDR + 1] = 0x70;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_asl(void) {
    g_cpu.A = 0xc9;
    g_memory[PROG_ADDR] = OPCODE_ASL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x92, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_asl_zero(void) {
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_ASL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C | CPU_FLAG_Z, g_cpu.P
    );
}

void test_asl_carry_reset(void) {
    g_cpu.A = 0x40;
    g_memory[PROG_ADDR] = OPCODE_ASL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_asl_memory(void) {
    g_memory[PROG_ADDR] = OPCODE_ASL_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x40;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_bcc_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BCC_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_bcc_not_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_BCC_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_bcs_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_BCS_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_bcs_not_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BCS_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_beq_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_BEQ_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_beq_not_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BEQ_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_bit_neg_set(void) {
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_BIT_ZP;
    g_memory[PROG_ADDR + 1] = 0x0f;
    g_memory[0x0f] = 0x80;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_bit_zero_set(void) {
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_BIT_ZP;
    g_memory[PROG_ADDR + 1] = 0x0f;
    g_memory[0x0f] = 0x4f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_bit_overflow(void) {
    g_cpu.A = 0x40;
    g_memory[PROG_ADDR] = OPCODE_BIT_ZP;
    g_memory[PROG_ADDR + 1] = 0x0f;
    g_memory[0x0f] = 0x40;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_V, g_cpu.P);
}

void test_bmi_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_BMI_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_bmi_not_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BMI_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_bne_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BNE_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_bne_not_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_BNE_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_bpl_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BPL_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_bpl_not_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_BPL_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_brk(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N | CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_BRK_IMP;
    g_memory[0x0100 + g_cpu.S] = 0x15;
    g_memory[0x0100 + g_cpu.S - 1] = 0xff;
    g_memory[0x0100 + g_cpu.S - 2] =
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C;
    g_memory[0xfffe] = 0x15;
    g_memory[0xffff] = 0xff;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0xff15, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(15, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xfa, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_B | CPU_FLAG_N | CPU_FLAG_Z,
        g_memory[0x0100 + g_cpu.S + 1]
    );
    TEST_ASSERT_EQUAL_HEX8(0x80, g_memory[0x0100 + g_cpu.S + 2]);
    TEST_ASSERT_EQUAL_HEX8(0x02, g_memory[0x0100 + g_cpu.S + 3]);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_Z, g_cpu.P
    );
}

void test_bvc_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BVC_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_bvc_not_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_V);
    g_memory[PROG_ADDR] = OPCODE_BVC_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_V, g_cpu.P);
}

void test_bvs_taken(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_V);
    g_memory[PROG_ADDR] = OPCODE_BVS_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0x8017, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_V, g_cpu.P);
}

void test_bvs_not_taken(void) {
    g_memory[PROG_ADDR] = OPCODE_BVS_REL;
    g_memory[PROG_ADDR + 1] = 0x15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_clc(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_CLC_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_cld(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_D);
    g_memory[PROG_ADDR] = OPCODE_CLD_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_cli(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_I);
    g_memory[PROG_ADDR] = OPCODE_CLI_IMP;
    g_memory[PROG_ADDR + 1] = OPCODE_NOP_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_U, g_cpu.P);
}

void test_clv(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_V);
    g_memory[PROG_ADDR] = OPCODE_CLV_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_cmp_zero_set(void) {
    g_cpu.A = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_CMP_IMM;
    g_memory[PROG_ADDR + 1] = 0x7f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z | CPU_FLAG_C, g_cpu.P
    );
}

void test_cmp_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.A = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_CMP_IMM;
    g_memory[PROG_ADDR + 1] = 0x7e;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_cmp_neg_set(void) {
    g_cpu.A = 0xc8;
    g_memory[PROG_ADDR] = OPCODE_CMP_IMM;
    g_memory[PROG_ADDR + 1] = 0x32;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_cmp_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.A = 0xc8;
    g_memory[PROG_ADDR] = OPCODE_CMP_IMM;
    g_memory[PROG_ADDR + 1] = 0x64;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_cmp_carry_set(void) {
    g_cpu.A = 0xf0;
    g_memory[PROG_ADDR] = OPCODE_CMP_IMM;
    g_memory[PROG_ADDR + 1] = 0x0f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_cmp_carry_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_cpu.A = 0x0f;
    g_memory[PROG_ADDR] = OPCODE_CMP_IMM;
    g_memory[PROG_ADDR + 1] = 0xf0;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_cpx_zero_set(void) {
    g_cpu.X = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_CPX_IMM;
    g_memory[PROG_ADDR + 1] = 0x7f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z | CPU_FLAG_C, g_cpu.P
    );
}

void test_cpx_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.X = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_CPX_IMM;
    g_memory[PROG_ADDR + 1] = 0x7e;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_cpx_neg_set(void) {
    g_cpu.X = 0xc8;
    g_memory[PROG_ADDR] = OPCODE_CPX_IMM;
    g_memory[PROG_ADDR + 1] = 0x32;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_cpx_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.X = 0xc8;
    g_memory[PROG_ADDR] = OPCODE_CPX_IMM;
    g_memory[PROG_ADDR + 1] = 0x64;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_cpx_carry_set(void) {
    g_cpu.X = 0xf0;
    g_memory[PROG_ADDR] = OPCODE_CPX_IMM;
    g_memory[PROG_ADDR + 1] = 0x0f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_cpx_carry_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_cpu.X = 0x0f;
    g_memory[PROG_ADDR] = OPCODE_CPX_IMM;
    g_memory[PROG_ADDR + 1] = 0xf0;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_cpy_zero_set(void) {
    g_cpu.Y = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_CPY_IMM;
    g_memory[PROG_ADDR + 1] = 0x7f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z | CPU_FLAG_C, g_cpu.P
    );
}

void test_cpy_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.Y = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_CPY_IMM;
    g_memory[PROG_ADDR + 1] = 0x7e;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_cpy_neg_set(void) {
    g_cpu.Y = 0xc8;
    g_memory[PROG_ADDR] = OPCODE_CPY_IMM;
    g_memory[PROG_ADDR + 1] = 0x32;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_cpy_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.Y = 0xc8;
    g_memory[PROG_ADDR] = OPCODE_CPY_IMM;
    g_memory[PROG_ADDR + 1] = 0x64;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_cpy_carry_set(void) {
    g_cpu.Y = 0xf0;
    g_memory[PROG_ADDR] = OPCODE_CPY_IMM;
    g_memory[PROG_ADDR + 1] = 0x0f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_cpy_carry_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_cpu.Y = 0x0f;
    g_memory[PROG_ADDR] = OPCODE_CPY_IMM;
    g_memory[PROG_ADDR + 1] = 0xf0;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_dec_neg_set(void) {
    g_memory[PROG_ADDR] = OPCODE_DEC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x81;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_dec_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_DEC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x80;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7f, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_dec_zero_set(void) {
    g_memory[PROG_ADDR] = OPCODE_DEC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x01;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_dec_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_DEC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xff, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_dex_neg_set(void) {
    g_cpu.X = 0x81;
    g_memory[PROG_ADDR] = OPCODE_DEX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_dex_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.X = 0x80;
    g_memory[PROG_ADDR] = OPCODE_DEX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7f, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_dex_zero_set(void) {
    g_cpu.X = 0x01;
    g_memory[PROG_ADDR] = OPCODE_DEX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_dex_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.X = 0x00;
    g_memory[PROG_ADDR] = OPCODE_DEX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xff, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_dey_neg_set(void) {
    g_cpu.Y = 0x81;
    g_memory[PROG_ADDR] = OPCODE_DEY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_dey_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.Y = 0x80;
    g_memory[PROG_ADDR] = OPCODE_DEY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7f, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_dey_zero_set(void) {
    g_cpu.Y = 0x01;
    g_memory[PROG_ADDR] = OPCODE_DEY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_dey_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.Y = 0x00;
    g_memory[PROG_ADDR] = OPCODE_DEY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xff, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_eor_zero_set(void) {
    g_cpu.A = 0xff;
    g_memory[PROG_ADDR] = OPCODE_EOR_IMM;
    g_memory[PROG_ADDR + 1] = 0xff;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_eor_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.A = 0xfe;
    g_memory[PROG_ADDR] = OPCODE_EOR_IMM;
    g_memory[PROG_ADDR + 1] = 0xff;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_eor_neg_set(void) {
    g_cpu.A = 0xdd;
    g_memory[PROG_ADDR] = OPCODE_EOR_IMM;
    g_memory[PROG_ADDR + 1] = 0x7a;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xa7, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_eor_neg_reset(void) {
    g_cpu.A = 0xdd;
    g_memory[PROG_ADDR] = OPCODE_EOR_IMM;
    g_memory[PROG_ADDR + 1] = 0xfa;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x27, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_inc_neg_set(void) {
    g_memory[PROG_ADDR] = OPCODE_INC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x7f;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_inc_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_INC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x40;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x41, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_inc_zero_set(void) {
    g_memory[PROG_ADDR] = OPCODE_INC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0xff;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_inc_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_INC_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;
    g_memory[0xff15] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_memory[0xff15]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_inx_neg_set(void) {
    g_cpu.X = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_INX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_inx_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.X = 0x40;
    g_memory[PROG_ADDR] = OPCODE_INX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x41, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_inx_zero_set(void) {
    g_cpu.X = 0xff;
    g_memory[PROG_ADDR] = OPCODE_INX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_inx_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.X = 0x00;
    g_memory[PROG_ADDR] = OPCODE_INX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_iny_neg_set(void) {
    g_cpu.Y = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_INY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_iny_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.Y = 0x40;
    g_memory[PROG_ADDR] = OPCODE_INY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x41, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_iny_zero_set(void) {
    g_cpu.Y = 0xff;
    g_memory[PROG_ADDR] = OPCODE_INY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_iny_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.Y = 0x00;
    g_memory[PROG_ADDR] = OPCODE_INY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_jmp(void) {
    g_memory[PROG_ADDR] = OPCODE_JMP_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0xff15, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_jsr(void) {
    g_memory[PROG_ADDR] = OPCODE_JSR_ABS;
    g_memory[PROG_ADDR + 1] = 0x15;
    g_memory[PROG_ADDR + 2] = 0xff;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0xff15, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_memory[0x0100 + g_cpu.S + 1]);
    TEST_ASSERT_EQUAL_HEX8(0x02, g_memory[0x0100 + g_cpu.S + 2]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_lda_neg_set(void) {
    g_memory[PROG_ADDR] = OPCODE_LDA_IMM;
    g_memory[PROG_ADDR + 1] = 0x80;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_lda_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_LDA_IMM;
    g_memory[PROG_ADDR + 1] = 0x40;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x40, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_lda_zero_set(void) {
    g_cpu.A = 0xff;
    g_memory[PROG_ADDR] = OPCODE_LDA_IMM;
    g_memory[PROG_ADDR + 1] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_lda_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_LDA_IMM;
    g_memory[PROG_ADDR + 1] = 0x01;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ldx_neg_set(void) {
    g_memory[PROG_ADDR] = OPCODE_LDX_IMM;
    g_memory[PROG_ADDR + 1] = 0x80;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_ldx_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_LDX_IMM;
    g_memory[PROG_ADDR + 1] = 0x40;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x40, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ldx_zero_set(void) {
    g_cpu.X = 0xff;
    g_memory[PROG_ADDR] = OPCODE_LDX_IMM;
    g_memory[PROG_ADDR + 1] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_ldx_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_LDX_IMM;
    g_memory[PROG_ADDR + 1] = 0x01;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ldy_neg_set(void) {
    g_memory[PROG_ADDR] = OPCODE_LDY_IMM;
    g_memory[PROG_ADDR + 1] = 0x80;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_ldy_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_LDY_IMM;
    g_memory[PROG_ADDR + 1] = 0x40;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x40, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ldy_zero_set(void) {
    g_cpu.Y = 0xff;
    g_memory[PROG_ADDR] = OPCODE_LDY_IMM;
    g_memory[PROG_ADDR + 1] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_ldy_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_LDY_IMM;
    g_memory[PROG_ADDR + 1] = 0x01;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_lsr_zero_set(void) {
    g_cpu.A = 0x01;
    g_memory[PROG_ADDR] = OPCODE_LSR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z | CPU_FLAG_C, g_cpu.P
    );
}

void test_lsr_zero_reset(void) {
    g_cpu.A = 0x02;
    g_memory[PROG_ADDR] = OPCODE_LSR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_nop(void) {
    g_memory[PROG_ADDR] = OPCODE_NOP_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ora_zero_set(void) {
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_ORA_IMM;
    g_memory[PROG_ADDR + 1] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_ora_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_ORA_IMM;
    g_memory[PROG_ADDR + 1] = 0x73;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x73, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ora_neg_set(void) {
    g_cpu.A = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_ORA_IMM;
    g_memory[PROG_ADDR + 1] = 0x80;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xff, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_ora_neg_reset(void) {
    g_cpu.A = 0x7f;
    g_memory[PROG_ADDR] = OPCODE_ORA_IMM;
    g_memory[PROG_ADDR + 1] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7f, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_pha(void) {
    g_cpu.A = 0x3d;
    g_memory[PROG_ADDR] = OPCODE_PHA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x3d, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x3d, g_memory[0x0100 + g_cpu.S + 1]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_php(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_D);
    BIT_SET(g_cpu.P, CPU_FLAG_I);
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_PHP_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(11, g_cpu.clock_cycles);

    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_I | CPU_FLAG_C |
            CPU_FLAG_B | CPU_FLAG_U,
        g_memory[0x0100 + g_cpu.S + 1]
    );

    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_I | CPU_FLAG_C, g_cpu.P
    );
}

void test_pla_zero_set(void) {
    g_cpu.A = 0x3d;
    g_memory[PROG_ADDR] = OPCODE_PLA_IMP;
    g_memory[0x0100 + g_cpu.S] = 0x00;
    --g_cpu.S;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_pla_zero_reset(void) {
    g_cpu.A = 0x00;
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_PLA_IMP;
    g_memory[0x0100 + g_cpu.S] = 0x01;
    --g_cpu.S;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_pla_neg_set(void) {
    g_cpu.A = 0x3d;
    g_memory[PROG_ADDR] = OPCODE_PLA_IMP;
    g_memory[0x0100 + g_cpu.S] = 0x80;
    --g_cpu.S;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_pla_neg_reset(void) {
    g_cpu.A = 0x80;
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_PLA_IMP;
    g_memory[0x0100 + g_cpu.S] = 0x01;
    --g_cpu.S;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_plp(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N | CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_PLP_IMP;
    g_memory[0x0100 + g_cpu.S] =
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C;
    --g_cpu.S;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C, g_cpu.P
    );
}

void test_plp_ub_are_ignored(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N | CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_PLP_IMP;
    g_memory[0x0100 + g_cpu.S] =
        CPU_FLAG_I | CPU_FLAG_D | CPU_FLAG_C | CPU_FLAG_B;
    --g_cpu.S;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C, g_cpu.P
    );
}

void test_plp_i_is_delayed(void) {
    BIT_CLEAR(g_cpu.P, CPU_FLAG_I);
    g_memory[PROG_ADDR] = OPCODE_PLP_IMP;
    g_memory[PROG_ADDR + 1] = OPCODE_NOP_IMP;
    g_memory[0x0100 + g_cpu.S] =
        CPU_FLAG_I | CPU_FLAG_D | CPU_FLAG_C | CPU_FLAG_B;
    --g_cpu.S;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C, g_cpu.P);

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C, g_cpu.P
    );
}

void test_rol_zero_set(void) {
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_ROL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_rol_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.A = 0x01;
    g_memory[PROG_ADDR] = OPCODE_ROL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x02, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_rol_neg_set(void) {
    g_cpu.A = 0x40;
    g_memory[PROG_ADDR] = OPCODE_ROL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_rol_neg_reset(void) {
    g_cpu.A = 0x80;
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_ROL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C | CPU_FLAG_Z, g_cpu.P
    );
}

void test_rol_carry_set(void) {
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_ROL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z | CPU_FLAG_C, g_cpu.P
    );
}

void test_rol_carry_copy(void) {
    g_cpu.A = 0x02;
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_ROL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x05, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_rol_carry_copy_and_set(void) {
    g_cpu.A = 0x80;
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_ROL_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_ror_zero_set(void) {
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_ROR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_ror_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.A = 0x04;
    g_memory[PROG_ADDR] = OPCODE_ROR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x02, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ror_neg_set(void) {
    g_cpu.A = 0x40;
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_ROR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xa0, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_ror_neg_reset(void) {
    g_cpu.A = 0x80;
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_memory[PROG_ADDR] = OPCODE_ROR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x40, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_ror_carry_set(void) {
    g_cpu.A = 0x81;
    g_memory[PROG_ADDR] = OPCODE_ROR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x40, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_ror_carry_copy(void) {
    g_cpu.A = 0x02;
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_ROR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x81, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_ror_carry_copy_and_set(void) {
    g_cpu.A = 0x03;
    BIT_SET(g_cpu.P, CPU_FLAG_C);
    g_memory[PROG_ADDR] = OPCODE_ROR_ACC;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x81, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_rti(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N | CPU_FLAG_Z);
    g_memory[PROG_ADDR] = OPCODE_RTI_IMP;
    g_memory[0x0100 + g_cpu.S] = 0x15;
    g_memory[0x0100 + g_cpu.S - 1] = 0xff;
    g_memory[0x0100 + g_cpu.S - 2] =
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C;
    g_cpu.S -= 3;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0xff15, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D | CPU_FLAG_C, g_cpu.P
    );
}

void test_rts(void) {
    g_memory[PROG_ADDR] = OPCODE_RTS_IMP;
    g_memory[0x0100 + g_cpu.S] = 0x15;
    g_memory[0x0100 + g_cpu.S - 1] = 0xff;
    g_cpu.S -= 2;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(0xff16, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(14, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0xfd, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_sbc_zero_set(void) {
    g_cpu.A = 21;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 20;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(0, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z | CPU_FLAG_C, g_cpu.P
    );
}

void test_sbc_carry_set(void) {
    g_cpu.A = 115;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 20;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(94, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_sbc_carry_reset(void) {
    g_cpu.A = 10;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 255;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(10, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_sbc_carry_underflow(void) {
    g_cpu.A = 5;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 255;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(5, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_sbc_neg_set(void) {
    g_cpu.A = 140;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 10;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(129, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N | CPU_FLAG_C, g_cpu.P
    );
}

void test_sbc_neg_reset(void) {
    g_cpu.A = 128;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 1;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(126, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_sbc_signed_underflow_positive(void) {
    g_cpu.A = 10;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 15;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(250, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_V | CPU_FLAG_N, g_cpu.P
    );
}

void test_sbc_signed_underflow_negative(void) {
    g_cpu.A = 230;
    g_memory[PROG_ADDR] = OPCODE_SBC_IMM;
    g_memory[PROG_ADDR + 1] = 200;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_UINT8(29, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(
        CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_V | CPU_FLAG_C, g_cpu.P
    );
}

void test_sec(void) {
    g_memory[PROG_ADDR] = OPCODE_SEC_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_C, g_cpu.P);
}

void test_sed(void) {
    g_memory[PROG_ADDR] = OPCODE_SED_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_D, g_cpu.P);
}

void test_sei(void) {
    g_memory[PROG_ADDR] = OPCODE_SEI_IMP;
    g_memory[PROG_ADDR + 1] = OPCODE_NOP_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 2, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_sta(void) {
    g_cpu.A = 0x7e;
    g_memory[PROG_ADDR] = OPCODE_STA_ABS;
    g_memory[PROG_ADDR + 1] = 0xfa;
    g_memory[PROG_ADDR + 2] = 0xde;
    g_memory[0xdefa] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_memory[0xdefa]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_stx(void) {
    g_cpu.X = 0x7e;
    g_memory[PROG_ADDR] = OPCODE_STX_ABS;
    g_memory[PROG_ADDR + 1] = 0xfa;
    g_memory[PROG_ADDR + 2] = 0xde;
    g_memory[0xdefa] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_memory[0xdefa]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_sty(void) {
    g_cpu.Y = 0x7e;
    g_memory[PROG_ADDR] = OPCODE_STY_ABS;
    g_memory[PROG_ADDR + 1] = 0xfa;
    g_memory[PROG_ADDR + 2] = 0xde;
    g_memory[0xdefa] = 0x00;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 3, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(12, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_memory[0xdefa]);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tax_zero_set(void) {
    g_cpu.A = 0x00;
    g_cpu.X = 0xff;
    g_memory[PROG_ADDR] = OPCODE_TAX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_tax_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.A = 0x7e;
    g_cpu.X = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TAX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tax_neg_set(void) {
    g_cpu.A = 0x80;
    g_cpu.X = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TAX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_tax_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.A = 0x01;
    g_cpu.X = 0x80;
    g_memory[PROG_ADDR] = OPCODE_TAX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tay_zero_set(void) {
    g_cpu.A = 0x00;
    g_cpu.Y = 0xff;
    g_memory[PROG_ADDR] = OPCODE_TAY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_tay_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.A = 0x7e;
    g_cpu.Y = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TAY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tay_neg_set(void) {
    g_cpu.A = 0x80;
    g_cpu.Y = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TAY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_tay_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.A = 0x01;
    g_cpu.Y = 0x80;
    g_memory[PROG_ADDR] = OPCODE_TAY_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tsx_zero_set(void) {
    g_cpu.S = 0x00;
    g_cpu.X = 0xff;
    g_memory[PROG_ADDR] = OPCODE_TSX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_tsx_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.S = 0x7e;
    g_cpu.X = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TSX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tsx_neg_set(void) {
    g_cpu.S = 0x80;
    g_cpu.X = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TSX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_tsx_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.S = 0x01;
    g_cpu.X = 0x80;
    g_memory[PROG_ADDR] = OPCODE_TSX_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_txa_zero_set(void) {
    g_cpu.X = 0x00;
    g_cpu.A = 0xff;
    g_memory[PROG_ADDR] = OPCODE_TXA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_txa_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.X = 0x7e;
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TXA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_txa_neg_set(void) {
    g_cpu.X = 0x80;
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TXA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_txa_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.X = 0x01;
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_TXA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_txs(void) {
    g_cpu.X = 0x00;
    g_cpu.S = 0xff;
    g_memory[PROG_ADDR] = OPCODE_TXS_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.S);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.X);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tya_zero_set(void) {
    g_cpu.Y = 0x00;
    g_cpu.A = 0xff;
    g_memory[PROG_ADDR] = OPCODE_TYA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x00, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_Z, g_cpu.P);
}

void test_tya_zero_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_Z);
    g_cpu.Y = 0x7e;
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TYA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x7e, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

void test_tya_neg_set(void) {
    g_cpu.Y = 0x80;
    g_cpu.A = 0x00;
    g_memory[PROG_ADDR] = OPCODE_TYA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U | CPU_FLAG_N, g_cpu.P);
}

void test_tya_neg_reset(void) {
    BIT_SET(g_cpu.P, CPU_FLAG_N);
    g_cpu.Y = 0x01;
    g_cpu.A = 0x80;
    g_memory[PROG_ADDR] = OPCODE_TYA_IMP;

    cpu_step(&g_cpu);

    TEST_ASSERT_EQUAL_HEX16(PROG_ADDR + 1, g_cpu.PC);
    TEST_ASSERT_EQUAL_UINT64(10, g_cpu.clock_cycles);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.A);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_cpu.Y);
    TEST_ASSERT_EQUAL_HEX8(CPU_FLAG_I | CPU_FLAG_U, g_cpu.P);
}

int main(void) {
    RUN_TEST(test_reset_vector);
    RUN_TEST(test_fetch_byte);
    RUN_TEST(test_fetch_word);
    RUN_TEST(test_fetch_abs);
    RUN_TEST(test_fetch_zp);
    RUN_TEST(test_fetch_abs_x);
    RUN_TEST(test_fetch_abs_x_p);
    RUN_TEST(test_fetch_abs_y);
    RUN_TEST(test_fetch_abs_y_p);
    RUN_TEST(test_fetch_indirect_abs);
    RUN_TEST(test_fetch_zp_x);
    RUN_TEST(test_fetch_zp_y);
    RUN_TEST(test_fetch_indexed_indirect);
    RUN_TEST(test_fetch_indexed_indirect_overflow);
    RUN_TEST(test_fetch_indirect_indexed);
    RUN_TEST(test_fetch_indirect_indexed_p);
    RUN_TEST(test_fetch_relative_positive);
    RUN_TEST(test_fetch_relative_negative);
    RUN_TEST(test_adc);
    RUN_TEST(test_adc_carry);
    RUN_TEST(test_adc_carry_overflow);
    RUN_TEST(test_adc_zero);
    RUN_TEST(test_adc_neg_positive);
    RUN_TEST(test_adc_neg_reset);
    RUN_TEST(test_adc_signed_overflow_positive);
    RUN_TEST(test_adc_signed_overflow_negative);
    RUN_TEST(test_and);
    RUN_TEST(test_and_zero);
    RUN_TEST(test_and_neg_set);
    RUN_TEST(test_and_neg_reset);
    RUN_TEST(test_asl);
    RUN_TEST(test_asl_zero);
    RUN_TEST(test_asl_carry_reset);
    RUN_TEST(test_asl_memory);
    RUN_TEST(test_bcc_taken);
    RUN_TEST(test_bcc_not_taken);
    RUN_TEST(test_bcs_taken);
    RUN_TEST(test_bcs_not_taken);
    RUN_TEST(test_beq_taken);
    RUN_TEST(test_beq_not_taken);
    RUN_TEST(test_bit_neg_set);
    RUN_TEST(test_bit_overflow);
    RUN_TEST(test_bmi_taken);
    RUN_TEST(test_bmi_not_taken);
    RUN_TEST(test_bne_taken);
    RUN_TEST(test_bne_not_taken);
    RUN_TEST(test_bpl_taken);
    RUN_TEST(test_bpl_not_taken);
    RUN_TEST(test_brk);
    RUN_TEST(test_bvc_taken);
    RUN_TEST(test_bvc_not_taken);
    RUN_TEST(test_bvs_taken);
    RUN_TEST(test_bvs_not_taken);
    RUN_TEST(test_clc);
    RUN_TEST(test_cld);
    RUN_TEST(test_cli);
    RUN_TEST(test_clv);
    RUN_TEST(test_cmp_zero_set);
    RUN_TEST(test_cmp_zero_reset);
    RUN_TEST(test_cmp_neg_set);
    RUN_TEST(test_cmp_neg_reset);
    RUN_TEST(test_cmp_carry_set);
    RUN_TEST(test_cmp_carry_reset);
    RUN_TEST(test_cpx_zero_set);
    RUN_TEST(test_cpx_zero_reset);
    RUN_TEST(test_cpx_neg_set);
    RUN_TEST(test_cpx_neg_reset);
    RUN_TEST(test_cpx_carry_set);
    RUN_TEST(test_cpx_carry_reset);
    RUN_TEST(test_cpy_zero_set);
    RUN_TEST(test_cpy_zero_reset);
    RUN_TEST(test_cpy_neg_set);
    RUN_TEST(test_cpy_neg_reset);
    RUN_TEST(test_cpy_carry_set);
    RUN_TEST(test_cpy_carry_reset);
    RUN_TEST(test_dec_neg_set);
    RUN_TEST(test_dec_neg_reset);
    RUN_TEST(test_dec_zero_set);
    RUN_TEST(test_dec_zero_reset);
    RUN_TEST(test_dex_neg_set);
    RUN_TEST(test_dex_neg_reset);
    RUN_TEST(test_dex_zero_set);
    RUN_TEST(test_dex_zero_reset);
    RUN_TEST(test_dey_neg_set);
    RUN_TEST(test_dey_neg_reset);
    RUN_TEST(test_dey_zero_set);
    RUN_TEST(test_dey_zero_reset);
    RUN_TEST(test_eor_zero_set);
    RUN_TEST(test_eor_zero_reset);
    RUN_TEST(test_eor_neg_set);
    RUN_TEST(test_eor_neg_reset);
    RUN_TEST(test_inc_neg_set);
    RUN_TEST(test_inc_neg_reset);
    RUN_TEST(test_inc_zero_set);
    RUN_TEST(test_inc_zero_reset);
    RUN_TEST(test_inx_neg_set);
    RUN_TEST(test_inx_neg_reset);
    RUN_TEST(test_inx_zero_set);
    RUN_TEST(test_inx_zero_reset);
    RUN_TEST(test_iny_neg_set);
    RUN_TEST(test_iny_neg_reset);
    RUN_TEST(test_iny_zero_set);
    RUN_TEST(test_iny_zero_reset);
    RUN_TEST(test_jmp);
    RUN_TEST(test_jsr);
    RUN_TEST(test_lda_neg_set);
    RUN_TEST(test_lda_neg_reset);
    RUN_TEST(test_lda_zero_set);
    RUN_TEST(test_lda_zero_reset);
    RUN_TEST(test_ldx_neg_set);
    RUN_TEST(test_ldx_neg_reset);
    RUN_TEST(test_ldx_zero_set);
    RUN_TEST(test_ldx_zero_reset);
    RUN_TEST(test_ldy_neg_set);
    RUN_TEST(test_ldy_neg_reset);
    RUN_TEST(test_ldy_zero_set);
    RUN_TEST(test_ldy_zero_reset);
    RUN_TEST(test_lsr_zero_set);
    RUN_TEST(test_lsr_zero_reset);
    RUN_TEST(test_nop);
    RUN_TEST(test_ora_zero_set);
    RUN_TEST(test_ora_zero_reset);
    RUN_TEST(test_ora_neg_set);
    RUN_TEST(test_ora_neg_reset);
    RUN_TEST(test_pha);
    RUN_TEST(test_php);
    RUN_TEST(test_pla_zero_set);
    RUN_TEST(test_pla_zero_reset);
    RUN_TEST(test_pla_neg_set);
    RUN_TEST(test_pla_neg_reset);
    RUN_TEST(test_plp);
    RUN_TEST(test_plp_ub_are_ignored);
    RUN_TEST(test_plp_i_is_delayed);
    RUN_TEST(test_rol_zero_set);
    RUN_TEST(test_rol_zero_reset);
    RUN_TEST(test_rol_neg_set);
    RUN_TEST(test_rol_neg_reset);
    RUN_TEST(test_rol_carry_set);
    RUN_TEST(test_rol_carry_copy);
    RUN_TEST(test_rol_carry_copy_and_set);
    RUN_TEST(test_ror_zero_set);
    RUN_TEST(test_ror_zero_reset);
    RUN_TEST(test_ror_neg_set);
    RUN_TEST(test_ror_neg_reset);
    RUN_TEST(test_ror_carry_set);
    RUN_TEST(test_ror_carry_copy);
    RUN_TEST(test_ror_carry_copy_and_set);
    RUN_TEST(test_rti);
    RUN_TEST(test_rts);
    RUN_TEST(test_sbc_zero_set);
    RUN_TEST(test_sbc_carry_set);
    RUN_TEST(test_sbc_carry_reset);
    RUN_TEST(test_sbc_carry_underflow);
    RUN_TEST(test_sbc_neg_set);
    RUN_TEST(test_sbc_neg_reset);
    RUN_TEST(test_sbc_signed_underflow_positive);
    RUN_TEST(test_sbc_signed_underflow_negative);
    RUN_TEST(test_sec);
    RUN_TEST(test_sed);
    RUN_TEST(test_sei);
    RUN_TEST(test_sta);
    RUN_TEST(test_stx);
    RUN_TEST(test_sty);
    RUN_TEST(test_tax_zero_set);
    RUN_TEST(test_tax_zero_reset);
    RUN_TEST(test_tax_neg_set);
    RUN_TEST(test_tax_neg_reset);
    RUN_TEST(test_tay_zero_set);
    RUN_TEST(test_tay_zero_reset);
    RUN_TEST(test_tay_neg_set);
    RUN_TEST(test_tay_neg_reset);
    RUN_TEST(test_tsx_zero_set);
    RUN_TEST(test_tsx_zero_reset);
    RUN_TEST(test_tsx_neg_set);
    RUN_TEST(test_tsx_neg_reset);
    RUN_TEST(test_txa_zero_set);
    RUN_TEST(test_txa_zero_reset);
    RUN_TEST(test_txa_neg_set);
    RUN_TEST(test_txa_neg_reset);
    RUN_TEST(test_txs);
    RUN_TEST(test_tya_zero_set);
    RUN_TEST(test_tya_zero_reset);
    RUN_TEST(test_tya_neg_set);
    RUN_TEST(test_tya_neg_reset);

    return 0;
}
