
Marc Lanctot, Dec 18th, 2012
Questions? Comments? Contact me at marc.lanctot@gmail.com

Here's the part where I say you can have the code for free with the 
understanding that if your computer explodes as a result of using it (or 
something equally as drastic) then it's not my fault, etc. etc.. Consider 
yourself warned!

This code implements the following algorithms: 

  - Vanilla CFR                    (cfr)
  - Chance-sampled CFR             (cfrcs)
  - Oucome Sampling MCCFR          (cfros)
  - External Sampling MCCFR        (cfres)
  - Public Chance Sampling         (pcs)
  - Pure CFR                       (purecfr)
  - Expectimax-based best response

on the game of Bluff(1,1). It follows the descriptions and algorithms in my
thesis quite closely, so consider Chapter 4 of my thesis as the manual for 
this package. Here's a direct link to it... ! :) 

   http://mlanctot.info/files/papers/PhD_Thesis_MarcLanctot.pdf
  
This code has been compiled (using g++) and tested using on Linux, MacOS, and 
Windows (under Cygwin using its g++ and Code::Blocks using the MinGW's g++ that
comes with Code::Blocks). Please report any problems you have building or 
running.

Compiling & Building
====================

   To compile, type 'make' at the command-line. That's it. 
   You may want to customize some of the options in the Makefile.

   Before running anything, make sure you have a subdirectory called scratch/ 
   --> All the report and data files will get dumped into this directory.

   To run one of the algorithms:

     1. Run ./<alg> with no command-line arguments, where <alg> is one of 
        the algorithms above. This will create the initial strategies file:
        scratch/iss.initial.dat.

     2. Then run ./<alg> scratch/iss.initial.dat
        Stats and convergence reports will be sent to: 
        scratch/<alg>.bluff11.report.txt
        (conv is sum of the best response values; at equilibrium this value
         would be zero)


Bluff(1,1): 
===========

Copied almost verbatim from my thesis chapter 3:

Bluff, also known as Liar's Dice, Dudo, and Perudo, is a dice-bidding game. In
our version, Bluff(D1,D2), each die has six sides with faces 1, 2, 3, 4, 5, *.  
Each player i rolls Di of these dice and looks at them without showing them to 
their opponent. Each round, players alternate by bidding on the outcome of all
dice in play until one player "calls bluff", i.e. claims that their opponent's 
latest bid does not hold. A bid consists of a quantity of dice and a face value.  
A face of * is considered wild and counts as matching any other face. For example, 
the bid 2-5 represents the claim that there are at least two dice with a face of 
5 or * among both players' dice. To place a new bid, the player must increase 
either the quantity or face value of the current bid (or both); in addition, 
lowering the face is allowed if the quantity is increased. The losing player 
removes a number of dice L from the game and a new round begins, starting with 
the player who won the previous round. The number of dice removed is equal to the 
difference in the number of dice claimed and the number of actual matches; 
in the case where the bid is exact, the calling player loses a single die (note:
in (1,1) the number of dice lost is always 1, but I left in the full rules for
the interested reader). When a player has no more dice left, they have lost the 
game. A utility of +1 is given for a win and -1 for a loss.

For more info, http://en.wikipedia.org/wiki/Liar%27s_dice

Code Overview
=============

Most of the code is sufficiently documented, so here I'll just give a summary
of the Big Picture (tm). 

- bluffcounter.cpp is used to count the number of information sets and 
  (infoset,action) pairs. These numbers are needed to create the strategies 
  files. You won't need to use it at all if you just run Bluff(1,1) but if you 
  want to implement a different game, then you can use this file as a place to 
  start to create strategies files for your game.

- bluff.cpp constains all the game-specific code pertaining to Bluff and some
  common functions to the solvers. The Bluff code is more general than it needs
  to be for (1,1) because I am using it for larger games of Bluff. You shouldn't
  need to look through this file much unless you're implementing your own game.

- infosetstore.{h,cpp} contains the strategies data structures. The strategies 
  files are hash tables that use linear probing for collision avoidance. In 
  Bluff(1,1), the key for an entry is based on the information set. Each entry
  corresponds to an information set, storing 2 doubles + 2 doubles for each 
  action available at that infoset (one for r_I[a] and one for s_I[a]). The 
  extra 2 doubles per infoset are used for storing the number of actions and 
  the c_I time stamp when using optimistic averaging. Both are unnecessary for
  this particular code base (firstly, given the sequence you can always compute
  the number of actions available, and secondly optimistic averaging is not 
  appearing in this film) but I left them in there as I suspect most games will 
  need some metadata at each infoset so you have the basic structure in place.

The rest should mostly be obvious from the comments in the code. If anything 
seems weird, do not hesitate to email me.

I recommend you look at the algorithms in this order: 

   cfr -> cfrcs -> other sampling versions


