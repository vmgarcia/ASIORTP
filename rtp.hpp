#ifndef RTP_H
#define RTP_H

namespace rtp
{
	class socket
	{
	public:
		socket(boost::asio::ip::io_service io_service_): io_service_(io_service_);
		connection& create_connection(std::string ip, std::string port);
		connection& get_connection(std::string ip, std::string port);
		void close_connection(std::string ip, std::string port);

	private:
		boost::asio::ip::udp::socket socket_;
		boost::asio::io_service io_service_;
		std::unordered_map<std::string, connection> connections;

	};

	class connection
	{
	public:
		void async_write(boost::function<void()> callback);
		void async_rcv(boost::function<void()> callback);


	private:
		write_buff;
		rcv_buff;
		std::string dest_ip;
		std::string dest_port;
	};
};

#endif