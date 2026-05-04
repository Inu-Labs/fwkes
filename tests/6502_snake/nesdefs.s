;; Based on https://github.com/sehugg/8bitworkshop/blob/master/presets/nes/nesdefs.dasm.

PPU_CTRL          = $2000
PPU_MASK          = $2001
PPU_STATUS        = $2002
OAM_ADDR          = $2003
OAM_DATA          = $2004
PPU_SCROLL        = $2005
PPU_ADDR          = $2006
PPU_DATA          = $2007
PPU_OAM_DMA       = $4014
DMC_FREQ          = $4010
APU_STATUS        = $4015
APU_NOISE_VOL     = $400c
APU_NOISE_FREQ    = $400e
APU_NOISE_TIMER   = $400f
APU_DMC_CTRL      = $4010
APU_CHAN_CTRL     = $4015
APU_FRAME         = $4017
JOYPAD1           = $4016
JOYPAD2           = $4017
OAM_DMA           = $4014
OAM_RAM           = $0200

CTRL_NMI	      = %10000000	; Execute Non-Maskable Interrupt on VBlank
CTRL_8x8	      = %00000000 	; Use 8x8 Sprites
CTRL_8x16	      = %00100000 	; Use 8x16 Sprites
CTRL_BG_0000	  = %00000000 	; Background Pattern Table at $0000 in VRAM
CTRL_BG_1000	  = %00010000 	; Background Pattern Table at $1000 in VRAM
CTRL_SPR_0000	  = %00000000 	; Sprite Pattern Table at $0000 in VRAM
CTRL_SPR_1000	  = %00001000 	; Sprite Pattern Table at $1000 in VRAM
CTRL_INC_1	      = %00000000 	; Increment PPU Address by 1 (Horizontal rendering)
CTRL_INC_32	      = %00000100 	; Increment PPU Address by 32 (Vertical rendering)
CTRL_NT_2000	  = %00000000 	; Name Table Address at $2000
CTRL_NT_2400	  = %00000001 	; Name Table Address at $2400
CTRL_NT_2800	  = %00000010 	; Name Table Address at $2800
CTRL_NT_2C00	  = %00000011 	; Name Table Address at $2C00

MASK_TINT_RED	  = %00100000	; Red Background
MASK_TINT_BLUE	  = %01000000	; Blue Background
MASK_TINT_GREEN	  = %10000000	; Green Background
MASK_SPR	      = %00010000 	; Sprites Visible
MASK_BG		      = %00001000 	; Backgrounds Visible
MASK_SPR_CLIP	  = %00000100 	; Sprites clipped on left column
MASK_BG_CLIP	  = %00000010 	; Background clipped on left column
MASK_COLOR	      = %00000000 	; Display in Color
MASK_MONO	      = %00000001 	; Display in Monochrome

F_BLANK		      = %10000000 	; VBlank Active
F_SPRITE0	      = %01000000 	; VBlank hit Sprite 0
F_SCAN8		      = %00100000 	; More than 8 sprites on current scanline
F_WIGNORE	      = %00010000 	; VRAM Writes currently ignored.

;; INES_HEADER PRG_SIZE CHR_SIZE MIRRORING MAPPER_BITS
.macro INES_HEADER prg_size, chr_size, mirroring, mapper
    .segment "INES"

    .byte "NES", $1a
    .byte prg_size
    .byte chr_size
    .byte ((mapper & $0f) << 4) | (mirroring & 1)
    .byte mapper & $f0
    .byte 0, 0, 0, 0, 0, 0, 0, 0
.endmacro
