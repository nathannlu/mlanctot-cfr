
#include <iostream>
#include <cassert>
#include <cstdlib>

#include "bluff.h"

// opponent sampling

using namespace std; 

static string runname = "";

// This one is slightly different than the algorithm presented in the thesis; it still implements
// the alternating form, but this one uses stochastically-weighted averaging.

double cfros(GameState & gs, int player, int depth, unsigned long long bidseq, 
              double reach1, double reach2, double sprob1, double sprob2, int updatePlayer, 
              double & suffixreach, double & rtlSampleProb)
{
  CHKPROB(reach1); 
  CHKPROB(reach2); 

  // check: at terminal node?
  if (terminal(gs))
  {
    suffixreach = 1.0; 
    rtlSampleProb = sprob1*sprob2;
    
    return payoff(gs, updatePlayer); 
  }
  
  nodesTouched++;

  // chance nodes
  if (gs.p1roll == 0) 
  {
    GameState ngs = gs; 
    double sampleProb;
    sampleChanceEvent(1, ngs.p1roll, sampleProb);

    // don't need to worry about keeping track of sampled chance probs for outcome sampling
    return cfros(ngs, player, depth+1, bidseq, reach1, reach2, sprob1, sprob2, updatePlayer, suffixreach, rtlSampleProb); 
  }
  else if (gs.p2roll == 0)
  {
    GameState ngs = gs; 
    double sampleProb;
    sampleChanceEvent(2, ngs.p2roll, sampleProb);
    
    // don't need to worry about keeping track of sampled chance probs for outcome sampling
    return cfros(ngs, player, depth+1, bidseq, reach1, reach2, sprob1, sprob2, updatePlayer, suffixreach, rtlSampleProb); 
  }

  // declare the variables
  Infoset is;
  unsigned long long infosetkey = 0;
  int action = -1;

  // count the number of actions. Can't call Bluff as first action when there is no bid
  int maxBid = (gs.curbid == 0 ? BLUFFBID-1 : BLUFFBID);
  int actionshere = maxBid - gs.curbid; 
  assert(actionshere > 0);
  
  // get the info set (also set is.curMoveProbs using regret matching)
  getInfoset(gs, player, bidseq, is, infosetkey, actionshere); 

  // sample the action to take. Epsilon on-policy at my nodes. On-Policy at opponents.

  double sampleprob = -1; 
  int takeAction = 0; 
  double newsuffixreach = 1.0; 

  if (player == updatePlayer)
    takeAction = sampleAction(is, actionshere, sampleprob, 0.6, false); 
  else
    takeAction = sampleAction(is, actionshere, sampleprob, 0.0, false); 

  CHKPROBNZ(sampleprob); 
  double newsprob1 = (player == 1 ? sampleprob*sprob1 : sprob1); 
  double newsprob2 = (player == 2 ? sampleprob*sprob2 : sprob2); 

  double ctlReach = 0;
  double itlReach = 0; 
  double updatePlayerPayoff = 0;

  // take the action. find the i for this action
  int i;
  for (i = gs.curbid+1; i <= maxBid; i++) 
  {
    action++; 

    if (action == takeAction)
      break; 
  }
  
  assert(i >= gs.curbid+1 && i <= maxBid); 
  double moveProb = is.curMoveProbs[action]; 

  // do this only once

  unsigned long long newbidseq = bidseq;
  GameState ngs = gs; 
  ngs.prevbid = gs.curbid;
  ngs.curbid = i; 
  ngs.callingPlayer = player;
  newbidseq |= (1 << (BLUFFBID-i)); 
  
  CHKPROB(moveProb); 
  double newreach1 = (player == 1 ? moveProb*reach1 : reach1); 
  double newreach2 = (player == 2 ? moveProb*reach2 : reach2); 

  updatePlayerPayoff = cfros(ngs, 3-player, depth+1, newbidseq, newreach1, newreach2, newsprob1, newsprob2, 
                             updatePlayer, newsuffixreach, rtlSampleProb);

  ctlReach = newsuffixreach; 
  itlReach = newsuffixreach*is.curMoveProbs[action]; 
  suffixreach = itlReach;

  // payoff always in view of update player
  double myreach = (player == 1 ? reach1 : reach2); 
  double oppreach = (player == 1 ? reach2 : reach1); 
 
  // update regrets (my infosets only)
  if (player == updatePlayer) 
  { 
    for (int a = 0; a < actionshere; a++)
    {
      // oppreach and sprob2 cancel in the case of stochastically-weighted averaging
      //is.cfr[a] += (moveEVs[a] - stratEV); 
    
      double U = updatePlayerPayoff * oppreach / rtlSampleProb;
      double r = 0.0; 
      if (a == takeAction) 
        r = U * (ctlReach - itlReach);
      else 
        r = -U * itlReach;

      is.cfr[a] += r; 
    }
  }
 
  if (player != updatePlayer) { 
    // update av. strat
    for (int a = 0; a < actionshere; a++)
    {
      // stochastically-weighted averaging
      double inc = (1.0 / (sprob1*sprob2))*myreach*is.curMoveProbs[a];
      is.totalMoveProbs[a] += inc; 
    }
  }

  // save the infoset back to the store

  iss.put(infosetkey, is, actionshere, 0); 

  return updatePlayerPayoff;
}

int main(int argc, char ** argv)
{
  unsigned long long maxIters = 0; 
  unsigned long long maxNodesTouched = 0; 
  init();

  if (argc < 2)
  {
    initInfosets();
    cout << "Created infosets. Exiting." << endl;
    exit(0);
  }
  else
  {
    cout << "Reading infosets from " << argv[1] << endl;
    if (!iss.readFromDisk(argv[1]))
    {
      cerr << "Problem reading file. " << endl; 
      exit(-1); 
    }

    if (argc >= 3) 
      runname = argv[2];
    else
      runname = "bluff11";
  }

  // get the iteration
  string filename = argv[1];
  vector<string> parts; 
  split(parts, filename, '.'); 
  if (parts.size() != 3 || parts[1] == "initial")
    iter = 1; 
  else
    iter = to_ull(parts[1]); 
  cout << "Set iteration to " << iter << endl;
  iter = MAX(1,iter);
    
  // try loading metadeta. files have form scratch/iss-runname.iter.dat
  if (iter > 1) { 
    string filename2 = filename;
    replace(filename2, "iss", "metainfo"); 
    cout << "Loading metadata from file " << filename2 << "..." << endl;
    loadMetaData(filename2); 
  }
  

  unsigned long long bidseq = 0; 
    
  double totaltime = 0; 
  StopWatch stopwatch;

  for (; true; iter++)
  {
    GameState gs1; bidseq = 0;
    double suffixreach = 1.0; 
    double rtlSampleProb = 1.0; 
    cfros(gs1, 1, 0, bidseq, 1.0, 1.0, 1.0, 1.0, 1, suffixreach, rtlSampleProb);
    
    GameState gs2; bidseq = 0;
    suffixreach = 1.0; 
    rtlSampleProb = 1.0; 
    cfros(gs1, 1, 0, bidseq, 1.0, 1.0, 1.0, 1.0, 2, suffixreach, rtlSampleProb);

    if (   (maxNodesTouched > 0 && nodesTouched >= maxNodesTouched)
        || (maxNodesTouched == 0 && nodesTouched >= ntNextReport))
    {
      totaltime += stopwatch.stop();

      double nps = nodesTouched / totaltime; 
      cout << endl;      
      cout << "total time: " << totaltime << " seconds. " << endl;
      cout << "nodes = " << nodesTouched << endl;
      cout << "nodes per second = " << nps << endl; 
      
      // again the bound here is weird for sampling versions
      double b1 = 0.0, b2 = 0.0;
      iss.computeBound(b1, b2); 
      cout << "b1 = " << b1 << ", b2 = " << b2 << ", sum = " << (b1+b2) << endl;

      ntNextReport *= ntMultiplier; // need this here, before dumping metadata

      double conv = 0;
      conv = computeBestResponses(false);

      //cout << "**alglne: iter = " << iter << " nodes = " << nodesTouched << " conv = " << conv << " time = " << totaltime << endl; 

      string str = "cfros." + runname + ".report.txt"; 
      report(str, totaltime, 2.0*MAX(b1,b2), conv);
      //dumpInfosets("iss-" + runname); 
      //dumpMetaData("metainfo-" + runname, totaltime); 
      
      cout << "Report done at: " << getCurDateTime() << endl;

      cout << endl;
      
      stopwatch.reset(); 
    
      if (maxNodesTouched > 0 && nodesTouched >= maxNodesTouched) 
        break;
    
      if (iter == maxIters) break;
    }
  }
  
  return 0;
}

