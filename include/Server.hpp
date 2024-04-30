#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <ctime>

#include "Client.hpp"
#include "Channel.hpp"
#include "Replies.hpp"

class Client;
class Channel;

class Server{
	public:
		Server(void);
		Server(Server const &src);
		~Server(void);
		
		Server &operator=(Server const &src);

		void		setUsername(std::string &username, int fd);
		void		setNickname(std::string cmd, int fd);
		
		int			getFd(void);
		void		setFd(int serverSocketFd);
		
		int			getPort(void);
		void		setPort(int port);
		
		std::string	getPassword(void);
		void		setPassword(std::string password);
		
		Client		*getClientFromFd(int fd);
		Client		*getClientFromNickname(std::string nickname);
		static void	signalHandler(int signum);
		
		void		init(int port, std::string pass);
		void		acceptNewClient(void);
		void		receiveNewData(int fd);
		void		authenticateClient(int fd, std::string pass);

		void		sendError(int code, std::string clientname, int fd, std::string msg);
		void		sendError(int code, std::string clientname, std::string channelname, int fd, std::string msg);
		void		sendResponse(std::string response, int fd);
		
		void		addFd(pollfd newFd);
		void		removeFd(int fd);
		
		void		addClient(Client newClient);
		void		removeClient(int fd);
		
		Channel		*getChannel(std::string name);
		void		addChannel(Channel newChannel);
		void		removeChannel(int fd);
		void		removeChannel(std::string name);
		
		void		setServerSocket(void);		
		void		closeFds(void);

		std::vector<std::string>	splitReceivedBuffer(std::string str);
		std::vector<std::string>	splitCommand(std::string &str);
		void						parseAndExecCommand(std::string &cmd, int fd);

		bool		isRegistered(int fd);
		bool		isNicknameInUse(std::string &nickname);
		bool		isValidNickname(std::string &nickname);

		void		joinCommand(std::string cmd, int fd);
		void		joinExistingChannel(std::vector<std::pair<std::string, std::string> > &token, int i, int j, int fd);
		void		joinNonExistingChannel(std::vector<std::pair<std::string, std::string> > &token, int i, int fd);
		int			parseJoinCommand(std::vector<std::pair<std::string, std::string> > &token, std::string cmd, int fd);
		int			countJoinedChannels(std::string nickname);

		void		partCommand(std::string cmd, int fd);
		int			parsePartCommand(std::string cmd, std::vector<std::string> &tmp, std::string &reason, int fd);

		void		kickCommand(std::string cmd, int fd);
		std::string	parseKickCommand(std::string cmd, std::vector<std::string> &tmp, std::string &user, int fd);

		void		privmsgCommand(std::string cmd, int fd);
		void		CheckForChannelsAndClients(std::vector<std::string> &tmp, int fd);

		void		quitCommand(std::string cmd, int fd);

		void		modeCommand(std::string &cmd, int fd);
		std::string	inviteOnlyOption(Channel *channel, char opera, std::string chain);
		std::string	topicRestrictionOption(Channel *channel, char opera, std::string chain);
		std::string	channelKeyOption(std::vector<std::string> splited, Channel *channel, size_t &pos, char opera, int fd, std::string chain, std::string &arguments);
		std::string	operatorPrivilegeOption(std::vector<std::string> splited, Channel *channel, size_t &pos, int fd, char opera, std::string chain, std::string &arguments);
		std::string	userLimitOption(std::vector<std::string> splited, Channel *channel, size_t &pos, char opera, int fd, std::string chain, std::string &arguments);
		bool		isValidUserLimit(std::string &limit);
		std::string	appendToMode(std::string chain, char opera, char mode);
		std::vector<std::string>	splitParams(std::string params);
		void		getCommandArgs(std::string cmd, std::string &name, std::string &modeset, std::string &params);

		void		topicCommand(std::string &cmd, int &fd);
		std::string	getTopicTime(void);
		std::string	getTopic(std::string &input);
		int			getColonPos(std::string &cmd);

		void		inviteCommand(std::string &cmd, int &fd);

	private:
		int							port;
		int							serverSocketFd;
		static bool					signaled;
		std::string					password;
		std::vector<Client>			clients;
		std::vector<Channel>		channels;
		std::vector<struct pollfd>	fds;
		struct sockaddr_in			address;
		struct sockaddr_in			clientAddress;
		struct pollfd				newClient;
};

#endif
