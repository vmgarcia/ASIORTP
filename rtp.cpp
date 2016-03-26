#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
//#include <boost/system.hpp>
#include <boost/bind.hpp>
#include <unordered_map>
#include "packed_message.h"
#include "segment.pb.h"
#include "rtp.hpp"

#define DEBUG true
#define BUFFER_SIZE 10000

using boost::asio::ip::udp;
typedef boost::shared_ptr<rtp::Segment> SegmentPtr;
typedef std::vector<uint8_t> data_buffer;

// constructor for socket
rtp::Socket::Socket(boost::asio::io_service& io_service_, std::string source_ip, std::string source_port): 
	io_service_(io_service_),
	socket_(io_service_, udp::endpoint(boost::asio::ip::address::from_string(source_ip), std::stoi(source_port))),
	source_port(source_port),
	source_ip(source_ip)
{
	// accept incoming rtp segments
	std::cout << rtp::get_endpoint_str(socket_.local_endpoint()) << std::endl; 
	start_receive();
}

/**
 *	Accept incoming rtp segments
 */
void rtp::Socket::start_receive()
{
	data_buffer tmp_buf;
	socket_.async_receive_from(boost::asio::buffer(tmp_buf), remote_endpoint_, 
		boost::bind(&rtp::Socket::multiplex, this,
			tmp_buf,
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred));

}

void rtp::Socket::multiplex(data_buffer& tmp_buf,
	const boost::system::error_code& error,
	std::size_t /*bytes_transferred*/)
{
	std::string identifier = rtp::get_endpoint_str(remote_endpoint_);
	
	if (connections.count(identifier) == 0 )
	{
		boost::shared_ptr<Connection> connection(new Connection(remote_endpoint_));
		connections.insert({rtp::get_endpoint_str(remote_endpoint_), connection});
		std::cout << rtp::get_endpoint_str(remote_endpoint_) << std::endl;
		connection_establishment(tmp_buf, connections.at(identifier));


	}
	else if(!(connections.at(identifier)->is_valid())) // connection not in list of connections
	{
		connection_establishment(tmp_buf, connections.at(identifier));
	}
	// else
	// {
	// 	connections.at(identifier).handle_receive()
	// }

}

boost::shared_ptr<rtp::Connection> rtp::Socket::create_connection(std::string ip, std::string port)
{
	udp::resolver resolver_(io_service_);
	udp::resolver::query query_(ip, port);
	udp::endpoint remote_endpoint_ = *resolver_.resolve(query_);
	boost::shared_ptr<Connection> connection(new Connection(remote_endpoint_));

	connections.insert({rtp::get_endpoint_str(remote_endpoint_), connection});

	PackedMessage<rtp::Segment> m_packed_segment(SegmentPtr(new rtp::Segment()));
	boost::shared_ptr<data_buffer> message(new data_buffer);
	SegmentPtr ackseg(new rtp::Segment());
	ackseg->set_ack(true);
	std::cout << ackseg->ack() << std::endl;
	PackedMessage<rtp::Segment> initialack(ackseg);
	initialack.pack(*message);

	unsigned size = initialack.decode_header(*message, 0);
	initialack.unpack(*message, size, 0);
	SegmentPtr ackseg2 = initialack.get_msg();
	std::cout <<ackseg->ack() << std::endl;

  	socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
	  boost::bind(&rtp::Socket::handle_send, this, message,
	    boost::asio::placeholders::error,
	    boost::asio::placeholders::bytes_transferred));

  	return connection;

}

void rtp::Socket::connection_establishment(data_buffer& m_readbuf, boost::shared_ptr<Connection> connection)
{
	int buffer_position(0);
	PackedMessage<rtp::Segment> m_packed_segment(boost::shared_ptr<rtp::Segment>(new rtp::Segment()));
	boost::shared_ptr<data_buffer> message(new data_buffer);

    unsigned msg_len = m_packed_segment.decode_header(m_readbuf, buffer_position);
    //buffer_position += HEADER_SIZE;
    m_packed_segment.unpack(m_readbuf, msg_len, buffer_position);
    //buffer_position += msg_len;

    SegmentPtr synackseg = m_packed_segment.get_msg();
    std::cout << "sending the pack" <<std::endl;

    std::cout << synackseg->syn() <<std::endl;
    std::cout << synackseg->ack() <<std::endl;
    std::cout << "___________________________________" <<std::endl;

    if (synackseg->syn() && synackseg->ack())
    {
    	std::cout << "synack" <<std::endl;
    	SegmentPtr ackseg(new rtp::Segment());
    	ackseg->set_ack(true);
    	PackedMessage<rtp::Segment> finalack(ackseg);
    	finalack.pack(*message);

    	connection->set_valid(true);

	    socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
	    	boost::bind(&rtp::Socket::handle_send, this,
	    		message,
	    		boost::asio::placeholders::error,
	    		boost::asio::placeholders::bytes_transferred));
	    std::cout << "sent ack" <<std::endl;

    }
    else if (synackseg->syn() )
    {
    	std::cout << "syn" <<std::endl;
    	SegmentPtr synackseg(new rtp::Segment());
    	synackseg->set_ack(true);
    	synackseg->set_syn(true);
    	PackedMessage<rtp::Segment> synack(synackseg);
    	synack.pack(*message);

	    socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
	    	boost::bind(&rtp::Socket::handle_send, this,
	    		message,
	    		boost::asio::placeholders::error,
	    		boost::asio::placeholders::bytes_transferred));

	    std::cout << "sent synack" <<std::endl;
    }
    else if (synackseg->ack())
    {
    	std::cout << "ack" <<std::endl;
    	connection->set_valid(true);
    }
    
    start_receive();
}

void rtp::Socket::handle_send(boost::shared_ptr<data_buffer> /*message*/,
			const boost::system::error_code& /*error*/, 
			std::size_t /*bytes_transferred*/)
{
}

/**
 *	Get remote peer ip and port in string "<ip>:<port>"
 */
std::string rtp::get_endpoint_str(udp::endpoint remote_endpoint_)
{
	std::string ip = remote_endpoint_.address().to_string();
	std::string port = std::to_string(remote_endpoint_.port());
	return ip + ":" + port;
}


rtp::Connection::Connection(udp::endpoint remote_endpoint_):
	remote_endpoint_(remote_endpoint_),
	dest_ip(remote_endpoint_.address().to_string()),
	dest_port(std::to_string(remote_endpoint_.port())),
	valid(false)
{
}

bool rtp::Connection::is_valid()
{
	std::cout << "is valid" << std::endl;
	return valid;
}

void rtp::Connection::set_valid(bool val)
{
	if(DEBUG) std::cerr << "Connection Created" << std::endl;
	valid = val;
}


int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cerr << "Not enough args" << std::endl;
		return 1;
	}
	boost::asio::io_service io_service_;
	std::unique_ptr<rtp::Socket> socket;
	if (std::string(argv[1]) == u8"server")
	{
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4545"));
	}
	else if (std::string(argv[1]) == u8"client")
	{
		socket.reset(new rtp::Socket(io_service_, u8"127.0.0.1", u8"4546"));
		socket->create_connection(u8"127.0.0.1", u8"4545");

	}
	io_service_.run();
	std::cout << "fuk" << std::endl;
	return 0;

}