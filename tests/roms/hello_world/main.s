; Copyright (C) 2025-present inunix3, kubaa-ma
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

.setcpu "6502"

.include "nesdefs.s"

;; 16 KiB of PRG
;; 8 KiB of CHR
;; Vertical mirroring
;; Mapper: NROM
INES_HEADER 1, 1, 1, 0

.segment "VECTOR"
.addr nmi
.addr reset
.addr irq

.code

.proc nmi
    ;; Set the beginning of the palette area
    lda #$3f
    sta PPU_ADDR
    lda #$00
    sta PPU_ADDR

    ;; Set the background color.
    sty PPU_DATA

    iny

    rti
.endproc

.proc irq
    rti
.endproc

.proc reset
    sei    ;; disable IRQs
    cld    ;; disable decimal mode

    ;; set stack pointer
    ldx #$ff
    txs

    ;; disable PPU rendering and interrupts
    inx
    stx PPU_CTRL   ;; disable PPU interrupts
    stx PPU_MASK   ;; disable PPU rendering
    stx APU_STATUS ;; disable APU
    stx DMC_FREQ   ;; disable DMC interrupts

    ;; Warm PPU up: we need to wait 29 658 cycles for the PPU to initialize.
    ;; We'll wait two video frames to achieve that.
:   bit PPU_STATUS
    bpl :-
:   bit PPU_STATUS
    bpl :-

    ;; Zero RAM.
    txa
:   sta $000, x
    sta $100, x
    sta $200, x
    sta $300, x
    sta $400, x
    sta $500, x
    sta $600, x
    sta $700, x
    inx
    bne :-

    ;; Final wait for PPU warmup.
:   bit PPU_STATUS
    bpl :-

    ;; Set the beginning of the palette area
    lda #$3f
    sta PPU_ADDR
    lda #$00
    sta PPU_ADDR

    ;; Set the background color to black.
    lda #$05
    sta PPU_DATA

    lda #MASK_BG    ;; draw background
    sta PPU_MASK    ;; enable rendering
    lda #CTRL_NMI   ;; enable NMI interrupt
    sta PPU_CTRL

    ldy #$00

    lda #$01
    sta $4015
    lda #$08
    sta $4002
    lda #$02
    sta $4003
    lda #$ff
    sta $4000
loop:
    jmp loop
.endproc

.segment "CHR0a"
.segment "CHR0b"
