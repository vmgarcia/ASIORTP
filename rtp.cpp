#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
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
rtp::Socket::Socket(boost::asio::io_service io_service_, std::string source_port): 
	socket_(io_service_, udp::endpoint(udp::v4(), std::stoi(source_port))),
	source_port(source_port)
{

}

//rtp::Socket::Socket()

// initialize connection object
boost::shared_ptr<rtp::Connection> rtp::Socket::create_connection(std::string dest_ip, std::string dest_port)
{
	boost::shared_ptr<rtp::Connection> connection(new Connection{socket_, dest_ip, dest_port});
	return connection;
}


// use three way handshake to create connection
rtp::Connection::Connection(boost::asio::ip::udp::socket& socket_, std::string dest_ip, std::string dest_port):
	socket_(socket_),
	write_buff(),
	rcv_buff(),
	dest_ip(dest_ip),
	dest_port(dest_port),
	m_packed_segment(SegmentPtr(new rtp::Segment()))

{
	write_buff.reserve(BUFFER_SIZE);
	rcv_buff.reserve(BUFFER_SIZE);

	udp::resolver resolver_(socket_.get_io_service());
	udp::resolver::query query_(udp::v4(), dest_ip, dest_port);
	remote_endpoint = *resolver_.resolve(query_);
	SegmentPtr segment_(new Segment);

	std::string source_port = std::to_string(socket_.local_endpoint().port());
	segment_->set_source_port(source_port);
	segment_->set_dest_port(dest_port);
	segment_->set_ack(true);
	PackedMessage<Segment> ack_msg(segment_);
	data_buffer tmp_buf;
	ack_msg.pack(tmp_buf);
	socket_.async_send_to(boost::asio::buffer(tmp_buf), remote_endpoint, 
		boost::bind(&rtp::Connection::wait_for_syn_ack, this, boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));


}

// void rtp::Connection::handle_seg_header(const boost::system::error_code& error)
// {

// }

void rtp::Connection::handle_syn_ack(const boost::system::error_code& error,
      std::size_t /*bytes_transferred*/)
{
    DEBUG && (cerr << "handle read " << error.message() << '\n');
    if (!error) {
        DEBUG && (cerr << "Got header!\n");
        DEBUG && (cerr << show_hex(m_readbuf) << endl);
        unsigned msg_len = m_packed_request.decode_header(m_readbuf);
        DEBUG && (cerr << msg_len << " bytes\n");
        start_read_body(msg_len);

    }

}

rtp::Connection
rtp::Acceptor
int main()
{

}