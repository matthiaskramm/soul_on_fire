#importonce
.filenamespace common

.macro @DELAY(cycles) {
    .if (cycles != 0) {
        .errorif cycles <= 1, "DELAY argument must be >= 2: " + cycles
        .var remaining = cycles
        .while (remaining > 3 || remaining == 2) {
            nop
            .eval remaining -= 2
        }
        .if (remaining == 3) {
            .byte $44, $00  // nop zp
            .eval remaining -= 3
        }
        .errorif remaining != 0, "Internal error in DELAY code: " + remaining
    }
}
