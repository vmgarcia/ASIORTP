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

using boost::asio;
using boost::asio::ip::udp;
typedef boost::shared_ptr<rtp::Segment> SegmentPtr;
typedef std::vector<uint8_t> data_buffer;

// constructor for socket
rtp::Socket::Socket(boost::asio::io_service io_service_, std::string source_port): 
	socket_(io_service_, udp::endpoint(udp::v4(), std::stoi(source_port))),
	source_port(source_port)
{
	start_receive();
}

void rtp::Socket::start_receive() // have udp socket start receiving data for all remote ports
{
	data_buffer tmp_buf;
	socket_.async_receive_from(asio::buffer(tmp_buf), remote_endpoint_, 
		boost::bind(&handle_receive, this,
			tmp_buf
			asio::placeholders::error, 
			asio::placeholder::bytes_transferred));
}

void rtp::Socket::multiplex(data_buffer& tmp_buf,
	const boost::system::error_code& error,
	std::size_t /*bytes_transferred*/)
{
	std::string identifier = rtp::get_endpoint_str(remote_endpoint_);
	
	if (connections.count(identifier) == 0 || !(connections.at(identifier)->valid())) // connection not in list of connections
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
	asio::io_service io_service_ = socket_.get_io_service();
	udp::resolver resolver_(io_service_);
	udp::resolver::query query_(udp::v4(), ip, std::stoi(port));
	udp::endpoint remote_endpoint_ = *resolver.resolve(query_);
	boost::shared_ptr<Connection> connection(new Connection(remote_endpoint_));

	connections.insert({rtp::get_endpoint_str(remote_endpoint_), connection});

	PackedMessage<rtp::Segment> m_packed_segment(boost::shared_ptr<rtp::Segment>(new rtp::Segment()));
	data_buffer message;
	SegmentPtr ackseg(new Segment);
	ackseg->set_ack(true);
	PackedMessage<Segment> initialack(ackseg);
	initialack.pack(message);



  	socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
	  boost::bind(&udp_server::handle_send, this, message,
	    boost::asio::placeholders::error,
	    boost::asio::placeholders::bytes_transferred));

  	return connection;

}

void rtp::Socket::connection_establishment(data_buffer m_readbuf, Connection& connection)
{
	int buffer_position(0);
	PackedMessage<rtp::Segment> m_packed_segment(boost::shared_ptr<rtp::Segment>(new rtp::Segment()));
	data_buffer message;

    unsigned msg_len = m_packed_segment.decode_header(m_readbuf, buffer_position);
    buffer_position += HEADER_SIZE;
    DEBUG && (cerr << msg_len << " bytes\n");
    m_packed_segment.unpack(m_readbuf, msg_len, buffer_position);
    buffer_position += msg_len;


    SegmentPtr synackseg = m_packed_segment.get_msg();
    if (synackseg->syn() && synackseg->ack())
    {
    	SegmentPtr ackseg(new Segment);
    	ackseg->set_ack(true);
    	PackedMessage<Segment> finalack(ackseg);
    	finalack.pack(message);

    	connection.set_valid(true);

	    socket.async_send_to(asio::buffer(*message), remote_endpoint_,
	    	boost::bind(handle_send, this,
	    		message,
	    		asio::placeholder::error,
	    		asio::placeholder::bytes_transferred));

    }
    else if (synackseg->syn() )
    {
    	SegmentPtr synackseg(new Segment);
    	synackseg->set_ack(true);
    	synackseg->set_syn(true);
    	PackedMessage<Segment> synack(synackseg);
    	synack.pack(message);

	    socket.async_send_to(asio::buffer(*message), remote_endpoint_,
	    	boost::bind(handle_send, this,
	    		message,
	    		asio::placeholder::error,
	    		asio::placeholder::bytes_transferred));


    }
    else if (synackseg->ack())
    {
    	connection.set_valid(true);
    }
    
    start_receive();
}

void rtp::Socket::handle_send(boost::shared_ptr<std::string /*message*/
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

// int main(int argc, char* argv[])
// {
// 	if (argc != true)
// 	{
// 		std::cerr << "Not enough args" << std::endl;
// 		return 1;
// 	}
// 	asio::io_service io_service_;

// 	if (argv[1] == "server")
// 	{
// 		rtp::Socket sock();
// 	}

// }