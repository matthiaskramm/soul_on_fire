#importonce
.filenamespace rotate

.import source "init.inc"
.import source "common.inc"

.const R = $2d80

.const @MAX_SPRITES = 64
.const @MATRIX_STRIDE = 32
.const @MAX_PCOORDS = MAX_SPRITES
.const MAX_EDGES = 32
.const CORNER_OFFSET = MAX_EDGES

.segment Object
edge_start: .fill 32, 0
edge_end: .fill 32, 0

.macro @ROTATE_ZP() {
@matrix:
.label @matrix_lo = *
.label @matrix_hi = * + 1
//Identity matrix:
//.word R, 0, 0; .fill MAX_VERTICES - 6, -1
//.word 0, R, 0; .fill MAX_VERTICES - 6, -1
//.word 0, 0, R; .fill MAX_VERTICES - 6, -1
/*MATRIX_ROW_0*/ .word ( 10283 * R) / $4600, (  6399 * R) / $4600, ( 13207 * R) / $4600
@coords_xyz:
@coords_x: .fill MATRIX_STRIDE-6, 0
/*MATRIX_ROW_1*/ .word ( 14493 * R) / $4600, ( -1890 * R) / $4600, (-10368 * R) / $4600
@coords_y: .fill MATRIX_STRIDE-6, 0
/*MATRIX_ROW_2*/ .word ( -2309 * R) / $4600, ( 16631 * R) / $4600, ( -6260 * R) / $4600
@coords_z: .fill MATRIX_STRIDE-6, 0
@matrix_end:

@src_matrix_ptr: .word matrix_lo & $ff00
@dst_matrix_ptr: .word matrix_lo & $ff00

@visible_vertex_start: .byte 0
@visible_vertex_end: .byte 0
@z_clip: .byte 0
@vertices_start: .byte 0
@vertices_end: .byte 0
@rotation: .byte $26

@vertices_count_minus_one: .byte $ff

@stride: .byte 0
@sign: .byte 0
@tmp_1: .byte 0
@tmp_2: .byte 0
@tmp_3: .byte 0

// For object initialization / code generation:
@dest_lo: .byte <compute_vertices
@dest_hi: .byte >compute_vertices

@log_table_x: .byte 0

@xd1: .word sqr_tables
@xd2: .word sqr_tables+$0400
@xd3: .word sqr_tables+$0200
@xd4: .word sqr_tables+$0600

// for sorting
@radix_ptr:  .for(var i = 0; i < 16; i++) {.word lonext}
}

.macro @ROTATE_SCRATCH() {

@pcoords:
@pcoords_x:
        .fill MAX_PCOORDS, 0
@pcoords_y:
        .fill MAX_PCOORDS, $ff  // $ff for sorting
@pshape:
        .fill MAX_PCOORDS, 0

@compute_vertices:
        .fill 1024, 0
}

.macro @ROTATE_TABLES() {
        .fill 256, i
}

.macro @GENERATE_OBJECT() {
        ldy @read_object_pos_lo:#$00
        ldx #$00
!:
        lda all_objects, y
        sta edge_start, x
        bmi done
        iny
        lda all_objects, y
        sta edge_end, x
        iny
        inx
        bne !-
done:
        iny
        lda all_objects, y
        sta visible_vertex_start
        iny
        lda all_objects, y
        sta visible_vertex_end
        iny
        lda all_objects, y
        sta z_clip
        iny
        lda all_objects, y
        sta vertices_start
        iny
        lda all_objects, y
        sta vertices_end
        iny
        lda all_objects, y
        sta rotation
        iny
        sty read_object_pos_lo


/* We generate code that looks like this:
            LDA #$00
            LDY matrix_hi+4, x
            SEC
            SBC scale, y
            LDY matrix_hi+2, x
            SEC
            SBC scale, y
            LDY matrix_hi, x
            SEC
            SBC scale, y
            STA coords_xyz+i, x
   Followed by:
            RTS
*/

        ldx vertices_start
        // assume we always have at least one vertex
vertex_loop:
        // 2
        lda #LDA_IMM
        pha
        lda #$00
        pha
      .for (var i = 0 ; i < 3 ; i++) {
        // 6
        lda #LDY_ZPX
        pha
        lda #matrix_hi + i * 2
        pha
        lda all_vertices, x  // t0
        lsr
        tay
        bcc plus
        lda #SEC
        pha
        lda #SBC_ABSY
        pha
        always_bcs store_table_addr
plus:   lda #CLC
        pha
        lda #ADC_ABSY
        pha
store_table_addr:
        anc #$00  // table_lo
        pha
        tya
        adc #>mul0
        pha
        inx
      }
        // 3
        lda #STA_ZPX
        pha
        lda #<coords_xyz
        sec
        adc vertices_count_minus_one
        inc vertices_count_minus_one
        pha

        ldy #$15
!:
        pla
        sta (dest_lo), y
        dey
        bpl !-

        lda dest_lo
        sec
        adc #$15
        sta dest_lo
        bcc !+
        inc dest_hi
!:
        cpx vertices_end
        bne_far vertex_loop
done2:
        ldy #$00
        lda #RTS
        sta (dest_lo), y
}

.macro MATRIX_ADD() {
        ldx #$03
        ldy #$00
loop2:
        lda (src_matrix_ptr), y
        eor sign
        sec
        sbc sign
        sta tmp_1

        iny  // hi
        lda.zp (src_matrix_ptr), y
        eor sign
        sbc sign
        sta tmp_2

        lda.zp (src_matrix_ptr), y
        bpl !+
        lda #$ff //>> 3
        bne s
!:      lda #$00 //>> 3
s:
        eor sign
        sbc sign

        asl tmp_1
        rol tmp_2
        rol
        sta tmp_3
        lda tmp_2
        asl tmp_1
        rol
        rol tmp_3
        asl tmp_1
        rol
        rol tmp_3
        clc
        dey  // lo
        adc (dst_matrix_ptr), y
        sta (dst_matrix_ptr), y
        iny  // hi
        lda tmp_3
        adc (dst_matrix_ptr), y
        sta (dst_matrix_ptr), y
        tya
        clc
        adc stride
        tay
        dex
        bne loop2
}

.macro @ROTATE() {
// ----------------------------------------------------------------------------
//      Rotate Matrix
// ----------------------------------------------------------------------------

        // Byte: AABBCCDD
        //   AA: row
        //   BB: row
        //   CC: column
        //   DD: column
        // First add rows, then columns:
        //   Add row AA to row BB
        //   Subtract row BB from row AA
        //   Add column CC to column DD
        //   Subtract column DD from column CC
        lda rotation
        alr #$c0
        adc #<matrix_lo
        sta src_matrix_ptr
        lda rotation
        and #$30
        asl
        adc #<matrix_lo
        sta dst_matrix_ptr

        ldx #$02-1
        stx stride
        dex
        stx sign

rotate_more:
        MATRIX_ADD()
        ldx sign
        bmi inner_done
        dec sign
        ldx src_matrix_ptr
        ldy dst_matrix_ptr
        sty src_matrix_ptr
        stx dst_matrix_ptr
        always_bne rotate_more
inner_done:
        lda #MATRIX_STRIDE-1
        cmp stride
        beq done_with_rotation
        sta stride

        lda rotation
        alr #$0c
        adc #<matrix_lo
        sta src_matrix_ptr
        lda rotation
        and #$03
        asl
        adc #<matrix_lo
        sta dst_matrix_ptr

        inc sign
        always_beq rotate_more
done_with_rotation:

// ----------------------------------------------------------------------------
//      Compute Vertices
// ----------------------------------------------------------------------------

        ldx #0
        jsr compute_vertices
        ldx #MATRIX_STRIDE
        jsr compute_vertices
        ldx #MATRIX_STRIDE*2
        jsr compute_vertices

// ----------------------------------------------------------------------------
//      Prepare for sorting
// ----------------------------------------------------------------------------

        ldx #$ff-4
        ldy #>lonext
      .for(var i = 0; i < 16; i += 2) {
entry:
        lda #<joinlo+1+i*4+4
        sax radix_ptr+(15-i)*2
        sty radix_ptr+(15-i)*2+1
        sta radix_ptr+(14-i)*2
        sty radix_ptr+(14-i)*2+1
     }

// ----------------------------------------------------------------------------
//      Divide by Z
// ----------------------------------------------------------------------------

        // "Divide" by z to get perspective-adjusted 2D coords
        ldx vertices_count_minus_one
perspective:

        // So yeah, we don't actually divide. We just multiply by (C-z).
        // Wouldn't work for a 3D world engine, but works just fine for model
        // view, and it saves a handful of bytes.
        lda coords_z, x
        lsr
        eor #$ff^($80>>1)
        //tay
        //lda divz, y
        sta xd1
        sta xd3
        eor #$ff
        sta xd2
        sta xd4

        ldy coords_x, x
        lda (xd1), y
        sec
        sbc (xd2), y
        sta tmp
        lda (xd3), y
        sbc (xd4), y
        cpy #$00
        bpl !+
        sbc xd1
!:
        asl tmp
        rol
        eor #$80
store_x:
        sta pcoords_x + MAX_EDGES, x

        ldy coords_y, x
        lda (xd1), y
        sec
        sbc (xd2), y
        sta tmp
        lda (xd3), y
        sbc (xd4), y
        cpy #$00
        bpl !+
        sbc xd1
!:
        asl tmp
        rol
        eor #$80
store_y:
        sta pcoords_y + MAX_EDGES, x

        cpx visible_vertex_start
        bcc skip
        cpx visible_vertex_end
        bcs skip

        stx tmp

        and #$0f
        asl
        tax
        lda tmp
        clc
        adc #MAX_EDGES
        sta (radix_ptr, x)
        sta radix_ptr, x

        ldx tmp
        ldy coords_z, x
        lda z_to_sprite, y
        sta pshape + MAX_EDGES, x
skip:

        dex
        bpl perspective

// ----------------------------------------------------------------------------
//      Generate edges
// ----------------------------------------------------------------------------

        ldy #$00
        clc
        jmp check_for_sentinel
edge_xy_loop:
     // ldx edge_start, y
        lda pcoords_x + MAX_EDGES, x
        clc
        ldx edge_end, y
        adc pcoords_x + MAX_EDGES, x
        ror
        sta pcoords_x, y
        lda pcoords_y + MAX_EDGES, x
        ldx edge_start, y
        clc
        adc pcoords_y + MAX_EDGES, x
        ror
        sta pcoords_y, y

        lda z_clip
        beq no_z_clip
        lda coords_z, x
        eor #$80
        sta tmp
        ldx edge_end, y
        lda coords_z, x
        eor #$80
        clc
        adc tmp
        arr #$fe
        cmp #$72
        bcs skip_edge2
        ldx edge_start, y
no_z_clip:
        lda pcoords_y + MAX_EDGES, x
        sec
        ldx edge_end, y
        sbc pcoords_y + MAX_EDGES, x
        bcs notinverted
        eor #$ff
        adc #$01
        sta log_table_x
        lda pcoords_x + MAX_EDGES, x
        sec
        ldx edge_start, y
        sbc pcoords_x + MAX_EDGES, x
        jmp after_sbc
notinverted:
        sta log_table_x
        lax edge_start, y
        lda pcoords_x + MAX_EDGES, x
        sec
        ldx edge_end, y
        sbc pcoords_x + MAX_EDGES, x
after_sbc:
        bcs notinverted2
        eor #$ff
        adc #$01
        tax  // log_table_y
        lda log_table, x
        eor #$ff
        sec
        ldx log_table_x
        adc log_table, x
        tax
        lda atan_xy1, x
        jmp store
notinverted2:
        tax
        lda log_table, x
        sec
        ldx log_table_x
        sbc log_table, x
        tax
        lda atan_xy2, x
store:
        sta pshape, y

        lda pcoords_y, y
        and #$0f
        asl
        tax
        tya
        sta (radix_ptr, x)
        sta radix_ptr, x

skip_edge2:
        iny
check_for_sentinel:
        ldx edge_start, y
        bpl_far edge_xy_loop
skip_edges:

// ----------------------------------------------------------------------------
//      Sort.
//      See https://codebase64.org/doku.php?id=base:a_faster_radix_sort for
//      the original algorithm. Thank you, LFT!
// ----------------------------------------------------------------------------

        // add sentinel
        ldx #$0f*2
        lda #$ff
        sta (radix_ptr, x)
        sta radix_ptr, x

        ldy #0
        jmp joinlo

.align 256
@lonext:
.fill   MAX_SPRITES, 0
@joinlo:
loop1:
       .const LINK_LISTS = 4
       .for(var i = 15-1; i >= 0; i--) {
entry:
        lda #0      // operand is first[i+1]
        sta (radix_ptr+2*i),y
        .errorif (*) - entry != LINK_LISTS , "LINK_LISTS is wrong, should be " + (* - entry)
       }
        lda #0      // operand is first[0]

        pha
        ldx #$ff-4
        ldy #>hinext
      .for(var i = 0; i < 16; i += 2) {
entry:
       .if(i < 16) {
        lda #<joinlo+1+i*4+4
        sax radix_ptr+(16-1-i)*2
        sty radix_ptr+(16-1-i)*2+1
        sta radix_ptr+(16-2-i)*2
        sty radix_ptr+(16-2-i)*2+1
       }
      }
        pla

        tax
hiloop:
        ldy pcoords_y, x
        ldx shr_4_times_2, y
        sta (radix_ptr,x)
        sta radix_ptr,x
        tay
        lax lonext,y
        bpl hiloop

        // add sentinel
        ldx #$0f*2
        lda #$ff
        sta (radix_ptr, x)
        sta radix_ptr, x

        ldy #0
        jmp joinhi

.align 256
@hinext:
        // TODO: why can't this be 0?
    .fill   MAX_SPRITES, $ff
@joinhi:

loop2:
       .for(var i = 16-2; i >= 0; i--) {
        lda #0      // operand is first[i+1]
        sta (radix_ptr+2*i),y
       }

        lda #0      // operand is first[0]
}
