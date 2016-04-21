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
#include "fta_request.pb.h"

#include "rtp.hpp"

#define FTA_DEBUG true

typedef boost::shared_ptr<fta_request> request_ptr;

bool not_in_post = true;
bool not_in_get = true;

void accept_command(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket);
void handle_req(request_ptr request, int req_type, boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket, bool killed);
void get(request_ptr request, boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket,
	boost::shared_ptr<data_buffer> data, unsigned index, bool killed);
void post(request_ptr request, boost::shared_ptr<rtp::Connection> conn, bool killed);

unsigned decode_header(const data_buffer& buf) 
{
    if (buf.size()  < 4)
        return 0;
    unsigned msg_size = 0;
    for (unsigned i = 0; i < 4; ++i)
        msg_size = msg_size * 256 + (static_cast<unsigned>(buf[i]) & 0xFF);
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

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

void close(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket)
{
	if(not_in_get && not_in_post)
	{
		conn->close_connection();
		socket->close();
	}
}
void post(request_ptr request, boost::shared_ptr<rtp::Connection> conn, bool killed)
{

}

void get(request_ptr request, boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket,
	boost::shared_ptr<data_buffer> data, unsigned index, bool killed)
{
	if (killed)
	{
		not_in_get = true;
		close(conn, socket);
	}
	if (!conn->is_valid())
	{
		not_in_get = true;
		close(conn, socket);
		return;
	}
	std::string filename = "get_" + request->get_filename();
	std::ifstream writer;

	if (index == 0)
	{
		writer.open(filename, std::ios::out | std::ios::app);
	}
	else
	{
		writer.open(filename, std::ios::out | std::ios::app);
	}
	
	for (unsigned i =0; i < data->size(); i++)
	{
		writer << (*data)[i];
	}
	writer.close();

	index += data->size();
	data = boost::make_shared<data_buffer>(0);
	conn->async_rcv(data, boost::bind(&get,request, conn, socket, data,  index, _1));
		

	

}
void handle_req(request_ptr request, int req_type, boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket, bool killed)
{
	if (req_type == 1 || req_type == 3)
	{
		not_in_get = false;
		boost::shared_ptr<data_buffer> data(boost::make_shared<data_buffer>(1000));
		conn->async_rcv(data, boost::bind(&get,request, conn, socket, data,  0, _1));
	}
	if (req_type == 2 || req_type == 3)
	{
		not_in_post = false;
	}

}
void accept_command(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket)
{

	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>(0);
	request_ptr request= boost::make_shared<fta_request>();
	std::string command_str;
	std::cout << "Command: ";
	getline(std::cin, command_str);

	std::vector<std::string> command_info = split(command_str, ' ');
	std::string command_type = command_info[0];
	std::cout << command_str << std::endl;
	if	(command_type == "disconnect"&& (int)command_info.size()==1)
	{
		conn->close_connection();
		socket->close();
	}
	else if (command_type == "get" && (int)command_info.size()==2)
	{
		request->set_get(true);
		request->set_get_filename(command_info[1]);
		PackedMessage<fta_request> m_packed_request(request);
		m_packed_request.pack(*message);
		if(FTA_DEBUG)
		{
			std::cout <<"MESSAGE SIZE\n";
			std::cout << message->size() << std::endl;
		}
		conn->async_send(message, boost::bind(&handle_req, request, 1, conn, socket, _1));

	}
	else if (command_type == "post" && (int)command_info.size()==2)
	{
		request->set_post(true);
		request->set_post_filename(command_info[2]);
		PackedMessage<fta_request> m_packed_request(request);
		m_packed_request.pack(*message);
		conn->async_send(message, boost::bind(&handle_req, request, 2, conn, socket, _1));

	}
	else if (command_type == "get-post" && (int)command_info.size()==3)
	{
		request->set_get(true);
		request->set_get_filename(command_info[1]);
		request->set_post(true);
		request->set_post_filename(command_info[2]);
		PackedMessage<fta_request> m_packed_request(request);
		m_packed_request.pack(*message);
		conn->async_send(message, boost::bind(&handle_req, request, 3, conn, socket, _1));

	}

    if (FTA_DEBUG)
    {
    	std::cout << "Is Get: " << request->get() << "\n";
    	std::cout << "Is Post: " << request->post() << "\n";
    	std::cout << "POST FILENAME" << "\n";
    	std::cout << request->post_filename()<< "\n";
    	std::cout << "GET FILENAME" << "\n";
    	std::cout << request->get_filename() << std::endl;;

    }
}
int main(int argc, char* argv[])
{


	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;
	std::string ip;
	std::string port;
	int window_size = 20000;
	if (argc == 3)
	{
		std::vector<std::string> ip_port = split(argv[1], ':');
		ip =ip_port[0];
		port = ip_port[0];
		window_size = std::stoi(std::string(argv[2]));
	}
	else
	{
		ip = u8"127.0.0.1";
		port = u8"4545";
	}

	socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4546", window_size));
	boost::shared_ptr<rtp::Connection> conn = socket->create_connection(ip, port);
	boost::asio::deadline_timer starttimer(io_service_, boost::posix_time::milliseconds(1000));

	starttimer.async_wait(boost::bind(&accept_command, conn, socket));

	io_service_.run();
	
}

