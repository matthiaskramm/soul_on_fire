#importonce
.filenamespace my_timers

.import source "delay.inc"
.import source "pseudocommands.inc"
.import source "init.inc"

.macro @MY_TIMERS() {
        lda #$25
        sta $01

        ldy #$00
        sty $dc0e
        sty $dc07

        lda #<(312*63-1)  // $c7
        sta $dd04
        sta $dd06
        lda #>(312*63-1)  // $4c
        sta $dd05
        sta $dd07

.const timer_width = 63 - 1
        lda #timer_width
        sta $dc06

        iny
loop2:
        ldx #$f3
        cpx $d012
        bne *-3

        // 12
        shy $ffff, x
        lda $ff, x
        beq loop2

        lda #$f9
        cmp $d012
        bne *-3
        .errorif (>*) != (>loop2), "page break in timer setup"
        DELAY(8)
        sty $dd0f  // line 249 cycle 15: start timer B (open lower border #1)
        DELAY(46-19)
        sty $dc0f  //          cycle 46: start timer B (jitter compensation)

        bit $d011
        bpl *-3
        bit $d011
        bmi *-3
        sty $dd0e  // line 0 cycle 6: start timer A  (open lower border #2)

        lda #$7f
        sta $dc0d-1, y
        sta $dd0d-1, y

        ldx #<nmi
        ldy #>nmi
        stx $fffa
        sty $fffb
}
