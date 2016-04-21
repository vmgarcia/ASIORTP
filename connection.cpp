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
	timeout_count(0),
	old_sequence_no(0),
	write_index(0),
	send_sequence_no(0)
{
}

bool rtp::Connection::is_valid()
{
	// if (DEBUG && valid) 
	// {
	// 	std::cout << "is valid"<<std::endl;
	// }
	// else if (DEBUG)
	// {
	// 	std::cout << "not valid" << std::endl;
	// }
	return valid;
}

void rtp::Connection::set_valid(bool val)
{
	if(DEBUG && val) std::cerr << "Connection Created: " << rtp::get_endpoint_str(remote_endpoint_) 
		<< std::endl;
	if(DEBUG && !val) std::cerr << "Connection Destroyed: " << rtp::get_endpoint_str(remote_endpoint_) 
		<<std::endl;
	reset_timeout();
	valid = val;
	if (val)
	{
		write_index = 0;

		old_sequence_no = send_sequence_no;
		if(DEBUG)
		{
			std::cout<<"UPDATING SEQUENCE NO: " << old_sequence_no<<std::endl;
		}
	}
}
void rtp::Connection::close_connection()
{
	boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>(0);
	SegmentPtr finseg= boost::make_shared<rtp::Segment>();

	finseg->set_fin(true);
	finseg->set_header_checksum(create_header_checksum(finseg));
	PackedMessage<rtp::Segment> packeddata(finseg);
	packeddata.pack(*message);
	std::cout << "SENDING FIN" << std::endl;
	socket_->udp_send_to(message, remote_endpoint_, boost::bind(&rtp::Connection::handle_fin, this,
		boost::asio::placeholders::error, 
		boost::asio::placeholders::bytes_transferred));


}
// void rtp::Connection::handle_fin()
// {
// 	auto timer = new_timer(socket_->get_io_service(), boost::posix_time::milliseconds(300));
// 	timer->async_wait(boost::bind(&rtp::Connection::wait_for_death, this));


// }
void rtp::Connection::handle_fin(	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{

	if (send_handler) send_handler(true);
	if (rcv_handler) rcv_handler(true);
	set_valid(false);
	sequence_no = -2;
	timeout_count = 0;
	congestion_window =1;
	rcv_window = boost::make_shared<data_buffer>(0);
	valid_rcv_handler = false;
	valid_send_handler = false;
	valid_send_handler = false;
	remote_window_size = 0;
	old_sequence_no=0;
	write_index=0;
	send_sequence_no=0;
}


void rtp::Connection::async_send(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> send_handler)
{
	set_send_handler(data_buff, send_handler);
	send();


}

void rtp::Connection::set_send_handler(boost::shared_ptr<data_buffer> write_buff_, boost::function<void(bool)> send_handler)
{
	this->send_handler = send_handler;
	write_buff = write_buff_;
	valid_send_handler = true;
	if (is_valid())
	{

		write_index = 0;

		old_sequence_no = send_sequence_no;
		if(DEBUG)
		{
			std::cout<<"UPDATING SEQUENCE NO: " << old_sequence_no<<std::endl;
		}
	

	}

}

void rtp::Connection::send()
{
	if (valid_send_handler && is_valid())
	{

		boost::shared_ptr<data_buffer> message(package_message());
		if (message->size() > 0)
		{
			std::cout<<"SENDING PART OF IT" <<std::endl;
			socket_->udp_send_to(message, remote_endpoint_, boost::bind(&rtp::Connection::handle_send, this, message,
				send_sequence_no,
				boost::asio::placeholders::error, 
				boost::asio::placeholders::bytes_transferred));
		}
		call_send_handler();

	}
}
void rtp::Connection::call_send_handler()
{
	if ((unsigned) send_sequence_no >= write_buff->size() + old_sequence_no && valid_send_handler )
	{
		valid_send_handler = false;
		send_handler(false);
		old_sequence_no=send_sequence_no;

	}

}

void rtp::Connection::handle_send(boost::shared_ptr<data_buffer> message, int next_seq_no,
	const boost::system::error_code& error,
	std::size_t bytes_transferred)

{
	if (!error)
	{
		auto timer = new_timer(socket_->get_io_service(), boost::posix_time::milliseconds(200));
		timer->async_wait(boost::bind(&rtp::Connection::handle_send_timeout, this,
			message,
			next_seq_no,
			timer,
			error,
			bytes_transferred));
	}

}

void rtp::Connection::handle_send(boost::shared_ptr<data_buffer> message,
	int next_seq_no,
	boost::shared_ptr<boost::asio::deadline_timer> timer, 
	const boost::system::error_code& error,
	std::size_t bytes_transferred)
{
	if (!error)
	{
		timer->expires_at(timer->expires_at() + boost::posix_time::milliseconds(200));

		timer->async_wait(boost::bind(&rtp::Connection::handle_send_timeout, this,
			message,
			next_seq_no,
			timer,
			error,
			bytes_transferred));
	}
	else
	{
		delete_timer(timer);
	}

}
void rtp::Connection::handle_send_timeout(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred)
{

	if(DEBUG) 
	{
		std::cout << "GOT TO HANDLE SEND TIMEOUT" <<std::endl;
		std::cout << send_sequence_no << "<- SEQUENCE NO\n";
		std::cout << next_seq_no << "<- NEXT SEQ NO" << std::endl;
	}
	if (send_sequence_no <= next_seq_no && !error && is_valid())
	{

		if (DEBUG) 
		{
			std::cout << "Timeout occurred at sequence_no " << next_seq_no << std::endl;
			std::cout << "Resending packets from " << send_sequence_no << std::endl;
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
	int pack_index(write_index);
	boost::shared_ptr<data_buffer> complete_msg(boost::make_shared<data_buffer>(0));
	if(false)
	{
		std::cout << "CONGESTION WINDOW GOD DAMNIT" << std::endl;
		std::cout << congestion_window <<std::endl;
	}
	for (int i = 0; i < congestion_window; i++)
	{
		int amount_to_send = std::min((int)930, (int)write_buff->size() - pack_index);
		if (amount_to_send > 0)
		{
			boost::shared_ptr<data_buffer> tmp_buff(boost::make_shared<data_buffer>(write_buff->begin()+pack_index,
				write_buff->begin()+pack_index+amount_to_send));

			boost::shared_ptr<data_buffer> message = boost::make_shared<data_buffer>(0);
			SegmentPtr dataseg= boost::make_shared<rtp::Segment>();
			dataseg->set_sequence_no(old_sequence_no + pack_index);
			std::string data(tmp_buff->begin(), tmp_buff->end());
			dataseg->set_data(data);
			if (DEBUG)
			{
				std::cout<< "PACKING THIS DATA \n";
				std::cout <<dataseg->data()<<std::endl;
				std::cout << "SEQUENCE NO OF THIS DATA\n";
				std::cout << dataseg->sequence_no() <<std::endl;
			}
			dataseg->set_header_checksum(create_header_checksum(dataseg));
			dataseg->set_data_checksum(create_data_checksum(dataseg));
			PackedMessage<rtp::Segment> packeddata(dataseg);
			packeddata.pack(*message);
			complete_msg->insert(complete_msg->end(), message->begin(), message->end());
			pack_index += amount_to_send;

		}
		else
		{
			break;
		}
	}
	return complete_msg;


}


void rtp::Connection::async_rcv(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> rcv_handler)
{
	set_rcv_handler(data_buff, rcv_handler);
	if (rcv_window->size() > 0)
	{
		if(DEBUG2) std::cout << "GOT TO THE RCV HANDLER" <<std::endl;
		call_rcv_handler();
	}

}




void rtp::Connection::set_rcv_handler(boost::shared_ptr<data_buffer> pass_back_buffer_, boost::function<void(bool)> rcv_handler)
{
	this->rcv_handler = rcv_handler;
	pass_back_buffer = pass_back_buffer_;
	valid_rcv_handler = true;
	// write_index=0;
	// old_sequence_no=send_sequence_no;

}

void rtp::Connection::call_rcv_handler()
{
	if (valid_rcv_handler)
	{
		if (DEBUG && false)
		{
			std::ofstream writer;
			writer.open("copy3.txt", std::ios_base::app |std::ios::ate |std::ios::out);
			for (unsigned i =0; i < rcv_window->size(); i++)
			{
				writer << (*rcv_window)[i];
			}
			writer << std::endl;
		}
		if (DEBUG2)
		{
			std::string dat(rcv_window->begin(), rcv_window->end());
			std::cout << "PUTTING THIS IN PASS_BACK_BUFFER" << std::endl;
			std::cout << dat <<std::endl;
		}
		pass_back_buffer->insert(pass_back_buffer->begin(), rcv_window->begin(), rcv_window->end());
		rcv_window = boost::make_shared<data_buffer>(0);
		valid_rcv_handler = false;

		rcv_handler(false);
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

				if (DEBUG) std::cout << "RECEIVED ACK FOR: " << rcvdseg->sequence_no() <<std::endl;
				if (rcvdseg->sequence_no() >= send_sequence_no)
				{
					send_sequence_no = rcvdseg->sequence_no();
					write_index = send_sequence_no -old_sequence_no;
					if(DEBUG)
					{
						std::cout << "NEW WRITE INDEX: " << write_index <<std::endl;
						std::cout << "OLD SEQUENCE NO: " << old_sequence_no <<std::endl;

					}
				}
				inc_congestion();
				reset_timeout();
				send();
			
			}
			else if (rcvdseg->fin())
			{
				std::cout << "GOT FIN, CLOSING" << std::endl;
				close_connection();
				break;
			}
			else if (rcvdseg->sequence_no() == sequence_no)
			{

				is_data = true;
	 
				//if ((int)(rcv_window->size() + data_s.size())  < window_size)
				{
					rcv_window->insert(rcv_window->end(), rcvdseg->data().begin(), rcvdseg->data().end());

					// rcv_window->insert(rcv_window->end(), m_readbuf->begin(), m_readbuf->end());
					sequence_no += (int)rcvdseg->data().size();
					if(DEBUG)
					{
						std::cout << "SEQUENCE NO OF THIS DATA" << "\n";
						std::cout << rcvdseg->sequence_no() << "\n";
						std::cout << "INCREASE SEQUENCE NO " << sequence_no << "\n";
						// std::cout << "RECEIVED THIS DATA\n";
						// std::cout << rcvdseg->data() <<std::endl;
					}	
				}
				reset_timeout();
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
		// std::cout << m_readbuf->size() <<std::endl;
		// std::cout << buffer_position <<std::endl;
		// std::cout << "____________________________________________" << std::endl;

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
	if(DEBUG)
	{
		std::cout << "ACK " << sequence_no <<std::endl;
	}
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
		if(DEBUG) 
		{
			std::cout << "GOT TO HANDLE ACK TIMEOUT" <<std::endl;
			std::cout << sequence_no << std::endl;
			std::cout << next_seq_no << std::endl;
		}

		if (sequence_no <= next_seq_no && !error)
		{

			if (DEBUG) 
			{
				std::cout << "Timeout occurred at sequence_no " << next_seq_no << std::endl;
				std::cout << "Resending ack from " << sequence_no << std::endl;
			}
			send_ack();
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
	timer->cancel();
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
		close_connection();
	}
}

void rtp::Connection::reset_timeout()
{
	timeout_count = 0;
}

void rtp::Connection::inc_congestion()
{
	if (false)
	{
		std::cout << "REMOTE WINDOW SIZE" <<std::endl;
		std::cout << remote_window_size <<std::endl;
		std::cout << congestion_window <<std::endl;
	}
	if (congestion_window < remote_window_size/930 && 
		congestion_window < 50000/930)
	{
		congestion_window++;
	}
}

void rtp::Connection::set_remote_window_size(int size)
{

	remote_window_size = size;
}