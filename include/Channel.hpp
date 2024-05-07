#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Client.hpp"
#include "Server.hpp"

class Client;

class Channel {
	public:
		Channel(void);
		Channel(Channel const &src);
		~Channel(void);
		
		Channel &operator=(Channel const &src);

		void		addClient(Client newClient);
		void		removeClient(int fd);
		
		void		addAdmin(Client newClient);
		void		removeAdmin(int fd);
		
		bool		changeClientToAdmin(std::string &nick);
		bool		changeAdminToClient(std::string &nick);
		bool		clientInChannel(std::string &nick);
		void		sendToAll(std::string rpl1);
		void		sendToAll(std::string rpl1, int fd);
		
		int			getInviteOnly(void);
		void		setInviteOnly(int inviteOnly);
		
		int			getTopic(void);
		void		setTopic(int topic);
		
		int			getKey(void);
		void		setKey(int key);
		
		int			getLimit(void);
		void		setLimit(int limit);
		
		std::string	getTopicName(void);
		void		setTopicName(std::string topicName);
		
		std::string	getPassword(void);
		void		setPassword(std::string password);
		
		std::string	getName(void);
		void		setName(std::string name);
		
		std::string	getTime(void);
		void		setTime(std::string time);
		
		bool		getTopicRestriction(void) const;
		void		setTopicRestriction(bool value);
		
		bool		getModeAtIndex(size_t index);
		void		setModeAtindex(size_t index, bool mode);
		
		void		setCreatedAt(void);
		int			getClientCount(void);
		std::string	getCreationTime(void);
		std::string	getModes(void);
		std::string	clientChannelList(void);
		Client		*getClientFromFd(int fd);
		Client		*getAdmin(int fd);
		Client		*getClientInChannel(std::string name);

	private:
		int			inviteOnly;
		int			topic;
		int			key;
		int			limit;
		bool		topicRestrictionOption;
		std::string	name;
		std::string	time;
		std::string	password;
		std::string	createdAt;
		std::string	topicName;
		std::vector<Client>	clients;
		std::vector<Client>	admins;
		std::vector<std::pair<char , bool> >	modes;
};

#endif
