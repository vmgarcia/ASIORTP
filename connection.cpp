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
typedef boost::shared_ptr<rtp::Segment> SegmentPtr;
typedef std::vector<uint8_t> data_buffer;


rtp::Connection::Connection(udp::endpoint remote_endpoint_, boost::shared_ptr<rtp::Socket> socket_):
	remote_endpoint_(remote_endpoint_),
	socket_(socket_),
	timer_vec(),
	dest_ip(remote_endpoint_.address().to_string()),
	dest_port(std::to_string(remote_endpoint_.port())),
	sequence_no(-2),
	valid(false),
	timeout_exp(1)
{
}

bool rtp::Connection::is_valid()
{
	if (DEBUG) std::cout << "is valid" << std::endl;
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

boost::shared_ptr<boost::asio::deadline_timer> rtp::Connection::new_timer(
	boost::asio::io_service& io, 
	boost::posix_time::seconds seconds)
{
	auto timer = boost::make_shared<boost::asio::deadline_timer>(io, seconds);
	timer_vec.push_back(timer);
	return timer;
}

void rtp::Connection::delete_timer(boost::shared_ptr<boost::asio::deadline_timer> timer)
{
	auto it = std::find(timer_vec.begin(), timer_vec.end(), timer);
	if(it != timer_vec.end())
	    timer_vec.erase(it);

}

void rtp::Connection::handle_send(boost::shared_ptr<data_buffer> /*message*/,
			const boost::system::error_code& /*error*/, 
			std::size_t /*bytes_transferred*/)
{
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