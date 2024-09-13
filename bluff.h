
#ifndef __BLUFF_H__
#define __BLUFF_H__

#include <sys/timeb.h>

#include <cmath>
#include <string>
#include <limits>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <iostream>

#include "infosetstore.h"

#define P1DICE   1
#define P2DICE   1
#define DIEFACES 6

// The number assigned to the "calling bluff action".
// Also an upper bound for |A(I)|, equal to 13 in Bluff(1,1)
#define BLUFFBID (((P1DICE+P2DICE)*DIEFACES)+1)

#include "defs.h"

struct GameState
{
  int p1roll;           // the outcome of p1's roll
  int p2roll;           // the outcome of p2's roll
  int curbid;           // current bid (between 1 and 13)
  int prevbid;          // prev bid from last turn
  int callingPlayer;    // the player calling bluff (1 or 2)

  GameState()
  {
    p1roll = p2roll = curbid = prevbid = 0;
    callingPlayer = 0;
  }
};

struct Infoset
{
  double cfr[BLUFFBID];
  double totalMoveProbs[BLUFFBID];
  double curMoveProbs[BLUFFBID];

  int actionshere;
  unsigned long long lastUpdate;
};

// game-specific function defs (implemented in bluff.cpp)
bool terminal(GameState & gs);
double payoff(GameState & gs, int player);
double payoff(int bid, int bidder, int callingPlayer, int p1roll, int p2roll, int player);
int whowon(GameState & gs);
int whowon(GameState & gs, int & delta);
int whowon(int bid, int bidder, int callingPlayer, int p1roll, int p2roll, int & delta);
void init();
double getChanceProb(int player, int outcome);
void convertbid(int & dice, int & face, int bid);
int countMatchingDice(const GameState & gs, int player, int face);
void getRoll(int * roll, int chanceOutcome, int player);  // currently array must be size 3 (may contain 0s)
int numChanceOutcomes(int player);

// util function defs (implemented in util.cpp)
std::string to_string(double i);
std::string to_string(unsigned long long i);
std::string to_string(int i);
int to_int(std::string str);
unsigned long long to_ull(std::string str);
double to_double(std::string str);
void getSortedKeys(std::map<int,bool> & m, std::list<int> & kl);
void split(std::vector<std::string> & tokens, const std::string line, char delimiter);
unsigned long long pow2(int i);
void bubsort(int * array, int size);
std::string infosetkey_to_string(unsigned long long infosetkey);
bool replace(std::string& str, const std::string& from, const std::string& to);
std::string getCurDateTime();
void seedCurMicroSec();
double unifRand01();

// solver-specific function defs
void newInfoset(Infoset & is, int actionshere);
unsigned long long getInfosetKey(GameState & gs, int player, unsigned long long bidseq);
void getInfoset(GameState & gs, int player, unsigned long long bidseq, Infoset & is, unsigned long long & infosetkey, int actionshere);
void initInfosets();
void initSeqStore();
void allocSeqStore();
double computeBestResponses(bool avgFix);
double computeBestResponses(bool avgFix, double & p1value, double & p2value);
void report(std::string filename, double totaltime, double bound, double conv);
void dumpInfosets(std::string prefix);
void dumpSeqStore(std::string prefix);
void dumpMetaData(std::string prefix, double totaltime);
void loadMetaData(std::string file);
double getBoundMultiplier(std::string algorithm);
double evaluate();
unsigned long long absConvertKey(unsigned long long fullkey);
void setBRTwoFiles();
void estimateValue();
void loadUCTValues(Infoset & is, int actions);
void saveUCTValues(Infoset & is, int actions);
void UCTBR(int fixed_player);
void fsiBR(int fixed_player);
double absComputeBestResponses(bool abs, bool avgFix, double & p1value, double & p2value);
double absComputeBestResponses(bool abs, bool avgFix);

// sampling (impl in sampling.cpp)
void sampleMoveAvg(Infoset & is, int actionshere, int & index, double & prob);
void sampleChanceEvent(int player, int & outcome, double & prob);
void sampleMoveAvg(Infoset & is, int actionshere, int & index, double & prob);
int sampleAction(Infoset & is, int actionshere, double & sampleprob, double epsilon, bool firstTimeUniform);

// global variables
class InfosetStore;
extern InfosetStore iss;                 // the strategies are stored in here (for both players)
extern int iscWidth;                     // number of bits for chance outcome
extern unsigned long long iter;          // the current iteration
extern std::string filepref;             // prefix path for saving files
extern double cpWidth;                   // used for timing/stats
extern double nextCheckpoint;            // used for timing/stats
extern unsigned long long ntNextReport;  // used for timing/stats
extern unsigned long long ntMultiplier;  // used for timing/stats
extern unsigned long long nodesTouched;  // used for timing/stats

class StopWatch
{
  timeb tstart, tend;
public:
  StopWatch() { ftime(&tstart); }
  void reset() { ftime(&tstart); }
  double stop()
  {
    ftime(&tend);
    return ((tend.time*1000 + tend.millitm)
            - (tstart.time*1000 + tstart.millitm) ) / 1000.0;
  }
};

#endif

