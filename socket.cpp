#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/steady_timer.hpp>

//#include <boost/system.hpp>
#include <boost/bind.hpp>
#include <unordered_map>
#include "packed_message.h"
#include "segment.pb.h"
#include "rtp.hpp"



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
	boost::shared_ptr<data_buffer> tmp_buf = boost::make_shared<data_buffer>(2000);
	socket_.async_receive_from(boost::asio::buffer(*tmp_buf), remote_endpoint_, 
		boost::bind(&rtp::Socket::multiplex, this,
			tmp_buf,
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred));

}

void rtp::Socket::multiplex(boost::shared_ptr<data_buffer> tmp_buf,
	const boost::system::error_code& error,
	std::size_t /*bytes_transferred*/)
{
	std::string identifier = rtp::get_endpoint_str(remote_endpoint_);
	
	if (connections.count(identifier) == 0 )  // connection not in list of connections
	{
		boost::shared_ptr<Connection> connection = boost::make_shared<Connection>(remote_endpoint_, shared_from_this());
		connections.insert({rtp::get_endpoint_str(remote_endpoint_), connection});
		std::cout << rtp::get_endpoint_str(remote_endpoint_) << std::endl;
		connection_establishment(tmp_buf, connections.at(identifier));


	}
	else if(!(connections.at(identifier)->is_valid())) // connection hasn't been completely established yet
	{
		std::cout << "Not valid yet" << std::endl;

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
	boost::shared_ptr<Connection> connection = boost::make_shared<Connection>(remote_endpoint_,
		shared_from_this());

	connections.insert({rtp::get_endpoint_str(remote_endpoint_), connection});

	PackedMessage<rtp::Segment> m_packed_segment = boost::make_shared<rtp::Segment>();
	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>();
	SegmentPtr ackseg= boost::make_shared<rtp::Segment>();
	ackseg->set_syn(true);
	ackseg->set_sequence_no(connection->get_sequence_no());
	std::cout << ackseg->ack() << std::endl;
	PackedMessage<rtp::Segment> initialack(ackseg);
	initialack.pack(*message);

	// unsigned size = initialack.decode_header(*message, 0);
	// initialack.unpack(*message, size, 0);
	// SegmentPtr ackseg2 = initialack.get_msg();
	// std::cout <<ackseg->ack() << std::endl;
	std::cout << show_hex(*message) <<std::endl;
  	socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
	  boost::bind(&rtp::Socket::handle_connection_est, this, message,
	  	connection,
	  	connection->get_sequence_no() + 1,
	    boost::asio::placeholders::error,
	    boost::asio::placeholders::bytes_transferred));

  	return connection;

}

void rtp::Socket::connection_establishment(boost::shared_ptr<data_buffer> m_readbuf, boost::shared_ptr<Connection> connection)
{
	int buffer_position(0);
	PackedMessage<rtp::Segment> m_packed_segment(boost::make_shared<rtp::Segment>());
	std::cout << show_hex(*m_readbuf) <<std::endl;

    unsigned msg_len = m_packed_segment.decode_header(*m_readbuf, buffer_position);
    //buffer_position += HEADER_SIZE;

    m_packed_segment.unpack(*m_readbuf, msg_len, buffer_position);
    //buffer_position += msg_len;

    SegmentPtr rcvdseg = m_packed_segment.get_msg();
    std::cout << "sending the pack" <<std::endl;

    std::cout << rcvdseg->syn() <<std::endl;
    std::cout << rcvdseg->ack() <<std::endl;
    std::cout << rcvdseg->sequence_no() << std::endl;
    std::cout << "___________________________________" <<std::endl;


	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>();

	// if receiving a syn ack packet send the final ack packet
    if (rcvdseg->syn() && rcvdseg->sequence_no() == -2)
    {
    	if(DEBUG)std::cout << "syn" <<std::endl;
    	SegmentPtr synackseg = boost::make_shared<rtp::Segment>();
    	synackseg->set_ack(true);
    	synackseg->set_syn(true);

    	connection->set_sequence_no(rcvdseg->sequence_no() + 1);
    	synackseg->set_sequence_no(connection->get_sequence_no());

    	PackedMessage<rtp::Segment> synack(synackseg);
    	synack.pack(*message);

    	int next_seq_no = connection->get_sequence_no() + 1;
	    socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
	    	boost::bind(&rtp::Socket::handle_connection_est, this,
	    		message,
	    		connection,
	    		next_seq_no,
	    		boost::asio::placeholders::error,
	    		boost::asio::placeholders::bytes_transferred));

	    if(DEBUG) std::cout << "sent synack" <<std::endl;
    }
    else if (rcvdseg->syn() && rcvdseg->ack() && rcvdseg->sequence_no() == -1)
    {

    	if(DEBUG) std::cout << "synack" <<std::endl;
    	SegmentPtr ackseg = boost::make_shared<rtp::Segment>();
    	ackseg->set_ack(true);

    	connection->set_sequence_no(rcvdseg->sequence_no() + 1);
    	ackseg->set_sequence_no(connection->get_sequence_no());

    	PackedMessage<rtp::Segment> finalack(ackseg);
    	finalack.pack(*message);

    	int next_seq_no = connection->get_sequence_no();

	    socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
	    	boost::bind(&rtp::Socket::handle_connection_est, this,
	    		message,
	    		connection,
	    		next_seq_no,
	    		boost::asio::placeholders::error,
	    		boost::asio::placeholders::bytes_transferred));
    	connection->set_valid(true);
	    if(DEBUG) std::cout << "sent ack" <<std::endl;

    }
    else if (rcvdseg->ack() && rcvdseg->sequence_no() == 0)
    {
    	connection->set_sequence_no(rcvdseg->sequence_no());
    	if(DEBUG) std::cout << "ack" <<std::endl;
    	connection->set_valid(true);
    }
    
    start_receive();
}



void rtp::Socket::handle_connection_est(boost::shared_ptr<data_buffer> message,
	boost::shared_ptr<rtp::Connection> connection,
	int next_seq_no,
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	auto timer = connection->new_timer(io_service_, boost::posix_time::seconds(2));
	timer->async_wait(boost::bind(&rtp::Socket::handle_connection_timeout, this,
		message,
		connection,
		next_seq_no,
		timer,
		error,
		bytes_transferred));

}

void rtp::Socket::handle_connection_est(boost::shared_ptr<data_buffer> message,
	boost::shared_ptr<rtp::Connection> connection,
	int next_seq_no,
	boost::shared_ptr<boost::asio::deadline_timer> timer, 
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	timer->expires_at(timer->expires_at() + boost::posix_time::seconds(2));

	timer->async_wait(boost::bind(&rtp::Socket::handle_connection_timeout, this,
		message,
		connection,
		next_seq_no,
		timer,
		error,
		bytes_transferred));

}
// once the deadline timer expires, run this method which will resend the message
void rtp::Socket::handle_connection_timeout(boost::shared_ptr<std::vector<uint8_t>> message, 
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error,
			std::size_t bytes_transferred)
{
	if(DEBUG) std::cout << "GOT TO HANDLE CONNECTION TIMEOUT" <<std::endl;
	if (connection->get_sequence_no() < next_seq_no && !error)
	{

		if (DEBUG) 
		{
			std::cout << "Timeout occurred at sequence_no " << next_seq_no << std::endl;
			std::cout << "Resending packets from " << connection->get_sequence_no() << std::endl;
		}
	    socket_.async_send_to(boost::asio::buffer(*message), connection->get_endpoint(),
	    	boost::bind(&rtp::Socket::handle_connection_est, this,
	    		message,
	    		connection,
	    		next_seq_no,
	    		timer,
	    		boost::asio::placeholders::error,
	    		boost::asio::placeholders::bytes_transferred));
	}

	else
	{

		if (error)
		{
			std::cout << "There was an error in connection establishment, closing timer" << std::endl;
		}
		connection->delete_timer(timer);
	}
}

boost::asio::io_service& rtp::Socket::get_io_service()
{
	return io_service_;
}

void rtp::Socket::close_connection(udp::endpoint connection_endpoint)
{
	// connections.
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





int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cerr << "Not enough args" << std::endl;
		return 1;
	}
	boost::asio::io_service io_service_;
	boost::shared_ptr<rtp::Socket> socket;
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