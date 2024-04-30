#include "Client.hpp"

Client::Client(void){
	this->nickname = "";
	this->username = "";
	this->fd = -1;
	this->isOperator = false;
	this->registered = false;
	this->buffer = "";
	this->ipadd = "";
	this->isLoggedIn = false;
}

Client::Client(std::string nickname, std::string username, int fd) : fd(fd), nickname(nickname), username(username){
}

Client::Client(Client const &src){
	*this = src;
}

Client::~Client(void){
}

Client &Client::operator=(Client const &src){
	if (this != &src){
		this->nickname = src.nickname;
		this->username = src.username;
		this->fd = src.fd;
		this->channelInvites = src.channelInvites;
		this->buffer = src.buffer;
		this->registered = src.registered;
		this->ipadd = src.ipadd;
		this->isLoggedIn = src.isLoggedIn;
	}
	return *this;
}

int Client::getFd(void){ 
	return this->fd; 
}

bool Client::getRegistered(void){
	return registered;
}

bool Client::isInvitedToChannel(std::string &ChName){
	for (size_t i = 0; i < this->channelInvites.size(); i++)
	{
		if (this->channelInvites[i] == ChName)
			return true;
	}
	return false;
}

std::string Client::getNickname(void){
	return this->nickname;
}

bool Client::getIsLoggedIn(void){
	return this->isLoggedIn;
}

std::string Client::getUsername(void){
	return this->username;
}

std::string Client::getBuffer(void){
	return buffer;
}

std::string Client::getIpAdd(void){
	return ipadd;
}

std::string Client::getHostname(void){
	std::string hostname = this->getNickname() + "!" + this->getUsername();
	return hostname;
}

void Client::setFd(int fd){
	this->fd = fd;
}

void Client::setNickname(std::string &nickName){
	this->nickname = nickName;
}

void Client::setIsLoggedIn(bool value){
	this->isLoggedIn = value;
}

void Client::setUsername(std::string &username){
	this->username = username;
}

void Client::setBuffer(std::string recived){
	buffer += recived;
}

void Client::setRegistered(bool value){
	registered = value;
}

void Client::setIpAdd(std::string ipadd){
	this->ipadd = ipadd;
}

void Client::clearBuffer(void){
	buffer.clear();
}

void Client::addChannelInvite(std::string &chname){
	channelInvites.push_back(chname);
}

void Client::removeChannelInvite(std::string &chname){
	for (size_t i = 0; i < this->channelInvites.size(); i++){
		if (this->channelInvites[i] == chname){
			this->channelInvites.erase(this->channelInvites.begin() + i);
			return;
		}
	}
}
