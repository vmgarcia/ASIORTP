#include <iostream>
#include <vector>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

using boost::asio::ip::tcp;
int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: client <host>" << std::endl;
			return 1; 
		}

		std::vector<std::string> ip_vector;
		boost::split(ip_vector, argv[1], boost::is_any_of(":"));

		std::string ip_addr = ip_vector[0];
		std::string port = ip_vector[1];

		std::cout << "Connecting to IP: " << ip_addr << " Port: " << port << std::endl;

		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}