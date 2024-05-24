#importonce
.filenamespace bit

.function @bitreverse(x) {
    .var j = 0
    .var result = 0
    .for (j = 0; j < 8; j++) {
        .eval result = result | ((x >> j) & 1) << (7-j)
    }
    .return result
}

