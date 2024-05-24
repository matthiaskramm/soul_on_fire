.filenamespace agsp

.macro @AGSP(initial_ypos, crunch_max, d011_linecrunch, d011_after) {
// "Any given screen position." Shift the screen using $d011.
//
// Call this on line 47 + initial_ypos cycle 46.
//
// Args:
//   initial_ypos: The rasterline we start calling this code on. 0 if it's called on line 47.
//   crunch_max: How many lines to crunch at maximum. E.g. 26.
//   d011_linecrunch: The upper bits of d011, in the linecrunch area.
//   d011_after: What to set d011 to, after the linecrunch.
//
// Needs zp vars:
//
//   d011_linecrunch: What to set to d011 to while we're linecrunching.
//   d011_after: What to set to d011 to after we're done with agsp.
//   crunch_start: The lines from crunch_start to (including) CRUNCH_MAX-1 will be crunched.
//                 So a value of crunch_start = CRUNCH_MAX results in no crunched lines.
// Exports:
//
//   vsppos:       The x position of the screen. 0..40. (A value of 40 works, but does the same
//                 as crunching one more line.)

        DELAY(4)
        // line $2f, cycle 50
        lda #((initial_ypos-2)&7)|d011_linecrunch
        sta $d011  // display starts at $30 and we're on $2f, so this doesn't trigger a badline

        clc
        ldy #$00
        // TODO: make this into a loop?
      .for (var i = 0 ; i < crunch_max; i++) {
        jsr delay_44
        cpy crunch_start
        lda $d012
        adc #$ff
        and #$07
        ora #d011_linecrunch
        sta $d011
        iny
      }
        lda #d011_linecrunch + ((initial_ypos+crunch_max+6)&7)  // e.g. $1f for 25
        sta $d011

.label @vsppos = *+1
        jmp vsp

// TODO: move this so it doesn't cause a huge .align, below
delay_44:
        DELAY(44-12)
        rts

.align 256
vsp:
        .fill 40, $80
        .byte $64, $ea  // nop zp

        DELAY(62)

        lda #d011_linecrunch + ((initial_ypos + crunch_max+1)&7)  // e.g. $1a for 25
        sta $d011  // trigger vsp
        lda #d011_after + ((initial_ypos + crunch_max+2)&7)  // e.g. $1b for 25
        sta $d011  // and badline in next line, to reload screen row
}

.macro @INC_VSP(crunch_max) {
        ldx crunch_start
        lda #40
        sec
        isc vsppos
        bne !+
        sta vsppos
        dex
!:
        cpx #$01
        bne !+
        lda vsppos
        eor #24  // cmp
        bne !+
        sta vsppos
        ldx #crunch_max
!:
        stx crunch_start
}
