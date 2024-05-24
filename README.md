# Soul on Fire

This is the source code of "[Soul on Fire](https://csdb.dk/release/?id=242826)",
which won the 4k intro competition at [X2024](https://csdb.dk/event/?id=3294)

To build the source, you need [gmake](https://www.gnu.org/software/make/)
and [KickAssembler](http://theweb.dk/KickAssembler/Main.html#frontpage).

Run
```
    gmake intro.prg
```
to build. This will create an uncompressed file (37k in size). The release version used
[alz64](https://csdb.dk/release/?id=77754) to compress this further, and is 4091 bytes
(padded to 4096).
Note, however, that
[upkr](https://github.com/exoticorn/upkr) with 0xF's 6502 decruncher
([upkr6502](https://github.com/pfusik/upkr6502)) (released two days after Soul on Fire!)
would have saved another ~60 bytes.

High-level overview of the source structure:

 * `intro.asm`: The intro itself. Calls the code/data generators, starts the music, and contains the main NMI code. Uses:
     * `music_data.inc`: The intro's song.
     * `music.inc`: Music player. Uses around two rasterlines.
     * `timers.inc`: Timer setup. We have an NMI that runs throughout the entire demo (for music
          and opening the upper/lower border), and a horizontal timer, for raster stabilization.
 * `multiplex.inc`: Main code file for the rotating sprite vectors. Invoked six times. Uses:
     * `objects.inc`: All the 3D objects.
     * `rotate.inc`: Code for matrix rotation, vertex computation and slope calculation.
     * `genspr.inc`: Generates the sprites (expanding circles & rotated lines)
 * `roto.inc`: Main code file for the logo/chessboard distorter. Invoked three times.
     * `agsp.inc`: Code for making the VIC move the screen in x and y
     * `genchr.inc`: Generates the "charset" used to fill in a new column of bitmap which scrolls in from the right.
     * `gencol.inc`: Generates the texture that "color-cycles" through the bitmap. Two logos and a chessboard.
 * `bit.inc`: Bitreverse function. Used for texture mapping. See [BRR Lines](http://www.quiss.org/boo/).
 * `common.inc`: Reference tables and constants.
 * `init.inc`: Definitions of segments and memory locations.
 * `delay.inc`: Code for delaying a given number of cycles.
 * `generate.inc`: Generate various math tables, like sine, log, square, and multiply-by-constant.
 * `pseudocommands.inc`: Definitions of convenience ops, like `beq_far`.
