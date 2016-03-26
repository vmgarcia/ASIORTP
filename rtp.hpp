#ifndef RTP_H
#define RTP_H

namespace rtp
{

	
	std::string get_endpoint_str(boost::asio::ip::udp::endpoint remote_endpoint_);

	class Connection;
	class Acceptor;
	class Socket
	{
	public:
		Socket(boost::asio::io_service io_service_, std::string source_port);
		boost::shared_ptr<Connection> create_connection(std::string ip, std::string port);
		boost::asio::io_service& get_io_service();
		void start_receive();
	private:
		void multiplex(std::vector<uint8_t>& dbuf, const boost::system::error_code& error, 
			std::size_t bytes_transferred);
		void connection_establishment(std::vector<uint8_t>& dbuf, boost::shared_ptr<Connection> connection);
		void handle_send(bool finished, boost::shared_ptr<std::string> message,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		boost::asio::ip::udp::socket socket_;
		std::unordered_map<std::string, boost::shared_ptr<Connection>> connections;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		std::string source_port;

	};

	class Connection
	{
	public:
		Connection(boost::asio::ip::udp::endpoint remote_endpoint_);
		bool valid();
		bool set_valid(bool val);
	private:

		//boost::asio::ip::udp::socket& socket_;
		// std::vector<boost::uint8_t> write_buff;
		// std::vector<boost::uint8_t> rcv_buff;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		// PackedMessage<Segment> m_packed_segment;
		// int buffer_position;
		std::string dest_ip;
		std::string dest_port;

		bool valid;
	};

	// class Acceptor
	// {
	// public:
	// 	Acceptor(boost::asio::io_service io_service_, boost::asio::ip::udp::endpoint endpoint_);
	// 	void async_accept(Socket& socket_, boost::function<void()> accept_handler);

	// private:
	// 	void receive_final_ack(const boost::system::error_Code& error);
	// 	boost::asio::io_service io_service_;
	// 	boost::asio::ip::udp::endpoint endpoint_;
	// };
};

#endif