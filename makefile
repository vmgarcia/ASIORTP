CC=g++
CXXFLAGS=-Wall -std=c++11
CXXFLAGS+= -pthread -I/usr/local/include   #$(pkg-config --cflags protobuf)
BOOST=/home/vagrant/boost_1_60_0
BOOSTLIB=/home/vagrant/boost_1_60_0/stage/lib
LIBS=-lboost_system -lboost_date_time -lboost_thread
LINKFLAGS=-Wl,-rpath,/home/vagrant/boost_1_60_0/stage/lib
LINKFLAGS+= -pthread -L/usr/local/lib -lprotobuf -lpthread #$(pkg-config --libs protobuf)
OUTPUT=example
DEBUG=-g
all:
	$(CC) $(CXXFLAGS) -I $(BOOST) main.cpp -o $(OUTPUT) -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS)

debug:
	$(CC) $(CXXFLAGS) -I $(BOOST) dbclient.cpp -o dbclient -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS) 2> error.txt

rtp:
	$(CC) $(CXXFLAGS) $(DEBUG) -I $(BOOST) test1.cpp socket.cpp connection.cpp segment.pb.cc -o rtp -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS) 2> error.txt


ftaclient:
	$(CC) $(CXXFLAGS) $(DEBUG) -I $(BOOST) ftaclient.cpp socket.cpp connection.cpp segment.pb.cc -o ftaclient -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS) 2> error.txt

ftaserver:
	$(CC) $(CXXFLAGS) $(DEBUG) -I $(BOOST) ftaserver.cpp socket.cpp connection.cpp segment.pb.cc -o ftaserver -L$(BOOSTLIB) $(LIBS) $(LINKFLAGS) 2> error.txt

rem:
	/opt/rh/devtoolset-2/root/usr/bin/g++ \
	 -pthread -I/nethome/vgarcia7/proto/include $(DEBUG) -I/nethome/vgarcia7/boost_1_60_0 \
	 test1.cpp socket.cpp connection.cpp segment.pb.cc -o rtp -L/nethome/vgarcia7/boost_1_60_0/stage/lib \
	 $(LIBS) -Wl,-rpath,/nethome/vgarcia7/boost_1_60_0/stage/lib -Wl,-rpath -Wl,/nethome/vgarcia7/proto/lib -lprotobuf -lpthread2> error.txt
