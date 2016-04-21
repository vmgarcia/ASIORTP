#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <fstream>

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/crc.hpp>

//#include <boost/system.hpp>
#include <boost/bind.hpp>
#include <unordered_map>
#include "packed_message.h"
#include "segment.pb.h"
#include "rtp.hpp"

#define FTA_DEBUG true

bool not_receiving=true;
bool not_sending=true;
void test_rcv_handler(boost::shared_ptr<rtp::Connection> conn);
void client_rcv_handler(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> sock);

unsigned decode_header(const data_buffer& buf) 
{
    if (buf.size()  < 4)
        return 0;
    unsigned msg_size = 0;
    for (unsigned i = 0; i < 4; ++i)
        msg_size = msg_size * 256 + (static_cast<unsigned>(buf[i]) & 0xFF);
    // std::cout << "BUFF SIZE " << buf.size() << std::endl;
    return msg_size;
}

void encode_header(data_buffer& buf, unsigned size) 
{
    assert(buf.size() >= HEADER_SIZE);
    buf[0] = static_cast<boost::uint8_t>((size >> 24) & 0xFF);
    buf[1] = static_cast<boost::uint8_t>((size >> 16) & 0xFF);
    buf[2] = static_cast<boost::uint8_t>((size >> 8) & 0xFF);
    buf[3] = static_cast<boost::uint8_t>(size & 0xFF);
}
void final_send(boost::shared_ptr<rtp::Connection> conn, bool is_get, bool killed)
{
	if (is_get)
	{
		not_sending = true;
	}
	else
	{
		not_receiving=true;
	}
	if(not_sending && not_receiving && !killed)
	{
		std::cout<<"Closing"<<std::endl;
		conn->close_connection();
	}

}

void server_post_handler(boost::shared_ptr<data_buffer> data, boost::shared_ptr<rtp::Connection> conn, bool killed)
{
	if(!killed)
	{
		std::string msg(data->begin(), data->end());
		//std::cout << msg << std::endl;
	    std::ofstream out;
	    out.open("post_3251.jpg", std::ios::app);
	    out << msg;
	    out.close();

		boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
		conn->async_rcv(storage, boost::bind(&server_post_handler, storage, conn, _1));


	}
	else
	{
		final_send(conn, false, killed);
	}
}

void get(boost::shared_ptr<rtp::Connection> conn,  bool last, bool killed)
{

	if (!killed)
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
		data_buffer size_header(4);
		encode_header(size_header, fileSize);
	    for (int i=0; i < 4; i++)
	    {
	    	bytes->push_back(size_header[i]);
	    }
	    if (FTA_DEBUG)
	    {
			int test= decode_header(*bytes);
			std::cout << "HEADER SIZE INDICATOR " << test<<std::endl;
		}
	    while (!file.eof())
		{
		    char byte;

		    file >> byte;

		    if (file.fail())
		    {
		        //error
		        break;
		    }

		    bytes->push_back((uint8_t)byte);
		}
		conn->async_send(bytes, boost::bind(&final_send, conn, true, _1));

	}
	else
	{
		std::cout << "KILLED" << std::endl;
	}

}





void rcv_respond(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<data_buffer> data, bool killed)
{
	if (!killed)
	{
		not_sending=false;
		not_receiving=false;
		boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
		conn->async_rcv(storage, boost::bind(&server_post_handler, storage, conn, _1));
		get(conn, false, killed);
		// std::string msg("GOT ONE");
		// boost::shared_ptr<data_buffer> buf(boost::make_shared<data_buffer>(msg.begin(), msg.end()));

		// conn->async_send(buf, boost::bind(&int_send, conn, false, _1));

	}
	else
	{
		std::cout << "KILLTED" <<std::endl;
	}
}


void test_rcv_handler(boost::shared_ptr<rtp::Connection> conn)
{
	boost::shared_ptr<data_buffer> data(boost::make_shared<data_buffer>(0));
	conn->async_rcv(data, boost::bind(&rcv_respond, conn, data, _1));

}
int main(int argc, char* argv[])
{

	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;

	socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4545"));
	socket->create_receiver(&test_rcv_handler);



	io_service_.run();
	return 0;

}