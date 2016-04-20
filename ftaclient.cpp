#include <iostream>
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/crc.hpp>

//#include <boost/system.hpp>
#include <boost/bind.hpp>
#include <unordered_map>
#include "packed_message.h"
#include "segment.pb.h"
#include "rtp.hpp"


void fta_send(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket)
{
	conn->close_connection();
	socket->close();
}

int main(int argc, char* argv[])
{
	std::ifstream file;
	file.open("3251.jpg", std::ios::binary);
	file.unsetf(std::ios::skipws);
	std::streampos fileSize;
	file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    std::cout << fileSize <<std::endl;
    file.seekg(0, std::ios::beg);
    boost::shared_ptr<data_buffer> bytes(boost::make_shared<data_buffer>(0));
    while (!file.eof())
	{
	    uint8_t byte;

	    file >> byte;

	    if (file.fail())
	    {
	        //error
	        break;
	    }

	    bytes->push_back(byte);
	}
	std::cout << "SIZE: " << bytes->size() <<std::endl;
	file.close();
	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;

	socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4546"));
	boost::shared_ptr<rtp::Connection> conn = socket->create_connection(u8"127.0.0.1", u8"4545");
	conn->async_send(bytes, boost::bind(&fta_send, conn, socket));

	io_service_.run();
	
}