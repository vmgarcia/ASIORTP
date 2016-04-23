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

#include <utility>
#include <boost/bind.hpp>
#include <unordered_map>
#include "packed_message.h"
#include "segment.pb.h"
#include "rtp.hpp"
#include <stdlib.h>     /* srand, rand */
#include <ctime>

#define DB_DEBUG false

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

// when the data is received, this callback process the data and outputs it
void handle_response(boost::shared_ptr<data_buffer> buff, boost::shared_ptr<rtp::Connection> conn, 
	boost::shared_ptr<rtp::Socket> socket, bool killed)
{
	if(!killed)
	{
		if (buff->size() > 0)
		{
			std::string result(buff->begin(), buff->end());
			std::cout << result <<std::endl;
			conn->close_connection();
			socket->close();
		}
		else
		{
			boost::shared_ptr<data_buffer> data(boost::make_shared<data_buffer>(0));
			conn->async_rcv(data, boost::bind(&handle_response, data, conn, socket, _1));
		}
	}
}

// callback that is called when data is sent
// initiates async_rcv to process data sent from the server
void await_response(boost::shared_ptr<data_buffer> buff, boost::shared_ptr<rtp::Connection> conn,
	boost::shared_ptr<rtp::Socket> socket, bool killed)
{
	boost::shared_ptr<data_buffer> data(boost::make_shared<data_buffer>(0));
	conn->async_rcv(data, boost::bind(&handle_response, data, conn, socket, _1));


}

// based on user input connect to a server then send a query
int main(int argc, char* argv[])
{
	srand(time(0));
	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;
	std::string ip;
	std::string port;

	int window_size = 20000;

	std::vector<std::string> ip_port = split(argv[1], ':');
	ip =ip_port[0];
	port = ip_port[1];
	
	int lp = rand() % 30000 + 4000;
	std::string local_port = std::to_string(lp);
	socket.reset(new rtp::Socket(io_service_, u8"0.0.0.0", local_port, window_size));
	boost::shared_ptr<rtp::Connection> conn = socket->create_connection(ip, port);

	std::string query_string="";
	for (int i=2; i < argc; i++)
	{
		std::string col(argv[i]);
		query_string+= col+" ";
	}
	if (DB_DEBUG)std::cout << query_string <<std::endl;
	boost::shared_ptr<data_buffer> buff(boost::make_shared<data_buffer>(query_string.begin(), query_string.end()));
	conn->async_send(buff, boost::bind(&await_response, buff, conn, socket, _1));

	io_service_.run();
	return 0;

}