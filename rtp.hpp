#ifndef RTP_H
#define RTP_H

#define DEBUG true
#define BUFFER_SIZE 10000

namespace rtp
{

	
	std::string get_endpoint_str(boost::asio::ip::udp::endpoint remote_endpoint_);
	boost::uint32_t create_checksum(uint8_t* bytes);
	boost::uint32_t create_header_checksum(boost::shared_ptr<rtp::Segment> segment);
	boost::uint32_t create_data_checksum(boost::shared_ptr<rtp::Segment> segment);
	bool check_header_checksum(boost::shared_ptr<rtp::Segment> segment);
	bool check_data_checksum(boost::shared_ptr<rtp::Segment> segment);

	class Connection;
	class Acceptor;
	class Socket :  public boost::enable_shared_from_this<Socket>
	{
	public:
		Socket(boost::asio::io_service& io_service_,std::string source_ip, std::string source_port);
		boost::shared_ptr<Connection> create_connection(std::string ip, std::string port);
		boost::asio::io_service& get_io_service();
		void start_receive();
		boost::shared_ptr<Socket> this_shared();
		void close_connection(boost::asio::ip::udp::endpoint connection_endpoint);
	private:
		void multiplex(boost::shared_ptr<std::vector<uint8_t>> dbuf, const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		void connection_establishment(boost::shared_ptr<std::vector<uint8_t>> dbuf, boost::shared_ptr<Connection> connection);


		void handle_connection_est(boost::shared_ptr<std::vector<uint8_t>> message,
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		void handle_connection_est(boost::shared_ptr<std::vector<uint8_t>> message,
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		void handle_connection_timeout(boost::shared_ptr<std::vector<uint8_t>> message, 
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error,
			std::size_t bytes_transferred);

		boost::asio::io_service& io_service_;
		boost::asio::ip::udp::socket socket_;
		std::unordered_map<std::string, boost::shared_ptr<Connection>> connections;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		std::string source_port;
		std::string source_ip;
	};

	class Connection
	{
	public:
		Connection(boost::asio::ip::udp::endpoint remote_endpoint_, boost::shared_ptr<rtp::Socket> socket_);
		bool is_valid();
		void set_valid(bool val);
		void handle_send(boost::shared_ptr<std::vector<uint8_t>> message,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);
		int get_sequence_no();
		void set_sequence_no(int new_seq_no);
		void inc_sequence_no();
		boost::asio::ip::udp::endpoint get_endpoint();
		void close_connection();
		void inc_timeout_exp();
		int get_timeout_seconds();
		void reset_timeout();
		boost::shared_ptr<boost::asio::deadline_timer> new_timer(boost::asio::io_service& io, 
			boost::posix_time::milliseconds milliseconds);
		void delete_timer(boost::shared_ptr<boost::asio::deadline_timer> timer);

	private:
		// std::vector<boost::uint8_t> write_buff;
		// std::vector<boost::uint8_t> rcv_buff;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		boost::shared_ptr<rtp::Socket> socket_;
		std::vector<boost::shared_ptr<boost::asio::deadline_timer>> timer_vec;
		// PackedMessage<Segment> m_packed_segment;
		// int buffer_position;
		std::string dest_ip;
		std::string dest_port;
		int sequence_no;
		bool valid;
		int timeout_exp;

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