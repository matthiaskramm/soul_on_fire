#importonce
.filenamespace init
.import source "bit.inc"
.import source "pseudocommands.inc"

.label @COLORIZE_LOOP_START = $0200  // - $3e03
.label @TEXTURE = $e400
.label @CHARSET = $b800
.label @XYTABLES = $a800
.const @SIZE_OF_ONE_COLOR_BLOCK = 15
.label @COLORIZE_LOOP_END = COLORIZE_LOOP_START + SIZE_OF_ONE_COLOR_BLOCK * 1024
.const @ZP_START = $0005
.const @COLRAM = $6000
.const @BITMAP = $4000

.label @music_lo = 3
.label @music_hi = 4

.label @zp_d011 = $02

.segmentdef Memory [start=$0000, max=$ffff]
.segment Code [start=$6400]
@start_of_intro:
.segment GeneratorCode [startAfter="Code", max=$fff9]
.segmentdef Object [start=$0200, max=$02ff]
.segmentdef Tables [startAfter="GeneratorCode", align=256]
.segmentdef Generated [startAfter="Tables", align=256, max=$fff9]
.segmentdef Sprites [start=$c000, max=$e3ff, hide]
.segmentdef Texture [start=TEXTURE, max=$f6ff, hide]
.segmentdef GenChrScratch [start=$0800, max=$0fff, hide, virtual]
.segmentdef GenSprScratch [start=$0800, max=$0fff, hide, virtual]
.segmentdef MultiplexGenerated [start=$0800, max=$27ff, hide]
.segmentdef MultiplexScratch2 [start=$2800, max=$3bff, hide, virtual]
.segmentdef Log [start=$f800, max=$f8ff]
.segmentdef SinCos [start=$f940, max=$fa80]
.segmentdef Atan [start=$fb00, max=$fcff]
