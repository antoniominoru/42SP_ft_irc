#include "Server.hpp"

#include <ostream>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"

Server::Server(void) { this->serverSocketFd = -1; }

Server::Server(Server const &src) { *this = src; }

Server::~Server(void) {}

Server &Server::operator=(Server const &src) {
  if (this != &src) {
    this->port = src.port;
    this->serverSocketFd = src.serverSocketFd;
    this->password = src.password;
    this->clients = src.clients;
    this->channels = src.channels;
    this->fds = src.fds;
  }
  return *this;
}

int Server::getPort(void) { return this->port; }

int Server::getFd(void) { return this->serverSocketFd; }

Client *Server::getClientFromFd(int fd) {
  for (size_t i = 0; i < this->clients.size(); i++) {
    if (this->clients[i].getFd() == fd) return &this->clients[i];
  }
  return NULL;
}

Client *Server::getClientFromNickname(std::string nickname) {
  for (size_t i = 0; i < this->clients.size(); i++) {
    if (this->clients[i].getNickname() == nickname) return &this->clients[i];
  }
  return NULL;
}

Channel *Server::getChannel(std::string name) {
  for (size_t i = 0; i < this->channels.size(); i++) {
    if (this->channels[i].getName() == name) return &channels[i];
  }
  return NULL;
}

void Server::setFd(int fd) { this->serverSocketFd = fd; }

void Server::setPort(int port) { this->port = port; }

void Server::setPassword(std::string password) { this->password = password; }

std::string Server::getPassword(void) { return this->password; }

void Server::addClient(Client newClient) { this->clients.push_back(newClient); }

void Server::addChannel(Channel newChannel) {
  this->channels.push_back(newChannel);
}

void Server::addFd(pollfd newFd) { this->fds.push_back(newFd); }

void Server::removeClient(int fd) {
  for (size_t i = 0; i < this->clients.size(); i++) {
    if (this->clients[i].getFd() == fd) {
      this->clients.erase(this->clients.begin() + i);
      return;
    }
  }
}

void Server::removeChannel(std::string name) {
  for (size_t i = 0; i < this->channels.size(); i++) {
    if (this->channels[i].getName() == name) {
      this->channels.erase(this->channels.begin() + i);
      return;
    }
  }
}

void Server::removeFd(int fd) {
  for (size_t i = 0; i < this->fds.size(); i++) {
    if (this->fds[i].fd == fd) {
      this->fds.erase(this->fds.begin() + i);
      return;
    }
  }
}

void Server::removeChannel(int fd) {
  for (size_t i = 0; i < this->channels.size(); i++) {
    int flag = 0;
    if (channels[i].getClientFromFd(fd)) {
      channels[i].removeClient(fd);
      flag = 1;
    } else if (channels[i].getAdmin(fd)) {
      channels[i].removeAdmin(fd);
      flag = 1;
    }
    if (channels[i].getClientCount() == 0) {
      channels.erase(channels.begin() + i);
      i--;
      continue;
    }
    if (flag) {
      std::string rpl = ":" + getClientFromFd(fd)->getNickname() + "!~" +
                        getClientFromFd(fd)->getUsername() +
                        "@localhost quitCommand Quit\r\n";
      channels[i].sendToAll(rpl);
    }
  }
}

void Server::sendError(int code, std::string clientname, int fd,
                       std::string msg) {
  std::stringstream ss;
  ss << ":localhost " << code << " " << clientname << msg;
  std::string resp = ss.str();
  if (send(fd, resp.c_str(), resp.size(), 0) == -1)
    std::cerr << "send() faild" << std::endl;
}

void Server::sendError(int code, std::string clientname,
                       std::string channelname, int fd, std::string msg) {
  std::stringstream ss;
  ss << ":localhost " << code << " " << clientname << " " << channelname << msg;
  std::string resp = ss.str();
  if (send(fd, resp.c_str(), resp.size(), 0) == -1)
    std::cerr << "send() faild" << std::endl;
}

void Server::sendResponse(std::string response, int fd) {
  std::cout << "Response:\n" << response;
  if (send(fd, response.c_str(), response.size(), 0) == -1)
    std::cerr << "Response send() faild" << std::endl;
}

bool Server::signaled = false;

void Server::signalHandler(int signum) {
  (void)signum;
  std::cout << std::endl << "signaled Received!" << std::endl;
  Server::signaled = true;
}

void Server::closeFds(void) {
  for (size_t i = 0; i < clients.size(); i++) {
    std::cout << "Client <" << clients[i].getFd() << "> Disconnected"
              << std::endl;
    close(clients[i].getFd());
  }
  if (serverSocketFd != -1) {
    std::cout << "Server <" << serverSocketFd << "> Disconnected" << std::endl;
    close(serverSocketFd);
  }
}

void Server::init(int port, std::string pass) {
  this->password = pass;
  this->port = port;
  this->setServerSocket();

  std::cout << "Waiting to accept a connection...\n";
  while (Server::signaled == false) {
    if ((poll(&fds[0], fds.size(), -1) == -1) && Server::signaled == false)
      throw(std::runtime_error("poll() faild"));
    for (size_t i = 0; i < fds.size(); i++) {
      if (fds[i].revents & POLLIN) {
        if (fds[i].fd == serverSocketFd)
          this->acceptNewClient();
        else
          this->receiveNewData(fds[i].fd);
      }
    }
  }
  closeFds();
}

void Server::setServerSocket(void) {
  int en = 1;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocketFd == -1) throw(std::runtime_error("faild to create socket"));
  if (setsockopt(serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) ==
      -1)
    throw(std::runtime_error("faild to set option (SO_REUSEADDR) on socket"));
  if (fcntl(serverSocketFd, F_SETFL, O_NONBLOCK) == -1)
    throw(std::runtime_error("faild to set option (O_NONBLOCK) on socket"));
  if (bind(serverSocketFd, (struct sockaddr *)&address, sizeof(address)) == -1)
    throw(std::runtime_error("faild to bind socket"));
  if (listen(serverSocketFd, SOMAXCONN) == -1)
    throw(std::runtime_error("listen() faild"));
  newClient.fd = serverSocketFd;
  newClient.events = POLLIN;
  newClient.revents = 0;
  fds.push_back(newClient);
}

void Server::acceptNewClient(void) {
  Client cli;
  memset(&clientAddress, 0, sizeof(clientAddress));
  socklen_t len = sizeof(clientAddress);
  int incofd = accept(serverSocketFd, (sockaddr *)&(clientAddress), &len);
  if (incofd == -1) {
    std::cout << "accept() failed" << std::endl;
    return;
  }
  if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1) {
    std::cout << "fcntl() failed" << std::endl;
    return;
  }
  newClient.fd = incofd;
  newClient.events = POLLIN;
  newClient.revents = 0;
  cli.setFd(incofd);
  cli.setIpAdd(inet_ntoa((clientAddress.sin_addr)));
  clients.push_back(cli);
  fds.push_back(newClient);
  std::cout << "Client <" << incofd << "> Connected" << std::endl;
}

void Server::receiveNewData(int fd) {
  std::vector<std::string> cmd;
  char buff[1024];
  memset(buff, 0, sizeof(buff));
  Client *cli = getClientFromFd(fd);
  ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);
  if (bytes <= 0) {
    std::cout << "Client <" << fd << "> Disconnected" << std::endl;
    removeChannel(fd);
    removeClient(fd);
    removeFd(fd);
    close(fd);
  } else {
    cli->setBuffer(buff);
    if (cli->getBuffer().find_first_of("\r\n") == std::string::npos) return;
    cmd = splitReceivedBuffer(cli->getBuffer());
    for (size_t i = 0; i < cmd.size(); i++)
      this->parseAndExecCommand(cmd[i], fd);
    if (getClientFromFd(fd)) getClientFromFd(fd)->clearBuffer();
  }
}

std::vector<std::string> Server::splitReceivedBuffer(std::string str) {
  std::vector<std::string> vec;
  std::istringstream stm(str);
  std::string line;
  while (std::getline(stm, line)) {
    size_t pos = line.find_first_of("\r\n");
    if (pos != std::string::npos) line = line.substr(0, pos);
    vec.push_back(line);
  }
  return vec;
}

std::vector<std::string> Server::splitCommand(std::string &cmd) {
  std::vector<std::string> vec;
  std::istringstream stm(cmd);
  std::string token;
  while (stm >> token) {
    vec.push_back(token);
    token.clear();
  }
  return vec;
}

bool Server::isRegistered(int fd) {
  if (!getClientFromFd(fd) || getClientFromFd(fd)->getNickname().empty() ||
      getClientFromFd(fd)->getUsername().empty() ||
      getClientFromFd(fd)->getNickname() == "*" ||
      !getClientFromFd(fd)->getIsLoggedIn())
    return false;
  return true;
}

void Server::parseAndExecCommand(std::string &cmd, int fd) {
  if (cmd.empty()) return;
  std::vector<std::string> splittedCmd = splitCommand(cmd);
  size_t found = cmd.find_first_not_of(" \t\v");
  if (found != std::string::npos) cmd = cmd.substr(found);
  if (splittedCmd.size() && (splittedCmd[0] == "BONG")) return;
  if (splittedCmd.size() && (splittedCmd[0] == "PASS"))
    authenticateClient(fd, cmd);
  else if (splittedCmd.size() && (splittedCmd[0] == "NICK"))
    setNickname(cmd, fd);
  else if (splittedCmd.size() && (splittedCmd[0] == "USER"))
    setUsername(cmd, fd);
  else if (splittedCmd.size() && (splittedCmd[0] == "QUIT"))
    quitCommand(cmd, fd);
  else if (isRegistered(fd)) {
    if (splittedCmd.size() && (splittedCmd[0] == "KICK"))
      kickCommand(cmd, fd);
    else if (splittedCmd.size() && (splittedCmd[0] == "JOIN"))
      joinCommand(cmd, fd);
    else if (splittedCmd.size() && (splittedCmd[0] == "TOPIC"))
      topicCommand(cmd, fd);
    else if (splittedCmd.size() && (splittedCmd[0] == "MODE"))
      modeCommand(cmd, fd);
    else if (splittedCmd.size() && (splittedCmd[0] == "PART"))
      partCommand(cmd, fd);
    else if (splittedCmd.size() && (splittedCmd[0] == "PRIVMSG"))
      privmsgCommand(cmd, fd);
    else if (splittedCmd.size() && (splittedCmd[0] == "INVITE"))
      inviteCommand(cmd, fd);
    else if (splittedCmd.size())
      sendResponse(
          ERR_CMDNOTFOUND(getClientFromFd(fd)->getNickname(), splittedCmd[0]),
          fd);
  } else if (!isRegistered(fd))
    sendResponse(ERR_NOTREGISTERED(std::string("*")), fd);
}
