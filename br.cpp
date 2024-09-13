
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <cstring>

#include "normalizer.h"
#include "fvector.h"
#include "bluff.h"

// The code in here is quite complicated. See Appendix B of my thesis. 
// This code implements algorithm 8, "Best Response Algorithm for Bluff and Poker Games", 
// *not* GEBR. The GEBR code is even more complicated. :) 

static bool mccfrAvgFix = false;

using namespace std; 

static vector<unsigned long long> oppChanceOutcomes; // all roll outcomes

// only called at opponent (fixed player) nodes in computeActionDist
// get the information set for update player where the chance outcome is replaced by the specified one
void getInfoset(unsigned long long & infosetkey, Infoset & is, unsigned long long bidseq, int player,  
                int actionshere, int chanceOutcome)
{
  infosetkey = bidseq << iscWidth; 
  infosetkey |= chanceOutcome; 
  infosetkey <<= 1; 
  if (player == 2) infosetkey |= 1; 

  bool ret = false; 

  ret = iss.get(infosetkey, is, actionshere, 0); 

  if (!ret) cerr << "infoset get failed, infosetkey = " << infosetkey << endl;
  assert(ret);  
}

double getMoveProb(Infoset & is, int action, int actionshere)
{ 
  double den = 0.0; 
  
  for (int a = 0; a < actionshere; a++)
    if (is.totalMoveProbs[a] > 0.0)
      den += is.totalMoveProbs[a];

  if (den > 0.0) 
    return (is.totalMoveProbs[action] / den); 
  else
    return (1.0 / actionshere);
}

// This implements the average strategy patch needed by optimisitc averaging, from section 4.4 of my thesis.
// This function should never be called by this code base because optimistic averaging is not included here.
void fixAvStrat(unsigned long long infosetkey, Infoset & is, int actionshere, double myreach)
{
  if (iter > is.lastUpdate)
  {
    for (int a = 0; a < actionshere; a++) 
    {
      double inc =   (iter - is.lastUpdate)
                   * myreach
                   * is.curMoveProbs[a];

      is.totalMoveProbs[a] += inc; 
    }

    iss.put(infosetkey, is, actionshere, 0); 
  }
}


// Compute the weight for this action over all chance outcomes
// Used for determining probability of action
// Done only at fixed_player nodes
void computeActionDist(unsigned long long bidseq, int player, int fixed_player, 
                       NormalizerMap & oppActionDist, int action, FVector<double> & newOppReach, 
                       int actionshere)
{
  double weight = 0.0; 

  // for all possible chance outcomes
  for (unsigned int i = 0; i < oppChanceOutcomes.size(); i++) 
  {
    unsigned int CO = (fixed_player == 1 ? numChanceOutcomes(1) : numChanceOutcomes(2));
    assert(oppChanceOutcomes.size() == CO);

    int chanceOutcome = oppChanceOutcomes[i]; 
  
    // get the information set that corresponds to it
    Infoset is;
    unsigned long long infosetkey = 0; 
    getInfoset(infosetkey, is, bidseq, player, actionshere, chanceOutcome); 

    // apply out-of-date mccfr patch if needed. note: we know it's always the fixed player here
    if (mccfrAvgFix)
      fixAvStrat(infosetkey, is, actionshere, newOppReach[i]); 
   
    double oppProb = getMoveProb(is, action, actionshere); 
    CHKPROB(oppProb); 
    newOppReach[i] = newOppReach[i] * oppProb; 

    weight += getChanceProb(fixed_player, chanceOutcome)*newOppReach[i]; 
  }

  CHKDBL(weight);
  oppActionDist.add(action, weight); 
}

// return the payoff of this game state if the opponent's chance outcome is replaced by specified 
double getPayoff(GameState gs, int fixed_player, int oppChanceOutcome)
{
  int updatePlayer = 3-fixed_player; 
  int & oppRoll = (updatePlayer == 1 ? gs.p2roll : gs.p1roll); 
  oppRoll = oppChanceOutcome;

  return payoff(gs, updatePlayer); 
}

double expectimaxbr(GameState gs, unsigned long long bidseq, int player, int fixed_player, int depth, FVector<double> & oppReach)
{
  assert(fixed_player == 1 || fixed_player == 2); 

  int updatePlayer = 3-fixed_player;

  // opponent never players here, don't choose this!
  if (player == updatePlayer && oppReach.allEqualTo(0.0))
    return NEGINF;
  
  if (terminal(gs))
  {
    if (oppReach.allEqualTo(0.0))
      return NEGINF;

    NormalizerVector oppDist; 
  
    for (unsigned int i = 0; i < oppChanceOutcomes.size(); i++) 
      oppDist.push_back(getChanceProb(fixed_player, oppChanceOutcomes[i])*oppReach[i]); 

    oppDist.normalize(); 

    double expPayoff = 0.0; 

    for (unsigned int i = 0; i < oppChanceOutcomes.size(); i++) 
    {
      double payoff = getPayoff(gs, fixed_player, oppChanceOutcomes[i]); 

      CHKPROB(oppDist[i]); 
      CHKDBL(payoff); 

      expPayoff += (oppDist[i] * payoff); 
    }

    return expPayoff; 
  }
  
  // check for chance node
  if (gs.p1roll == 0) 
  {
    // on fixed player chance nodes, just use any die roll
    if (fixed_player == 1) 
    {
      GameState ngs = gs; 
      ngs.p1roll = 1;  // assign a dummy outcome, never used

      FVector<double> newOppReach = oppReach; 
      return expectimaxbr(ngs, bidseq, player, fixed_player, depth+1, newOppReach);
    }
    else
    {
      double EV = 0.0; 

      for (int i = 1; i <= numChanceOutcomes(1); i++) 
      {
        GameState ngs = gs; 
        ngs.p1roll = i; 

        FVector<double> newOppReach = oppReach; 
        EV += getChanceProb(1,i) * expectimaxbr(ngs, bidseq, player, fixed_player, depth+1, newOppReach);
      }

      return EV;
    }
  }
  else if (gs.p2roll == 0)
  {
    // on fixed player chance nodes, just use any die roll
    if (fixed_player == 2)
    {
      GameState ngs = gs; 
      ngs.p2roll = 1; // assign a dummy outcome, never used

      FVector<double> newOppReach = oppReach; 
      return expectimaxbr(ngs, bidseq, player, fixed_player, depth+1, newOppReach);
    }
    else
    {
      double EV = 0.0; 

      for (int i = 1; i <= numChanceOutcomes(2); i++)
      {
        GameState ngs = gs; 
        ngs.p2roll = i; 
        
        FVector<double> newOppReach = oppReach; 
        EV += getChanceProb(2,i) * expectimaxbr(ngs, bidseq, player, fixed_player, depth+1, newOppReach);
      }

      return EV;
    }
  }

  // declare variables and get # actions available
  double EV = 0.0; 
  
  int maxBid = (gs.curbid == 0 ? BLUFFBID-1 : BLUFFBID);
  int actionshere = maxBid - gs.curbid; 
  assert(actionshere > 0);

  // iterate over the moves 
  double maxEV = NEGINF;
  double childEVs[actionshere];
  int action = -1;
  NormalizerMap oppActionDist;

  for (int i = gs.curbid+1; i <= maxBid; i++) 
  {
    action++;    

    double childEV = 0;
    FVector<double> newOppReach = oppReach;
      
    if (player == fixed_player) 
      computeActionDist(bidseq, player, fixed_player, oppActionDist, action, newOppReach, actionshere); 

    // state transition + recursion
    GameState ngs = gs; 
    ngs.prevbid = gs.curbid;
    ngs.curbid = i; 
    ngs.callingPlayer = player;
    unsigned long long newbidseq = bidseq; 
    newbidseq |= (1ULL << (BLUFFBID-i)); 

    childEV = expectimaxbr(ngs, newbidseq, 3-player, fixed_player, depth+1, newOppReach);
    
    // post recurse
    if (player == updatePlayer) 
    {
      // check if higher than current max
      if (childEV >= maxEV)
      {
        maxEV = childEV;
      }
    }
    else if (player == fixed_player)
    {
      assert(action >= 0 && action < actionshere);
      childEVs[action] = childEV; 
    }
  }

  // post move iteration
  if (player == updatePlayer)
  {
    EV = maxEV;
  }
  else if (player == fixed_player)
  {
    assert(static_cast<int>(oppActionDist.size()) == actionshere);
    oppActionDist.normalize(); 
    
    for (int i = 0; i < actionshere; i++) 
    {
      CHKPROB(oppActionDist[i]); 
      CHKDBL(childEVs[i]); 

      EV += (oppActionDist[i] * childEVs[i]); 
    }
    
  }

  assert(updatePlayer != fixed_player); 
  assert(updatePlayer + fixed_player == 3); 

  return EV; 
}

double computeBestResponses(bool avgFix)
{
  double p1value = 0.0;
  double p2value = 0.0;
  return computeBestResponses(avgFix, p1value, p2value);
}

double computeBestResponses(bool avgFix, double & p1value, double & p2value)
{
  mccfrAvgFix = avgFix;

  cout << "Computing ISS bounds... "; cout.flush(); 
  double b1 = 0.0, b2 = 0.0;
  iss.computeBound(b1, b2); 
  cout << " b1 = " << b1 << ", b2 = " << b2 << ", bound = " << (2.0*MAX(b1,b2)) << endl;
  
  // fill chance outcomes for player 1
  oppChanceOutcomes.clear();
  for (int i = 1; i <= numChanceOutcomes(1); i++)
  {
    oppChanceOutcomes.push_back(i); 
  }

  cout << "Running best response, fp = 1 ... "; cout.flush(); 

  StopWatch sw; 

  GameState gs1; unsigned long long bidseq = 0; 
  FVector<double> reach1(numChanceOutcomes(1), 1.0); 
  p2value = expectimaxbr(gs1, bidseq, 1, 1, 0, reach1);

  cout << "time taken: " << sw.stop() << " seconds." << endl; 
  cout.precision(15);
  cout << "p2value = " << p2value << endl; 

  // fill chance outcomes for player 2
  oppChanceOutcomes.clear();
  for (int i = 1; i <= numChanceOutcomes(2); i++)
  {
    oppChanceOutcomes.push_back(i); 
  }

  cout << "Running best response, fp = 2 ... "; cout.flush(); 

  sw.reset(); 

  GameState gs2; bidseq = 0;
  FVector<double> reach2(numChanceOutcomes(2), 1.0); 
  p1value = expectimaxbr(gs2, bidseq, 1, 2, 0, reach2);
 
  cout << "time taken: " << sw.stop() << " seconds." << endl; 
  cout.precision(15);
  cout << "p1value = " << p1value << endl; 

  double conv = p1value + p2value; 

  cout.precision(15);
  cout << "iter = " << iter << " nodes = " << nodesTouched << " conv = " << conv << endl; 

  return conv;
}



