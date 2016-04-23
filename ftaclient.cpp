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
#include "fta_request.pb.h"
#include "rtp.hpp"
#include <stdlib.h>     /* srand, rand */
#include <ctime>

#define FTA_DEBUG false

typedef boost::shared_ptr<fta_request> request_ptr;

// not_receiving and not sending are used to track whether requests are done before
// prompting for a new one
bool not_receiving=true;
bool not_sending=true;
void client_get_handler(request_ptr request, boost::shared_ptr<data_buffer> data, boost::shared_ptr<rtp::Connection> conn,
	boost::shared_ptr<rtp::Socket> socket, int size, int current_index, bool killed);
void post(request_ptr request, boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket);
void handle_req(request_ptr request, int req_type, boost::shared_ptr<rtp::Connection> conn, 
	boost::shared_ptr<rtp::Socket> socket, bool killed);
void accept_command(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket);

unsigned decode_header(const data_buffer& buf) 
{
    if (buf.size()  < 4)
        return 0;
    unsigned msg_size = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        msg_size = msg_size * 256 + (static_cast<unsigned>((char)buf[i]) & 0xFF);
    }

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
// splits a string based on a delimiter
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


// get the size of a file based on its filename
int get_file_size(std::string filename)
{
	std::ifstream file;
	file.open(filename, std::ios::binary);
	file.unsetf(std::ios::skipws);
	int fileSize;
	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.close();
	return fileSize;
}

// finish handler for the client, called when all requests, get or post, are done
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
		accept_command(conn, socket_); // when the client is done sending and receiving prompt for a 
										// new request
	}
	if(killed)
	{
		socket_->close();
	}


}


void handle_req(request_ptr request, int req_type, boost::shared_ptr<rtp::Connection> conn, 
	boost::shared_ptr<rtp::Socket> socket, bool killed)
{
	//client_finish(conn, socket,  killed);
	boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
	if(req_type==1 || req_type==3)
	{
		not_receiving=false;
		conn->async_rcv(storage, boost::bind(&client_get_handler,request, storage, conn, socket, 0, 0, _1));
	}
	if(req_type==2 || req_type==3)
	{
		not_sending=false;
		post(request, conn, socket);

	}



}

// based on user input creates a request packet which is then sent to the server which indicates
// to the server what it should do. ie get or post 
void accept_command(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket)
{

	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>(0);
	request_ptr request= boost::make_shared<fta_request>();
	std::string command_str;
	std::cout << "Command: ";
	getline(std::cin, command_str);

	std::vector<std::string> command_info = split(command_str, ' ');
	std::string command_type = command_info[0];
	if(FTA_DEBUG)std::cout << command_str << std::endl;
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
		// sends the message, when the message is sent run the handle_req callback
		conn->async_send(message, boost::bind(&handle_req, request, 1, conn, socket, _1));

	}
	else if (command_type == "post" && (int)command_info.size()==2)
	{
		request->set_post(true);
		request->set_post_filename(command_info[1]);
		request->set_post_size(get_file_size(command_info[1]));
		PackedMessage<fta_request> m_packed_request(request);
		m_packed_request.pack(*message);
		// sends the message, when the message is sent run the handle_req callback
		conn->async_send(message, boost::bind(&handle_req, request, 2, conn, socket, _1));

	}
	else if (command_type == "get-post" && (int)command_info.size()==3)
	{
		request->set_get(true);
		request->set_get_filename(command_info[1]);
		request->set_post(true);
		request->set_post_filename(command_info[2]);
		request->set_post_size(get_file_size(command_info[2]));

		PackedMessage<fta_request> m_packed_request(request);
		m_packed_request.pack(*message);
		// sends the message, when the message is sent run the handle_req callback
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

// get request handler, receives data from server
void client_get_handler(request_ptr request, boost::shared_ptr<data_buffer> data, boost::shared_ptr<rtp::Connection> conn,
	boost::shared_ptr<rtp::Socket> socket, int size, int current_index, bool killed)
{
	if(!killed)
	{
		if (data->size() > 0)
		{
			std::string msg;
			if(FTA_DEBUG)std::cout << "IN THE GET FUNCTION" << std::endl;
		    std::ofstream out;
		    if (size > 0)
		    {
		    	msg.assign(data->begin(), data->end());
		    	out.open("get_"+request->get_filename(), std::ios::app | std::ios::out);
		    }
		    else if(data->size() >= 4)
		    {
		    	out.open("get_"+request->get_filename(), std::ios::out);
		    	size = decode_header(*data);
		    	if(FTA_DEBUG) std::cout << "MESSAGE SIZE " << size <<std::endl;
		    	msg.assign(data->begin()+4, data->end());
		    }
		    if (current_index < size)
		    {
			    out << msg;
			    out.close();
			    current_index += msg.size();
			    if (current_index < size)
			    {
					boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
					conn->async_rcv(storage, boost::bind(&client_get_handler, request, storage, 
						conn, socket, size, current_index, _1));
				}
				else 
				{
					client_finish(conn, socket, true, killed);
				}
			}
		}

		else
		{
			boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
			conn->async_rcv(storage, boost::bind(&client_get_handler, request, storage, conn, socket, 
				size, current_index, _1));

		}


	}
	else
	{
		client_finish(conn, socket, true, killed);
	}
}

// post request handler, loads file into memory then sends it
void post(request_ptr request, boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> socket)
{
	std::ifstream file;
	file.open(request->post_filename(), std::ios::binary);
	file.unsetf(std::ios::skipws);
	std::streampos fileSize;
	file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    if(FTA_DEBUG)std::cout << fileSize <<std::endl;
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


// based on command line arguments connect to server
int main(int argc, char* argv[])
{
	srand(time(0));
	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;
	std::string ip;
	std::string port;
	int window_size = 20000;
	if (argc == 3)
	{
		std::vector<std::string> ip_port = split(argv[1], ':');
		ip =ip_port[0];
		port = ip_port[1];
		window_size = std::stoi(std::string(argv[2]));
	}
	else
	{
		ip = u8"127.0.0.1";
		port = u8"4545";
	}

	int lp =  rand()% 30000 + 4000;
	std::string local_port = std::to_string(lp);

	socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", local_port, window_size));
	boost::shared_ptr<rtp::Connection> conn = socket->create_connection(ip, port);


	boost::asio::deadline_timer starttimer(io_service_, boost::posix_time::milliseconds(1000));
	starttimer.async_wait(boost::bind(&accept_command, conn, socket));


	io_service_.run();
	return 0;

}