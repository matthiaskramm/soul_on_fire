#importonce
.filenamespace genchr

#import "pseudocommands.inc"
#import "delay.inc"
#import "init.inc"
#import "common.inc"

// Make cell references always positive
.const XPAD = 1
.const YPAD = 1

.segment GenchrZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]
tmp: .byte 0
tmp_lo: .byte 0
tmp_hi: .byte 0

// genchr:
table_nr: .byte 0
char_nr: .byte 0
m11_lo:
m00_lo: .byte 0
m11_hi:
m00_hi: .byte 0
m01_lo: .byte 0
m01_hi: .byte 0
m10_lo: .byte 0
m10_hi: .byte 0

max: .byte 0, 0, 0
char: .word 0

// for mirroring:
xxx: .byte 0
yyy: .byte 0
plain_x: .byte 0
plain_y: .byte 0
inverted_x: .byte 0
inverted_y: .byte 0

progress: .word $0500 - ALZ_GAP + NUM_CIRCLES + NUM_LINES - $a0

.segment Code

.macro ADD_SLOPES(dxlo, dxhi, dylo, dyhi) {
        lda x_lo, y
        clc
        adc dxlo
        sta x_lo, x
        lda x_hi, y
        adc dxhi
        sta x_hi, x
        lda y_lo, y
        clc
        adc dylo
        sta y_lo, x
        lda y_hi, y
        adc dyhi
        sta y_hi, x
}

// --------------------------------------------------------------------
//      GENERATE_CHARSET macro
// --------------------------------------------------------------------

.macro @GENERATE_CHARSET() {
        ldx #ZP_START
!:
        lda genchr_zp-ZP_START, x
        sta $00, x
        inx
        bne !-

// --------------------------------------------------------------------
//      Loop over all characters
// --------------------------------------------------------------------

charset_loop:
        lda #$a0
        tay
        sta (progress), y
        iny
        sta (progress), y
        iny
        sta (progress), y

        lda progress
        clc
        adc #$03
        sta progress
        lda progress+1
        adc #$00
        sta progress+1
// --------------------------------------------------------------------
//      Look up slopes
// --------------------------------------------------------------------
        lda char_nr
        lsr
        lsr
        lsr
        lsr
        lsr
        lsr
        tax
        // TODO: shift less and copy a whole block
        lda m00_lo_per_angle, x
        sta m00_lo
        lda m00_hi_per_angle, x
        sta m00_hi
        lda m01_lo_per_angle, x
        sta m01_lo
        eor #$ff
        clc
        adc #$01
        sta m10_lo
        lda #$00
        sta m01_hi
        lda #$ff
        adc #$00
        sta m10_hi

// --------------------------------------------------------------------
//      Generate the positions within the texture
// --------------------------------------------------------------------
        lax char_nr
        and #$07
        ora #YPAD*8
        sta y_hi
        txa
        lsr
        lsr
        alr #$07<<1
        ora #XPAD*8
        sta x_hi

        ldx #$00
        stx x_lo
        stx y_lo

        ldy #$00
        inx
!:
        ADD_SLOPES(m00_lo, m00_hi, m01_lo, m01_hi)
        iny
        inx
        cpx #$08
        bne !-

        ldy #$00
!:
        ADD_SLOPES(m10_lo, m10_hi, m11_lo, m11_hi)
        iny
        inx
        cpx #$40
        bne !-

// --------------------------------------------------------------------
//      Compute the direction vectors and count occurrences
// --------------------------------------------------------------------

        ldx #11
        lda #$00
!:
        sta count, x
        dex
        bpl !-

        ldy #$3f
!:
        lda x_hi, y
        lsr
        and #$fc
        sta tmp
        lda y_hi, y
        lsr
        lsr
        alr #$0f << 1
        // A = 1..3
        ora tmp
        sta dir, y
        tax
        inc count, x
        dey
        bpl !-

// --------------------------------------------------------------------
//      Find the maximum occurrences
// --------------------------------------------------------------------
        ldy #$00

max_loop:
        lax #$00
!:
        cmp count, x
        bcs not_more
        txa
        sta max, y
        lda count, x
not_more:
        inx
        cpx #12
        bne !-

        ldx max, y
        lda #$00
        sta count, x

        iny  // TODO: dey
        cpy #$03
        bne max_loop

// --------------------------------------------------------------------
//      Make the bitmap
// --------------------------------------------------------------------
        ldx #$3f
mask_loop:
        // pattern

        /* TODO (shifts pattern by 1):
        lda x_hi, x
        ora y_hi, x
        and #$07
        cmp #$01
        bcc store_zero */
        lda x_hi, x
        and #$07
        tay
        lda pattern, y
        beq store_zero
        lda y_hi, x
        and #$07
        tay
        lda pattern, y
        beq store_zero

        lda dir, x
        ldy #$00
        cmp max+0
        beq pixel1
        cmp max+1
        beq pixel2
        cmp max+2
        beq pixel3
store_zero:
        ldy #-3
pixel3: iny
pixel2: iny
pixel1: iny
        tya
        sta bitmap, x
        dex
        bpl mask_loop

// --------------------------------------------------------------------
//      Compute the character
// --------------------------------------------------------------------
        // TODO: fold into loop above
        lax #$00
char_loop:
        asl
        asl
        ora bitmap, x
        sta bitmap, x
        inx
        inx
        cpx #$40
        bne char_loop

        lda #(>CHARSET) + 7
        sta char+1

        // TODO: consider folding this into the loop above, as well.
        ldx #$3e
        ldy char_nr
!:
        lda bitmap, x
        sta (char), y
        dec char+1
        txa
        axs #$08
        bcs !-

// --------------------------------------------------------------------
//      Store the relative positions
// --------------------------------------------------------------------
        tya  // char_nr
        ldx #$07
        sax yyy
        lsr
        lsr
        lsr
        sax xxx

        lda #$02
        sta table_nr
xy_loop:
        ldy table_nr
        lax max, y
        lsr
        lsr
        clc
        adc #2-XPAD
        sta plain_x
        eor #$ff
        ldy xxx
        cpy #$01
        adc #2*2
        sta inverted_x
        txa
        and #$03
        clc
        adc #2-YPAD
        sta plain_y
        eor #$ff
        ldy yyy
        cpy #$01
        adc #2*2
        sta inverted_y

        lda table_nr
        asl
        asl
        adc #>XYTABLES
        sta tmp_hi
        ldy char_nr

        lda plain_x
        sta (tmp_lo), y
        inc tmp_hi

        lda plain_y
        sta (tmp_lo), y
        inc tmp_hi

        lda inverted_x
        sta (tmp_lo), y
        inc tmp_hi

        lda inverted_y
        sta (tmp_lo), y
        inc tmp_hi

        dec table_nr
        bpl xy_loop

// --------------------------------------------------------------------
//      Process the next character
// --------------------------------------------------------------------

        inc char_nr
        bne_far charset_loop

.label col0_x_00  = XYTABLES + $0000
.label col0_y_00  = XYTABLES + $0100
.label col0_ix_00  = XYTABLES + $0200
.label col0_iy_00  = XYTABLES + $0300
.label col1_x_00  = XYTABLES + $0400
.label col1_y_00  = XYTABLES + $0500
.label col1_ix_00  = XYTABLES + $0600
.label col1_iy_00  = XYTABLES + $0700
.label col2_x_00  = XYTABLES + $0800
.label col2_y_00  = XYTABLES + $0900
.label col2_ix_00  = XYTABLES + $0a00
.label col2_iy_00  = XYTABLES + $0b00
.label col0_x_80 = XYTABLES + $0c00 + $0000
.label col0_y_80 = XYTABLES + $0c00 + $0100
.label col0_ix_80 = XYTABLES + $0c00 + $0200
.label col0_iy_80 = XYTABLES + $0c00 + $0300
        ldx #$00
!:
        // delta compressable
        lda col0_x_00, x
        ora #$80
        sta col0_x_80, x
        lda col0_y_00, x
        ora #$80
        sta col0_y_80, x
        lda col0_ix_00, x
        ora #$80
        sta col0_ix_80, x
        lda col0_iy_00, x
        ora #$80
        sta col0_iy_80, x
        inx
        bne !-
}

.segment Tables

genchr_zp:
.segmentout [segments="GenchrZeroPage"]

pattern: .byte 0, 1, 1, 1, 1, 1, 1, 0

m00_lo_per_angle: .byte $00, 236,  181,  98
m00_hi_per_angle: .byte $01,   0,    0,   0
m01_lo_per_angle: .byte $00,  98,  181, 236
//m01_hi_per_angle: .byte $00,   0,    0,   0

.segment GenChrScratch

.align 256
count: .fill 256, 0

.align 256
x_lo: .fill 64, 0
x_hi: .fill 64, 0
y_lo: .fill 64, 0
y_hi: .fill 64, 0
dir: .fill 64, 0
bitmap: .fill 64, 0
