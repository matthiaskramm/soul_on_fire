#importonce
.filenamespace common_tables
.import source "delay.inc"
.import source "pseudocommands.inc"
.import source "init.inc"
.import source "timers.inc"

.const @NUM_CIRCLES = 43
.const @NUM_LINES = 100
.const @LINE_START = 43
.const @ALZ_GAP = 22

.segment Log
.align 256
@log_table:
.byte 0, 20, 30, 36, 41, 45, 49, 51
.byte 54, 56, 58, 60, 62, 63, 65, 66
.byte 67, 68, 70, 71, 72, 73, 73, 74
.byte 75, 76, 77, 77, 78, 79, 80, 80
.byte 81, 81, 82, 83, 83, 84, 84, 85
.byte 85, 86, 86, 87, 87, 88, 88, 88
.byte 89, 89, 90, 90, 90, 91, 91, 92
.byte 92, 92, 93, 93, 93, 94, 94, 94
.byte 95, 95, 95, 95, 96, 96, 96, 97
.byte 97, 97, 97, 98, 98, 98, 98, 99
.byte 99, 99, 99, 100, 100, 100, 100, 101
.byte 101, 101, 101, 102, 102, 102, 102, 102
.byte 103, 103, 103, 103, 103, 104, 104, 104
.byte 104, 104, 105, 105, 105, 105, 105, 106
.byte 106, 106, 106, 106, 106, 107, 107, 107
.byte 107, 107, 107, 108, 108, 108, 108, 108
.byte 108, 108, 109, 109, 109, 109, 109, 109
.byte 110, 110, 110, 110, 110, 110, 110, 111
.byte 111, 111, 111, 111, 111, 111, 112, 112
.byte 112, 112, 112, 112, 112, 112, 113, 113
.byte 113, 113, 113, 113, 113, 113, 114, 114
.byte 114, 114, 114, 114, 114, 114, 115, 115
.byte 115, 115, 115, 115, 115, 115, 115, 116
.byte 116, 116, 116, 116, 116, 116, 116, 116
.byte 116, 117, 117, 117, 117, 117, 117, 117
.byte 117, 117, 117, 118, 118, 118, 118, 118
.byte 118, 118, 118, 118, 118, 118, 119, 119
.byte 119, 119, 119, 119, 119, 119, 119, 119
.byte 120, 120, 120, 120, 120, 120, 120, 120
.byte 120, 120, 120, 121, 121, 121, 121, 121
.byte 121, 121, 121, 121, 121, 121, 121, 121
.byte 122, 122, 122, 122, 122, 122, 122, 122

.segment MultiplexGenerated

.function SMUL(byte, ratio) {
  .var ratio16 = floor(ratio * 256)
  .if (byte >= 128) {
        .return - (((256-byte) * ratio16) >> 8)
  } else {
        .return (byte * ratio16) >> 8
  }
}

@shr_4_times_2:  .fill $100, (i >> 4) * 2

.const @GOLDEN_RATIO = 1.618033988749895

.align 256
@linear_tables:
@mul0: .fill 256, 0
@mul1: .fill 256, SMUL(i, 1)
@mul_igr: .fill 256, SMUL(i, 1/1.618033988749895)
@mul_1ph: .fill 256, SMUL(i, 1+1.0/(GOLDEN_RATIO))  // same as SMUL(i, GOLDEN_RATIO)

.function compute_z_to_sprite(i, lower, upper) {
  .var z = ((i >= 128 ? i - 256 : i) + 128)
  .if (z < lower) {
    .return NUM_CIRCLES - 1
  }
  .if (z >= upper) {
    .return 0
  }
  .var step16 = floor((256 * NUM_CIRCLES) / (upper - lower))
  .return NUM_CIRCLES - 1 - (((z - lower) * step16) >> 8)
}
.const @Z_LO = $45
.const @Z_HI = $b8
@z_to_sprite:
.fill 256, compute_z_to_sprite(i, Z_LO, Z_HI)

.align 256
@sqr_tables:
sqr_lo: .fill 512, (i*i)>>2
sqr_hi: .fill 512, (i*i)>>10
nsqr_lo: .fill 512, (i < 256 ? ((i^255)*(i^255)) : ((i-255)*(i-255))) >> 2
nsqr_hi: .fill 512, (i < 256 ? ((i^255)*(i^255)) : ((i-255)*(i-255))) >> 10

.segment SinCos

// 0 at 192
@sin_table: .fill 256+64, (sin(i * PI / 128) * 128) ^ $80

.segment Atan
@atan_xy1:
.byte $44, $46, $46, $47, $48, $48, $4a, $4a, $4b, $4c, $4d, $4d, $4e, $4f, $4f, $4f
.byte $50, $51, $51, $52, $52, $53, $54, $54, $54, $54, $55, $55, $56, $56, $56, $57
.byte $57, $57, $58, $58, $58, $59, $59, $59, $59, $59, $5a, $5a, $5a, $5a, $5a, $5a
.byte $5a, $5b, $5b, $5b, $5b, $5b, $5b, $5b, $5b, $5b, $5c, $5c, $5c, $5c, $5c, $5c
.byte $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c
.byte $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c
.byte $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c
.byte $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c, $5c
.byte $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b, $2b
.byte $2b, $2b, $2b, $2b, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c
.byte $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c
.byte $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2c, $2d, $2d, $2d
.byte $2d, $2d, $2d, $2d, $2d, $2d, $2d, $2d, $2d, $2d, $2e, $2e, $2e, $2e, $2e, $2e
.byte $2e, $2e, $2f, $2f, $2f, $2f, $2f, $30, $30, $30, $30, $30, $31, $31, $31, $32
.byte $32, $32, $33, $33, $33, $34, $34, $34, $35, $36, $36, $36, $37, $37, $38, $39
.byte $39, $3a, $3a, $3b, $3c, $3d, $3d, $3e, $3f, $3f, $40, $41, $42, $43, $43, $44
// Same as above, plus 50
@atan_xy2:
.byte $76, $78, $78, $79, $7a, $7a, $7c, $7c, $7d, $7e, $7f, $7f, $80, $81, $81, $81
.byte $82, $83, $83, $84, $84, $85, $86, $86, $86, $86, $87, $87, $88, $88, $88, $89
.byte $89, $89, $8a, $8a, $8a, $8b, $8b, $8b, $8b, $8b, $8c, $8c, $8c, $8c, $8c, $8c
.byte $8c, $8d, $8d, $8d, $8d, $8d, $8d, $8d, $8d, $8d, $8e, $8e, $8e, $8e, $8e, $8e
.byte $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e
.byte $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e
.byte $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e
.byte $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e, $8e
.byte $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d, $5d
.byte $5d, $5d, $5d, $5d, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e
.byte $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e
.byte $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5e, $5f, $5f, $5f
.byte $5f, $5f, $5f, $5f, $5f, $5f, $5f, $5f, $5f, $5f, $60, $60, $60, $60, $60, $60
.byte $60, $60, $61, $61, $61, $61, $61, $62, $62, $62, $62, $62, $63, $63, $63, $64
.byte $64, $64, $65, $65, $65, $66, $66, $66, $67, $68, $68, $68, $69, $69, $6a, $6b
.byte $6b, $6c, $6c, $6d, $6e, $6f, $6f, $70, $71, $71, $72, $73, $74, $75, $75, $76

.segment Generated
.align 256
// Texture coord (x and y): LLLLLsss
// Char: aaxxxyyy
@tex_x_to_char_bits:
.fill 256, ((i << 3) & %00111000) | $c0
@tex_x_to_char_bits_inverted:
.fill 256, (((-i) << 3) & %00111000) | $c0
@tex_y_to_char_bits:
.fill 256, ((i << 0) & %00000111) | $c0
@tex_y_to_char_bits_inverted:
.fill 256, (((-i) << 0) & %00000111) | $c0

@texture_hi_nibble: .fill 256, ((i>>3)&$0f) + (>TEXTURE)
@texture_lo: .fill 256, i>>3

.function add40(hi, n, carry) {
  .eval hi += >(SIZE_OF_ONE_COLOR_BLOCK * n)
  .eval hi += carry
  .eval hi = hi & 255
  .if (hi >= (>COLORIZE_LOOP_END)) {
    .return hi - (>COLORIZE_LOOP_END) + (>COLORIZE_LOOP_START)
  } else {
    .return hi
  }
}

.align 256
@identity: .fill 256, i
@add40_clc: .fill 64, add40(i, 40, 0)
@add40_sec: .fill 64, add40(i, 40, 1)
@inc_block_hi: .fill 64, add40(i, 0, 1)
 .fill 64, 0 // padding
