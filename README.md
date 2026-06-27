<div align="center">
<h1>🌹 Rose</h1>
</div>

Rose is an experimental classical chess and <a href="https://en.wikipedia.org/wiki/Fischer_random_chess">chess960</a> engine.
Most notably, Rose was the first engine to utilise <a href="https://87flowers.com/byteboard-attack-tables-1/">SIMD-accelerated attack tables</a>.

Rose makes heavy use of SIMD; it relies on AVX512 for performance. Performance on other microarchitectures is not a focus of this project.

Rose is capable of communicating over both the <a href="https://en.wikipedia.org/wiki/Universal_Chess_Interface">Universal Chess Interface (UCI)</a> protocol
and the <a href="https://www.gnu.org/software/xboard/engine-intf.html">Chess Engine Communication Protocol (CECP/Xboard)</a>,
although UCI is the preferred protocol.

The Rose project prides itself in only using original self-generated training data for its neural networks.

## Releases

For official releases, please visit: https://github.com/87flowers/Rose/releases

We recommend using the AVX512 build for your operating system if you have a CPU that supports it, as this would be the fastest.
You can get a speed comparison by running `bench` on each downloaded executable.

## Building Rose

Building requires Make and a C++26 compiler. Clang is the recommended compiler, and GCC support is not guaranteed.

Run the command
```
make
```
in the root directory to build a `./rose` executable. The current network will be automatically downloaded.

If you are building on Windows, using MSYS2 (UCRT64) is recommended.
Rose is regularly tested to build with the `mingw-w64-ucrt-x86_64-clang`, `mingw-w64-ucrt-x86_64-git`, and `mingw-w64-ucrt-x86_64-lld` packages installed.

## Non-standard UCI commands

* `wait`: Waits for the current search to complete before continuing.
* `bench`: Runs a search benchmark on a built-in list of positions, to provide a search fingerprint.
* `perft <depth>`: <a href="https://www.chessprogramming.org/Perft">Counts all leaf nodes</a> to the specified depth from the current position.
* `moves [<move>]*`: Make the specified list of moves on the current position.
* `d`: Print the current position as an ASCII board.
* `getposition`: Print the current position as a UCI command.
* `dumpposition`: Dumps internal position structure information.
* `hashstack`: Prints the current hash stack, which is used for repetition detection.
* `xboard`: Switch to CECP mode.

## Non-standard Xboard commands

* `wait`: Waits for the current search to complete before continuing.
* `getposition`: Print the current position as a UCI command.
* `dumpposition`: Dumps internal position structure information.
* `hashstack`: Prints the current hash stack, which is used for repetition detection.
* `uci`: Switch to UCI mode.

## Acknowledgements

* Members of the Stockfish, Engine Programming, and Alpha-Beta discords, for being a helpful and fun community to be a part of.
* The many public available open-source chess engines.
* Authors of <a href="https://github.com/yukarichess/yukari">Yukari</a>, for ideas, hardware, and support.
* Authors of <a href="https://github.com/codedeliveryservice/Reckless">Reckless</a>,
             <a href="https://github.com/JonathanHallstrom/pawnocchio">pawnocchio</a>,
             <a href="https://github.com/cosmobobak/viridithas">viridithas</a>,
             <a href="https://github.com/Ciekce/Stormphrax">Stormphrax</a>,
             <a href="https://github.com/Aethdv/Soul">Soul</a>,
             <a href="https://github.com/official-clockwork/Clockwork">Clockwork</a>, and many others.
* <a href="https://github.com/jw1912">jw1912</a> for <a href="https://github.com/jw1912/bullet">bullet</a>, used to train the NNUE networks.
* The excellent <a href="https://github.com/fmtlib/fmt">{fmt}</a> library, used in this project.
