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
#include <algorithm>

using boost::asio::ip::udp;


rtp::Connection::Connection(udp::endpoint remote_endpoint_, boost::shared_ptr<rtp::Socket> socket_, int window_size):
	remote_endpoint_(remote_endpoint_),
	socket_(socket_),
	timer_vec(),
	dest_ip(remote_endpoint_.address().to_string()),
	dest_port(std::to_string(remote_endpoint_.port())),
	sequence_no(-2),
	valid(false),
	timeout_exp(1),
	congestion_window(1),
	window_size(window_size),
	rcv_window(boost::make_shared<data_buffer>(0)),
	valid_rcv_handler(false),
	valid_send_handler(false),
	timeout_count(0)
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
	//if(DEBUG) std::cerr << "Connection Created" << std::endl;
	valid = val;
}

void rtp::Connection::close_connection()
{
	set_valid(false);
	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>(0);
	SegmentPtr finseg= boost::make_shared<rtp::Segment>();
	finseg->set_fin(true);
	finseg->set_header_checksum(create_header_checksum(finseg));
	PackedMessage<rtp::Segment> packeddata(finseg);
	packeddata.pack(*message);

	socket_->udp_send_to(message, remote_endpoint_, boost::bind(&rtp::Connection::handle_fin, this,
		boost::asio::placeholders::error, 
		boost::asio::placeholders::bytes_transferred));


}
void rtp::Connection::handle_fin()
{
	set_valid(false);
	auto timer = new_timer(socket_->get_io_service(), boost::posix_time::milliseconds(300));
	timer->async_wait(boost::bind(&rtp::Connection::wait_for_death, this));


}
void rtp::Connection::handle_fin(	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	auto timer = new_timer(socket_->get_io_service(), boost::posix_time::milliseconds(300));
	timer->async_wait(boost::bind(&rtp::Connection::wait_for_death, this));


}

void rtp::Connection::wait_for_death()
{
	socket_->delete_connection(remote_endpoint_);

}

void rtp::Connection::async_send(boost::shared_ptr<data_buffer> data_buff, boost::function<void()> send_handler)
{
	set_send_handler(data_buff, send_handler);
	send();


}

void rtp::Connection::set_send_handler(boost::shared_ptr<data_buffer> write_buff_, boost::function<void()> send_handler)
{
	this->send_handler = send_handler;
	write_buff = write_buff_;
	valid_send_handler = true;

}

void rtp::Connection::send()
{
	if (valid_send_handler && is_valid())
	{

		boost::shared_ptr<data_buffer> message(package_message());
		socket_->udp_send_to(message, remote_endpoint_, boost::bind(&rtp::Connection::handle_send, this, message,
			sequence_no,
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred));
		call_send_handler();

	}
}
void rtp::Connection::call_send_handler()
{
	if ((unsigned) sequence_no >= write_buff->size() )
	{
		valid_send_handler = false;
		send_handler();

	}

}

void rtp::Connection::handle_send(boost::shared_ptr<data_buffer> message, int next_seq_no,
	const boost::system::error_code& error,
	std::size_t bytes_transferred)

{
	auto timer = new_timer(socket_->get_io_service(), boost::posix_time::milliseconds(200));
	timer->async_wait(boost::bind(&rtp::Connection::handle_send_timeout, this,
		message,
		next_seq_no,
		timer,
		error,
		bytes_transferred));

}

void rtp::Connection::handle_send(boost::shared_ptr<data_buffer> message,
	int next_seq_no,
	boost::shared_ptr<boost::asio::deadline_timer> timer, 
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	timer->expires_at(timer->expires_at() + boost::posix_time::milliseconds(200));

	timer->async_wait(boost::bind(&rtp::Connection::handle_send_timeout, this,
		message,
		next_seq_no,
		timer,
		error,
		bytes_transferred));

}
void rtp::Connection::handle_send_timeout(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred)
{

	if(DEBUG) std::cout << "GOT TO HANDLE CONNECTION TIMEOUT" <<std::endl;
	if (sequence_no < next_seq_no && !error && is_valid())
	{

		if (DEBUG) 
		{
			std::cout << "Timeout occurred at sequence_no " << next_seq_no << std::endl;
			std::cout << "Resending packets from " << sequence_no << std::endl;
		}
		congestion_window = congestion_window /2;
		boost::shared_ptr<data_buffer> message(package_message());
	    socket_->udp_send_to(message, remote_endpoint_,
	    	boost::bind(&rtp::Connection::handle_send, this,
	    		message,
	    		next_seq_no,
	    		timer,
	    		boost::asio::placeholders::error,
	    		boost::asio::placeholders::bytes_transferred));
	    inc_timeout();
	}

	else
	{

		if (error)
		{
			std::cout << "There was an error, closing timer" << std::endl;
		}
		delete_timer(timer);
	}
}

boost::shared_ptr<data_buffer> rtp::Connection::package_message()
{
	boost::shared_ptr<data_buffer> complete_msg(boost::make_shared<data_buffer>(0));
	int write_index(sequence_no);
	for (int i = 0; i < congestion_window; i++)
	{
		int amount_to_send = std::min((int)930, (int)write_buff->size() - write_index);
		if (amount_to_send > 0)
		{
			boost::shared_ptr<data_buffer> tmp_buff(boost::make_shared<data_buffer>(write_buff->begin()+write_index,
				write_buff->begin()+write_index+amount_to_send));

			boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>(0);
			SegmentPtr dataseg= boost::make_shared<rtp::Segment>();
			dataseg->set_sequence_no(write_index);
			std::string data(tmp_buff->begin(), tmp_buff->end());
			dataseg->set_data(data);
			dataseg->set_header_checksum(create_header_checksum(dataseg));
			dataseg->set_data_checksum(create_data_checksum(dataseg));
			PackedMessage<rtp::Segment> packeddata(dataseg);
			packeddata.pack(*message);
			complete_msg->insert(complete_msg->end(), message->begin(), message->end());
			write_index += amount_to_send;

		}
		else
		{
			break;
		}
	}
	return complete_msg;


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
	int buffer_position(0);
	bool is_data(false);
	while (buffer_position < (int)m_readbuf->size() && is_valid())
	{
		PackedMessage<rtp::Segment> m_packed_segment(boost::make_shared<rtp::Segment>());
		//std::cout << show_hex(*m_readbuf) <<std::endl;

	    int msg_len = m_packed_segment.decode_header(*m_readbuf, buffer_position);

	    m_packed_segment.unpack(*m_readbuf, msg_len, buffer_position);
		SegmentPtr rcvdseg = m_packed_segment.get_msg();

		if (check_header_checksum(rcvdseg) && check_data_checksum(rcvdseg))
		{
			if (rcvdseg->ack())
			{
				sequence_no = rcvdseg->sequence_no();
				congestion_window += 1;
				send();
				reset_timeout();
			}
			else if (rcvdseg->sequence_no() == sequence_no)
			{

				is_data = true;
	 
				std::string data_s(rcvdseg->data());
				//if ((int)(rcv_window->size() + data_s.size())  < window_size)
				{
					rcv_window->insert(rcv_window->end(), data_s.begin(), data_s.end());

					// rcv_window->insert(rcv_window->end(), m_readbuf->begin(), m_readbuf->end());
					sequence_no += (int)data_s.size();
				}
				reset_timeout();
			}
			else if (rcvdseg->fin())
			{
				handle_fin();
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
		std::cout << m_readbuf->size() <<std::endl;
		std::cout << buffer_position <<std::endl;
		std::cout << "____________________________________________-" << std::endl;

		buffer_position+=msg_len + HEADER_SIZE;
	}
	if (is_data)
	{
		send_ack();
	}

	call_rcv_handler();

}

void rtp::Connection::send_ack()
{

	boost::shared_ptr<data_buffer> ack = boost::make_shared<data_buffer>(0);
	SegmentPtr ackseg= boost::make_shared<rtp::Segment>();
	ackseg->set_ack(true);
	ackseg->set_sequence_no(sequence_no);
	std::cout << "ACK " << sequence_no <<std::endl;
	ackseg->set_header_checksum(create_header_checksum(ackseg));
	PackedMessage<rtp::Segment> packeddata(ackseg);
	packeddata.pack(*ack);
	socket_->udp_send_to(ack, remote_endpoint_,
		boost::bind(&rtp::Connection::handle_ack, this,
			ack,
			sequence_no,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

}

void rtp::Connection::handle_ack(boost::shared_ptr<data_buffer> message, 
		int next_seq_no,
		const boost::system::error_code& error, 
		std::size_t bytes_transferred)
{
	auto timer = new_timer(socket_->get_io_service(), boost::posix_time::milliseconds(200));
	timer->async_wait(boost::bind(&rtp::Connection::handle_ack_timeout, this,
		message,
		next_seq_no,
		timer,
		error,
		bytes_transferred));

}

void rtp::Connection::handle_ack(boost::shared_ptr<data_buffer> message, 
		int next_seq_no,
		boost::shared_ptr<boost::asio::deadline_timer> timer,
		const boost::system::error_code& error, 
		std::size_t bytes_transferred)
{
	timer->expires_at(timer->expires_at() + boost::posix_time::milliseconds(200));

	timer->async_wait(boost::bind(&rtp::Connection::handle_ack_timeout, this,
		message,
		next_seq_no,
		timer,
		error,
		bytes_transferred));
}

void rtp::Connection::handle_ack_timeout(boost::shared_ptr<data_buffer> message,
	int next_seq_no,
	boost::shared_ptr<boost::asio::deadline_timer> timer,
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	if (is_valid())
	{
		if(DEBUG) std::cout << "GOT TO HANDLE CONNECTION TIMEOUT" <<std::endl;
		if (sequence_no <= next_seq_no && !error)
		{

			if (DEBUG) 
			{
				std::cout << "Timeout occurred at sequence_no " << next_seq_no << std::endl;
				std::cout << "Resending ack from " << sequence_no << std::endl;
			}
		    socket_->udp_send_to(message, remote_endpoint_,
		    	boost::bind(&rtp::Connection::handle_ack, this,
		    		message,
		    		next_seq_no,
		    		timer,
		    		boost::asio::placeholders::error,
		    		boost::asio::placeholders::bytes_transferred));
		    inc_timeout();
		}

		else
		{

			if (error)
			{
				std::cout << "There was an error, closing timer" << std::endl;
			}
			delete_timer(timer);
		}
	}
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

void rtp::Connection::inc_timeout()
{
	if (timeout_count < MAX_TIMEOUT_COUNT)
	{
		timeout_count++;
	}
	else
	{
		handle_fin();
	}
}

void rtp::Connection::reset_timeout()
{
	timeout_count = 0;
}