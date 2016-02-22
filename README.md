# juddperft - perft utility to count the number of chess positions at a given depth (number of plys)
uses features from c++11, and requires VC++ 2015 to compile
Experimenting with concurrency, a global atomic hashtable, and recursive vs iterative techniques
Move generation is implemented with bitboards

run this program from console.
Once running, app will accept the following commands:

Supported Commands:
   quit
   setboard <FEN position>
   memory <size in bytes>
   cores <number of THREADS to use>
   movelist
   showposition
   showhash 
   perft <n>
   perftfast <n>
   divide <n>

defaults to normal chess starting position
perftfast uses hashtable, but doesn't tally stats on checks, EPs, captures etc
perft doesn't use hashtable (and therefore slower), but collects stats
divide splits position by legal move, and then does perft on each of those moves
