#ifndef RTP_H
#define RTP_H

#define DEBUG true
#define DEBUG2 false
#define MAX_DATAGRAM_SIZE 50000
#define MAX_TIMEOUT_COUNT 20
#define DEFAULT_WINDOW_SIZE 1000
namespace rtp
{

	typedef boost::shared_ptr<rtp::Segment> SegmentPtr;
	typedef std::vector<uint8_t> data_buffer;
	typedef boost::function<void(const boost::system::error_code&, const std::size_t)> handler_t;


	std::string get_endpoint_str(boost::asio::ip::udp::endpoint remote_endpoint_);
	boost::uint32_t create_checksum(uint8_t* bytes);
	boost::uint32_t create_header_checksum(SegmentPtr segment);
	boost::uint32_t create_data_checksum(SegmentPtr segment);
	bool check_header_checksum(SegmentPtr segment);
	bool check_data_checksum(SegmentPtr segment);

	class Connection;
	class Socket :  public boost::enable_shared_from_this<Socket>
	{
	public:
		Socket(boost::asio::io_service& io_service_,std::string source_ip, std::string source_port, int max_window_size=DEFAULT_WINDOW_SIZE);
		boost::shared_ptr<Connection> create_connection(std::string ip, std::string port);
		boost::asio::io_service& get_io_service();
		void start_receive();
		boost::shared_ptr<Socket> this_shared();
		void delete_connection(boost::asio::ip::udp::endpoint connection_endpoint);

		void udp_send_to(boost::shared_ptr<data_buffer> message, 
			boost::asio::ip::udp::endpoint endpoint_,
			handler_t send_handler);

		void create_receiver(boost::function<void(boost::shared_ptr<rtp::Connection>)> receiver_);
		void close();



	private:
		void multiplex(boost::shared_ptr<data_buffer> dbuf, const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		void connection_establishment(boost::shared_ptr<data_buffer> dbuf, boost::shared_ptr<Connection> connection);


		void handle_connection_est(boost::shared_ptr<data_buffer> message,
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		void handle_connection_est(boost::shared_ptr<data_buffer> message,
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		void handle_connection_timeout(boost::shared_ptr<data_buffer> message, 
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
		bool is_server;
		boost::function<void(boost::shared_ptr<rtp::Connection>)> receiver;
		bool valid;
		int max_window_size;
	};

	class Connection
	{
	public:
		Connection(boost::asio::ip::udp::endpoint remote_endpoint_, boost::shared_ptr<rtp::Socket> socket_, int window_size=DEFAULT_WINDOW_SIZE);
		bool is_valid();
		void set_valid(bool val);

		void handle_send(boost::shared_ptr<data_buffer> message, int next_seq_no,
			const boost::system::error_code& error,
			std::size_t bytes_transferred);


		void handle_send(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error,
			std::size_t bytes_transferred);

		void handle_send_timeout(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		void handle_ack(boost::shared_ptr<data_buffer> message, int next_seq_no,
			const boost::system::error_code& error,
			std::size_t bytes_transferred);


		void handle_ack(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error,
			std::size_t bytes_transferred);

		void handle_ack_timeout(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		int get_sequence_no();
		void set_sequence_no(int new_seq_no);
		void inc_sequence_no();
		boost::asio::ip::udp::endpoint get_endpoint();
		void close_connection();
		boost::shared_ptr<boost::asio::deadline_timer> new_timer(boost::asio::io_service& io, 
			boost::posix_time::milliseconds milliseconds);
		void delete_timer(boost::shared_ptr<boost::asio::deadline_timer> timer);
		void async_receive(boost::function<void()> accept_handler);
		void async_send(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> send_handler);
		void async_rcv(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> rcv_handler);
		void handle_rcv(boost::shared_ptr<data_buffer> message);
		void set_rcv_handler(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> rcv_handler);
		void call_rcv_handler();
		void set_send_handler(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> send_handler);
		void call_send_handler();
		boost::shared_ptr<data_buffer> package_message();
		void send();
		void send_ack();
		void handle_fin();
		void handle_fin(const boost::system::error_code& error, 
			std::size_t bytes_transferred);
		void wait_for_death();
		void inc_timeout();
		void reset_timeout();
		void inc_congestion();
		void set_remote_window_size(int size);



	private:
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
		int congestion_window;
		int window_size;
		boost::shared_ptr<data_buffer> rcv_window;
		boost::function<void(bool)> rcv_handler;
		bool valid_rcv_handler;
		boost::shared_ptr<data_buffer> write_buff;
		boost::function<void(bool)> send_handler;
		bool valid_send_handler;
		boost::shared_ptr<data_buffer> pass_back_buffer;
		int timeout_count;
		int remote_window_size;
		int old_sequence_no;
		int write_index;
		int send_sequence_no;

	};


};

#endif