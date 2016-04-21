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

bool not_receiving=true;
bool not_sending=true;
void client_get_handler(boost::shared_ptr<data_buffer> data, boost::shared_ptr<rtp::Connection> conn, 
	boost::shared_ptr<rtp::Socket> socket, int size, int current_index, bool killed);
void post(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket);

unsigned decode_header(const data_buffer& buf) 
{
    if (buf.size()  < 4)
        return 0;
    unsigned msg_size = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        msg_size = msg_size * 256 + (static_cast<unsigned>((char)buf[i]) & 0xFF);
    }
    // for (unsigned i = 0; i < 40; ++i)
    // {
    //  	std::cout << "CHAR " << (char)buf[i] <<std::endl;

    // }
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
void client_finish(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket_, bool is_get, bool killed)
{
	if (is_get)
	{
		not_receiving=true;
	}
	else
	{
		not_sending=true;
	}
	if(not_sending && not_receiving && !killed)
	{
		std::cout<<"Closing"<<std::endl;
		conn->close_connection();
	}
	if(not_sending && not_receiving)
	{
		socket_->close();
	}


}


void client_send_request(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket, bool last, bool killed)
{
	//client_finish(conn, socket,  killed);
	boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
	not_receiving=false;
	conn->async_rcv(storage, boost::bind(&client_get_handler, storage, conn, socket, 0, 0, _1));
	not_sending=false;
	post(conn, socket);



}




void client_get_handler(boost::shared_ptr<data_buffer> data, boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket, 
	int size, int current_index, bool killed)
{
	if(!killed)
	{
		if (data->size() > 0)
		{
			std::string msg;
			//std::cout << msg << std::endl;
		    std::ofstream out;
		    if (size > 0)
		    {
		    	msg.assign(data->begin(), data->end());
		    	out.open("get_3251.jpg", std::ios::app | std::ios::out);
		    }
		    else if(data->size() >= 4)
		    {
		    	out.open("get_3251.jpg", std::ios::out);
		    	size = decode_header(*data);
		    	std::cout << "MESSAGE SIZE " << size <<std::endl;
		    	msg.assign(data->begin()+4, data->end());
		    }
		    if (current_index < size)
		    {
			    out << msg;
			    out.close();
			    current_index += msg.size();
				boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
				conn->async_rcv(storage, boost::bind(&client_get_handler, storage, conn, socket, 
					size, current_index, _1));
			}
		}
		else if (current_index >= size)
		{
			client_finish(conn, socket, true, killed);

		}
		else
		{
			boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
			conn->async_rcv(storage, boost::bind(&client_get_handler, storage, conn, socket, 
				size, current_index, _1));

		}


	}
	else
	{
		client_finish(conn, socket, true, killed);
	}
}

void post(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket)
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
	    char byte;

	    file >> byte;

	    if (file.fail())
	    {
	        //error
	        break;
	    }

	    bytes->push_back((uint8_t)byte);
	}
	conn->async_send(bytes, boost::bind(&client_finish, conn, socket, false, _1));
}

int main(int argc, char* argv[])
{
	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;

	socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4546"));
	boost::shared_ptr<rtp::Connection> conn = socket->create_connection(u8"127.0.0.1", u8"4545");
	std::cout << "CREATING CONNECTION" <<std::endl;
	std::string msg("Hello");
	std::cout << "SEND ONE" <<std::endl;
	boost::shared_ptr<data_buffer> buff(boost::make_shared<data_buffer>(msg.begin(), msg.end()));
	not_sending=false;
	not_receiving=false;

	conn->async_send(buff, boost::bind(&client_send_request, conn, socket, true, _1));
	io_service_.run();
	return 0;

}