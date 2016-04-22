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

#define FTA_DEBUG false
typedef boost::shared_ptr<fta_request> request_ptr;

// not sending and not receiving are used to make sure the connection isn't
// processing any information before ending the requests
bool not_receiving=true;
bool not_sending=true;
void fta_rcv_handler(boost::shared_ptr<rtp::Connection> conn);
void client_rcv_handler(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<rtp::Socket> sock);

// get 4 bytes from front of vector and make them into int
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

// get 4 bytes and store them in data_buffer
void encode_header(data_buffer& buf, unsigned size) 
{
    assert(buf.size() >= HEADER_SIZE);
    buf[0] = static_cast<boost::uint8_t>((size >> 24) & 0xFF);
    buf[1] = static_cast<boost::uint8_t>((size >> 16) & 0xFF);
    buf[2] = static_cast<boost::uint8_t>((size >> 8) & 0xFF);
    buf[3] = static_cast<boost::uint8_t>(size & 0xFF);
}

// called when we are done with the request
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
		if(FTA_DEBUG)std::cout<<"Done with Get and Post"<<std::endl;
		fta_rcv_handler(conn);
	}

}

// this is the post handler
// receives data being sent by the client and stores it
void post(request_ptr request, boost::shared_ptr<data_buffer> data, boost::shared_ptr<rtp::Connection> conn,
	int size, int current_index, bool killed)
{
	if(!killed)
	{

		if(FTA_DEBUG) std::cout <<"IN THE POST FUNCTION" << std::endl;
		if (data->size() > 0) //  if the data has no bytes ignore it
		{
			if(FTA_DEBUG) std::cout << "GETTING POST DATA" << std::endl;
			std::string msg(data->begin(), data->end()); // load the data into a string
		    std::ofstream out;
		    if (size > 0)
		    {
		    	// if this is not the first time we are seeing the data, we want to append
		    	out.open("post_"+request->post_filename(), std::ios::app | std::ios::out);
		    }
		    else 
		    {
		    	// if it is the first time we want to create a new file
		    	out.open("post_"+request->post_filename(), std::ios::out);
		    	size = request->post_size();

		    }
		    // the current index tracks how much data we have processed
		    // once the current index is greater than or equal to the size of the file
		    // we know we can end the post requests
		    if (current_index < size)
		    {
			    out << msg;
			    out.close();
			    current_index += msg.size();
				boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
				conn->async_rcv(storage, boost::bind(&post, request, storage, 
					conn, size, current_index, _1));
			}
		}
		// when we process all the data end the post request
		else if (current_index >= request->post_size())
		{
			final_send(conn, false, killed);

		}
		else
		{
			// if we haven't process it all continue the post request
			boost::shared_ptr<data_buffer> storage(boost::make_shared<data_buffer>(0));
			conn->async_rcv(storage, boost::bind(&post, request, storage, conn,  
				size, current_index, _1));

		}

		
	}
	else
	{
		final_send(conn, false, killed);
	}
}


// loads the requested file then sends it
void get(request_ptr request, boost::shared_ptr<rtp::Connection> conn,  bool last, bool killed)
{

	if (!killed)
	{

		std::ifstream file;
		file.open(request->get_filename(), std::ios::binary); // loads the file that the request protobuf indicated
		file.unsetf(std::ios::skipws);
		std::streampos fileSize;
		file.seekg(0, std::ios::end);
	    fileSize = file.tellg();
	    file.seekg(0, std::ios::beg);
	    boost::shared_ptr<data_buffer> bytes(boost::make_shared<data_buffer>(0));
		data_buffer size_header(4);

		// encode header gets the size of the file we are sending
		// and makes it into 4 bytes which are prepended to that data being sent
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

		// load the file into memory
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
		// send the file
		conn->async_send(bytes, boost::bind(&final_send, conn, true, _1));

	}
	else
	{
		if(FTA_DEBUG)std::cout << "KILLED" << std::endl;
	}

}




// rcv_respond, when a message is received, respond depending on what typ of message it is
void rcv_respond(boost::shared_ptr<rtp::Connection> conn, boost::shared_ptr<data_buffer> data, bool killed)
{
	if (!killed) // if the connection is killed don't do what is below
	{
		if(data->size() > 0) // if the data doesn't have any bytes ignore it
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
			if (request->post()) // if a post request, handle it by receiving the data
			{
				not_receiving=false;
				boost::shared_ptr<data_buffer> buff(boost::make_shared<data_buffer>(0));
				conn->async_rcv(buff, boost::bind(&post, request, buff, conn, 0, 0, killed));
			}
			if (request->get()) // if a get request call the get function which loads the file and sends it
			{
				not_sending=false;
				get(request, conn, false, killed);	
			}


		}
		else
		{
			fta_rcv_handler(conn);
		}
	}
	else
	{
		if(FTA_DEBUG)std::cout << "KILLTED" <<std::endl;
	}


}

// this is the receiver for this server
// whenever a connection is created, this function will be called
// with a reference to the connection that was created
void fta_rcv_handler(boost::shared_ptr<rtp::Connection> conn)
{
	boost::shared_ptr<data_buffer> data(boost::make_shared<data_buffer>(0));
	conn->async_rcv(data, boost::bind(&rcv_respond, conn, data, _1));

}


int main(int argc, char* argv[])
{

	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;

	if (argc == 3)
	{
		std::string port = std::string(argv[1]);
		int max_window_size = std::stoi(std::string(argv[2]));
		// create the socket
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", port, max_window_size));
		//create the receiver. this will handle connections to the server
		socket->create_receiver(&fta_rcv_handler);

	}
	else if (argc == 1)
	{
		// create the socket
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4545"));
		//create the receiver. this will handle connections to the server
		socket->create_receiver(&fta_rcv_handler);

	}

	io_service_.run();
	return 0;

}