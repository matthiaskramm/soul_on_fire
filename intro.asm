.filenamespace intro

#import "pseudocommands.inc"
#import "delay.inc"
#import "init.inc"
#import "timers.inc"
#import "music.inc"
#import "common.inc"
#import "generate.inc"
#import "genchr.inc"
#import "genspr.inc"
#import "gencol.inc"

.segment IntroZeroPage [start=$0002, min=$02, max=$ff, hide, virtual]

.errorif (*) != zp_d011, "zp_d011 must be at " + zp_d011
.byte $00
MUSIC_ZP()

.segment Default

.segment BasicUpstart [start=$0801]
        BasicUpstart2(start)

.segment Code
@start:
@intro_init:
        sei
#if STANDALONE
        lda #$20
        sta $01
#endif
        MUSIC_INIT_DATA()

        GENERATE_LOG()
        GENERATE_SPRITES()
        GENERATE_CHARSET()
        MY_TIMERS()  // Also sets $01 to $25
        MUSIC_INIT_IO()  // click!

        lda #$00
        sta $d011
        //START_TRACING()

        // fallthrough to roto

.segment Default
#import "roto.inc"
#import "multiplex.inc"
#import "gencol.inc"

.segment Tables

.segment Code
bottom_nmi:
        and #$f7
        sta zp_d011
        sta $d011
        pla
        rti
@nmi:
        bit $dd0d
        pha
        lda zp_d011
        bit $d012
        bmi bottom_nmi
        ora #$08
        sta zp_d011
        sta $d011

        txa; pha; tya; pha
        MUSIC()
        lda music_lo
        cmp next_part_offset:next_part_start
        beq next_part
no_new_part:
        pla; tay; pla; tax

        pla
        rti

next_part:
        inc next_part_offset
        tax
        beq no_new_part
        lsr part_order
        bcs !+
        beq no_new_part
        jmp multiplex_entry
!:
        jmp roto_entry

.segment Tables
MUSIC_DATA()
next_part_start: .byte $fb, $fb, $fb, $d0, $00, $fb, $fb, $fb, $d0
part_order: .byte $88
.errorif (>*) != >(next_part_start-1), "page crossing in next_part table"
.label end_of_intro = *


.segment Default
.file [name=getFilename().substring(0, getFilename().size()-4)+".prg",
       segments="BasicUpstart,Code,GeneratorCode,Tables"]
