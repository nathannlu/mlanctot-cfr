
#include <iostream>
#include <map>
#include <cassert>
#include <cmath>

using namespace std;

// How many dice per player?
#define P1D   1
#define P2D   1
#define DICE  (P1D+P2D)
#define BIDS  (DICE*6)

// Number of last moves you're allowed to recall for imperfect recall
// Used only for imperfect recall abstractions, not used by this code base
#define RECALL 7

// Number of chance event outcomes at chance nodes.
#define P1CO 6
#define P2CO 6

unsigned long p1totalTurns = 0;
unsigned long p2totalTurns = 0;
unsigned long p1totalActions = 0;
unsigned long p2totalActions = 0;
unsigned long totalLeaves = 0;

map<unsigned long long,int> iapairs;

int iscWidth = 3;

string binrep(unsigned long long num) 
{
  string s = "";
  for (int i = 0; i < 64; i++) 
  {
    unsigned long long bit = 1; 
    bit <<= i;
    unsigned long long n = num & bit; 
    s = (n > 0 ? "1" : "0") + s; 
  }

  return s;
}

unsigned long long pow2(int i)
{
  unsigned long long answer = 1;
  return (answer << i); 
}

// Traverse the entire tree, counting the number of (information set, action pairs)
void bcount(int cbid, int player, int depth)
{
  if (cbid >= (BIDS+1))
  {
    totalLeaves++;
    return;
  }

  int numbids = (cbid == 0 ? BIDS : BIDS+1);
  
  int actions = numbids - (cbid+1) + 1;
  unsigned long & totalTurns = (player == 1 ? p1totalTurns : p2totalTurns); 
  unsigned long & totalActions = (player == 1 ? p1totalActions : p2totalActions); 
  totalTurns++; 
  totalActions += actions; 

  for (int b = cbid+1; b <= numbids; b++)
  {
    if (depth == 0)
      cout << "Depth 0, b = " << b << endl;

    bcount(b, 3-player, depth+1);
  }
}

void countnoabs()
{
  bcount(0, 1, 0); 

  cout << "for public tree:" << endl;
  cout << "  p1totalTurns = " << p1totalTurns << endl;
  cout << "  p2totalTurns = " << p2totalTurns << endl;
  cout << "  totalLeaves = " << totalLeaves << endl;
  unsigned long long totalseqs = (p1totalTurns + p2totalTurns);
  cout << "total sequences = " << totalseqs << endl;
  cout << "p1 info sets = " << (p1totalTurns*P1CO) << endl;
  cout << "p2 info sets = " << (p2totalTurns*P2CO) << endl;
  unsigned long long ttlinfosets = (p1totalTurns*P1CO) + (p2totalTurns*P2CO);
  cout << "total info sets = " << ttlinfosets << endl;
  cout << "p1totalActions = " << p1totalActions << endl;
  cout << "p2totalActions = " << p2totalActions << endl;


  unsigned long iapairs1 = (p1totalActions*P1CO);
  unsigned long iapairs2 = (p1totalActions*P2CO);
  unsigned long totaliap = (iapairs1+iapairs2);
  cout << "infoset actions pairs, p1 p2 total = " 
       << iapairs1 << " " << iapairs2 << " " << totaliap << endl;

  unsigned long td = (totaliap*2 + ttlinfosets*2);
  cout << "total doubles needed = " << td
       << ", bytes needed = " << (td*8) << endl;

  cout << "cache size ~ " << (ttlinfosets*4) << " entries =~ " << (ttlinfosets*4*2)*8 << " bytes" << endl;

  unsigned long long ttlbytes = ((td*8) + (ttlinfosets*4*2)*8); 
  cout << "total bytes = " << ttlbytes << " ( = " << (ttlbytes / (1024.0*1024.0*1024.0)) << " GB)" << endl;
}

bool nextbid(int * bidseq, int maxBid)
{
  for (int i = RECALL-1; i >= 0; i--)
  {
    int roffset = (RECALL-1)-i; 

    if (bidseq[i] < (maxBid-roffset))
    {
      bidseq[i]++;
      
      for (int j = i+1; j < RECALL; j++)
      {
        bidseq[j] = bidseq[j-1]+1;
      }

      return true;
    }
  }

  return false;
}


// This is for imperfect recall abstractions. Not used by this code base, 
// but I left it in case your game has abstraction, maybe this will be helpful
void countabs2()
{
  int bidseq[RECALL] = { 0 }; 
  bool ret = true;
  bool first = true;
  int maxBid = 6*DICE;
  int bluffBid = maxBid+1;

  while(ret)
  {
    int numzeroes = 0;
    for (int i = 0; i < RECALL; i++) 
      if (bidseq[i] == 0)
        numzeroes++; 

    int curbid = bidseq[RECALL-1];
    int actions = bluffBid-curbid;
    if (first) actions--;

    assert(actions >= 0 && actions <= maxBid);
    
    int player = 0;
    if (numzeroes > 0) 
    {
      if (RECALL % 2 == 0)
      {
        if (numzeroes % 2 == 0) 
          player = 1;
        else
          player = 2; 
      }
      else
      {
        if (numzeroes % 2 == 1) 
          player = 1;
        else
          player = 2; 
      }
    }
    else if (bidseq[0] == 1)
    {
      if (RECALL % 2 == 0)
        player = 1; 
      else
        player = 2;
    }
   
    // player = 0 means both players can share this bid sequence
    if (player == 1 || player == 0)
    {
      p1totalTurns++;
      p1totalActions += actions;
    }

    if (player == 2 || player == 0)
    {
      p2totalTurns++;
      p2totalActions += actions;
    }

    ret = nextbid(bidseq, maxBid); 
    first = false;
  }

  cout << "recall = " << RECALL << endl;
  cout << "p1totalTurns = " << p1totalTurns << endl;
  cout << "p2totalTurns = " << p2totalTurns << endl;
  cout << "p1totalActions = " << p1totalActions << endl;
  cout << "p2totalActions = " << p2totalActions << endl;
  
  unsigned long iapairs1 = (p1totalActions*P1CO);
  unsigned long iapairs2 = (p1totalActions*P2CO);
  unsigned long totaliap = (iapairs1+iapairs2);

  cout << "ia pairs p1 p2 total = " 
       << iapairs1 << " " << iapairs2 << " " << totaliap << endl;
  
  unsigned long long infosets1 = (p1totalTurns*P1CO);
  unsigned long long infosets2 = (p2totalTurns*P2CO);
  unsigned long long totalInfosets = infosets1 + infosets2; 
  
  unsigned long long btotalindex = totalInfosets*16*4; 
  unsigned long long tdoubles = (totaliap*2 + totalInfosets*2);
  unsigned long long btotalis = tdoubles*8;

  cout << "infosets p1 p2 total = "
       << infosets1 << " " << infosets2 << " " << totalInfosets << endl;


  cout << "total doubles for infoset store (2 per infoset + 2 per iapair) = " << tdoubles << endl;

  cout << endl;

  unsigned long long vfsi_iss = (totaliap*2 + totalInfosets*4);
  cout << "total doubles for Vanilla FSI using infoset store (4 per infoset + 2 per iapair) = " 
       << vfsi_iss;

  vfsi_iss += (4*2*totalInfosets); // index
  
  cout << " ( = " << (vfsi_iss*8) / (1024.0*1024.0*1024.0) << " GB)" << endl;
  
  unsigned long long totalseqs = (p1totalTurns + p2totalTurns);
  cout << "total sequences, 4 * total seq = " << totalseqs << ", " << (4*totalseqs) << endl;
  unsigned long long fsidoubles = (totalseqs + totaliap*2 + infosets1*3*P2CO + infosets2*3*P1CO);
  cout << "total doubles for FSIPCS with using SS (1 per sequence + 2 per iapair" << endl
       << "                  + 3*P1CO per p2 infoset and 3*P2CO per p1 infoset, for fsi) = " 
       << fsidoubles << endl;
  
  fsidoubles += (4*2*totalseqs); // index

  cout << "FSIPCS using SS gigabytes estimate: " << (fsidoubles*8)/(1024.0*1024.0*1024.0) << endl;
  
  // one actionshere per sequence   
  // then 2 per infoset action pair
  // and 3 per infoset (reach1, reach2, value)
  
  unsigned long long vfsi_ss = (totalseqs + totaliap*2 + totalInfosets*3); 
  cout << "total doubles for vanilla FSI using SS: " << vfsi_ss; 
  vfsi_ss += (4*2*totalseqs); // index
  cout << " ( = " << (vfsi_ss*8)/(1024.0*1024.0*1024.0) << " GB with index)" << endl;

  cout << endl;

  cout << "totals: iapairs, doubles, 4*infosets: " 
       << totaliap << " " << tdoubles << " " << (4*totalInfosets) << endl;

  cout << "bytes is index total = " << btotalis << " " << btotalindex << " " << (btotalis+btotalindex) << endl;

  cout << "Note: Sizes assume for infoset store: 2 doubles per infoset + 2 doubles per iapair, " << endl;
  cout << "                   for index: indexsize = 4*#infosets, and 2 doubles per index size . " << endl;
}

int main()
{
  // iscWidth is the number of bits needed by the chance outcome
  // in our case, six possibilties, 3 bits
  iscWidth = 3;

  countnoabs();
  //countabs2();

  return 0;
}
