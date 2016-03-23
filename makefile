CC=g++
CXXFLAGS=-Wall
BOOST=/home/vagrant/boost_1_60_0
BOOSTLIB=/home/vagrant/boost_1_60_0/stage/lib
LIBS=-lboost_system -lboost_date_time -lboost_thread
LINKFLAGS=-Wl,-rpath,/home/vagrant/boost_1_60_0/stage/lib
OUTPUT=example
DEBUG=-fmax-errors=1
all:
	$(CC) $(CXXFLAGS) -I $(BOOST) main.cpp -o $(OUTPUT) -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS)

debug:
	$(CC) $(CXXFLAGS) -I $(BOOST) dbclient.cpp -o dbclient -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS) 2> error.txt