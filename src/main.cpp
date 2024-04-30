#include "Server.hpp"

static bool isValidPort(std::string port);

int main(int argc, char **argv){
	Server ser;
	if (argc != 3){
		std::cout << "Usage: " << argv[0] << " <port_number> <password>" << std::endl;
		return 1;
	}
	std::cout << "---- SERVER ----" << std::endl;
	try	{
		signal(SIGINT, Server::signalHandler);
		signal(SIGQUIT, Server::signalHandler);
		if (!isValidPort(argv[1]) || !*argv[2] || std::strlen(argv[2]) > 20){
			std::cout << "invalid Port number / Password" << std::endl;
			return 1;
		}
		ser.init(std::atoi(argv[1]), argv[2]);
	} catch (const std::exception &e) {
		ser.closeFds();
		std::cerr << e.what() << std::endl;
	}
	std::cout << "Server closed" << std::endl;
}

static bool isValidPort(std::string port){
	return (port.find_first_not_of("0123456789") == std::string::npos &&
			std::atoi(port.c_str()) >= 1024 && std::atoi(port.c_str()) <= 65535);
}
