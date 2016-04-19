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
#include <boost/crc.hpp>

//#include <boost/system.hpp>
#include <boost/bind.hpp>
#include <unordered_map>
#include "packed_message.h"
#include "segment.pb.h"
#include "rtp.hpp"



using boost::asio::ip::udp;
// constructor for socket
rtp::Socket::Socket(boost::asio::io_service& io_service_, std::string source_ip, std::string source_port): 
	io_service_(io_service_),
	socket_(io_service_, udp::endpoint(boost::asio::ip::address::from_string(source_ip), std::stoi(source_port))),
	source_port(source_port),
	source_ip(source_ip),
	is_server(false),
	valid(true)
{
	// accept incoming rtp segments
	if(DEBUG)std::cout << rtp::get_endpoint_str(socket_.local_endpoint()) << std::endl; 
	start_receive();
}


/**
 *	Accept incoming rtp segments
 */
void rtp::Socket::start_receive()
{
	if(valid)
	{
		boost::shared_ptr<data_buffer> tmp_buf = boost::make_shared<data_buffer>(MAX_DATAGRAM_SIZE);
		socket_.async_receive_from(boost::asio::buffer(*tmp_buf), remote_endpoint_, 
			boost::bind(&rtp::Socket::multiplex, this,
			tmp_buf,
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred));
	}

}

void rtp::Socket::delete_connection(udp::endpoint endpoint_)
{
	connections.erase(rtp::get_endpoint_str(endpoint_));


}

void rtp::Socket::multiplex(boost::shared_ptr<data_buffer> tmp_buf,
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	std::string identifier = rtp::get_endpoint_str(remote_endpoint_);
	if (bytes_transferred)
	{
		if (connections.count(identifier) == 0 )  // connection not in list of connections
		{
			boost::shared_ptr<Connection> connection = boost::make_shared<Connection>(remote_endpoint_, shared_from_this());
			connections.insert({rtp::get_endpoint_str(remote_endpoint_), connection});
			// std::cout << rtp::get_endpoint_str(remote_endpoint_) << std::endl;
			connection_establishment(tmp_buf, connections.at(identifier));


		}
		else if(!(connections.at(identifier)->is_valid())) // connection hasn't been completely established yet
		{
			//std::cout << "Not valid yet" << std::endl;

			connection_establishment(tmp_buf, connections.at(identifier));
		}
		else
		{
			connections.at(identifier)->handle_rcv(tmp_buf);
		}

	}
    start_receive();


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
	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>(0);
	SegmentPtr synseg= boost::make_shared<rtp::Segment>();
	synseg->set_syn(true);
	// std::cout<<synseg->ack()<<std::endl;
	synseg->set_sequence_no(connection->get_sequence_no());
	synseg->set_header_checksum(create_header_checksum(synseg));
	PackedMessage<rtp::Segment> initialack(synseg);
	initialack.pack(*message);

	if (DEBUG)
	{
		std::cout << "first checksum" <<  std::endl;
		std::cout << synseg->header_checksum() << std::endl;
		std::cout <<synseg->syn() << std::endl;
	}
	// std::cout << show_hex(*message) <<std::endl;
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
	//`std::cout << show_hex(*m_readbuf) <<std::endl;

    int msg_len = m_packed_segment.decode_header(*m_readbuf, buffer_position);
    //buffer_position += HEADER_SIZE;

    m_packed_segment.unpack(*m_readbuf, msg_len, buffer_position);
    //buffer_position += msg_len;

    SegmentPtr rcvdseg = m_packed_segment.get_msg();
    if (DEBUG)
    {
	    std::cout << "Received connect segment" <<std::endl;

	    std::cout << "Syn" << rcvdseg->syn() <<std::endl;
	    std::cout << "Ack" << rcvdseg->ack() <<std::endl;
	    std::cout << "Seq" << rcvdseg->sequence_no() << std::endl;
	    std::cout << "___________________________________" <<std::endl;
	}


	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>();
	if (check_header_checksum(rcvdseg))
	{
		// if receiving a syn ack packet send the final ack packet
	    if (rcvdseg->syn() && rcvdseg->sequence_no() == -2)
	    {
	    	if(DEBUG)std::cout << "syn" <<std::endl;
	    	SegmentPtr synackseg = boost::make_shared<rtp::Segment>();
	    	synackseg->set_ack(true);
	    	synackseg->set_syn(true);
	    	connection->set_sequence_no(rcvdseg->sequence_no() + 1);
	    	synackseg->set_sequence_no(connection->get_sequence_no());

	    	synackseg->set_header_checksum(create_header_checksum(synackseg));

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

	    	if(DEBUG) std::cout << "received synack" <<std::endl;
	    	SegmentPtr ackseg = boost::make_shared<rtp::Segment>();
	    	ackseg->set_ack(true);

	    	connection->set_sequence_no(rcvdseg->sequence_no() + 1);
	    	ackseg->set_sequence_no(connection->get_sequence_no());

		    ackseg->set_header_checksum(create_header_checksum(ackseg));


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
	    	if (is_server)
	    	{	
	    		receiver(connection);
	    	}
		    if(DEBUG) std::cout << "sent ack" <<std::endl;
		    connection->send();

	    }
	    else if (rcvdseg->ack() && rcvdseg->sequence_no() == 0)
	    {
	    	connection->set_sequence_no(rcvdseg->sequence_no());
	    	if(DEBUG) std::cout << "received ack" <<std::endl;
	    	connection->set_valid(true);
	    	if (is_server)
	    	{
	    		receiver(connection);
	    	}
	    	connection->send();
	    }
	}
    
}


void rtp::Socket::create_receiver(boost::function<void(boost::shared_ptr<rtp::Connection>)> receiver_)
{
	is_server = true;
	receiver = receiver_;

}
void rtp::Socket::handle_connection_est(boost::shared_ptr<data_buffer> message,
	boost::shared_ptr<rtp::Connection> connection,
	int next_seq_no,
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	auto timer = connection->new_timer(io_service_, boost::posix_time::milliseconds(200));
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
	timer->expires_at(timer->expires_at() + boost::posix_time::milliseconds(200));

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
	// if(DEBUG) std::cout << "GOT TO HANDLE CONNECTION TIMEOUT" <<std::endl;
	if (connection->get_sequence_no() < next_seq_no && !error)
	{

		// if (DEBUG) 
		// {
		// 	std::cout << "Timeout occurred at sequence_no " << next_seq_no << std::endl;
		// 	std::cout << "Resending packets from " << connection->get_sequence_no() << std::endl;
		// }
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

void rtp::Socket::udp_send_to(boost::shared_ptr<std::vector<uint8_t>> message, 
		udp::endpoint endpoint_,
		 handler_t send_handler)
{
		socket_.async_send_to(boost::asio::buffer(*message), endpoint_,
			send_handler);
}


void rtp::Socket::close()
{
	valid = false;
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


boost::uint32_t rtp::create_checksum(uint8_t* bytes)
{
	std::size_t size = sizeof(bytes);
	boost::crc_32_type result;

	result.process_bytes(bytes, size);
	return (boost::uint32_t) result.checksum();
}

boost::uint32_t rtp::create_header_checksum(SegmentPtr segment)
{
	struct header_struct {
		// std::string source_port;
		// std::string dest_port;
		int sequence_no;
		bool ack;
		bool syn;
		bool fin;
		int receive_window;
	};
	header_struct header;
	// header.source_port = segment->source_port();
	// header.dest_port = segment->dest_port();
	header.sequence_no = segment->sequence_no();
	header.ack = segment->ack();
	header.syn = segment->syn();
	header.fin = segment->fin();
	header.receive_window = segment->receive_window();

	uint8_t* header_bytes = reinterpret_cast<uint8_t*>(&header);
	std::size_t size = sizeof(header_bytes) / sizeof(header_bytes[0]);
	boost::crc_32_type checksum_calculator;
	checksum_calculator.process_bytes(header_bytes, size);

	boost::uint32_t calculated_checksum = checksum_calculator.checksum();


	return calculated_checksum;

}

bool rtp::check_header_checksum(SegmentPtr segment)
{
	boost::uint32_t sent_checksum = segment->header_checksum();
	boost::uint32_t calculated_checksum = create_header_checksum(segment);
	// bool val = sent_checksum == calculated_checksum;
	// std::cout << "Checksum is " << val <<std::endl;
	// std::cout << sent_checksum << std::endl;
	// std::cout << calculated_checksum << std::endl;
	return sent_checksum == calculated_checksum; 

}

boost::uint32_t rtp::create_data_checksum(SegmentPtr segment)
{
	std::vector<uint8_t> data(segment->data().begin(), segment->data().end());
	uint8_t* data_bytes = &data[0];
	boost::uint32_t checksum = create_checksum(data_bytes);
	return checksum;
}

bool rtp::check_data_checksum(SegmentPtr segment)
{
	if (segment->data().size() > 0)
	{
		boost::uint32_t sent_checksum = segment->data_checksum();
		boost::uint32_t calculated_checksum = create_data_checksum(segment);
		// bool res = sent_checksum == calculated_checksum;
		// std::cout << res << std::endl;
		return sent_checksum == calculated_checksum;
	}
	else if (segment->data_checksum())
	{
		std::cout << segment->data_checksum() <<std::endl;
		return false;
	}
	return true;
}


