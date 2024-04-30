#include "Server.hpp"

void Server::authenticateClient(int fd, std::string cmd){
	Client *cli = getClientFromFd(fd);
	cmd = cmd.substr(4);
	size_t pos = cmd.find_first_not_of("\t\v ");
	if(pos < cmd.size())	{
		cmd = cmd.substr(pos);
		if(cmd[0] == ':')
			cmd.erase(cmd.begin());
	}
	if(pos == std::string::npos || cmd.empty())
		sendResponse(ERR_NOTENOUGHPARAM(std::string("*")), fd);
	else if(!cli->getRegistered()){
		std::string pass = cmd;
		if(pass == password)
			cli->setRegistered(true);
		else
            sendResponse(ERR_INCORPASS(std::string("*")), fd);
	}
	else
        sendResponse(ERR_ALREADYREGISTERED(getClientFromFd(fd)->getNickname()), fd);
}

bool Server::isValidNickname(std::string& nickname){
	if(!nickname.empty() && (nickname[0] == '&' || nickname[0] == '#' || nickname[0] == ':'))
		return false;
	for(size_t i = 1; i < nickname.size(); i++) {
		if(!std::isalnum(nickname[i]) && nickname[i] != '_')
			return false;
	}
	return true;
}

bool Server::isNicknameInUse(std::string& nickname){
	for (size_t i = 0; i < this->clients.size(); i++){
		if (this->clients[i].getNickname() == nickname)
			return true;
	}
	return false;
}

void Server::setNickname(std::string cmd, int fd){
	std::string inuse;
	cmd = cmd.substr(4);
	size_t pos = cmd.find_first_not_of("\t\v ");
	if(pos < cmd.size())	{
		cmd = cmd.substr(pos);
		if(cmd[0] == ':')
			cmd.erase(cmd.begin());
	}
	Client *cli = getClientFromFd(fd);
	if(pos == std::string::npos || cmd.empty())
		{sendResponse(ERR_NOTENOUGHPARAM(std::string("*")), fd); return;}
	if (isNicknameInUse(cmd) && cli->getNickname() != cmd){
		inuse = "*";
		if(cli->getNickname().empty())
			cli->setNickname(inuse);
	    sendResponse(ERR_NICKINUSE(std::string(cmd)), fd);
		return;
	}
	if(!isValidNickname(cmd)) {
		sendResponse(ERR_ERRONEUSNICK(std::string(cmd)), fd);
		return;
	}
	else {
		if(cli && cli->getRegistered())	{
			std::string oldnick = cli->getNickname();
			cli->setNickname(cmd);
			if(!oldnick.empty() && oldnick != cmd) {
				if(oldnick == "*" && !cli->getUsername().empty()){
					cli->setIsLoggedIn(true);
					sendResponse(RPL_CONNECTED(cli->getNickname()), fd);
					sendResponse(RPL_NICKCHANGE(cli->getNickname(),cmd), fd);
				}
				else
					sendResponse(RPL_NICKCHANGE(oldnick,cmd), fd);
				return;
			}

		}
		else if (cli && !cli->getRegistered())
			sendResponse(ERR_NOTREGISTERED(cmd), fd);
	}
	if(cli && cli->getRegistered() && !cli->getUsername().empty() && !cli->getNickname().empty() && cli->getNickname() != "*" && !cli->getIsLoggedIn()){
		cli->setIsLoggedIn(true);
		sendResponse(RPL_CONNECTED(cli->getNickname()), fd);
	}
}

void	Server::setUsername(std::string& cmd, int fd){
	std::vector<std::string> splited_cmd = splitCommand(cmd);

	Client *cli = getClientFromFd(fd);
	if((cli && splited_cmd.size() < 5)){
		sendResponse(ERR_NOTENOUGHPARAM(cli->getNickname()), fd); 
		return; 
	}
	if(!cli  || !cli->getRegistered())
		sendResponse(ERR_NOTREGISTERED(std::string("*")), fd);
	else if (cli && !cli->getUsername().empty()){
		sendResponse(ERR_ALREADYREGISTERED(cli->getNickname()), fd); 
		return;
	} else
		cli->setUsername(splited_cmd[1]);
	if(cli && cli->getRegistered() && !cli->getUsername().empty() && !cli->getNickname().empty() && cli->getNickname() != "*"  && !cli->getIsLoggedIn()){
		cli->setIsLoggedIn(true);
		sendResponse(RPL_CONNECTED(cli->getNickname()), fd);
	}
}
