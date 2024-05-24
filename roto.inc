.filenamespace roto

.import source "pseudocommands.inc"
.import source "delay.inc"
.import source "bit.inc"
.import source "agsp.inc"
.import source "generate.inc"
.import source "init.inc"
.import source "timers.inc"
.import source "gencol.inc"
.import source "genchr.inc"
.import source "music.inc"

.const CRUNCH_MAX = 26
.const HEIGHT = 15
.const BITADD = 1  // TODO: try different values for this
.const NUM_ANGLES = 16
.const FADEOUT_TIME = $b0 + $26
.const SIZE_OF_ONE_INTERPOLATE_BLOCK = 6
.const BITMAP_SPEEDCODE_LENGTH = 26+25+25
.const SIZE_OF_ONE_BITMAP_WRITE_BLOCK = 53
.const FADEOUT = true

.errorif (<COLORIZE_LOOP_END) != 0, "Need colorize loop to end on a page boundary"
.errorif COLORIZE_LOOP_END >= BITMAP && COLORIZE_LOOP_END < BITMAP + $2000, "colorize loop extends into bitmap"

.segment RotoZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]

@crunch_start: .byte CRUNCH_MAX
pos_in_colorize_lo: .byte <(COLORIZE_LOOP_START + 39 * SIZE_OF_ONE_COLOR_BLOCK)
pos_in_colorize_hi: .byte >(COLORIZE_LOOP_START + 39 * SIZE_OF_ONE_COLOR_BLOCK)
colorize_rts: .word COLORIZE_LOOP_START + 40*HEIGHT*SIZE_OF_ONE_COLOR_BLOCK
xadd: .byte 8*23
row: .byte 26

save_a: .byte 0
save_x: .byte 0
save_y: .byte 0
tmp: .byte 0

tex_x_lo: .byte 0
tex_y_lo: .byte 0

tex_x: .fill HEIGHT, $30
tex_y: .fill HEIGHT, 0

tex_lo: .fill HEIGHT, 0
chars: .fill HEIGHT, 0
hi_nibble_0: .fill HEIGHT, 0

activation: .byte 0

texture_extra_bit: .byte 0   // 0 or $20

angle_256_lo: .byte 0
angle_256_hi: .byte 0

starting_angle: .byte 0, $20, -12

//angle_bits_lo: .byte 0  // ll000000
angle_bits_hi: .byte 0  // 000000hh

offset: .byte 0

// TODO: reuse some other zp addr
write_bitmap_rts: .word 0

black_dest1: .word COLRAM+$0370
black_dest2: .word $db70

texture_x_shift: .byte $20

slope_x: .byte 0
slope_y: .byte 0
br_x: .byte 0
br_y: .byte 0

xc0_ptr: .word 0
xc1_ptr: .word 0
xc2_ptr: .word 0
yc0_ptr: .word 0
yc1_ptr: .word 0
yc2_ptr: .word 0

colorize_loop_addr: .word COLORIZE_LOOP_START

// For patching write_bitmap_loop
bitmap_src_lo: .byte <BITMAP
bitmap_src_hi: .byte >BITMAP
bitmap_dest_lo: .byte <write_bitmap_loop
bitmap_dest_hi: .byte >write_bitmap_loop

clearpos: .word BITMAP

adelta: .byte 0

zp_jmp0: jmp jsr0:COLORIZE_LOOP_START

colorize_template:
      .for (var i = 0; i < 1; i++) {
entry:
        // 15 bytes, 20 cycles
.label col1 = * + 1
        lda $ff00, x
.label col2 = * + 1
        ora $ff00, x
.label store1 = * + 1
        sta.abs COLRAM+i
.label col3 = * + 1
        lda $ff00, x
.label store2 = * + 1
        sta $d800 + i
.errorif (*) - entry != SIZE_OF_ONE_COLOR_BLOCK, "Expected colorize loop to be " + SIZE_OF_ONE_COLOR_BLOCK + " bytes"
      }
colorize_loop_tail:
.const TAIL_SIZE = 3
        jmp COLORIZE_LOOP_START

.label block_lo = *+0
.label block_hi = *+1
.fill HEIGHT*2, 0

// --------------------------------------------------------------------
//      Colorize loop (generator)
// --------------------------------------------------------------------

.macro GENERATE_COLORIZE_LOOP() {
        ldx #$00
one_block:
        ldy #SIZE_OF_ONE_COLOR_BLOCK + TAIL_SIZE - 1
!:
        lda colorize_template, y
        sta (colorize_loop_addr), y
        dey
        bpl !-

        inc.zp colorize_template[0].store1+0
        inc.zp colorize_template[0].store2+0
        bne noc1
        inc.zp colorize_template[0].store1+1
        inc.zp colorize_template[0].store2+1
noc1:
        lda.zp colorize_loop_addr+0
        clc
        adc #SIZE_OF_ONE_COLOR_BLOCK
        sta.zp colorize_loop_addr+0
        lda.zp colorize_loop_addr+1
        adc #$00
        sta.zp colorize_loop_addr+1
        cmp #>COLORIZE_LOOP_END
        bne one_block
}

// --------------------------------------------------------------------
//      Init
// --------------------------------------------------------------------
.segment Code

@roto_entry:
        GENERATE_ROTO_TABLES()  // also clears d015, d020, d021
        GENERATE_TEXTURE()
        GENERATE_SINCOS()

        ldx #ZP_START
!:
        lda roto_zp-ZP_START, x
        sta $00, x
        inx
        bne !-
        stx vsppos

        inc roto_zp + (activation - ZP_START)

        GENERATE_COLORIZE_LOOP()

        // Patch bitreverse loops
        // TODO: only do this if we haven't, yet?
        ldx #$00
        ldy #HEIGHT-2
!:
        lda bitreverse_lookup, y
        sta br_x_loop+1, x
        sta br_y_loop+1, x
        txa
        axs #-SIZE_OF_ONE_INTERPOLATE_BLOCK
        dey
        bpl !-

        // Patch write_bitmap_loop
        ldx #$00
patch_bitmap_loop:
        clc
        lda bitmap_src_lo
        ldy #(write_bitmap_loop[0].dest0 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01
        ldy #(write_bitmap_loop[0].dest1 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01
        ldy #(write_bitmap_loop[0].dest2 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01
        ldy #(write_bitmap_loop[0].dest3 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01
        ldy #(write_bitmap_loop[0].dest4 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01
        ldy #(write_bitmap_loop[0].dest5 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01
        ldy #(write_bitmap_loop[0].dest6 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01
        ldy #(write_bitmap_loop[0].dest7 - write_bitmap_loop[0].entry)
        sta (bitmap_dest_lo), y
        adc #$01

        lda bitmap_src_hi
        ldy #(write_bitmap_loop[0].dest0 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y
        ldy #(write_bitmap_loop[0].dest1 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y
        ldy #(write_bitmap_loop[0].dest2 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y
        ldy #(write_bitmap_loop[0].dest3 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y
        ldy #(write_bitmap_loop[0].dest4 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y
        ldy #(write_bitmap_loop[0].dest5 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y
        ldy #(write_bitmap_loop[0].dest6 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y
        ldy #(write_bitmap_loop[0].dest7 - write_bitmap_loop[0].entry + 1)
        sta (bitmap_dest_lo), y

        // pos+BITMAP in bitmap_src_lo/bitmap_src_hi
        lda bitmap_src_lo
        clc
        adc #$40
        sta bitmap_src_lo
        lda bitmap_src_hi
        adc #$01
        and #$1f
        ora #>BITMAP
        cmp #>(BITMAP+$1fc0)
        bne not_1fc0
        ldy bitmap_src_lo
        cpy #<(BITMAP+$1fc0)
        bne not_1fc0
        sty bitmap_src_lo
        lda #>(BITMAP-$40)
not_1fc0:
        sta bitmap_src_hi

        lda bitmap_dest_lo
        sta write_bitmap_lo, x
        clc
        adc #SIZE_OF_ONE_BITMAP_WRITE_BLOCK
        sta bitmap_dest_lo
        lda bitmap_dest_hi
        sta write_bitmap_hi, x
        adc #$00
        sta bitmap_dest_hi
        inx
        cpx #BITMAP_SPEEDCODE_LENGTH
        bne_far patch_bitmap_loop
nopatch:
        lda #$83
        sta $dd0d  // enable upper/lower border interrupt

        ldx activation
        lda starting_angle, x
        sta angle_256_hi

        // TODO: can we do this as part of patch_bitmap_loop?
        ldx #$20
        lda #$00
        tay
!:
        sta (clearpos), y
        iny
        bne !-
        inc clearpos+1
        dex
        bne !-

        lda #$02
        sta $dd00
        lda #$80
        sta $d018
        lda #$18
        sta $d016

        lda #$36
        sta $d012
        lda #$3f
        sta $d011
        sta zp_d011
        ldx #<irq
        ldy #>irq
        stx $fffe
        sty $ffff
        lda #$01
        sta $d01a
        sta $d019

        cli

delay_loop:
        bne *

.align 256
// --------------------------------------------------------------------
//      IRQ
// --------------------------------------------------------------------
irq:
        sta save_a
        stx save_x
        sty save_y

        lda	#$1e
        sec
        sbc	$dc06
        // A in 1..8
        sta	* + 4
branch:
        bpl	* + 2
        .byte $c2, $c2
        .byte $c2, $c2
        .byte $c2, $c2
        .byte $c2, $04
        .byte $ea
        .errorif (>branch) != (>*), "page crossing in dejitter"

        DELAY(4)
        AGSP(7, CRUNCH_MAX, $78, $38)
no_agsp:
        INC_VSP(CRUNCH_MAX)

// --------------------------------------------------------------------
//      Animation
// --------------------------------------------------------------------

        lda #$00
        ldx activation
        bne anim_one_or_two
anim_zero:
// --------------------------------------------------------------------
//      Animation 0
// --------------------------------------------------------------------
        sta br_x
        sta slope_x
        // music timer = $0026

        lax music_lo
        asl
        asl
        asl
        tay
        ror
        cpx #FADEOUT_TIME
        bpl  !+
    //  clc
        sbc #FADEOUT_TIME - $80 - 1
        adc music_lo
!:
        asl
        eor #$80
        tax

        lda sin_table, x
        clc
        adc sin_table, y
        ror
        sta tmp
        lsr
        lsr
        lsr
        lsr
        lsr
        sec
        sbc #$11
        sta tex_y+0
        lda #$08
        sta slope_y
        lda tmp
        eor #$ff
        sta br_y

        lsr tmp
        lsr tmp
        lsr tmp
        lsr tmp
        lsr tmp
        lsr tmp
        // tmp = 1 during fadein, 0-3 during wave

        lda #FADEOUT_TIME * 8 - 8
        ldx music_lo
        cpx #FADEOUT_TIME
        bcc no_fadeout
        ldx #FADEOUT_TIME-1
no_fadeout:
        txa
     // clc
        sbc #$04
        asl
        asl
        asl
        ora tmp
        sta tex_x+0

        lda #$00
        ror
        ror
        ror
        sta texture_extra_bit
        jmp anim_done

// --------------------------------------------------------------------
//      Animation 1
// --------------------------------------------------------------------
anim_one_or_two:
        dex
        bne anim_two
anim_one:
        // music timer = $0400
        sta slope_x
        sta slope_y

        lda music_lo
        asl
        asl
        tay
        lda sin_table, y
        lsr
        lsr
        lsr
        clc
        adc #$58
        sta br_x
        sta br_y

        jmp shift_slope_and_add_to_tex

// --------------------------------------------------------------------
//      Animation 2
// --------------------------------------------------------------------
anim_two:
        // music timer = $0800
        ldx music_hi
        cpx #$09
        bcc look_up_sin
        cpx #$0b
        bne not_b
        lda music_lo
        cmp #$f8
        bcs nofadeout
not_b:
        cpx #$0c
        bcc noplasma
        beq plasma
        ldx music_lo
        cpx #$30
        bcc incplasma
        bpl to_anim_done
        sta zp_d011
to_anim_done:
        jmp anim_done
plasma:
        ldx music_lo
        cpx #$c0
        bcc nofadeout
      .if (FADEOUT) {
        sta CHARSET+$0000, x
        sta CHARSET+$0100, x
        sta CHARSET+$0200, x
        sta CHARSET+$0300, x
        sta CHARSET+$0400, x
        sta CHARSET+$0500, x
        sta CHARSET+$0600, x
        sta CHARSET+$0700, x
      }
nofadeout:
        lda #$fe
        cpx #$a0
        bcc !+
incplasma:
        lda #$ff
!:
        sta.zp adelta
        anc #$80
        always_bne add_to_angle
noplasma:
        lda angle_256_hi
        bpl !+
        eor #$ff
!:      asl
        clc
        adc #$20
        bcc add_to_angle
        lda #$ff
add_to_angle:
        adc angle_256_lo
        sta angle_256_lo
        lda angle_256_hi
        adc.zp adelta
        sta angle_256_hi

look_up_sin:
        ldx angle_256_hi

        ldy #$00
        lda sin_table, x
        eor #$80
        sta br_x
        bpl !+
        ldy #$ff
!:
        sty slope_x

        ldy #$00
        lda sin_table+64, x
        eor #$80
        sta br_y
        bpl !+
        ldy #$ff
!:
        sty slope_y

shift_slope_and_add_to_tex:
        lda slope_x
        asl br_x
        rol
        asl br_x
        rol
        asl br_x
        rol
        asl br_x
        rol
        sta slope_x

        lda slope_y
        asl br_y
        rol
        asl br_y
        rol
        asl br_y
        rol
        asl br_y
        rol
        sta slope_y

        lda tex_y_lo
        sec
        sbc br_x
        sta tex_y_lo
        lda tex_y+0
        sbc slope_x
        sta tex_y+0

        lda tex_x_lo
        clc
        adc br_y
        sta tex_x_lo
        lda tex_x+0
        adc slope_y
        sta tex_x+0

anim_done:
// --------------------------------------------------------------------
//      Compute the locations we'll patch in the coloring code.
// --------------------------------------------------------------------
compute_locations:
        lda pos_in_colorize_lo
        ldx pos_in_colorize_hi
        clc
        adc #SIZE_OF_ONE_COLOR_BLOCK
        sta pos_in_colorize_lo
        bcc !+
        ldy inc_block_hi, x
        ldx identity, y  // only uses first 64 bytes
        clc
!:
        stx pos_in_colorize_hi

x_loop:
      .for (var i = 0; i < HEIGHT / 2; i++) {
entry:
        sta block_lo+i*4+0
        stx block_hi+i*4+0
        adc #SIZE_OF_ONE_COLOR_BLOCK * 40  // 600 ($258)
        bcc !+
        ldy add40_sec, x
        clc
        always_bcc store_y
!:
        ldy add40_clc, x
store_y:
        sta block_lo+i*4+2
        sty block_hi+i*4+2
      .if (i < HEIGHT / 2 - 1) {
        adc #SIZE_OF_ONE_COLOR_BLOCK * 40
        bcc !+
        ldx add40_sec, y
        clc
        always_bcc tail
!:
        ldx add40_clc, y
      }
tail:
     }

// --------------------------------------------------------------------
//      Interpolate positions in texture.
// --------------------------------------------------------------------

      ldy br_x
      lda tex_x+0
br_x_loop:
      .for (var i = 0; i < HEIGHT - 1; i++) {
entry:
        cpy #$00  // bitreverse(i+BITADD) (filled in during init)
        adc slope_x
        sta tex_x+i+1
        .errorif (*) - entry != SIZE_OF_ONE_INTERPOLATE_BLOCK, "size error"
      }

      ldy br_y
      lda tex_y+0
br_y_loop:
      .for (var i = 0; i < HEIGHT - 1; i++) {
entry:
        cpy #$00  // bitreverse(i+BITADD) (filled in during init)
        adc slope_y
        sta tex_y+i+1
        .errorif (*) - entry != SIZE_OF_ONE_INTERPOLATE_BLOCK, "size error"
      }

// --------------------------------------------------------------------
//      Compute angle
// --------------------------------------------------------------------
        anc #$00
        sta angle_bits_hi

        lda angle_256_hi
        and #$f0
        eor #$ff
        adc #$01
        asl
        rol angle_bits_hi
        asl
        rol angle_bits_hi
        ora #$3f
        tax  // angle_bits_lo

        ldy angle_bits_hi
        lda xc0_table, y
        sta xc0_ptr+1
        lda xc1_table, y
        sta xc1_ptr+1
        lda xc2_table, y
        sta xc2_ptr+1
        lda yc0_table, y
        sta yc0_ptr+1
        lda yc1_table, y
        sta yc1_ptr+1
        lda yc2_table, y
        sta yc2_ptr+1

        tya
        asl
        clc
        adc #<char_compute_hi
        sta jmp0

// --------------------------------------------------------------------
//      Compute the chars
// --------------------------------------------------------------------

        // x = angle_bits_lo
        jmp jmp0:(char_compute_hi)

char_compute_hi:
        .word compute_chars[0].loop
        .word compute_chars[1].loop
        .word compute_chars[2].loop
        .word compute_chars[3].loop
        .errorif (>*) != >char_compute_hi, "page break in jump table"

compute_chars:
    .for (var anglehi = 0; anglehi < 4; anglehi++) {
        .var x_table
        .var y_table
        .if (anglehi == 0) {
          .eval x_table = tex_x_to_char_bits
          .eval y_table = tex_y_to_char_bits
        }
        .if (anglehi == 1) {
          .eval x_table = tex_y_to_char_bits_inverted
          .eval y_table = tex_x_to_char_bits
        }
        .if (anglehi == 2) {
          .eval x_table = tex_x_to_char_bits_inverted
          .eval y_table = tex_y_to_char_bits_inverted
        }
        .if (anglehi == 3) {
          .eval x_table = tex_y_to_char_bits
          .eval y_table = tex_x_to_char_bits_inverted
        }
loop:
      .for (var i = 0; i < HEIGHT; i++) {
entry:
        // 22 cycles
        ldy tex_x+i
        lda texture_lo, y
        ora texture_extra_bit
        sta tex_lo+i
        // TODO: make the above into a separate loop, and x_table and y_table into ptrs?
        lda x_table, y
        ldy tex_y+i
        ora y_table, y
        sax chars+i  // and with angle
        lda texture_hi_nibble, y
        sta.zp hi_nibble_0+i
      }
        jmp continue
    }

continue:
// --------------------------------------------------------------------
//      Add color lookup to positions
// --------------------------------------------------------------------
.const COL1 = colorize_template[0].col1 - colorize_template[0].entry
.const COL2 = colorize_template[0].col2 - colorize_template[0].entry
.const COL3 = colorize_template[0].col3 - colorize_template[0].entry

        clc
      .for (var i = 0; i < HEIGHT; i++) {
entry:
        // col 0 x
        ldy chars+i
        lax (yc0_ptr), y
        lda (xc0_ptr), y
        adc tex_lo+i
        ldy #COL1+0
        sta (block_lo+i*2), y
        // col 0 y
        txa
        adc hi_nibble_0+i
        ldy #COL1+1
        sta (block_lo+i*2), y
        // col 1 x
        ldy chars+i
        lax (yc1_ptr), y
        lda (xc1_ptr), y
        adc tex_lo+i
        ldy #COL2+0
        sta (block_lo+i*2), y
        // col 1 y
        txa
        adc hi_nibble_0+i
        iny
        sta (block_lo+i*2), y
        // col 2 x
        ldy chars+i
        lax (yc2_ptr), y
        lda (xc2_ptr), y
        adc tex_lo+i
        ldy #COL3+0
        sta (block_lo+i*2), y
        // col 2 y
        txa
        adc hi_nibble_0+i
        iny
        sta (block_lo+i*2), y
      }

// --------------------------------------------------------------------
//      Colorize the bitmap
// --------------------------------------------------------------------
colorize:
        clc
        lda jsr0+0
        adc #SIZE_OF_ONE_COLOR_BLOCK
        sta jsr0+0
        bcc !+
        lda #>COLORIZE_LOOP_END
     // sec
        isc jsr0+1
        bne to_clc
        lda #>COLORIZE_LOOP_START
        sta jsr0+1
to_clc:
        clc
!:
        ldy #$00
        lda #LDA_ABSX
        sta (colorize_rts), y

        lda colorize_rts+0
     // clc
        adc #SIZE_OF_ONE_COLOR_BLOCK
        sta colorize_rts+0
        bcc !+
        lda #>COLORIZE_LOOP_END
    //  sec
        isc colorize_rts+1
        bne !+
        lda #>COLORIZE_LOOP_START
        sta colorize_rts+1
!:
        lda #RTS
        sta (colorize_rts), y

        ldx texture_x_shift
        jsr zp_jmp0
dex_colors:
        dex

        lda #$3f
        ldy activation
        beq !+
        lsr
!:
        sax texture_x_shift

// --------------------------------------------------------------------
//      Color one char black
// --------------------------------------------------------------------
black:
        lax #$00
        sta (black_dest1, x)
        sta (black_dest2, x)

        inc black_dest1+0
        inc black_dest2+0
        bne !+
        inc black_dest1+1
        lda #>($d800+$0400)
        sec
        isc black_dest2+1
        bne !+
        lda #>COLRAM
        sta black_dest1+1
        lda #>($d800)
        sta black_dest2+1
!:

// --------------------------------------------------------------------
//      Rewrite the bitmap data of the rightmost row.
// --------------------------------------------------------------------
write_bitmap:
        lda xadd
        clc
        adc #8
        cmp #24*8  // triggers on first frame
        bne no_row_change
        ldy row
        lda xadd_for_row, y
        pha
        lax next_row, y
        sta row
        eor #$ff
        sta offset
        lda write_bitmap_lo, x
        sta jsr1+0
        lda write_bitmap_hi, x
        sta jsr1+1
        lda write_bitmap_lo+HEIGHT, x
        sta write_bitmap_rts+0
        lda write_bitmap_hi+HEIGHT, x
        sta write_bitmap_rts+1
        pla
no_row_change:
        sta xadd

        ldy #$00
        lda #RTS
        sta (write_bitmap_rts), y

        //PRINT("{row} {xadd}")
        ldy xadd
        jsr jsr1:$ffff

        ldy #$00
        lda #LDX_ZP
        sta (write_bitmap_rts), y

irq_end:
        asl $d019

        lda save_a
        ldx save_x
        ldy save_y
        rti


write_bitmap_loop:
      .for (var i = 0; i < BITMAP_SPEEDCODE_LENGTH; i++) {
entry:
        ldx.zp offset
        lda.zp (chars+i+1)&$ff, x
        tax
        .var pos = (i * $140) & $1fff
        .if (pos == $1fc0) {
          .errorif i != 51, "internal error"
          // Use the fact that y is always >= 64 for rows >=26+25.
          .eval pos = $1fc0-$2000
        }
        .eval pos += BITMAP

        .eval pos = 0  // we fill this in during init

        lda chardata0, x
        .label dest0 = * + 1
        sta.abs $0000, y
        lda chardata1, x
        .label dest1 = * + 1
        sta.abs $0000, y
        lda chardata2, x
        .label dest2 = * + 1
        sta.abs $0000, y
        lda chardata3, x
        .label dest3 = * + 1
        sta.abs $0000, y
        lda chardata4, x
        .label dest4 = * + 1
        sta.abs $0000, y
        lda chardata5, x
        .label dest5 = * + 1
        sta.abs $0000, y
        lda chardata6, x
        .label dest6 = * + 1
        sta.abs $0000, y
        lda chardata7, x
        .label dest7 = * + 1
        sta.abs $0000, y
        .errorif (*) - entry != SIZE_OF_ONE_BITMAP_WRITE_BLOCK, "size error"
      }
        rts

// --------------------------------------------------------------------
//      Tables
// --------------------------------------------------------------------

.segment Tables

roto_zp:
.segmentout [segments="RotoZeroPage"]

xc0_table: .byte >col0_x_80
           .byte >col0_iy_80
           .byte >col0_ix_80
           .byte >col0_y_80
xc1_table: .byte >col1_x_00
           .byte >col1_iy_00
           .byte >col1_ix_00
           .byte >col1_y_00
xc2_table: .byte >col2_x_00
           .byte >col2_iy_00
           .byte >col2_ix_00
           .byte >col2_y_00

yc0_table: .byte >col0_y_00
           .byte >col0_x_00
           .byte >col0_iy_00
           .byte >col0_ix_00
yc1_table: .byte >col1_y_00
           .byte >col1_x_00
           .byte >col1_iy_00
           .byte >col1_ix_00
yc2_table: .byte >col2_y_00
           .byte >col2_x_00
           .byte >col2_iy_00
           .byte >col2_ix_00

.function NEXT_ROW(row) {
  .var up = row < 26
  .var down = row >= 26
  .if (up) {
    .if (row == 25) {
      .return 0
    } else {
      .return row + 26  // 0 -> 1040  (= 16 mod 1024)
    }
  } else {
    .return row - 25  // 1040 -> 40
  }
}

next_row:
  .fill 26+26, NEXT_ROW(i)
xadd_for_row:
  .fill 26+26, (i < 26 && i != 25) ? 8*8 : 0*8

bitreverse_lookup:
//.fill HEIGHT, bitreverse(i+BITADD)
.fill HEIGHT-1, bitreverse(HEIGHT-1-1-i)+BITADD

.segment Generated

write_bitmap_lo:
  .fill BITMAP_SPEEDCODE_LENGTH, 0  // <write_bitmap_loop[i].entry filled in by init
write_bitmap_hi:
  .fill BITMAP_SPEEDCODE_LENGTH, 0  // >write_bitmap_loop[i].entry filled in by init

.segment Charset [start=CHARSET, max=CHARSET+$7ff]

* = CHARSET
.label chardata0 = * + $000
.label chardata1 = * + $100
.label chardata2 = * + $200
.label chardata3 = * + $300
.label chardata4 = * + $400
.label chardata5 = * + $500
.label chardata6 = * + $600
.label chardata7 = * + $700

* = XYTABLES
.label col0_x_00  = * + $0000
.label col0_y_00  = * + $0100
.label col0_ix_00  = * + $0200
.label col0_iy_00  = * + $0300
.label col1_x_00  = * + $0400
.label col1_y_00  = * + $0500
.label col1_ix_00  = * + $0600
.label col1_iy_00  = * + $0700
.label col2_x_00  = * + $0800
.label col2_y_00  = * + $0900
.label col2_ix_00  = * + $0a00
.label col2_iy_00  = * + $0b00
.label col0_x_80  = * + $0c00
.label col0_y_80  = * + $0d00
.label col0_ix_80  = * + $0e00
.label col0_iy_80  = * + $0f00

.segment Texture  // $e400

.function color(x, y) {
  .if (x > 32) {
    .return (x+y)&15;
  } else {
    .return (x^y^(x>>3))&15;
  }
}
.for (var j = 0; j < $13; j++) {
    .align 128; .for (var r = 0; r < 2; r++) {.fill 64, color(i&63, j&15)}
    .align 128; .for (var r = 0; r < 2; r++) {.fill 64, color(i&63, j&15) << 4}
}
