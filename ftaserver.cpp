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

void save_data(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<data_buffer> data, unsigned size, bool killed)
{
	std::ofstream writer;
	writer.unsetf(std::ios::skipws);
	if (size)
	{
		writer.open("copy.jpg", std::ios::out|std::ios::app);
	}
	else
	{
		writer.open("copy.jpg", std::ios::out);
	}

	// std::ifstream reader;
	// reader.open("copy.jpg");
	// reader.seekg(0, std::ios::end);
 //    int readerSize = reader.tellg();
 //    std::cout << readerSize <<std::endl;
	
	for (unsigned i =0; i < data->size(); i++)
	{
		writer << (*data)[i];
	}
	writer.close();
	if (size < 23500215)
	{
		//std::cout << size <<std::endl;
		unsigned data_size = data->size();
		data = boost::make_shared<data_buffer>(0);
		conn->async_rcv(data, boost::bind(&save_data,conn, data,  size+data_size, _1));
	}

}
void finish(request_ptr request, boost::shared_ptr<rtp::Connection> conn, bool killed)
{
	if (!killed)
	{
		conn->close_connection();
	}
}

void handle_get_req(request_ptr request, boost::shared_ptr<rtp::Connection> conn, bool killed)
{
	if (killed) return;
	std::string filename = request->get_filename();
	std::ifstream file;
	file.open(filename, std::ios::binary);
	file.unsetf(std::ios::skipws);
	int fileSize;
	file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    std::cout << fileSize <<std::endl;
    file.seekg(0, std::ios::beg);




    boost::shared_ptr<data_buffer> bytes(boost::make_shared<data_buffer>(0));

// void fuckoff()
// {	
// 	std::ifstream file;
// 	file.open("3251.jpg", std::ios::binary);
// 	file.unsetf(std::ios::skipws);
// 	std::streampos fileSize;
// 	file.seekg(0, std::ios::end);
//     fileSize = file.tellg();
//     std::cout << fileSize <<std::endl;
//     file.seekg(0, std::ios::beg);
//     boost::shared_ptr<data_buffer> bytes(boost::make_shared<data_buffer>(0));
//     while (!file.eof())
// 	{
// 	    uint8_t byte;

// 	    file >> byte;

// 	    if (file.fail())
// 	    {
// 	        //error
// 	        break;
// 	    }

// 	    bytes->push_back(byte);
// 	}
// 	std::cout << "SIZE: " << bytes->size() <<std::endl;
// 	conn->async_send(bytes, boost::bind(&fta_send, conn, socket));

// 	file.close();
// }
    
    for(int i=0;!file.eof(); i++)
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
	file.close();

	// std::ofstream writer;
	// writer.open("copy.jpg", std::ios::out);
	// for (int i=0; i<bytes->size(); i++)
	// {
	// 	writer<<(*bytes)[i];
	// }
	// writer.close();
	// size_header->insert(size_header->begin(), bytes->begin(), bytes->end());
	// std::cout << "SIZE: " << size_header->size() <<std::endl;
	conn->async_send(bytes, boost::bind(&finish, request, conn, _1));
	

}

void handle_post_req(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<data_buffer> data)
{

}

void handle_req(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<data_buffer> data)
{
	int buffer_position(0);
	PackedMessage<fta_request> m_packed_request(boost::make_shared<fta_request>());
    int msg_len = m_packed_request.decode_header(*data, buffer_position);
    m_packed_request.unpack(*data, msg_len, 0);
    request_ptr request = m_packed_request.get_msg();

    if (FTA_DEBUG)
    {
    	std::cout << "Is Get: " << request->get() << "\n";
    	std::cout << "Is Post: " << request->post() << "\n";
    	std::cout << "POST FILENAME" << "\n";
    	std::cout << request->post_filename()<< "\n";
    	std::cout << "GET FILENAME" << "\n";
    	std::cout << request->get_filename()<<std::endl;
    	

    }
    if (request->get())
    {
		handle_get_req(request, conn, false);	
    }
}

void fta_rcv_handler(boost::shared_ptr<rtp::Connection> conn)
{
	boost::shared_ptr<data_buffer> data(boost::make_shared<data_buffer>(0));
	conn->async_rcv(data, boost::bind(&handle_req, conn, data));


}
int main(int argc, char* argv[])

{
	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;

	if (argc == 3)
	{
		std::string port = std::string(argv[1]);
		int max_window_size = std::stoi(std::string(argv[2]));
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", port, max_window_size));
		socket->create_receiver(&fta_rcv_handler);

	}
	else if (argc == 1)
	{
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4545"));
		socket->create_receiver(&fta_rcv_handler);

	}

	io_service_.run();


	

}