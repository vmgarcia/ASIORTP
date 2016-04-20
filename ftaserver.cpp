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


void save_data(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<data_buffer> data, unsigned size)
{
	std::ofstream writer;
	if (size)
	{
		writer.open("copy.jpg", std::ios::out | std::ios::app);
	}
	else
	{
		writer.open("copy.jpg", std::ios::out);
	}
	for (unsigned i =0; i < data->size(); i++)
	{
		writer << (*data)[i];
	}
	writer.close();
	if (size < 278153)
	{
		unsigned data_size = data->size();
		data = boost::make_shared<data_buffer>(0);
		conn->async_rcv(data, boost::bind(&save_data,conn, data,  size+data_size));
	}

}


void fta_rcv_handler(boost::shared_ptr<rtp::Connection> conn)
{
	boost::shared_ptr<data_buffer> data(boost::make_shared<data_buffer>(0));
	conn->async_rcv(data, boost::bind(&save_data, conn, data, 0));


}
int main(int argc, char* argv[])
{

	boost::asio::io_service io_service_;

	boost::shared_ptr<rtp::Socket> socket;
	socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4545"));
	socket->create_receiver(&fta_rcv_handler);
	io_service_.run();


	

}