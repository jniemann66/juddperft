## Synopsis
juddperft is a "perft" (short for "PERF-ormance T-est") utility which is used to count the number of chess positions reachable from a given starting position, after a given number of moves (depth). 

For example, from the normal starting position in chess, white has 20 available moves, resulting in 20 possible positions after white has made the first move. After Black replies, there are 400 possibilities, and after white replies again, there are 8902. We can use a type of short-hand notation to respresent this as: **perft(1)=20, perft(2)=400, perft(3)=8902** and so on ... 

The job of Juddperft is to calculate these perft numbers to an arbitrary depth, by counting all of the legal chess positions reached at a given depth. However, the perft numbers become very large, very quickly, making it a difficult to calculate the higher perft numbers within an acceptable timeframe. (As of this writing, the race is still on to calculate perft(14) ... )

![JuddPerft Screenshot](https://github.com/jniemann66/juddperft/blob/master/screenshot.jpg)  
*Screenshot of juddperft in action, showing the results for a search depth of 8*

## Description of code
juddperft employs a global hashtable, to accelerate the speed of the search, by storing previously reached positions. Since juddperft is multi-threaded, atomic operations are used on hash table entries to eliminate the possibility of race conditions. This is easily manageable using the std::atomic Atomic Operations library in the C++11 standard. Currently, some experimentation with iterative functions, as opposed to recursive, is underway, which would pave the way for future implementation on a GPU.

juddperft needs to be compiled on Visual C++ 2015, as it uses some C++11 features. (Porting to other environments is intended in the future)

## Motivation
juddperft was developed as a by-product of developing my own chess engine. Often, the perft function is used to verify that the move generator (at the heart of any chess program) is correctly generating all legal moves in all situations.
Additionally, I was inspired by other authors of perft utilities, and the sheer processing speed they were able to achieve by using some highly-optimised coding techniques. Although I have not yet surpassed their efforts on a per-thread basis, juddperft is nevertheless quite fast, achieving in excess of 380 Million nodes / second on my aging Lenovo S20 workstation with a W3520 processor, which has four cores and hyperthreading.

## Usage

just run the program as a console application on 64-bit windows. It will accept the following commands:

 * quit
 * setboard FENString
 * memory n
 * cores n
 * movelist
 * showposition
 * showhash
 * perft n
 * perftfast n
 * divide n
 * dividefast n

juddperft defaults to the normal chess starting position.
Other positions can be entered by using the "setboard" command, followed by a FEN string representing the position.
There are three variants on the perft function:
* perftfast - uses the hashtable, but doesn't tally stats on En Passants, captures, Castling etc
* perft - doesn't use hashtable (and therefore slower), but does collect stats
* divide - splits position by legal move, and then does perft on each of those moves
* dividefast - splits position by legal move, and then does perftfast on each of those moves (uses Hash tables)

note: using dividefast instead of perftfast is often faster for large n, because it puts less strain on the hash tables, by splitting the job into a number of perftfast(n-1) 's

