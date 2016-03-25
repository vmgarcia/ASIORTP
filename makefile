CC=g++
CXXFLAGS=-Wall -std=c++11
CXXFLAGS+= -pthread -I/usr/local/include   #$(pkg-config --cflags protobuf)
BOOST=/home/vagrant/boost_1_60_0
BOOSTLIB=/home/vagrant/boost_1_60_0/stage/lib
LIBS=-lboost_system -lboost_date_time -lboost_thread
LINKFLAGS=-Wl,-rpath,/home/vagrant/boost_1_60_0/stage/lib
LINKFLAGS+= -pthread -L/usr/local/lib -lprotobuf -lpthread #$(pkg-config --libs protobuf)
OUTPUT=example
DEBUG=-fmax-errors=1
all:
	$(CC) $(CXXFLAGS) -I $(BOOST) main.cpp -o $(OUTPUT) -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS)

debug:
	$(CC) $(CXXFLAGS) -I $(BOOST) dbclient.cpp -o dbclient -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS) 2> error.txt

rtp:
	$(CC) $(CXXFLAGS) -I $(BOOST) rtp.cpp -o rtp -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS) 2> error.txt