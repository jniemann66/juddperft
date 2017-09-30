## Synopsis
juddperft is a "perft" (short for "PERF-ormance T-est") utility, which is used to count the number of chess positions reachable from a given starting position, after a given number of moves (depth). 

For example, from the normal starting position in chess, white has **20** available moves, resulting in **20** possible positions after white has made the first move. After Black replies, there are **400** possibilities, and after white replies again, there are **8902**. We can use a type of short-hand notation to respresent this: **perft(1)=20, perft(2)=400, perft(3)=8902** and so on ... 

The job of Juddperft is to calculate these perft numbers to an arbitrary depth, by counting all of the legal chess positions reached at a given depth. However, the perft numbers become very large, very quickly, making it difficult to calculate the higher perft numbers within an acceptable timeframe.

![JuddPerft Screenshot](https://github.com/jniemann66/juddperft/blob/master/screenshot.jpg)  
*Screenshot of juddperft in action, showing the results for a search depth of 8*

## Description of code
Chess positions are represented internally as a set of *four* 64-bit "bitBoards", allowing each of the 64 squares on the chess board to be populated by one of 16 possible values, representing the piece (or absence of a piece) occupying each square, and it's colour. 

In dealing with bitboards, there is inevitably a large amount of "bit-twiddling" involved (lots of ANDs / ORs / XORs / ones-complements / bit-shifts etc). This is pretty standard for chess programs.  


juddperft employs two global lock-free hashtables (one for leaf nodes, and the other for "branch" nodes), to accelerate the speed of the search, by storing previously reached positions. Since juddperft is multi-threaded, atomic operations are used on hash table entries to eliminate the possibility of race conditions. This is managed using the **std::atomic** Atomic Operations library in the **C++11** standard.

juddperft was originally compiled on Visual C++ 2015, but work is underway to get it performing well on gcc and clang.

*note: Some of the source files in this project look a little bare, and this is because they have been stripped-down from the full chess-engine, leaving only the perft-related code.*

## Motivation
juddperft was developed as a by-product of developing my own chess engine. Often, the perft function is used to verify that the move generator (at the heart of any chess program) is correctly generating all legal moves in all situations. However, this project is also a vehicle for testing out various parallel processing techniques, and for optimising the various algorthims used to make the program run as fast as possible.

I have been inspired by other authors of perft utilities, and the sheer processing speed they were able to achieve by using some highly-optimised coding techniques. Although I have not yet surpassed their efforts on a per-thread basis, juddperft is nevertheless quite fast, achieving in excess of 380 Million nodes / second on my aging Lenovo S20 workstation with a W3520 processor, which has four cores and hyperthreading.

## Usage

JuddPerft is an interactive console application (since it is based on a stripped-down xboard / winboard engine).
Run the program as a console application, and enter commands as required. 
Accepted commands are as follows:

**perft &lt;depth&gt;**

**perftfast &lt;depth&gt;**

**divide &lt;depth&gt;**

**dividefast &lt;depth&gt;**

**setboard &lt;FENString&gt;**

**memory &lt;bytes&gt;** - attempt to (re)allocate *bytes* bytes of memory for the hashtables

**cores &lt;n&gt;** - use n threads for calculations

**movelist** - display a list of legal moves from the current position

**showposition** - display an ascii diagram representing the current position

**showhash** - display hash table statistics

**text-external** &lt;path to external app&gt; &lt;depth&gt;

**quit** - exit the app

juddperft defaults to the normal chess starting position.
You can start calculating immediately by typing perftfast &lt;depth&gt; eg:

**perftfast 7**

Other positions can be entered by using the **setboard** command, followed by a FEN string representing the position.
There are four variants on the perft function:

* **perftfast** - uses the hashtable, but doesn't tally stats on En Passants, captures, Castling etc
* **perft** - doesn't use hashtable (and therefore slower), but does collect stats
* **divide** - splits position by legal move, and then does perft on each of those moves
* **dividefast** - splits position by legal move, and then does perftfast on each of those moves (uses Hash tables)

note: using dividefast instead of perftfast is often faster for large n, because it puts less strain on the hash tables, by splitting the job into a number of perftfast(n-1) 's

## Validating against an external engine

juddperft can check its own calculations (from the current position) against another engine, to verify whether juddperft is wrong, or the other engine is wrong (or possibly both !)

It uses an automated divide-and-conquer approach to attempt zoom-in on exactly where the engines disagree.

The command is:

**text-external** &lt;path to external app&gt; &lt;depth&gt;

This will issue the following system command for each test position:
**&lt;external app&gt; "&lt;Fen String&gt;" &lt;depth&gt; &lt;perft value&gt;**

*external app* is expected to terminate with EXIT_SUCCESS (0) if it agrees with
the perft value for the given position.

It is up to the external engine to accept the 3 parameters (fen-string, depth, value) and return 0 if it agrees or any other value if it disagrees.

### description of algorithm used for test-external

Lets say you a position for which the two engines disagree on perft 5.
The test-external process might proceed like this:

~~~
starting position: (we want to test test perft 5, white to move)
    make white move 1 : get perft 4 (ok ; engines agree)
    make white move 2 : get perft 4 (ok ; engines agree)
    make white move 3 : get perft 4 (PROBLEM ! engines disagree )
        make black move 1 : get perft 3 (ok ; engines agree)
        make black move 2 : get perft 3 (ok ; engines agree)
        make black move 3 : get perft 3 (ok ; engines agree)
        ...
        make black move 15: get perft 3 (PROBLEM ! engines disagree )
            make white move 1: get perft 2 (ok ; engines agree)
            make white move 2: get perft 2 (ok ; engines agree)
            ...
            make white move 5: get perft 2 (PROBLEM ! engines disagree )
                make black move 1 : get perft 1 (ok ; engines agree)
                ...
                make black move 7 : get perft 1 (PROBLEM ! engines disagree) <= this is where the problem is - one (or both !) of the engines generated wrong number of moves

~~~


## Additional Information

The known perft numbers up to 15 are as follows:

	0	1
	1	20
	2	400
	3	8,902
	4	197,281
	5	4,865,609
	6	119,060,324
	7	3,195,901,860
	8	84,998,978,956
	9	2,439,530,234,167
	10	69,352,859,712,417
	11	2,097,651,003,696,806
	12	62,854,969,236,701,747
	13	1,981,066,775,000,396,239

*--- note: more than 64 bits needed beyond this point ---*

	14	61,885,021,521,585,529,237
	15	2,015,099,950,053,364,471,960

Note: The perft numbers have an entry in the [On-line Encycolpedia of Integer Sequences](https://oeis.org/A048987)

Additional information can be found on the [Chess Programming Wiki](https://chessprogramming.wikispaces.com/Perft "Chess Programming Wiki")

## build commands

*GCC 5.4 ; kubuntu 16.04*

~~~
g++ -pthread -std=c++11 *.cpp -o ./juddperft-gcc -latomic -O3
~~~
