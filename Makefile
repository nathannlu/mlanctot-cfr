
# Slowest version, for use with valgrind to find memory-related issues
#CPPFLAGS = -Wall -W -O0 -g

# Used to profile the code using gprof
#CPPFLAGS = -Wall -W -O2 -g -pg

# Includes debug symbols for use with gdb
CPPFLAGS = -Wall -W -O2 -g 

# Fastest version, no debug symbols or asserts enabled. For use during "production runs" :) 
#CPPFLAGS = -O3 -DNDEBUG 

EXECS = cfr cfrcs cfros cfres pcs purecfr
HEADERS = bluff.h infosetstore.h defs.h fvector.h svector.h
COMMON = bluff.o sampling.o br.o infosetstore.o util.o

all: $(EXECS)

clean: 
	rm -f $(EXECS) bluffcounter *.o


cfr: cfr.cpp $(COMMON) $(HEADERS)
	g++ $(CPPFLAGS) -o cfr cfr.cpp $(COMMON)           # Vanilla CFR

cfrcs: cfrcs.cpp $(COMMON) $(HEADERS)
	g++ $(CPPFLAGS) -o cfrcs cfrcs.cpp $(COMMON)       # Chance-sampled CFR

cfros: cfros.cpp $(COMMON) $(HEADERS)
	g++ $(CPPFLAGS) -o cfros cfros.cpp $(COMMON)       # Outcome Sampling MCCFR

cfres: cfres.cpp $(COMMON) $(HEADERS)
	g++ $(CPPFLAGS) -o cfres cfres.cpp $(COMMON)       # External Sampling MCCFR

pcs: pcs.cpp $(COMMON) $(HEADERS) svector.h
	g++ $(CPPFLAGS) -o pcs pcs.cpp $(COMMON)           # Public Chance Sampling

purecfr: purecfr.cpp $(COMMON) $(HEADERS)
	g++ $(CPPFLAGS) -o purecfr purecfr.cpp $(COMMON)   # Pure CFR

bluffcounter: bluffcounter.cpp
	g++ $(CPPFLAGS) -o bluffcounter bluffcounter.cpp

# object files

infosetstore.o: infosetstore.h infosetstore.cpp bluff.h
	g++ $(CPPFLAGS) -c -o infosetstore.o infosetstore.cpp
  
util.o: util.cpp bluff.h 
	g++ $(CPPFLAGS) -c -o util.o util.cpp

sampling.o: sampling.cpp bluff.h 
	g++ $(CPPFLAGS) -c -o sampling.o sampling.cpp

bluff.o: bluff.cpp bluff.h 
	g++ $(CPPFLAGS) -c -o bluff.o bluff.cpp

br.o: br.cpp bluff.h 
	g++ $(CPPFLAGS) -c -o br.o br.cpp
