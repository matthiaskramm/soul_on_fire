#import "pseudocommands.inc"
#import "delay.inc"
#import "init.inc"
#import "common.inc"
#import "timers.inc"
#import "rotate.inc"
#import "generate.inc"
#import "objects.inc"

.const INITIAL_OBJECT = 0

.var D020 = false

.const NUM_SPRITES = MAX_SPRITES

.const TOP_CLIP = 16
.const BOTTOM_CLIP = 283
.const MIN_SIZE = 11

.const SCREEN_MID_X = 172  // (24+160) - 24/2
.const SCREEN_MID_Y = 140  // ($33+100) - 21/2

.segment MultiplexZeroPage [start=ZP_START, min=ZP_START, max=$ff, hide]
tmp: .byte 0
event_pos: .byte 0

save_a: .byte 0
save_x: .byte 0
save_y: .byte 0

current_sprite_times_2: .byte 14

kickoff: .byte 0

leader: .byte 0  // set after sorting

ysave: .byte 0

countdown: .byte 2

new_y: .byte 14, 0, 2, 4, 6, 8, 10, 12
bits: .byte 1, 2, 4, 8, 16, 32, 64, 128
not_bits: .byte 255-1, 255-2, 255-4, 255-8, 255-16, 255-32, 255-64, 255-128

gradient_read: .word gradients

ROTATE_ZP()

.macro SET_NEW_SPRITE() {
        ldx current_sprite_times_2

        // set y position
        lda ypos_spr, y
        sta $d001, x

        // set x position
        lda xpos_hi, y
        sta $d000, x

        txa
        lsr
        tax

        lda xpos_d010, y
        asl
        lda $d010
        and not_bits, x
        bcc set_d010_off
        ora bits, x
set_d010_off:
        sta $d010
!:

        lda color, y
        sta $d027, x
        lda shape, y
        sta $e3f8, x

        lda new_y, x
        sta current_sprite_times_2
skip:
}

.segment Code
@multiplex_entry:
        lda zp_d011
        and #$7f
        // If we come from multiplex, we can just keep d011 at what it is.
        // If we come from roto, we need to turn off the screen.
        cmp #$3f
        bne !+
        asl
!:
        sta $d011
        sta zp_d011

        GENERATE_MUL_DIV()  // also sets d015 to zero
        GENERATE_LT()

        ldx #ZP_START
!:
        lda multiplex_zp-ZP_START, x
        sta $00, x
        inx
        bne !-

!:
        lda #$ff
        sta ypos_hi, x
        txa
        lsr
        lsr
        lsr
        lsr
        asl
        sta shr_4_times_2, x
        lsr
        lsr
        lsr
        tay
        iny
        lda (gradient_read), y
        sta color_gradient, x
        inx
        bne !-
        stx $dd00
        stx $d012

        lda multiplex_zp + gradient_read - ZP_START
        clc
        adc #$05
        sta multiplex_zp + gradient_read - ZP_START

        // Already done in roto.inc
        // lda #$80
        // sta $d018

        GENERATE_OBJECT()

        // Already done in roto.inc
        // lda #$01
        // sta $d01a

        lda #$01
        sta $d019

        ldx #<irq
        ldy #>irq
        stx $fffe
        sty $ffff
        // 9 frames after init, scanline ~$30
        lda #$08
        sta $d016

        lda #$08
        and zp_d011
        bne *-3
        lda #$40
        sta zp_d011
        cli
wait:
        lsr kickoff
        bcc wait

.if (D020) {
        lda #$03
        sta $d020
}

        ROTATE()

	sta leader

.if (D020) {
        lda #$06
        sta $d020
}
        jmp wait

bottom: // y=$fe, we're ~ at rasterline BOTTOM_CLIP+1
        lda #$ff

      // move sprites to y=$ff so they're out of the way.
      .for (var i = 0; i < 16; i+=2) {
        sta $d001 + i
      }
        inc event_pos  // $81

.if (D020) {
        inc $d020
}

        // Install new sprite coordinate data.
        ldx leader
        ldy #$00
install_loop:
        lda pcoords_x, x
        clc
        adc #SCREEN_MID_X-$80  // 44
        sta xpos_hi, y
        arr #$00
        sta xpos_d010, y

        lda pcoords_y, x
        adc #SCREEN_MID_Y-$80  // 12
        sta ypos_spr, y
        // Note: Carry is set here, but it's rare.
        ror
        sta ypos_hi, y

        clc
        adc pcoords_x, x
        ror

        sta gradient_pos
        lda gradient_pos:color_gradient
!:
        sta color, y

        lda pshape, x
        sta shape, y
        iny
        lda hinext, x
        tax
        bpl install_loop
        // a=$ff
        sta ypos_hi, y

        dec countdown
        bne !+
        inc countdown
        sta $d015
!:

        inc kickoff
.if (D020) {
        lda #$00
        sta $d020
}

        clc
        lda #TOP_CLIP
        jmp store_d012

top:    // y=$ff, we're ~ at rasterline TOP_CLIP+1
        ldy #$00
        sty current_sprite_times_2

        lda (gradient_read), y
        sta $d020

        ldy #RTS
        sty rts_or_ldy

        sta $d021

      .for (var i = 0; i < 8; i++) {
        ldy #i
        jsr set_new_sprite
      }

        lda #LDY_ZP
        sta rts_or_ldy
.if (D020) {
        dec $d020
}
        ldy #$00
        always_beq store_event_pos
done:
        // triggers at bottom of last sprite
        dey
        bmi top  // $81
        jmp bottom

wrap_up:
        // out of sprites.
        ldy #$80
        sty event_pos

        lda #BOTTOM_CLIP - $100
        sec
        jmp store_d012

irq:
        asl $d019
        stx save_x
        sty save_y
        sta save_a
        ldy event_pos
        bmi done
process_event:
        lda ypos_hi+8, y
        cmp #BOTTOM_CLIP/2  // first sprite we won't show at all
        bcs wrap_up
        tya
     // clc
        adc #$08
        tay
set_new_sprite:
        SET_NEW_SPRITE()
rts_or_ldy: ldy.zp event_pos
        iny
store_event_pos:
        sty event_pos

        lda $d012
        sec
        sbc #21
        cmp ypos_spr, y
        bcs process_event  // short path

        // We assume that y positions are always sorted. So if ypos_hi+8 is < $ff,
        // this one, ypos_hi+1, is, as well. (The first frame inits with all $ff)
        lda ypos_hi, y
compute_and_store_d012:
        // TODO: move the adc #12 into the computation of ypos_hi?
        clc
        adc #12  // trigger at the bottom of the sprite we'll (re-)use
        asl
store_d012:
        sta $d012

        lda zp_d011
        and #$7f
        bcc !+
        ora #$80
!:
        sta zp_d011
        sta $d011

        lda save_a
        ldx save_x
        ldy save_y
        rti

.segment Tables

gradients:
        .byte /*bg=*/2, 1, 15, 12, 11
        .byte /*bg=*/11, 5, 13, 1, 1
        .byte /*bg=*/1, 15, 12, 11, 0

        .byte /*bg=*/11, 4, 14, 3, 1
        .byte /*bg=*/9, 5, 3, 13, 1
        .byte /*bg=*/0, 6, 14, 1, 1

ROTATE_TABLES()

multiplex_zp:
.segmentout [segments="MultiplexZeroPage"]

.segment MultiplexScratch2

color_gradient: .fill 256, 0

ROTATE_SCRATCH()
.align 256
xpos_d010:    .fill NUM_SPRITES, 0
.align 256
xpos_hi:    .fill NUM_SPRITES, sin(i*PI*2/NUM_SPRITES)*80 + SCREEN_MID_X/2
.align 256
color:      .fill NUM_SPRITES, 1 + mod(i,15)
.align 256
shape:      .fill NUM_SPRITES, 30
.align 256
color_pattern: .fill NUM_SPRITES, 1 + mod(i,15)
.align 256
ypos_hi:    .fill NUM_SPRITES, $ff
.align 256
ypos_spr:   .fill NUM_SPRITES, 0
