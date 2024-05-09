# ft_irc

### Description

ft_irc is a project developed as part of the curriculum at 42 school. It is an implementation of an Internet Relay Chat (IRC) server and client using the C++ programming language. IRC is a protocol for real-time messaging and communication in a distributed network environment.
My full explanation of the project is in this Medium article. I hope you will like it.

### Features
- Multi-threaded architecture for handling concurrent client connections.
- Support for multiple simultaneous connections.
- Creation and management of IRC channels.
- User authentication and registration.
- Broadcasting messages to all users in a channel.
- Private messaging between users.
- Handling of various IRC commands such as JOIN, PRIVMSG, PART, etc.
- Support for user nicknames and channel names.
- Connect to the IRC server.
- Join channels and participate in group conversations.
- Send and receive messages.
- Change user nickname.
- Send private messages to other users.

### Principal codes

--------- SERVIDOR ---------------

./ircserv 2002 123

--------- CLIENTE NC ---------------

nc localhost 2002

PASS 123

NICK TESTE

USER TESTE * * :TESTE 123

JOIN #OVELHA

PRIVMSG #OVELHA TESTE

--------- CLIENTE IRSSI ---------------

/set nick lula

/connect localhost 2002 123

/join #OVELHA

para enviar mensagem é só escrever