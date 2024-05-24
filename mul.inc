#importonce
.filenamespace math

.macro @SQUARE_TABLES() {
.align 256
sqr_lo: .fill 512, (i*i)>>2
sqr_hi: .fill 512, (i*i)>>10
nsqr_lo: .fill 512, (i < 256 ? ((i^255)*(i^255)) : ((i-255)*(i-255))) >> 2
nsqr_hi: .fill 512, (i < 256 ? ((i^255)*(i^255)) : ((i-255)*(i-255))) >> 10
}
