#importonce
.filenamespace generate

.import source "pseudocommands.inc"
.import source "delay.inc"
.import source "init.inc"
.import source "common.inc"

.segment LogGeneratorZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]
i_lo: .byte 0
i_hi: .byte 0
tmp_lo: .byte 0
tmp_hi: .byte 1

.segment SinGeneratorZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]
// only zeros
value: .word 0
delta: .word 0

.segment MulGeneratorZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]
sqr_lo_ptr: .word sqr_tables
sqr_hi_ptr: .word sqr_tables+$200

.segment LtGeneratorZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]
.const num_lt_tables = 4
.label base_hi = * + 1
base_lo: .word 0
.label add_hi = * + 1
add_lo: .word 0
        .word 1*256
        .word 256/1.618033988749895
        .word 256*(1+1.0/(GOLDEN_RATIO))
ztmp_lo: .byte <((NUM_CIRCLES-1)*256 + 255)
ztmp_hi: .byte >((NUM_CIRCLES-1)*256 + 255)

.segment GeneratorCode

.macro @GENERATE_LOG() {
// ----------------------------------------------------------------------------
//      Init
// ----------------------------------------------------------------------------
        ldx #ZP_START
!:
        lda generate_log_zp-ZP_START, x
        sta $00, x
        inx
        bne !-

// ----------------------------------------------------------------------------
//      Generate log_table
// ----------------------------------------------------------------------------
        // table = []
        // x = 0x100
        // i = 0
        // while x < 0x10000:
        //     while len(table) < x >> 8:
        //         table.append(i >> 4)
        //     x += x >> 8
        //     i += 1

log_loop:
        lda i_hi
!:
        cpx tmp_hi
        beq nostore
        sta log_table, x
        inx
        bne !-
        always_beq done
nostore:
        lda i_lo
     // sec
        adc #20-1
        sta i_lo
        lda i_hi
        adc #$00
        sta i_hi

        lda tmp_lo
     // clc
        adc tmp_hi
        sta tmp_lo
        lda tmp_hi
        adc #$00
        sta tmp_hi
        jmp log_loop
done:
}

.macro @GENERATE_SINCOS() {
// ----------------------------------------------------------------------------
//      Init
// ----------------------------------------------------------------------------
        ldx #ZP_START
        lda #$00
!:
        // this is just zeros
        //lda generate_sin_zp-ZP_START, x
        sta $00, x
        inx
        bne !-
// ----------------------------------------------------------------------------
//      Generate sin / cos
// ----------------------------------------------------------------------------

     // ldx #$00
        ldy #$3f
generate_sin:
	lda value
	clc
	adc delta
	sta value
	lda value+1
	adc delta+1
	sta value+1
	sta sin_table+$c0, x
	sta sin_table+$80, y
	eor #$ff
	sta sin_table+$40, x
	sta sin_table, y
	sta sin_table+$100, y
	lda delta
    //  clc
	adc #$10
	sta delta
        lda delta+1
        adc #$00
        sta delta+1
        inx
        dey
        bpl generate_sin
}

.macro @GENERATE_MUL_DIV() {
// ----------------------------------------------------------------------------
//      Init
// ----------------------------------------------------------------------------
        ldx #ZP_START
!:
        lda generate_mul_zp-ZP_START, x
        sta $00, x
        inx
        bne !-
        stx $d015
// ----------------------------------------------------------------------------
//      Generate x^2 tables
//      See GENERATE_SQUARE_TABLES in mul.inc for the original version.
// ----------------------------------------------------------------------------
.const sqr_lo = sqr_tables
.const sqr_hi = sqr_tables+$200
.const nsqr_lo = sqr_tables+$400
.const nsqr_hi = sqr_tables+$600

    //  ldx #$00
        ldy #$00
lb2:
        clc
lb1:    txa
        adc #$00
ml1:    sta (sqr_hi_ptr),y  // hi
        tax
        // carry is clear
        sbc #$3f
        tya
        ror
ml9:    adc #$00
        sta ml9+1
        iny
ml0:    sta (sqr_lo_ptr),y  // lo
        bne lb1
        // carry is set
        inc sqr_hi_ptr+1
        inc sqr_lo_ptr+1
        inx
        bne lb2  // to clc, compensate for inx

        // x = 0, y = 0
        // sqr_lo_ptr = sqr_tables+$200 = sqr_hi
        // sqr_hi_ptr = sqr_tables+$400 = nsqr_lo
!:
        dey
        lda sqr_lo, x
        sta nsqr_lo, y
        lda sqr_hi, x
        sta nsqr_hi, y
        lda sqr_lo+1, x
        sta nsqr_lo+256, x
        lda sqr_hi+1, x
        sta nsqr_hi+256, x
        inx
        bne !-
}

.macro @GENERATE_LT() {
// ----------------------------------------------------------------------------
//      Init
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//      Generate linear tables
// ----------------------------------------------------------------------------
linear_loop:
      .for (var i = 0; i < num_lt_tables; i++) {
        ldx #ZP_START
!:
        lda generate_lt_zp-ZP_START, x
        sta $00, x
        inx
        bne !-
        txa

        ldy #$ff
!:
        sta linear_tables + i * 256, x
        eor #$ff
        clc
        adc #$01
        sta linear_tables + i * 256, y
        lda base_lo
        clc
        adc add_lo + 2*i
        sta base_lo
        lda base_hi
        adc add_hi + 2*i
        sta base_hi
        dey
        inx
        bpl !-
      }

        ldx #$80
z_to_sprite_loop:
        lda ztmp_hi
        sta z_to_sprite, x
        cpx #Z_LO^$80  // ~$c0
        bcs doadd
        cpx #Z_HI^$80  // ~$40
        bcs noadd2
doadd:
        lda ztmp_lo
        clc
        adc #<(-256*(NUM_CIRCLES / (Z_HI - Z_LO)))
        sta ztmp_lo
        lda ztmp_hi
        adc #>(-256*(NUM_CIRCLES / (Z_HI - Z_LO)))
        sta ztmp_hi
noadd2:
        inx
        cpx #$80
        bne z_to_sprite_loop
}

// ----------------------------------------------------------------------------
//      Generate roto lookup tables
// ----------------------------------------------------------------------------
.macro @GENERATE_ROTO_TABLES() {
        ldx #$00
        stx $d015
        stx $d020
        stx $d021
!:
        txa
        and #%00000111
        ora #$c0
        sta tex_y_to_char_bits, x
        eor #%00000111
        clc
        adc #$01
        and #%00000111
        sta tex_y_to_char_bits_inverted, x
        asl
        asl
        asl
        ora #$c0
        sta tex_x_to_char_bits_inverted, x
        txa
        and #%00000111
        asl
        asl
        asl
        ora #$c0
        sta tex_x_to_char_bits, x
        txa
        lsr
        lsr
        lsr
        sta texture_lo, x
        and #$0f
        clc
        adc #>TEXTURE
        sta texture_hi_nibble, x
        txa
        sta identity, x
        txa
        lsr
        lsr
        lsr
        lsr
        lsr
        lsr
        tay
        txa
        and #$3f
        clc
        adc add_sizes_40, y
        cmp #>COLORIZE_LOOP_END
        bcc nooverflow
        adc #(>COLORIZE_LOOP_START) - (>COLORIZE_LOOP_END) - 1
nooverflow:
        sta add40_clc, x
        inx
        bne !-
}

.segment Tables
generate_log_zp:
.segmentout [segments="LogGeneratorZeroPage"]
//generate_sin_zp:
//.segmentout [segments="SinGeneratorZeroPage"]
generate_mul_zp:
.segmentout [segments="MulGeneratorZeroPage"]
generate_lt_zp:
.segmentout [segments="LtGeneratorZeroPage"]
add_sizes_40:
.byte >(SIZE_OF_ONE_COLOR_BLOCK * 40)
.byte (>(SIZE_OF_ONE_COLOR_BLOCK * 40))+1
.byte 1
