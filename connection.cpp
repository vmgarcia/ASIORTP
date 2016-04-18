#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>


//#include <boost/system.hpp>
#include <cmath>
#include <boost/bind.hpp>
#include <unordered_map>
#include "packed_message.h"
#include "segment.pb.h"
#include "rtp.hpp"


using boost::asio::ip::udp;


rtp::Connection::Connection(udp::endpoint remote_endpoint_, boost::shared_ptr<rtp::Socket> socket_, unsigned window_size):
	remote_endpoint_(remote_endpoint_),
	socket_(socket_),
	timer_vec(),
	dest_ip(remote_endpoint_.address().to_string()),
	dest_port(std::to_string(remote_endpoint_.port())),
	sequence_no(-2),
	valid(false),
	timeout_exp(1),
	window_size(window_size),
	rcv_window(boost::make_shared<data_buffer>(0)),
	valid_rcv_handler(false),
	valid_send_handler(false)

{
}

bool rtp::Connection::is_valid()
{
	if (DEBUG && valid) 
	{
		std::cout << "is valid"<<std::endl;
	}
	else if (DEBUG)
	{
		std::cout << "not valid" << std::endl;
	}
	return valid;
}

void rtp::Connection::set_valid(bool val)
{
	if(DEBUG) std::cerr << "Connection Created" << std::endl;
	valid = val;
}

void rtp::Connection::close_connection()
{

}

void rtp::Connection::async_send(boost::shared_ptr<data_buffer> data_buff, boost::function<void()> send_handler)
{
	set_send_handler(data_buff, send_handler);
	call_send_handler();


}

void rtp::Connection::set_send_handler(boost::shared_ptr<data_buffer> write_buff_, boost::function<void()> send_handler)
{
	this->send_handler = send_handler;
	write_buff = write_buff_;
	valid_send_handler = true;

}

void rtp::Connection::call_send_handler()
{
	if (valid_send_handler && is_valid())
	{
		std::cout << "SENDING ASYNC STUFF" <<std::endl;
		socket_->udp_send_to(write_buff, remote_endpoint_, boost::bind(&rtp::Connection::handle_send_timeout, this, write_buff,
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred));
		send_handler();
		valid_send_handler = false;
	}
}

void rtp::Connection::handle_send_timeout(boost::shared_ptr<data_buffer> message,
			const boost::system::error_code& /*error*/, 
			std::size_t /*bytes_transferred*/)
{
	std::cout << "handle send timeout" <<std::endl;
}

void rtp::Connection::async_rcv(boost::shared_ptr<data_buffer> data_buff, boost::function<void()> rcv_handler)
{
	set_rcv_handler(data_buff, rcv_handler);
	if (rcv_window->size() > 0)
	{
		call_rcv_handler();
	}

}




void rtp::Connection::set_rcv_handler(boost::shared_ptr<data_buffer> pass_back_buffer_, boost::function<void()> rcv_handler)
{
	this->rcv_handler = rcv_handler;
	pass_back_buffer = pass_back_buffer_;
	valid_rcv_handler = true;

}

void rtp::Connection::call_rcv_handler()
{
	if (valid_rcv_handler)
	{
		pass_back_buffer->insert(pass_back_buffer->begin(), rcv_window->begin(), rcv_window->end());
		rcv_window->clear();
		rcv_handler();
		valid_rcv_handler = false;
	}
}

void rtp::Connection::handle_rcv(boost::shared_ptr<data_buffer> m_readbuf)
{
	// int buffer_position(0);
	// PackedMessage<rtp::Segment> m_packed_segment(boost::make_shared<rtp::Segment>());
	// //std::cout << show_hex(*m_readbuf) <<std::endl;

 //    unsigned msg_len = m_packed_segment.decode_header(*m_readbuf, buffer_position);
 //    //buffer_position += HEADER_SIZE;

 //    m_packed_segment.unpack(*m_readbuf, msg_len, buffer_position);
	// SegmentPtr rcvdseg = m_packed_segment.get_msg();

	//if (check_header_checksum(rcvdseg) && check_data_checksum(rcvdseg))
	//{
		// std::string data_s(rcvdseg->data());
		std::cout << "DATAAAAAADATAAAA" << std::endl;
		// std::cout << data_s <<std::endl;
		// if (rcv_window->size() + data_s.size()  < window_size)
		if (rcv_window->size() + m_readbuf->size()  < window_size)
		{
			// rcv_window->insert(rcv_window->end(), data_s.begin(), data_s.end());

			rcv_window->insert(rcv_window->end(), m_readbuf->begin(), m_readbuf->end());

		}

	//}
	call_rcv_handler();

}



void rtp::Connection::handle_rcv_timeout(boost::shared_ptr<data_buffer> message,
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{

}



boost::shared_ptr<boost::asio::deadline_timer> rtp::Connection::new_timer(
	boost::asio::io_service& io, 
	boost::posix_time::milliseconds milliseconds)
{
	auto timer = boost::make_shared<boost::asio::deadline_timer>(io, milliseconds);
	timer_vec.push_back(timer);
	return timer;
}

void rtp::Connection::delete_timer(boost::shared_ptr<boost::asio::deadline_timer> timer)
{
	auto it = std::find(timer_vec.begin(), timer_vec.end(), timer);
	if(it != timer_vec.end())
	    timer_vec.erase(it);

}



int rtp::Connection::get_sequence_no()
{
	return sequence_no;
}

void rtp::Connection::set_sequence_no(int new_seq_no)
{
	sequence_no = new_seq_no;
}

void rtp::Connection::inc_sequence_no()
{
	sequence_no++;
}

udp::endpoint rtp::Connection::get_endpoint()
{
	return remote_endpoint_;
}

void rtp::Connection::inc_timeout_exp()
{
	if (timeout_exp < 6)
	{
		timeout_exp++;
	}
}

int rtp::Connection::get_timeout_seconds()
{
	return pow(2, timeout_exp);
}

void rtp::Connection::reset_timeout()
{
	timeout_exp = 1;
}