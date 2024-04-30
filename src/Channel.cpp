#include "Channel.hpp"

Channel::Channel(void){
	this->inviteOnly = 0;
	this->topic = 0;
	this->key = 0;
	this->limit = 0;
	this->topicRestrictionOption = false;
	this->name = "";
	this->topicName = "";
	char charaters[5] = {'i', 't', 'k', 'o', 'l'};
	for(int i = 0; i < 5; i++)
		modes.push_back(std::make_pair(charaters[i],false));
	this->createdAt = "";
}

Channel::Channel(Channel const &src){
	*this = src;
}

Channel::~Channel(void){
}

Channel &Channel::operator=(Channel const &src){
	if (this != &src){
		this->inviteOnly = src.inviteOnly;
		this->topic = src.topic;
		this->key = src.key;
		this->limit = src.limit;
		this->topicRestrictionOption = src.topicRestrictionOption;
		this->name = src.name;
		this->password = src.password;
		this->createdAt = src.createdAt;
		this->topicName = src.topicName;
		this->clients = src.clients;
		this->admins = src.admins;
		this->modes = src.modes;
	}
	return *this;
}

void Channel::setInviteOnly(int inviteOnly){
	this->inviteOnly = inviteOnly;
}

void Channel::setTopic(int topic){
	this->topic = topic;
}

void Channel::setTime(std::string time){
	this->time = time;
}

void Channel::setKey(int key){
	this->key = key;
}

void Channel::setLimit(int limit){
	this->limit = limit;
}

void Channel::setTopicName(std::string topicName){
	this->topicName = topicName;
}

void Channel::setPassword(std::string password){
	this->password = password;
}

void Channel::setName(std::string name){
	this->name = name;
}

void Channel::setTopicRestriction(bool value){
	this->topicRestrictionOption = value;
}

void Channel::setModeAtindex(size_t index, bool mode){
	modes[index].second = mode;
}
void Channel::setCreatedAt(void){
	std::time_t _time = std::time(NULL);
	std::ostringstream oss;
	oss << _time;
	this->createdAt = std::string(oss.str());
}

int Channel::getInviteOnly(void){
	return this->inviteOnly;
}

int Channel::getTopic(void){
	return this->topic;
}

int Channel::getKey(void){
	return this->key;
}

int Channel::getLimit(void){
	return this->limit;
}

int Channel::getClientCount(void){
	return this->clients.size() + this->admins.size();
}

bool Channel::getTopicRestriction(void) const{
	return this->topicRestrictionOption;
}

bool Channel::getModeAtIndex(size_t index){
	return modes[index].second;
}

bool Channel::clientInChannel(std::string &nick){
	for(size_t i = 0; i < clients.size(); i++){
		if(clients[i].getNickname() == nick)
			return true;
	}
	for(size_t i = 0; i < admins.size(); i++){
		if(admins[i].getNickname() == nick)
			return true;
	}
	return false;
}

std::string Channel::getTopicName(void){
	return this->topicName;
}

std::string Channel::getPassword(void){
	return this->password;
}

std::string Channel::getName(void){
	return this->name;
}

std::string Channel::getTime(void){
	return this->time;
}

std::string Channel::getCreationTime(void){
	return this->createdAt;
}

std::string Channel::getModes(void){
	std::string	mode;
	for(size_t i = 0; i < modes.size(); i++){
		if(modes[i].first != 'o' && modes[i].second)
			mode.push_back(modes[i].first);
	}
	if(!mode.empty())
		mode.insert(mode.begin(),'+');
	return mode;
}

std::string Channel::clientChannelList(void){
	std::string list;
	for(size_t i = 0; i < admins.size(); i++){
		list += "@" + admins[i].getNickname();
		if((i + 1) < admins.size())
			list += " ";
	}
	if(clients.size())
		list += " ";
	for(size_t i = 0; i < clients.size(); i++){
		list += clients[i].getNickname();
		if((i + 1) < clients.size())
			list += " ";
	}
	return list;
}

Client *Channel::getClientFromFd(int fd){
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it){
		if (it->getFd() == fd)
			return &(*it);
	}
	return NULL;
}

Client *Channel::getAdmin(int fd){
	for (std::vector<Client>::iterator it = admins.begin(); it != admins.end(); ++it){
		if (it->getFd() == fd)
			return &(*it);
	}
	return NULL;
}

Client* Channel::getClientInChannel(std::string name){
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it){
		if (it->getNickname() == name)
			return &(*it);
	}
	for (std::vector<Client>::iterator it = admins.begin(); it != admins.end(); ++it){
		if (it->getNickname() == name)
			return &(*it);
	}
	return NULL;
}

void Channel::addClient(Client newClient){
	clients.push_back(newClient);
}

void Channel::addAdmin(Client newClient){
	admins.push_back(newClient);
}

void Channel::removeClient(int fd){
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it){
		if (it->getFd() == fd)
			{clients.erase(it); break;}
	}
}

void Channel::removeAdmin(int fd){
	for (std::vector<Client>::iterator it = admins.begin(); it != admins.end(); ++it){
		if (it->getFd() == fd)
			{admins.erase(it); break;}
	}
}

bool Channel::changeClientToAdmin(std::string& nick){
	size_t i = 0;
	for(; i < clients.size(); i++){
		if(clients[i].getNickname() == nick)
			break;
	}
	if(i < clients.size()){
		admins.push_back(clients[i]);
		clients.erase(i + clients.begin());
		return true;
	}
	return false;
}

bool Channel::changeAdminToClient(std::string& nick){
	size_t i = 0;
	for(; i < admins.size(); i++){
		if(admins[i].getNickname() == nick)
			break;
	}
	if(i < admins.size()){
		clients.push_back(admins[i]);
		admins.erase(i + admins.begin());
		return true;
	}
	return false;

}

void Channel::sendToAll(std::string rpl1){
	for(size_t i = 0; i < admins.size(); i++)
		if(send(admins[i].getFd(), rpl1.c_str(), rpl1.size(),0) == -1)
			std::cerr << "send() faild" << std::endl;
	for(size_t i = 0; i < clients.size(); i++)
		if(send(clients[i].getFd(), rpl1.c_str(), rpl1.size(),0) == -1)
			std::cerr << "send() faild" << std::endl;
}

void Channel::sendToAll(std::string rpl1, int fd){
	for(size_t i = 0; i < admins.size(); i++){
		if(admins[i].getFd() != fd)
			if(send(admins[i].getFd(), rpl1.c_str(), rpl1.size(),0) == -1)
				std::cerr << "send() faild" << std::endl;
	}
	for(size_t i = 0; i < clients.size(); i++){
		if(clients[i].getFd() != fd)
			if(send(clients[i].getFd(), rpl1.c_str(), rpl1.size(),0) == -1)
				std::cerr << "send() faild" << std::endl;
	}
}
