#importonce
.filenamespace pseudocommands

.pseudocommand @kil {
    .byte $02
}

.pseudocommand @beq_far label {
    bne *+5
    jmp label
}

.pseudocommand @bpl_far label {
    bmi *+5
    jmp label
}

.pseudocommand @bne_far label {
    beq *+5
    jmp label
}

.pseudocommand @bmi_far label {
    bpl *+5
    jmp label
}

.pseudocommand @bcc_far label {
    bcs *+5
    jmp label
}

.pseudocommand @bcs_far label {
    bcc *+5
    jmp label
}

.pseudocommand @sbx imm {
    axs imm
}

.pseudocommand @always_bcc rel {
    bcc rel
}

.pseudocommand @always_bcs rel {
    bcs rel
}

.pseudocommand @always_bpl rel {
    bpl rel
}

.pseudocommand @always_bmi rel {
    bmi rel
}

.pseudocommand @always_beq rel {
    beq rel
}

.pseudocommand @always_bne rel {
    bne rel
}
