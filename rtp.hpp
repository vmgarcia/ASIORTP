#ifndef RTP_H
#define RTP_H

namespace rtp
{

	class Connection;
	class Acceptor;
	class Socket
	{
	public:
		Socket(boost::asio::io_service io_service_, std::string source_port);
		boost::shared_ptr<Connection> create_connection(std::string ip, std::string port);
		Connection& get_connection(std::string ip, std::string port);
		void close_connection(std::string ip, std::string port);
		boost::asio::io_service& get_io_service();

	private:
		void finish_handshake(boost::asio::ip::udp::endpoint endpoint_);

		boost::asio::ip::udp::socket socket_;
		//boost::asio::io_service io_service_;
		//std::unordered_map<std::string, Connection> connections;
		std::string source_port;

	};

	class Connection
	{
	public:
		Connection(boost::asio::ip::udp::socket& socket_, std::string ip, std::string dest_port);
		void async_write(boost::function<void()> callback);
		void async_rcv(boost::function<void()> callback);
		void setup_connection(Socket socket_);

	private:
		void handle_seg_header(const boost::system::error_code& error);
		void handle_seg_body(const boost::system::error_code& error);
		void handle_syn_ack(const boost::system::error_code& error)

		boost::asio::ip::udp::socket& socket_;
		std::vector<boost::uint8_t> write_buff;
		std::vector<boost::uint8_t> rcv_buff;
		std::string dest_ip;
		std::string dest_port;
		boost::asio::ip::udp::endpoint remote_endpoint;
		PackedMessage<Segment> m_packed_segment;
	};

	class Acceptor
	{
	public:
		Acceptor(boost::asio::io_service io_service_, boost::asio::ip::udp::endpoint endpoint_);
		void async_accept(Socket& socket_, boost::function<void()> accept_handler);

	private:
		void receive_final_ack(const boost::system::error_Code& error);
		boost::asio::io_service io_service_;
		boost::asio::ip::udp::endpoint endpoint_;
	};
};

#endif