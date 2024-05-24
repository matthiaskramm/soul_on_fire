#importonce
.filenamespace objects

.import source "init.inc"
.import source "common.inc"

.segment Tables

.struct Object{edge_start,edge_end,vertex_start,vertex_end,z_clip_op,start,end,rotation}

.const NO_Z_CLIP = 0
.const NEEDS_Z_CLIP = $ff

// ----------------------------------------------------------------------------
//      Cube
// ----------------------------------------------------------------------------

.var CUBE_VERTICES = List().add(
       List().add(-1, -1, -1),
       List().add( 1, -1, -1),
       List().add(-1,  1, -1),
       List().add( 1,  1, -1),
       List().add(-1, -1, 1),
       List().add( 1, -1, 1),
       List().add(-1,  1, 1),
       List().add( 1,  1, 1)).lock()
.var CUBE_EDGE_START = List().add(0, 0, 2, 1, 4, 4, 5, 6, 0, 1, 2, 3).lock()
.var CUBE_EDGE_END = List().add(1, 2, 3, 3, 6, 5, 7, 7, 4, 5, 6, 7).lock()
.var CUBE_NORMALS = List().add(
       List().add(0, 0, -1),
       List().add(0, 0, 1),
       List().add(0, -1, 0),
       List().add(0, 1, 0),
       List().add(-1, 0, 0),
       List().add(1, 0, 0)
       ).lock()

// ----------------------------------------------------------------------------
//      Nested Cube
// ----------------------------------------------------------------------------

.var NESTEDCUBE_VERTICES = List().add(
       List().add(-1, -1, -1),
       List().add( 1, -1, -1),
       List().add(-1,  1, -1),
       List().add( 1,  1, -1),
       List().add(-1, -1, 1),
       List().add( 1, -1, 1),
       List().add(-1,  1, 1),
       List().add( 1,  1, 1),
       List().add(-2, -2, -2),
       List().add( 2, -2, -2),
       List().add(-2,  2, -2),
       List().add( 2,  2, -2),
       List().add(-2, -2, 2),
       List().add( 2, -2, 2),
       List().add(-2,  2, 2),
       List().add( 2,  2, 2)
       ).lock()

.var NESTEDCUBE_EDGE_START =        List().add(
8, 8, 10, 9, 12, 12, 13, 14,
0, 0, 2, 1, 4, 4, 5, 6
).lock()
.var NESTEDCUBE_EDGE_END =          List().add(
9, 10, 11, 11, 14, 13, 15, 15,
1, 2, 3, 3, 6, 5, 7, 7
).lock()

.var NESTEDCUBE2_EDGE_START =        List().add(
0, 1, 2, 3, 4, 5, 6, 7
).lock()
.var NESTEDCUBE2_EDGE_END =          List().add(
8, 9, 10, 11, 12, 13, 14, 15
).lock()

.var NESTEDCUBE_NORMALS = List().add(
     List().add(1, 1, 1),
     List().add(3, 4, 0)).lock()

// ----------------------------------------------------------------------------
//      Dodecahedron
// ----------------------------------------------------------------------------

.var DODECAHEDRON_VERTICES = List().add(
       List().add(-2, -2, -2),
       List().add( 2, -2, -2),
       List().add(-2,  2, -2),
       List().add( 2,  2, -2),
       List().add(-2, -2, 2),
       List().add( 2, -2, 2),
       List().add(-2,  2, 2),
       List().add( 2,  2, 2),

       List().add(-1, -1, -1),
       List().add( 1, -1, -1),
       List().add(-1,  1, -1),
       List().add( 1,  1, -1),
       List().add(-1, -1, 1),
       List().add( 1, -1, 1),
       List().add(-1,  1, 1),
       List().add( 1,  1, 1),

       List().add(-3, -2, 0),
       List().add(-3, 2, 0),
       List().add(3, -2, 0),
       List().add(3, 2, 0),
       List().add(0, -3, -2),
       List().add(0, -3, 2),
       List().add(0, 3, -2),
       List().add(0, 3, 2),
       List().add(-2, 0, -3),
       List().add(2, 0, -3),
       List().add(-2, 0, 3),
       List().add(2, 0, 3)
).lock()
.var DODECAHEDRON_EDGE_START =        List().lock()
.var DODECAHEDRON_EDGE_END =          List().lock()
.var DODECAHEDRON_NORMALS = List().lock()


.var DODECAHEDRON2_EDGE_START =       List().add(
0, 8, 2, 2, 0, 6, 4, 4,
6, 5, 10, 7, 7, 5, 3,
1, 1, 3, 0, 12, 4, 1, 5, 2, 6, 14, 3, 7, 16, 18

).lock()
.var DODECAHEDRON2_EDGE_END =         List().add(
8, 9, 9, 16, 16, 9, 8, 18,
18, 10, 11, 11, 19, 19, 11,
10, 17, 17, 12, 13, 13, 12, 13, 14, 15, 15, 14, 15, 17, 19
).lock()

.var DODECAHEDRON3_EDGE_START =       List().add(
0, 8, 2, 2, 0,
5, 10, 7, 7, 5,

4, 6, 14, 17, 12,
18, 15, 3, 1, 13
).lock()
.var DODECAHEDRON3_EDGE_END =         List().add(
8, 9, 9, 16, 16,
10, 11, 11, 19, 19,

6, 14, 17, 12, 4,
15, 3, 1, 13, 18
).lock()

// ----------------------------------------------------------------------------
//      Icosahedron
// ----------------------------------------------------------------------------

.const g = 2;
.var a = 3
.var b = 2
.var ICOSAHEDRON_VERTICES = List().add(
     List().add(a, b, 0),
     List().add(b, 0, a),
     List().add(0, a, b),

     List().add(-a, b, 0),
     List().add(0, -a, b),
     List().add(b, 0, -a),

     List().add(a, -b, 0),
     List().add(0, a, -b),
     List().add(-b, 0, a),

     List().add(-a, -b, 0),
     List().add(0, -a, -b),
     List().add(-b, 0, -a)).lock()
.var ICOSAHEDRON_EDGE_START =        List().add(0, 1, 2, 3, 9, 8, 4, 10, 6, 5, 7, 11).lock()
.var ICOSAHEDRON_EDGE_END =          List().add(1, 2, 0, 9, 8, 3, 10, 6, 4, 7, 11, 5).lock()
.var ICOSAHEDRON_NORMALS = List().add(
     List().add(1, 1, 1),
     List().add(3, 4, 0)).lock()
// ----------------------------------------------------------------------------
//      Object structs
// ----------------------------------------------------------------------------

// 1 2 4 6 8 9
.var DODECAHEDRON3 = Object(DODECAHEDRON3_EDGE_START, DODECAHEDRON3_EDGE_END, $ff, $ff, NO_Z_CLIP, 8, 28,
                   /*rotation=*/$69).lock()  // 62 68
.var NESTEDCUBE = Object(NESTEDCUBE_EDGE_START, NESTEDCUBE_EDGE_END, 8, 16, NO_Z_CLIP, 0, 16,
                   /*rotation=*/$24).lock()
.var DODECAHEDRON2 = Object(DODECAHEDRON2_EDGE_START, DODECAHEDRON2_EDGE_END, $ff, $ff, NEEDS_Z_CLIP, 8, 28,
                   /*rotation=*/$81).lock()
.var CUBE = Object(CUBE_EDGE_START, CUBE_EDGE_END, 0, $ff, NO_Z_CLIP, 8, 16,
                   /*rotation=*/$22).lock()
.var NESTEDCUBE2 = Object(NESTEDCUBE2_EDGE_START, NESTEDCUBE2_EDGE_END, 0, $ff, NO_Z_CLIP, 0, 16,
                   /*rotation=*/$26).lock()
.var DODECAHEDRON = Object(DODECAHEDRON_EDGE_START, DODECAHEDRON_EDGE_END, 0, $ff, NO_Z_CLIP, 8, 28,
                   /*rotation=*/$82).lock()

// unused:
.var ICOSAHEDRON = Object(ICOSAHEDRON_EDGE_START, ICOSAHEDRON_EDGE_END, 0, $ff, NO_Z_CLIP, 0, 12,
                   /*rotation=*/$26).lock()
// ----------------------------------------------------------------------------
//      Data
// ----------------------------------------------------------------------------

.macro coord_program(triplets) {
  .for (var i = 0; i < triplets.size(); i++) {
    .var t0 = triplets.get(i).get(0);
    .var t1 = triplets.get(i).get(1);
    .var t2 = triplets.get(i).get(2);
    .if (t0 < 0) {
      .byte ((-t0)<<1)|1
    } else {
      .byte t0<<1
    }
    .if (t1 < 0) {
      .byte ((-t1)<<1)|1
    } else {
      .byte t1<<1
    }
    .if (t2 < 0) {
      .byte ((-t2)<<1)|1
    } else {
      .byte t2<<1
    }
  }
}

.macro object_program(obj) {
  .var edge_start = obj.get("edge_start");
  .var edge_end = obj.get("edge_end");
  .var vertex_start = obj.get("vertex_start");
  .var vertex_end = obj.get("vertex_end");
  .var z_clip_op = obj.get("z_clip_op");
  .var start = obj.get("start");
  .var end = obj.get("end");
  .var rotation= obj.get("rotation")
  .for (var i = 0; i < edge_start.size(); i++) {
    .byte edge_start.get(i);
    .byte edge_end.get(i);
  }
  .byte $ff
  .byte vertex_start
  .byte vertex_end
  .byte z_clip_op
  .byte start*3, end*3
  .byte rotation
}

.align 256
@all_vertices:
  coord_program(DODECAHEDRON_VERTICES)

@all_objects:
@obj0: object_program(DODECAHEDRON3)
@obj1: object_program(CUBE)
@obj2: object_program(DODECAHEDRON2)
@obj3: object_program(NESTEDCUBE)
@obj4: object_program(NESTEDCUBE2)
@obj5: object_program(DODECAHEDRON)
