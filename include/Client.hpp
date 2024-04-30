#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Server.hpp"
#include "Channel.hpp"

class Client{
	public:
		Client(void);
		Client(Client const &src);
		Client(std::string nickname, std::string username, int fd);
		~Client(void);
		
		Client &operator=(Client const &src);

		void		clearBuffer(void);
		bool		isInvitedToChannel(std::string &ChName);
		
		void		addChannelInvite(std::string &chname);
		void		removeChannelInvite(std::string &chname);

		int			getFd(void);
		void		setFd(int fd);
		
		bool		getRegistered(void);
		void		setRegistered(bool value);
		
		bool		getIsLoggedIn(void);
		void		setIsLoggedIn(bool value);
		
		std::string	getNickname(void);
		void		setNickname(std::string &nickName);
		
		std::string	getUsername(void);
		void		setUsername(std::string &username);
		
		std::string	getIpAdd(void);
		void		setIpAdd(std::string ipadd);
		
		std::string	getBuffer(void);
		void		setBuffer(std::string recived);
		
		std::string	getHostname(void);
	
	private:
		int			fd;
		bool		isOperator;
		bool		registered;
		std::string	nickname;
		bool		isLoggedIn;
		std::string	username;
		std::string	buffer;
		std::string	ipadd;
		std::vector<std::string> channelInvites;
};

#endif
