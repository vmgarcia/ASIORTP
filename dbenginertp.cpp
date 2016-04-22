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

#define DB_DEBUG false

// the data in the database
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> gt_db = {
	{
		"903076259", {
			{"first_name", "Anthony"},
			{"last_name", "Peterson"},
			{"quality_points", "231"},
			{"gpa_hours", "63"},
			{"gpa", "3.666667"}

	}},
	{
		"903084074", {
			{"first_name", "Richard"},
			{"last_name", "Harris"},
			{"quality_points", "236"},
			{"gpa_hours", "66"},
			{"gpa", "3.575758"}

	}},
	{
		"903077650", {
			{"first_name", "Joe"},
			{"last_name", "Miller"},
			{"quality_points", "224"},
			{"gpa_hours", "65"},
			{"gpa", "3.446154"}

	}},
	{
		"903083691", {
			{"first_name", "Todd"},
			{"last_name", "Collins"},
			{"quality_points", "218"},
			{"gpa_hours", "56"},
			{"gpa", "3.892857"}

	}},
	{
		"903082265", {
			{"first_name", "Laura"},
			{"last_name", "Stuart"},
			{"quality_points", "207"},
			{"gpa_hours", "64"},
			{"gpa", "3.234375"}

	}},
	{
		"903075951", {
			{"first_name", "Marie"},
			{"last_name", "Cox"},
			{"quality_points", "246"},
			{"gpa_hours", "63"},
			{"gpa", "3.904762"}

	}},
	{
		"903084336", {
			{"first_name", "Stephen"},
			{"last_name", "Baker"},
			{"quality_points", "234"},
			{"gpa_hours", "66"},
			{"gpa", "3.545455"}

	}},
};

void dba_rcv_handler(boost::shared_ptr<rtp::Connection> conn);

// split data based on delimiter
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

// close connection with client
void close_connection(boost::shared_ptr<rtp::Connection> conn, bool killed)
{
	conn->close_connection();
}

// process query string and return information based on the query
std::string process_query(std::string query_string)
{
	auto args = split(query_string, ' ');
	std::string result="From server: ";
	for (auto it=args.begin()+1; it != args.end(); it++)
	{

		result += *it + ": " + gt_db[args[0]][*it]+ " ";
	}
	return result;
}

// when a query is received, handle it then respond
void handle_query(boost::shared_ptr<data_buffer> buff, boost::shared_ptr<rtp::Connection> conn, bool killed)
{
	if (!killed)
	{
		if (buff->size() > 0)
		{
			std::string query_string(buff->begin(), buff->end());
			if(DB_DEBUG) std::cout <<query_string <<std::endl;
			std::string query_result = process_query(query_string);
			if(DB_DEBUG) std::cout <<query_result<<std::endl;
			boost::shared_ptr<data_buffer> msg(boost::make_shared<data_buffer>(query_result.begin(), query_result.end()));
			conn->async_send(msg, boost::bind(&close_connection, conn, _1));

		}
		else
		{
			dba_rcv_handler(conn);
		}

	}
}

// the receive handler which is called whenever a connection is established
void dba_rcv_handler(boost::shared_ptr<rtp::Connection> conn)
{
	boost::shared_ptr<data_buffer> buff(boost::make_shared<data_buffer>(0));
	conn->async_rcv(buff, boost::bind(&handle_query, buff, conn, _1));
}
int main(int argc, char* argv[])
{


	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;

	if (argc == 2)
	{
		std::string port = std::string(argv[1]);
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", port));
		socket->create_receiver(&dba_rcv_handler);

	}
	else if (argc == 1)
	{
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4545"));
		socket->create_receiver(&dba_rcv_handler);

	}

	io_service_.run();
	return 0;
}