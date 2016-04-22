#ifndef RTP_H
#define RTP_H

#define DEBUG false
#define DEBUG2 false
#define MAX_DATAGRAM_SIZE 50000
#define MAX_TIMEOUT_COUNT 10000
#define DEFAULT_WINDOW_SIZE 1000
namespace rtp
{

	typedef boost::shared_ptr<rtp::Segment> SegmentPtr;
	typedef std::vector<uint8_t> data_buffer;
	typedef boost::function<void(const boost::system::error_code&, const std::size_t)> handler_t;


	//get_endpoint_str - function for creating string in the form of "ip:port"
	std::string get_endpoint_str(boost::asio::ip::udp::endpoint remote_endpoint_);

	//create_checksum - creates checksum from array of uint8_t bytes 
	boost::uint32_t create_checksum(uint8_t* bytes);

	//create_header_checksum - creates checksum based on header info of segment
	boost::uint32_t create_header_checksum(SegmentPtr segment);

	// create_data_checksum - creates checksum based on the data stored in the segment
	boost::uint32_t create_data_checksum(SegmentPtr segment);

	// checks whether the stored header_checksum in the segment matches the header
	bool check_header_checksum(SegmentPtr segment);

	// checks whether the stored data_checksum in the segment matches the data
	bool check_data_checksum(SegmentPtr segment);

	class Connection;

	// Socket is a class that represents an RTP socket. 
	// Hides UDP semantics and provides methods for creating connections to other RTP sockets
	class Socket :  public boost::enable_shared_from_this<Socket>
	{
	public:
		// Constructor for Socket
		// args:
		// io_service_ - this is an asio io_service for passing to async operations
		// source_ip - the ip address of this socket
		// source_port - the port of this socket
		// max_window_size - the size of the receive window for this socket
		Socket(boost::asio::io_service& io_service_,std::string source_ip, std::string source_port, int max_window_size=DEFAULT_WINDOW_SIZE);
		
		// create a connection with another RTP socket
		boost::shared_ptr<Connection> create_connection(std::string ip, std::string port);

		// get the io_service that this socket is using
		boost::asio::io_service& get_io_service();

		// this functions makes the Socket listen for any incoming data or connection requests
		void start_receive();

		// returns a shared_ptr pointing to a this pointer of a Socket object
		boost::shared_ptr<Socket> this_shared();

		// deletes a connection from the map of connections based on an endpoint which is its index
		void delete_connection(boost::asio::ip::udp::endpoint connection_endpoint);

		// wrapper for async_send_to method of udp socket that Socket wraps
		void udp_send_to(boost::shared_ptr<data_buffer> message, 
			boost::asio::ip::udp::endpoint endpoint_,
			handler_t send_handler);

		// create_receiver is a function used by users wanting to create a server. It passes a connection object
		// to the callback function receiver_ whenever a connection is established
		void create_receiver(boost::function<void(boost::shared_ptr<rtp::Connection>)> receiver_);

		// close the socket and stop the io_service
		void close();



	private:

		// multiplex, when data is received, figure out which connection it belongs to and provide
		// the data to it. If a connection isn't established yet, establish one
		void multiplex(boost::shared_ptr<data_buffer> dbuf, const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		// continue the process of establishing a connection or begin it
		void connection_establishment(boost::shared_ptr<data_buffer> dbuf, boost::shared_ptr<Connection> connection);


		// call back function that initiates a timeout. if the timeout happens that means
		// that a previous connection establishment packet needs to be resent
		void handle_connection_est(boost::shared_ptr<data_buffer> message,
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		// modified version of connection establishment function that takes in
		// as a paramater the timer object created by the first. 
		void handle_connection_est(boost::shared_ptr<data_buffer> message,
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		// if there is a timeout in the connection establishment process, resend the 
		// packet that was timed out
		void handle_connection_timeout(boost::shared_ptr<data_buffer> message, 
			boost::shared_ptr<rtp::Connection> connection,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error,
			std::size_t bytes_transferred);

		// io_service for boost asio
		boost::asio::io_service& io_service_;
		// udp socket used for actually sending the segments
		boost::asio::ip::udp::socket socket_;
		// map that contains connections index by a string created from the ip and port
		std::unordered_map<std::string, boost::shared_ptr<Connection>> connections;
		// the remote endpoint, used for sending data
		boost::asio::ip::udp::endpoint remote_endpoint_;
		// the source port
		std::string source_port;
		// the source ip
		std::string source_ip;
		// is it a server? this is set to true when the create_receiver function is called
		bool is_server;
		// the function set by create_receiver. 
		boost::function<void(boost::shared_ptr<rtp::Connection>)> receiver;
		// is the socket valid? this is set to true on Socket creation and false when the close() function is called
		bool valid;
		// the maximum size of the receive window
		int max_window_size;
	};

	// Connection class. Represents a connection between two RTP sockets
	// Connection is what is used by a user for sending and receiving data
	class Connection
	{
	public:
		// Connection constructor
		// remote_endpoint_ - an endpoint is an object in boost asio that represents the location of a socket. 
		// 		This endpoint represents the socket the connection is connected to
		// socket_ - this is a shared_ptr to the RTP Socket that created this connection
		// window_size - the size of the receive window
		Connection(boost::asio::ip::udp::endpoint remote_endpoint_, boost::shared_ptr<rtp::Socket> socket_, int window_size=DEFAULT_WINDOW_SIZE);
		
		// returns whether the connection is valid or not. Only set to false when the connection is closed
		bool is_valid();

		// sets whether the connection is valid or not
		void set_valid(bool val);

		// creates a timer for send timeouts. A send timeout occurs when an ack for a segment that is sent
		// doesn't occur after 200 ms
		void handle_send(boost::shared_ptr<data_buffer> message, int next_seq_no,
			const boost::system::error_code& error,
			std::size_t bytes_transferred);


		// extends the already created timer
		void handle_send(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error,
			std::size_t bytes_transferred);

		// 200ms after sending a segment, if an ack hasn't been received for that segment
		// then the congestion window is decreased and data is resent from the current sequence number
		void handle_send_timeout(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		// another helper function for creating a timer to handle timeouts after sending acks
		void handle_ack(boost::shared_ptr<data_buffer> message, int next_seq_no,
			const boost::system::error_code& error,
			std::size_t bytes_transferred);

		// this helper extends the already created timeout if new data hasn't been received
		// that has a sequence number greater than the sequence number of the ack that was sent
		void handle_ack(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer, 
			const boost::system::error_code& error,
			std::size_t bytes_transferred);

		// 200ms after sending an ack, if the sequenc number hasn't increased then resend the ack
		void handle_ack_timeout(boost::shared_ptr<data_buffer> message,
			int next_seq_no,
			boost::shared_ptr<boost::asio::deadline_timer> timer,
			const boost::system::error_code& error, 
			std::size_t bytes_transferred);

		// returns the sequence number of the connection
		int get_sequence_no();
		// sets the sequence number of the connection
		void set_sequence_no(int new_seq_no);
		// increments the sequence number
		void inc_sequence_no();
		// returns the endpoint of the connection
		boost::asio::ip::udp::endpoint get_endpoint();
		// close the connection, sends a fin packet to the remote endpoint
		void close_connection();
		// creates a new timer and stores it in the list of timerin the connection
		boost::shared_ptr<boost::asio::deadline_timer> new_timer(boost::asio::io_service& io, 
			boost::posix_time::milliseconds milliseconds);
		// deletes the newly created timer
		void delete_timer(boost::shared_ptr<boost::asio::deadline_timer> timer);
		// send data, once all the data has been sent, call the send handler
		void async_send(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> send_handler);
		// receive data, when the connection has data, pass it to the accept handler
		void async_rcv(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> rcv_handler);
		// when a segment is received, determine whether it's an ack or data and handle it accordingly
		void handle_rcv(boost::shared_ptr<data_buffer> message);
		// set the function that is called when data is received by the connection
		void set_rcv_handler(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> rcv_handler);
		// call the handler that was set in set_rcv_handler
		void call_rcv_handler();
		// set the function that is called once all the data has been sent
		void set_send_handler(boost::shared_ptr<data_buffer> data_buff, boost::function<void(bool)> send_handler);
		// call the function set by set_send_handler
		void call_send_handler();
		// package a segment based on all the parameters such as congestion window and receive window size
		boost::shared_ptr<data_buffer> package_message();
		// send data
		void send();
		// send an ack
		void send_ack();
		// handle a fin, aka close the connection
		void handle_fin();
		// handle a fin but with a callback style
		void handle_fin(const boost::system::error_code& error, 
			std::size_t bytes_transferred);
		// function used when waiting for connection call backs to end
		void wait_for_death();
		// increment the timeout
		void inc_timeout();
		// reset the timeout
		void reset_timeout();
		// increment the congestion window
		void inc_congestion();
		// set the remote window size. this is used in determining how many
		// segments and how much data to put in when calling package message
		void set_remote_window_size(int size);



	private:
		// endpoint of socket that is connected to
		boost::asio::ip::udp::endpoint remote_endpoint_;
		// the RTP Socket
		boost::shared_ptr<rtp::Socket> socket_;
		// a vector holding a bunch of timers that are used for timeouts
		std::vector<boost::shared_ptr<boost::asio::deadline_timer>> timer_vec;
		// the destination ip address
		std::string dest_ip;
		// the destination port number
		std::string dest_port;
		// the current sequence number
		int sequence_no;
		// the validity of the connection. set to valid after 3 way handshake
		// set invalid when connection is destroyed
		bool valid;
		// unused
		int timeout_exp;
		// the congestion window, how many packets in a segment
		int congestion_window;
		// the receive window size
		int window_size;
		// the actual receive window. this is packed with bytes of data
		boost::shared_ptr<data_buffer> rcv_window;
		// the function that is called once data is put in the rcv window
		boost::function<void(bool)> rcv_handler;
		// whether or not the rcv_handler is valid
		bool valid_rcv_handler;
		// the buffer that contains the data that is being sent
		boost::shared_ptr<data_buffer> write_buff;
		// the function called once all the data in the write_buff is sent
		boost::function<void(bool)> send_handler;
		// whether or not the send_handler is valid
		bool valid_send_handler;
		//the buffer that is used to pass the data in the receive window back 
		// to the person using the RTP 
		boost::shared_ptr<data_buffer> pass_back_buffer;
		// the count of timeouts that have occured conseqcutively
		int timeout_count;
		// the window size of the remote socket
		int remote_window_size;
		// the sequence number after the last send
		int old_sequence_no;
		// the index in the write buffer that is being sent
		int write_index;
		// the sequence number of the sent packets
		int send_sequence_no;

	};


};

#endif