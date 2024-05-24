.filenamespace genspr

#import "pseudocommands.inc"
#import "delay.inc"
#import "init.inc"
#import "common.inc"
#import "generate.inc"

.const R = 5
.const NUM_CIRCLES = 43
.const NUM_LINES = 100
.const NUM_SPRITES = NUM_CIRCLES + NUM_LINES

.const THICKNESS = 4  // even works best

.const DEST = $c000

.segment GensprZeroPage [start=ZP_START, min=ZP_START, max=$ff]
tmp: .byte 0
tmp_lo: .byte 0
tmp_hi: .byte 0

sprite_ptr: .word DEST

sprite_nr: .byte 0

// u, v = (1000, 0)
m11_lo:
m00_lo: .byte <(1000)
m11_hi:
m00_hi: .byte >(1000)
m01_lo: .byte <(0)
m01_hi: .byte >(0)
m10_lo: .byte 0
m10_hi: .byte 0
row: .byte 0
radius_lo: .byte 0
radius_hi: .byte 2
radius_add_lo: .byte $20
radius_add_hi: .byte 2
thres_hi: .byte 0
thres_sbc: .byte 0
// for atan:
ax_lo: .byte 0
ax_hi: .byte 0
ay_lo: .byte 0
ay_hi: .byte 0

last_xy1: .byte 0

sqradd: .byte -$13

mask_lo: .byte <(DEST+NUM_CIRCLES*64)
mask_hi: .byte >(DEST+NUM_CIRCLES*64)

square21_hi: .fill 24, 0  //min(pow(i - 10, 2), $7f)

y_lo: .fill 32, 0
y_hi: .fill 32, 0

.segment GeneratorCode

.macro ADD_SLOPES(dxlo, dxhi, dylo, dyhi) {
        lda y_lo, y
        clc
        adc dylo
        sta y_lo, x
        lda y_hi, y
        adc dyhi
        sta y_hi, x
}

.macro @GENERATE_SPRITES() {
// --------------------------------------------------------------------
//      Init
// --------------------------------------------------------------------
        ldx #ZP_START
!:
        lda genspr_zp-ZP_START, x
        sta $00, x
        inx
        bne !-

        lda #$64
!:
        sta square21_hi, x
        clc
        adc sqradd
        bvc ns
        lda #$7f
ns:
        inc sqradd
        inc sqradd
        inx
        cpx #21+3
        bne !-


// --------------------------------------------------------------------
//      Loop over all sprites
// --------------------------------------------------------------------
sprite_loop:
        ldx sprite_nr
        lda #$a0
        sta $0500-ALZ_GAP, x
// --------------------------------------------------------------------
//      Store atan table
// --------------------------------------------------------------------
        cpx #NUM_CIRCLES
        bcc no_atan

        lda m00_lo
        sta ax_lo
        lda m00_hi
        sta ax_hi
        lda m01_lo
        sta ay_lo
        lda m01_hi
        sta ay_hi

        // TODO: lda ay_hi, then just rol a below
        // TODO: try unrolling
        ldy #$05
!:
        asl ax_lo
        rol ax_hi
        asl ay_lo
        rol ay_hi
        dey
        bne !-

        lda ay_hi
        eor #$ff
        tay
        iny

        lax ax_hi
        bmi atan_done
        lda log_table, y
        sec
        sbc log_table, x
        cmp #$4d
        bne not_finish
        lda #$80
not_finish:
        sta tmp
        ldx last_xy1
        // TODO: sta last_xy1
acmp1:
        cpx tmp
        beq atan_done
        lda sprite_nr
        sta atan_xy1, x
        clc
        adc #NUM_LINES/2
        sta atan_xy2, x
        inx
        stx last_xy1  // TODO: move out of loop
        // TODO: always_bcc acmp1
        jmp acmp1
atan_done:


no_atan:
// --------------------------------------------------------------------
//      Update rotation direction
// --------------------------------------------------------------------
        lda sprite_nr
        cmp #NUM_CIRCLES
        bcc no_rotation
        // beq no_rotation  <- TODO: so we start with a perfect horizontal?

        // m01 -= m00 >> 5
        lda m00_lo
        sta tmp_lo
        // TODO: put tmp_hi into a instead, so we can do eor #$ff adc m01_lo below
        lda m00_hi
        ldy #$05
!:      cmp #$80
        ror
        ror tmp_lo
        dey
        bne !-
        sta tmp_hi
        lda m01_lo
        sec
        sbc tmp_lo
        sta m01_lo
        eor #$ff
        sta m10_lo
        lda m01_hi
        sbc tmp_hi
        sta m01_hi
        eor #$ff
        sta m10_hi

        // TODO: try to omit
        inc m10_lo
        bne !+
        inc m10_hi
!:
        // m00 += m01 >> 5
        lda m01_lo
        sta tmp_lo
        lda m01_hi
        // TODO: put tmp_hi into a instead, so we can simplify the add below
        ldy #$05
!:      cmp #$80
        ror
        ror tmp_lo
        dey
        bne !-
        sta tmp_hi
        lda m00_lo
        clc
        adc tmp_lo
        sta m00_lo
        lda m00_hi
        adc tmp_hi
        sta m00_hi

        //PRINT("{m00_lo:2} {m01_lo:2} {m10_lo:2} {m11_lo:2}")
no_rotation:

// --------------------------------------------------------------------
//      Compute coords of the upper left corner
// --------------------------------------------------------------------
        lda #$00
        sta y_lo
        sta y_hi
        sta row

        ldx #10
!:
        lda y_lo
        sec
        sbc m10_lo
        sta y_lo
        lda y_hi
        sbc m10_hi
        sta y_hi
        lda y_lo
        sec
        sbc m11_lo
        sta y_lo
        lda y_hi
        sbc m11_hi
        sta y_hi

        dex
        bne !-

// --------------------------------------------------------------------
//      Generate the positions within the texture
// --------------------------------------------------------------------

        ldx #$00
!:
        lda y_lo, x
        clc
        adc m10_lo
        sta y_lo+1, x
        lda y_hi, x
        adc m10_hi
        sta y_hi+1, x
        inx
        cpx #$20
        bne !-

// --------------------------------------------------------------------
//      Loop over each row
// --------------------------------------------------------------------

row_loop:
        lda #$ff-THICKNESS
        sta thres_sbc
        lda #$08
        sta thres_hi
// --------------------------------------------------------------------
//      Compute a circle
// --------------------------------------------------------------------
        lda sprite_nr
        cmp #NUM_CIRCLES
        bcs no_circle
        ldy row
        ldx #23
!:
        lda #$00
        sta y_lo, x
        lda square21_hi, x
        clc
        adc square21_hi, y
        sta y_hi, x
        dex
        bpl !-
        stx thres_sbc
        lda radius_hi
        sta thres_hi
no_circle:

// --------------------------------------------------------------------
//      Generate one row of the sprite
// --------------------------------------------------------------------

// TODO: this is the only piece of code shared between circles and lines.
//       Move into subroutine?

        lda row
        asl
        adc row
        tay
        ldx #$00
row_to_sprite:
        lda thres_sbc
        sec
        sbc y_hi, x
        clc
        adc thres_hi
        rla (sprite_ptr), y
        inx
        txa
        and #$07
        bne row_to_sprite
        iny
        cpx #24
        bne row_to_sprite

// --------------------------------------------------------------------
//      Update coordinates of row
// --------------------------------------------------------------------
        lda sprite_nr
        cmp #NUM_CIRCLES
        bcc no_rotation2

        ldx #$00
!:
        lda y_lo, x
        clc
        adc m11_lo
        sta y_lo, x
        lda y_hi, x
        adc m11_hi
        sta y_hi, x
        inx
        cpx #24
        bne !-
no_rotation2:

// --------------------------------------------------------------------
//      Iterate over rows
// --------------------------------------------------------------------
        lda #21
        sec
        isc row
        bne row_loop

// --------------------------------------------------------------------
//      Process the next sprite
// --------------------------------------------------------------------
        lda radius_lo
        clc
        adc radius_add_lo
        sta radius_lo
        lda radius_hi
        clc
        adc radius_add_hi
        sta radius_hi

        lda radius_add_lo
        clc
        adc #$0b
        sta radius_add_lo
        lda radius_add_hi
        adc #$00
        sta radius_add_hi

        lda sprite_ptr
     // clc
        adc #$40
        sta sprite_ptr
        bcc !+
        inc sprite_ptr+1
!:
        lda #143
        sec
        isc sprite_nr
        bne_far sprite_loop

// --------------------------------------------------------------------
//      Mask lines with circle
// --------------------------------------------------------------------

        ldy #$00
!:
        tya
        and #$3f
        tax
        lda (mask_lo), y
        and DEST+NUM_CIRCLES*64-64, x
        sta (mask_lo), y
        iny
        bne !-
        lda #>(DEST+NUM_SPRITES*64)
        sec
        isc mask_hi
        bne !-

}

.segment Tables

genspr_zp:
.segmentout [segments="GensprZeroPage"]
