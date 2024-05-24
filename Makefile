all test: intro.prg

%.prg: %.asm *.asm *.inc
	KickAssembler :main=$< -vicesymbols -define STANDALONE -define main_$(basename $<) $< -o $(basename $<).prg

.PHONY: all
