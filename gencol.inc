#importonce
.filenamespace gencol

#import "pseudocommands.inc"
#import "delay.inc"
#import "init.inc"

.segment GencolZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]
bitpos: .byte $80
source_lo: .byte <(TEXTURE)
source_hi: .byte >(TEXTURE)

shadow_lo: .byte 0
shadow_hi: .byte 0
@texture_type: .byte 0

read_random: .word $8d00
tdest: .word TEXTURE

configuration:
white: .byte 0
black: .byte 0
color_type: .byte 0
shadow_start: .byte 0
size_mask: .byte 0
logo_start: .byte 0
unused: .byte 0

.var d = diagonal-shadow_lookup-1
.var c = diagonal2-shadow_lookup-1
.var o = outline-shadow_lookup-1
.var n = none-shadow_lookup-1

configurations:
//    white  black  color  shadow  size  logo_start             unused
.byte 0,     6,     0,     d,      $3f,  0,                     0
.byte $8d,   0,     6,     c,      $1f,  checkerboard-logo_src, 0
.byte $81,   0,     $80,   o,      $1f,  0,                     0

shadow_lookup:
outline:  .byte $89, $79, $98, $77, $87, $78, $97  /* fallthrough */
diagonal2: .byte $98  /* fallthrough */
diagonal: .byte $99  /* fallthrough */
none:     .byte $00

logo_src:
// logo3
.byte 0, 0, 0, 0, 0, 0, 96, 48, 0, 0, 97, 176, 60, 115, 227, 48, 54, 198, 99, 54, 54, 198, 99, 54, 54, 198, 99, 54, 60, 231, 99, 152, 54, 198, 99, 54, 54, 198, 99, 54, 54, 118, 63, 54, 6, 6, 3, 6, 6, 6, 3, 54, 0, 6, 1, 246, 0, 6, 0, 6, 0, 0, 0, 0
// checkerboard
checkerboard:
.for (var i = 0; i < 16; i++) {
    .if ((i & 8) == 0) {
    .byte 0, 255, 0, 255
    } else {
    .byte 255, 0, 255, 0
    }
}

.segment GeneratorCode
// ----------------------------------------------------------------------------
//      Decode the logo
// ----------------------------------------------------------------------------
.macro @GENERATE_TEXTURE() {
        ldx #ZP_START
!:
        lda gencol_zp-ZP_START, x
        sta $00, x
        inx
        bne !-
        inc gencol_zp + (texture_type - ZP_START)

        lda texture_type
        asl
        adc texture_type
        asl
        adc texture_type
        tay
!:
        lda configurations, y
        sta configuration, x
        iny
        inx
        cpx #$07
        bne !-

        ldx logo_start
bitloop_ldy00:
        ldy #$00
bitloop:
        asl bitpos
        bne !+
        lda logo_src, x
     // sec
        rol
        sta bitpos
        inx
!:
        lda white
        bcs do_white
        // black
        lda color_type
        bpl store
        lda (read_random), y
        inc read_random+0
        and #$07
        adc #$02
        // fallthrough, always bne
do_white:
        bne store
     // sec
        lda (read_random), y
        inc read_random+0
        lsr
        lda #$83
        bcc store
        eor #$8d^$83
store:
        sta (tdest), y
        iny
        cpy #$20
        bne bitloop
        lda #>(TEXTURE+$1000)
        sec
        isc.zp tdest+1
        bne bitloop_ldy00

// ----------------------------------------------------------------------------
//      Upper/lower shadow
// ----------------------------------------------------------------------------

shadow_loop:
shadow_row_loop:
        ldy #$00
        lda (source_lo), y
        bpl shadow_done

        ldx shadow_start
        lda #$88
shadow_table_loop:
        inx
        pha
        lsr
        lsr
        lsr
        alr #$fe
     // clc
        sbc #$08-1
        clc
        adc source_lo
        and #$1f
        tay
        pla
        anc #$0f
     // clc
        sbc #$08-1
        adc source_hi
        sbc #>TEXTURE
        anc #$0f
        adc #>TEXTURE
        sta shadow_hi

        lda (shadow_lo), y
        bmi noshadow
        lda black
        sta (shadow_lo), y
noshadow:
        lda shadow_lookup, x
        bne shadow_table_loop
shadow_done:
        lda #$20
        sec
        isc source_lo
        bne shadow_row_loop
        sta source_lo
        lda #>(TEXTURE+$1000)
        sec
        isc source_hi
        bne shadow_loop

// ----------------------------------------------------------------------------
//      Texture repeat
// ----------------------------------------------------------------------------
        ldx #$00
repeat_loop:
        txa
        and size_mask
        cmp #$20
        tay
        lda (tdest), y
        and #$0f
        bcc visible
        lda #$00
visible:
        pha
        txa
        cmp #$80
        tay
        pla
        bcc !+
        asl
        asl
        asl
        asl
!:
        sta (tdest), y
        inx
        bne repeat_loop
        lda #>(TEXTURE-$0100)
        dcp tdest+1
        bne repeat_loop

     // ldx #$00
!:
        lda TEXTURE, x
        sta TEXTURE+$1000, x
        lda TEXTURE+$100, x
        sta TEXTURE+$1100, x
        lda TEXTURE+$200, x
        sta TEXTURE+$1200, x
        // roto with activation=1 overflows to TEXTURE+$1400
        lda TEXTURE+$300, x
        sta TEXTURE+$1300, x
        inx
        bne !-
done:
}

.segment Tables

@gencol_zp: .segmentout [segments="GencolZeroPage"]
