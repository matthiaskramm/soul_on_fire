#importonce
.filenamespace music

.import source "delay.inc"
.import source "pseudocommands.inc"
.import source "music_data.inc"
.import source "init.inc"

.const MELODY_LENGTH = 64

.macro @MUSIC_ZP() {
  @music_zp_start:
  .errorif (*) != music_lo, "music_lo must be at " + music_lo
  .errorif (*+1) != music_hi, "music_lo must be at " + (music_hi+1)
  .byte 0
  .byte 0  // pattern nr
  @music_zp_end:
}

.macro @MUSIC_DATA() {
@music_registers:
// voice 1
.word 0      // freq
.word 0      // pw
.byte $00    // ctrl
.byte $00, $fb  // adsr
// voice 2
.word 0      // freq
.word $00c0  // pw
.byte $00    // ctrl
.byte $5b, $0b  // adsr
// voice 3
.word 0      // freq
.word $0500  // pw
.byte $11    // ctrl
.byte 0, $7b  // adsr
// filter
.byte $00    // cutoff (low)
.byte $00    // cutoff (high)
.byte $f1    // filter resonance & voices
.byte $1f    // filter mode

@three:
  .byte 3
@fe_or_ff:
  .byte $ff, $fe, $fe, $fe
@transposes: TRANSPOSES()
@c1: C1()
@c2: C2()
@c3: C3()
@filt_ofs: FILT_OFS()
@sound_pattern_ofs: SND_PAT_OFS()
@filter_data: SND_PAT_DATA()
@melody: MELODY()

@freq_lo:
  .fill 128-12, 0
  .byte 140, 254, 194, 223, 88, 52, 120, 43, 83, 247, 31, 210
@freq_hi:
  .fill 128-12, 0
  .byte 91, 96, 102, 108, 115, 122, 129, 137, 145, 153, 163, 172
}

.macro @MUSIC_INIT_DATA() {
        lda #$00
      .for (var addr = music_zp_start; addr < music_zp_end; addr++) {
        sta addr
      }

        ldx #$7f-12
!:
        lda freq_hi+12, x
        lsr
        sta freq_hi, x
        lda freq_lo+12, x
        ror
        sta freq_lo, x
        dex
        bpl !-
}


.macro @MUSIC_INIT_IO() {
        ldx #$18
!:
        lda music_registers, x
        sta $d400, x
        dex
        bpl !-
}

.macro @MUSIC() {
        lax music_lo
        bit three
        beq note_start
        inx
        bne to_skip
        inc music_hi
// new pattern
        ldx music_hi
        lda transposes, x
        sta transpose
        lda c2, x
        sta c2_ctrl
to_skip:
        jmp skip
note_start:
        ldx music_hi
        cpx #c2 - c1
        bcs noplay
        lsr
        lsr
        pha
        alr #$03<<1
        tay  // full_note & 3
        adc sound_pattern_ofs, x
        tax
        lda #$01
     // clc
        sbc filter_data, x
        sta v1_offset

        ldx music_hi
        lda c1, x
        and fe_or_ff, y
        sta $d404 + 7*0
        lda c3, x
        sta $d404 + 7*2

        tya
        clc
        adc filt_ofs, x
        tay
        lda filter_data, y
        sta $d416

        pla
        tay // half_note
        lax melody, y
        bmi skip
        adc @transpose:#$00
        tax
        lda freq_lo + BASE, x
        sta $d400 + 7 * 2
        lda freq_hi + BASE, x
        sta $d401 + 7 * 2

        lda c2_ctrl:#$00
        beq skipv2
        ldy freq_lo + 12*4 + BASE, x
        sty $d400 + 7 * 1
        ldy freq_hi + 12*4 + BASE, x
        sty $d401 + 7 * 1
        ldy music_lo
        bpl nov2
        and #$fe  // remove gate bit
nov2:
        sta $d404 + 7 * 1
skipv2:

        txa
.label @v1_offset = * + 1
        sbx #$00
        lda freq_lo + BASE, x
        adc #$00
        sta $d400 + 7 * 0
        lda freq_hi + BASE, x
        sta $d401 + 7 * 0

skip:
        inc music_lo
noplay:
}
