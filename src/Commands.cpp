#include <ostream>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Server::inviteCommand(std::string &cmd, int &fd) {
  std::vector<std::string> scmd = splitCommand(cmd);
  if (scmd.size() < 3) {
    sendError(461, getClientFromFd(fd)->getNickname(), fd,
              " :Not enough parameters\r\n");
    return;
  }
  std::string channelname = scmd[2].substr(1);
  if (scmd[2][0] != '#' || !getChannel(channelname)) {
    sendError(403, channelname, fd, " :No such channel\r\n");
    return;
  }
  if (!(getChannel(channelname)->getClientFromFd(fd)) &&
      !(getChannel(channelname)->getAdmin(fd))) {
    sendError(442, channelname, fd, " :You're not on that channel\r\n");
    return;
  }
  if (getChannel(channelname)->getClientInChannel(scmd[1])) {
    sendError(443, getClientFromFd(fd)->getNickname(), channelname, fd,
              " :is already on channel\r\n");
    return;
  }
  Client *clt = getClientFromNickname(scmd[1]);
  if (!clt) {
    sendError(401, scmd[1], fd, " :No such nick\r\n");
    return;
  }
  if (getChannel(channelname)->getInviteOnly() &&
      !getChannel(channelname)->getAdmin(fd)) {
    sendError(482, getChannel(channelname)->getClientFromFd(fd)->getNickname(),
              scmd[1], fd, " :You're not channel operator\r\n");
    return;
  }
  if (getChannel(channelname)->getLimit() &&
      getChannel(channelname)->getClientCount() >=
          getChannel(channelname)->getLimit()) {
    sendError(473, getChannel(channelname)->getClientFromFd(fd)->getNickname(),
              channelname, fd, " :Cannot invit to channel (+i)\r\n");
    return;
  }
  clt->addChannelInvite(channelname);
  std::string rep1 = ": 341 " + getClientFromFd(fd)->getNickname() + " " +
                     clt->getNickname() + " " + scmd[2] + "\r\n";
  sendResponse(rep1, fd);
  std::string rep2 = ":" + clt->getHostname() + " INVITE " +
                     clt->getNickname() + " " + scmd[2] + "\r\n";
  sendResponse(rep2, clt->getFd());
}

int Server::parseJoinCommand(
    std::vector<std::pair<std::string, std::string> > &token, std::string cmd,
    int fd) {
  std::vector<std::string> tmp;
  std::string ChStr, PassStr, buff;
  std::istringstream iss(cmd);
  while (iss >> cmd) tmp.push_back(cmd);
  if (tmp.size() < 2) {
    token.clear();
    return 0;
  }
  tmp.erase(tmp.begin());
  ChStr = tmp[0];
  tmp.erase(tmp.begin());
  if (!tmp.empty()) {
    PassStr = tmp[0];
    tmp.clear();
  }
  for (size_t i = 0; i < ChStr.size(); i++) {
    if (ChStr[i] == ',') {
      token.push_back(std::make_pair(buff, ""));
      buff.clear();
    } else
      buff += ChStr[i];
  }
  token.push_back(std::make_pair(buff, ""));
  if (!PassStr.empty()) {
    size_t j = 0;
    buff.clear();
    for (size_t i = 0; i < PassStr.size(); i++) {
      if (PassStr[i] == ',') {
        token[j].second = buff;
        j++;
        buff.clear();
      } else
        buff += PassStr[i];
    }
    token[j].second = buff;
  }
  for (size_t i = 0; i < token.size(); i++) {
    if (token[i].first.empty()) token.erase(token.begin() + i--);
  }
  for (size_t i = 0; i < token.size(); i++) {
    if (*(token[i].first.begin()) != '#') {
      sendError(403, getClientFromFd(fd)->getNickname(), token[i].first,
                getClientFromFd(fd)->getFd(), " :No such channel\r\n");
      token.erase(token.begin() + i--);
    } else
      token[i].first.erase(token[i].first.begin());
  }
  return 1;
}

int Server::countJoinedChannels(std::string nickname) {
  int count = 0;
  for (size_t i = 0; i < this->channels.size(); i++) {
    if (this->channels[i].getClientInChannel(nickname)) count++;
  }
  return count;
}

bool IsInvited(Client *cli, std::string ChName, int flag) {
  if (cli->isInvitedToChannel(ChName)) {
    if (flag == 1) cli->removeChannelInvite(ChName);
    return true;
  }
  return false;
}

void Server::joinExistingChannel(
    std::vector<std::pair<std::string, std::string> > &token, int i, int j,
    int fd) {
  if (this->channels[j].getClientInChannel(getClientFromFd(fd)->getNickname()))
    return;
  if (countJoinedChannels(getClientFromFd(fd)->getNickname()) >= 10) {
    sendError(405, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(),
              " :You have joined too many channels\r\n");
    return;
  }
  if (!this->channels[j].getPassword().empty() &&
      this->channels[j].getPassword() != token[i].second) {
    if (!IsInvited(getClientFromFd(fd), token[i].first, 0)) {
      sendError(475, getClientFromFd(fd)->getNickname(), "#" + token[i].first,
                getClientFromFd(fd)->getFd(),
                " :Cannot join channel (+k) - bad key\r\n");
      return;
    }
  }
  if (this->channels[j].getInviteOnly()) {
    if (!IsInvited(getClientFromFd(fd), token[i].first, 1)) {
      sendError(473, getClientFromFd(fd)->getNickname(), "#" + token[i].first,
                getClientFromFd(fd)->getFd(), " :Cannot join channel (+i)\r\n");
      return;
    }
  }
  if (this->channels[j].getLimit() &&
      this->channels[j].getClientCount() >= this->channels[j].getLimit()) {
    sendError(471, getClientFromFd(fd)->getNickname(), "#" + token[i].first,
              getClientFromFd(fd)->getFd(), " :Cannot join channel (+l)\r\n");
    return;
  }
  Client *cli = getClientFromFd(fd);
  this->channels[j].addClient(*cli);
  if (channels[j].getTopicName().empty())
    sendResponse(RPL_JOINMSG(getClientFromFd(fd)->getHostname(),
                             getClientFromFd(fd)->getIpAdd(), token[i].first) +
                     RPL_NAMREPLY(getClientFromFd(fd)->getNickname(),
                                  channels[j].getName(),
                                  channels[j].clientChannelList()) +
                     RPL_ENDOFNAMES(getClientFromFd(fd)->getNickname(),
                                    channels[j].getName()),
                 fd);
  else
    sendResponse(
        RPL_JOINMSG(getClientFromFd(fd)->getHostname(),
                    getClientFromFd(fd)->getIpAdd(), token[i].first) +
            RPL_TOPICIS(getClientFromFd(fd)->getNickname(),
                        channels[j].getName(), channels[j].getTopicName()) +
            RPL_NAMREPLY(getClientFromFd(fd)->getNickname(),
                         channels[j].getName(),
                         channels[j].clientChannelList()) +
            RPL_ENDOFNAMES(getClientFromFd(fd)->getNickname(),
                           channels[j].getName()),
        fd);
  channels[j].sendToAll(
      RPL_JOINMSG(getClientFromFd(fd)->getHostname(),
                  getClientFromFd(fd)->getIpAdd(), token[i].first),
      fd);
}

void Server::joinNonExistingChannel(
    std::vector<std::pair<std::string, std::string> > &token, int i, int fd) {
  if (countJoinedChannels(getClientFromFd(fd)->getNickname()) >= 10) {
    sendError(405, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(),
              " :You have joined too many channels\r\n");
    return;
  }
  Channel newChannel;
  newChannel.setName(token[i].first);
  newChannel.addAdmin(*getClientFromFd(fd));
  newChannel.setCreatedAt();
  this->channels.push_back(newChannel);

  sendResponse(
      RPL_JOINMSG(getClientFromFd(fd)->getHostname(),
                  getClientFromFd(fd)->getIpAdd(), newChannel.getName()) +
          RPL_NAMREPLY(getClientFromFd(fd)->getNickname(), newChannel.getName(),
                       newChannel.clientChannelList()) +
          RPL_ENDOFNAMES(getClientFromFd(fd)->getNickname(),
                         newChannel.getName()),
      fd);
}

void Server::joinCommand(std::string cmd, int fd) {
  std::vector<std::pair<std::string, std::string> > token;

  if (!parseJoinCommand(token, cmd, fd)) {
    sendError(461, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(), " :Not enough parameters\r\n");
    return;
  }
  if (token.size() > 10) {
    sendError(407, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(), " :Too many channels\r\n");
    return;
  }
  for (size_t i = 0; i < token.size(); i++) {
    bool flag = false;
    for (size_t j = 0; j < this->channels.size(); j++) {
      if (this->channels[j].getName() == token[i].first) {
        joinExistingChannel(token, i, j, fd);
        flag = true;
        break;
      }
    }
    if (!flag) joinNonExistingChannel(token, i, fd);
  }
}

void FindK(std::string cmd, std::string tofind, std::string &str) {
  size_t i = 0;
  for (; i < cmd.size(); i++) {
    if (cmd[i] != ' ') {
      std::string tmp;
      for (; i < cmd.size() && cmd[i] != ' '; i++) tmp += cmd[i];
      if (tmp == tofind)
        break;
      else
        tmp.clear();
    }
  }
  if (i < cmd.size()) str = cmd.substr(i);
  i = 0;
  for (; i < str.size() && str[i] == ' '; i++);
  str = str.substr(i);
}

std::string SplitCmdK(std::string &cmd, std::vector<std::string> &tmp) {
  std::stringstream ss(cmd);
  std::string str, reason;
  int count = 3;
  while (ss >> str && count--) tmp.push_back(str);
  if (tmp.size() != 3) return std::string("");
  FindK(cmd, tmp[2], reason);
  return reason;
}

std::string Server::parseKickCommand(std::string cmd,
                                     std::vector<std::string> &tmp,
                                     std::string &user, int fd) {
  std::string reason = SplitCmdK(cmd, tmp);
  if (tmp.size() < 3) return std::string("");
  tmp.erase(tmp.begin());
  std::string str = tmp[0];
  std::string str1;
  user = tmp[1];
  tmp.clear();
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == ',') {
      tmp.push_back(str1);
      str1.clear();
    } else
      str1 += str[i];
  }
  tmp.push_back(str1);
  for (size_t i = 0; i < tmp.size(); i++) {
    if (tmp[i].empty()) tmp.erase(tmp.begin() + i--);
  }
  if (reason[0] == ':')
    reason.erase(reason.begin());
  else {
    for (size_t i = 0; i < reason.size(); i++) {
      if (reason[i] == ' ') {
        reason = reason.substr(0, i);
        break;
      }
    }
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    if (*(tmp[i].begin()) == '#')
      tmp[i].erase(tmp[i].begin());
    else {
      sendError(403, getClientFromFd(fd)->getNickname(), tmp[i],
                getClientFromFd(fd)->getFd(), " :No such channel\r\n");
      tmp.erase(tmp.begin() + i--);
    }
  }
  return reason;
}

void Server::kickCommand(std::string cmd, int fd) {
  std::vector<std::string> tmp;
  std::string reason, user;
  reason = parseKickCommand(cmd, tmp, user, fd);
  if (user.empty()) {
    sendError(461, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(), " :Not enough parameters\r\n");
    return;
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    if (getChannel(tmp[i])) {
      Channel *ch = getChannel(tmp[i]);
      if (!ch->getClientFromFd(fd) && !ch->getAdmin(fd)) {
        sendError(442, getClientFromFd(fd)->getNickname(), "#" + tmp[i],
                  getClientFromFd(fd)->getFd(),
                  " :You're not on that channel\r\n");
        continue;
      }
      if (ch->getAdmin(fd)) {
        if (ch->getClientInChannel(user)) {
          std::stringstream ss;
          ss << ":" << getClientFromFd(fd)->getNickname() << "!~"
             << getClientFromFd(fd)->getUsername() << "@" << "localhost"
             << " KICK #" << tmp[i] << " " << user;
          if (!reason.empty())
            ss << " :" << reason << "\r\n";
          else
            ss << "\r\n";
          ch->sendToAll(ss.str());
          if (ch->getAdmin(ch->getClientInChannel(user)->getFd()))
            ch->removeAdmin(ch->getClientInChannel(user)->getFd());
          else
            ch->removeClient(ch->getClientInChannel(user)->getFd());
          if (ch->getClientCount() == 0) channels.erase(channels.begin() + i);
        } else {
          sendError(441, getClientFromFd(fd)->getNickname(), "#" + tmp[i],
                    getClientFromFd(fd)->getFd(),
                    " :They aren't on that channel\r\n");
          continue;
        }
      } else {
        sendError(482, getClientFromFd(fd)->getNickname(), "#" + tmp[i],
                  getClientFromFd(fd)->getFd(),
                  " :You're not channel operator\r\n");
        continue;
      }
    } else
      sendError(403, getClientFromFd(fd)->getNickname(), "#" + tmp[i],
                getClientFromFd(fd)->getFd(), " :No such channel\r\n");
  }
}

std::string Server::appendToMode(std::string chain, char opera, char mode) {
  std::stringstream ss;

  ss.clear();
  char last = '\0';
  for (size_t i = 0; i < chain.size(); i++) {
    if (chain[i] == '+' || chain[i] == '-') last = chain[i];
  }
  if (last != opera)
    ss << opera << mode;
  else
    ss << mode;
  return ss.str();
}

void Server::getCommandArgs(std::string cmd, std::string &name,
                            std::string &modeset, std::string &params) {
  std::istringstream stm(cmd);
  stm >> name;
  stm >> modeset;
  size_t found = cmd.find_first_not_of(name + modeset + " \t\v");
  if (found != std::string::npos) params = cmd.substr(found);
}

std::vector<std::string> Server::splitParams(std::string params) {
  if (!params.empty() && params[0] == ':') params.erase(params.begin());
  std::vector<std::string> tokens;
  std::string param;
  std::istringstream stm(params);
  while (std::getline(stm, param, ',')) {
    tokens.push_back(param);
    param.clear();
  }
  return tokens;
}

void Server::modeCommand(std::string &cmd, int fd) {
  std::string channelName;
  std::string params;
  std::string modeset;
  std::stringstream mode_chain;
  std::string arguments = ":";
  Channel *channel;
  char opera = '\0';
  arguments.clear();
  mode_chain.clear();
  Client *cli = getClientFromFd(fd);
  size_t found = cmd.find_first_not_of("MODE \t\v");
  if (found != std::string::npos)
    cmd = cmd.substr(found);
  else {
    sendResponse(ERR_NOTENOUGHPARAM(cli->getNickname()), fd);
    return;
  }
  getCommandArgs(cmd, channelName, modeset, params);
  std::vector<std::string> tokens = splitParams(params);
  if (channelName[0] != '#' || !(channel = getChannel(channelName.substr(1)))) {
    sendResponse(ERR_CHANNELNOTFOUND(cli->getUsername(), channelName), fd);
    return;
  } else if (!channel->getClientFromFd(fd) && !channel->getAdmin(fd)) {
    sendError(442, getClientFromFd(fd)->getNickname(), channelName,
              getClientFromFd(fd)->getFd(), " :You're not on that channel\r\n");
    return;
  } else if (modeset.empty()) {
    sendResponse(RPL_CHANNELMODES(cli->getNickname(), channel->getName(),
                                  channel->getModes()) +
                     RPL_CREATIONTIME(cli->getNickname(), channel->getName(),
                                      channel->getCreationTime()),
                 fd);
    return;
  } else if (!channel->getAdmin(fd)) {
    sendResponse(ERR_NOTOPERATOR(channel->getName()), fd);
    return;
  } else if (channel) {
    size_t pos = 0;
    for (size_t i = 0; i < modeset.size(); i++) {
      if (modeset[i] == '+' || modeset[i] == '-')
        opera = modeset[i];
      else {
        if (modeset[i] == 'i')
          mode_chain << inviteOnlyOption(channel, opera, mode_chain.str());
        else if (modeset[i] == 't')
          mode_chain << topicRestrictionOption(channel, opera,
                                               mode_chain.str());
        else if (modeset[i] == 'k')
          mode_chain << channelKeyOption(tokens, channel, pos, opera, fd,
                                         mode_chain.str(), arguments);
        else if (modeset[i] == 'o')
          mode_chain << operatorPrivilegeOption(tokens, channel, pos, fd, opera,
                                                mode_chain.str(), arguments);
        else if (modeset[i] == 'l')
          mode_chain << userLimitOption(tokens, channel, pos, opera, fd,
                                        mode_chain.str(), arguments);
        else
          sendResponse(ERR_UNKNOWNMODE(cli->getNickname(), channel->getName(),
                                       modeset[i]),
                       fd);
      }
    }
  }
  std::string chain = mode_chain.str();
  if (chain.empty()) return;
  channel->sendToAll(RPL_CHANGEMODE(cli->getHostname(), channel->getName(),
                                    mode_chain.str(), arguments));
}

std::string Server::inviteOnlyOption(Channel *channel, char opera,
                                     std::string chain) {
  std::string param;
  param.clear();
  if (opera == '+' && !channel->getModeAtIndex(0)) {
    channel->setModeAtindex(0, true);
    channel->setInviteOnly(1);
    param = appendToMode(chain, opera, 'i');
  } else if (opera == '-' && channel->getModeAtIndex(0)) {
    channel->setModeAtindex(0, false);
    channel->setInviteOnly(0);
    param = appendToMode(chain, opera, 'i');
  }
  return param;
}

std::string Server::topicRestrictionOption(Channel *channel, char opera,
                                           std::string chain) {
  std::string param;
  param.clear();
  if (opera == '+' && !channel->getModeAtIndex(1)) {
    channel->setModeAtindex(1, true);
    channel->setTopicRestriction(true);
    param = appendToMode(chain, opera, 't');
  } else if (opera == '-' && channel->getModeAtIndex(1)) {
    channel->setModeAtindex(1, false);
    channel->setTopicRestriction(false);
    param = appendToMode(chain, opera, 't');
  }
  return param;
}

bool validPassword(std::string password) {
  if (password.empty()) return false;
  for (size_t i = 0; i < password.size(); i++) {
    if (!std::isalnum(password[i]) && password[i] != '_') return false;
  }
  return true;
}

std::string Server::channelKeyOption(std::vector<std::string> tokens,
                                     Channel *channel, size_t &pos, char opera,
                                     int fd, std::string chain,
                                     std::string &arguments) {
  std::string param;
  std::string pass;

  param.clear();
  pass.clear();
  if (tokens.size() > pos)
    pass = tokens[pos++];
  else {
    sendResponse(ERR_NEEDMODEPARM(channel->getName(), std::string("(k)")), fd);
    return param;
  }
  if (!validPassword(pass)) {
    sendResponse(ERR_INVALIDMODEPARM(channel->getName(), std::string("(k)")),
                 fd);
    return param;
  }
  if (opera == '+') {
    channel->setModeAtindex(2, true);
    channel->setPassword(pass);
    if (!arguments.empty()) arguments += " ";
    arguments += pass;
    param = appendToMode(chain, opera, 'k');
  } else if (opera == '-' && channel->getModeAtIndex(2)) {
    if (pass == channel->getPassword()) {
      channel->setModeAtindex(2, false);
      channel->setPassword("");
      param = appendToMode(chain, opera, 'k');
    } else
      sendResponse(ERR_KEYSET(channel->getName()), fd);
  }
  return param;
}

std::string Server::operatorPrivilegeOption(std::vector<std::string> tokens,
                                            Channel *channel, size_t &pos,
                                            int fd, char opera,
                                            std::string chain,
                                            std::string &arguments) {
  std::string user;
  std::string param;

  param.clear();
  user.clear();
  if (tokens.size() > pos)
    user = tokens[pos++];
  else {
    sendResponse(ERR_NEEDMODEPARM(channel->getName(), "(o)"), fd);
    return param;
  }
  if (!channel->clientInChannel(user)) {
    sendResponse(ERR_NOSUCHNICK(channel->getName(), user), fd);
    return param;
  }
  if (opera == '+') {
    channel->setModeAtindex(3, true);
    if (channel->changeClientToAdmin(user)) {
      param = appendToMode(chain, opera, 'o');
      if (!arguments.empty()) arguments += " ";
      arguments += user;
    }
  } else if (opera == '-') {
    channel->setModeAtindex(3, false);
    if (channel->changeAdminToClient(user)) {
      param = appendToMode(chain, opera, 'o');
      if (!arguments.empty()) arguments += " ";
      arguments += user;
    }
  }
  return param;
}

bool Server::isValidUserLimit(std::string &limit) {
  return (!(limit.find_first_not_of("0123456789") != std::string::npos) &&
          std::atoi(limit.c_str()) > 0);
}

std::string Server::userLimitOption(std::vector<std::string> tokens,
                                    Channel *channel, size_t &pos, char opera,
                                    int fd, std::string chain,
                                    std::string &arguments) {
  std::string limit;
  std::string param;
  param.clear();
  limit.clear();
  if (opera == '+') {
    if (tokens.size() > pos) {
      limit = tokens[pos++];
      if (!isValidUserLimit(limit)) {
        sendResponse(ERR_INVALIDMODEPARM(channel->getName(), "(l)"), fd);
      } else {
        channel->setModeAtindex(4, true);
        channel->setLimit(std::atoi(limit.c_str()));
        if (!arguments.empty()) arguments += " ";
        arguments += limit;
        param = appendToMode(chain, opera, 'l');
      }
    } else
      sendResponse(ERR_NEEDMODEPARM(channel->getName(), "(l)"), fd);
  } else if (opera == '-' && channel->getModeAtIndex(4)) {
    channel->setModeAtindex(4, false);
    channel->setLimit(0);
    param = appendToMode(chain, opera, 'l');
  }
  return param;
}

void FindPR(std::string cmd, std::string tofind, std::string &str) {
  size_t i = 0;
  for (; i < cmd.size(); i++) {
    if (cmd[i] != ' ') {
      std::string tmp;
      for (; i < cmd.size() && cmd[i] != ' '; i++) tmp += cmd[i];
      if (tmp == tofind)
        break;
      else
        tmp.clear();
    }
  }
  if (i < cmd.size()) str = cmd.substr(i);
  i = 0;
  for (; i < str.size() && str[i] == ' '; i++);
  str = str.substr(i);
}

std::string SplitCmdPR(std::string &cmd, std::vector<std::string> &tmp) {
  std::stringstream ss(cmd);
  std::string str, reason;
  int count = 2;
  while (ss >> str && count--) tmp.push_back(str);
  if (tmp.size() != 2) return std::string("");
  FindPR(cmd, tmp[1], reason);
  return reason;
}

int Server::parsePartCommand(std::string cmd, std::vector<std::string> &tmp,
                             std::string &reason, int fd) {
  reason = SplitCmdPR(cmd, tmp);
  if (tmp.size() < 2) {
    tmp.clear();
    return 0;
  }
  tmp.erase(tmp.begin());
  std::string str = tmp[0];
  std::string str1;
  tmp.clear();
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == ',') {
      tmp.push_back(str1);
      str1.clear();
    } else
      str1 += str[i];
  }
  tmp.push_back(str1);
  for (size_t i = 0; i < tmp.size(); i++) {
    if (tmp[i].empty()) tmp.erase(tmp.begin() + i--);
  }
  if (reason[0] == ':')
    reason.erase(reason.begin());
  else {
    for (size_t i = 0; i < reason.size(); i++) {
      if (reason[i] == ' ') {
        reason = reason.substr(0, i);
        break;
      }
    }
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    if (*(tmp[i].begin()) == '#')
      tmp[i].erase(tmp[i].begin());
    else {
      sendError(403, getClientFromFd(fd)->getNickname(), tmp[i],
                getClientFromFd(fd)->getFd(), " :No such channel\r\n");
      tmp.erase(tmp.begin() + i--);
    }
  }
  return 1;
}

void Server::partCommand(std::string cmd, int fd) {
  std::vector<std::string> tmp;
  std::string reason;
  if (!parsePartCommand(cmd, tmp, reason, fd)) {
    sendError(461, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(), " :Not enough parameters\r\n");
    return;
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    bool flag = false;
    for (size_t j = 0; j < this->channels.size(); j++) {
      if (this->channels[j].getName() == tmp[i]) {
        flag = true;
        if (!channels[j].getClientFromFd(fd) && !channels[j].getAdmin(fd)) {
          sendError(442, getClientFromFd(fd)->getNickname(), "#" + tmp[i],
                    getClientFromFd(fd)->getFd(),
                    " :You're not on that channel\r\n");
          continue;
        }
        std::stringstream ss;
        ss << ":" << getClientFromFd(fd)->getNickname() << "!~"
           << getClientFromFd(fd)->getUsername() << "@" << "localhost"
           << " PART #" << tmp[i];
        if (!reason.empty())
          ss << " :" << reason << "\r\n";
        else
          ss << "\r\n";
        channels[j].sendToAll(ss.str());
        if (channels[j].getAdmin(
                channels[j]
                    .getClientInChannel(getClientFromFd(fd)->getNickname())
                    ->getFd()))
          channels[j].removeAdmin(
              channels[j]
                  .getClientInChannel(getClientFromFd(fd)->getNickname())
                  ->getFd());
        else
          channels[j].removeClient(
              channels[j]
                  .getClientInChannel(getClientFromFd(fd)->getNickname())
                  ->getFd());
        if (channels[j].getClientCount() == 0)
          channels.erase(channels.begin() + j);
      }
    }
    if (!flag)
      sendError(403, getClientFromFd(fd)->getNickname(), "#" + tmp[i],
                getClientFromFd(fd)->getFd(), " :No such channel\r\n");
  }
}

void FindPM(std::string cmd, std::string tofind, std::string &str) {
  size_t i = 0;
  for (; i < cmd.size(); i++) {
    if (cmd[i] != ' ') {
      std::string tmp;
      for (; i < cmd.size() && cmd[i] != ' '; i++) tmp += cmd[i];
      if (tmp == tofind)
        break;
      else
        tmp.clear();
    }
  }
  if (i < cmd.size()) str = cmd.substr(i);
  i = 0;
  for (; i < str.size() && str[i] == ' '; i++);
  str = str.substr(i);
}

std::string SplitCmdPM(std::string &cmd, std::vector<std::string> &tmp) {
  std::stringstream ss(cmd);
  std::string str, msg;
  int count = 2;
  while (ss >> str && count--) tmp.push_back(str);
  if (tmp.size() != 2) return std::string("");
  FindPM(cmd, tmp[1], msg);
  return msg;
}

std::string SplitCmdPrivmsg(std::string cmd, std::vector<std::string> &tmp) {
  std::string str = SplitCmdPM(cmd, tmp);
  if (tmp.size() != 2) {
    tmp.clear();
    return std::string("");
  }
  tmp.erase(tmp.begin());
  std::string str1 = tmp[0];
  std::string str2;
  tmp.clear();
  for (size_t i = 0; i < str1.size(); i++) {
    if (str1[i] == ',') {
      tmp.push_back(str2);
      str2.clear();
    } else
      str2 += str1[i];
  }
  tmp.push_back(str2);
  for (size_t i = 0; i < tmp.size(); i++) {
    if (tmp[i].empty()) tmp.erase(tmp.begin() + i--);
  }
  if (str[0] == ':')
    str.erase(str.begin());
  else {
    for (size_t i = 0; i < str.size(); i++) {
      if (str[i] == ' ') {
        str = str.substr(0, i);
        break;
      }
    }
  }
  return str;
}

void Server::CheckForChannelsAndClients(std::vector<std::string> &tmp, int fd) {
  for (size_t i = 0; i < tmp.size(); i++) {
    if (tmp[i][0] == '#') {
      tmp[i].erase(tmp[i].begin());
      if (!getChannel(tmp[i])) {
        sendError(401, "#" + tmp[i], getClientFromFd(fd)->getFd(),
                  " :No such nick/channel\r\n");
        tmp.erase(tmp.begin() + i);
        i--;
      } else if (!getChannel(tmp[i])->getClientInChannel(
                     getClientFromFd(fd)->getNickname())) {
        sendError(404, getClientFromFd(fd)->getNickname(), "#" + tmp[i],
                  getClientFromFd(fd)->getFd(), " :Cannot send to channel\r\n");
        tmp.erase(tmp.begin() + i);
        i--;
      } else
        tmp[i] = "#" + tmp[i];
    } else {
      if (!getClientFromNickname(tmp[i])) {
        sendError(401, tmp[i], getClientFromFd(fd)->getFd(),
                  " :No such nick/channel\r\n");
        tmp.erase(tmp.begin() + i);
        i--;
      }
    }
  }
}

void Server::privmsgCommand(std::string cmd, int fd) {
  std::vector<std::string> tmp;
  std::string message = SplitCmdPrivmsg(cmd, tmp);
  if (!tmp.size()) {
    sendError(411, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(),
              " :No recipient given (PRIVMSG)\r\n");
    return;
  }
  if (message.empty()) {
    sendError(412, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(), " :No text to send\r\n");
    return;
  }
  if (tmp.size() > 10) {
    sendError(407, getClientFromFd(fd)->getNickname(),
              getClientFromFd(fd)->getFd(), " :Too many recipients\r\n");
    return;
  }
  CheckForChannelsAndClients(tmp, fd);
  for (size_t i = 0; i < tmp.size(); i++) {
    if (tmp[i][0] == '#') {
      tmp[i].erase(tmp[i].begin());
      std::string resp = ":" + getClientFromFd(fd)->getNickname() + "!~" +
                         getClientFromFd(fd)->getUsername() +
                         "@localhost PRIVMSG #" + tmp[i] + " :" + message +
                         "\r\n";
      getChannel(tmp[i])->sendToAll(resp, fd);
    } else {
      std::string resp = ":" + getClientFromFd(fd)->getNickname() + "!~" +
                         getClientFromFd(fd)->getUsername() +
                         "@localhost PRIVMSG " + tmp[i] + " :" + message +
                         "\r\n";
      sendResponse(resp, getClientFromNickname(tmp[i])->getFd());
    }
  }
}

void FindQ(std::string cmd, std::string tofind, std::string &str) {
  size_t i = 0;
  for (; i < cmd.size(); i++) {
    if (cmd[i] != ' ') {
      std::string tmp;
      for (; i < cmd.size() && cmd[i] != ' '; i++) tmp += cmd[i];
      if (tmp == tofind)
        break;
      else
        tmp.clear();
    }
  }
  if (i < cmd.size()) str = cmd.substr(i);
  i = 0;
  for (; i < str.size() && str[i] == ' '; i++);
  str = str.substr(i);
}

std::string SplitQuit(std::string cmd) {
  std::istringstream stm(cmd);
  std::string reason, str;
  stm >> str;
  FindQ(cmd, str, reason);
  if (reason.empty()) return std::string("QUIT");
  if (reason[0] != ':') {
    for (size_t i = 0; i < reason.size(); i++) {
      if (reason[i] == ' ') {
        reason.erase(reason.begin() + i, reason.end());
        break;
      }
    }
    reason.insert(reason.begin(), ':');
  }
  return reason;
}

void Server::quitCommand(std::string cmd, int fd) {
  std::string reason;
  reason = SplitQuit(cmd);
  for (size_t i = 0; i < channels.size(); i++) {
    if (channels[i].getClientFromFd(fd)) {
      channels[i].removeClient(fd);
      if (channels[i].getClientCount() == 0)
        channels.erase(channels.begin() + i);
      else {
        std::string rpl = ":" + getClientFromFd(fd)->getNickname() + "!~" +
                          getClientFromFd(fd)->getUsername() +
                          "@localhost QUIT " + reason + "\r\n";
        channels[i].sendToAll(rpl);
      }
    } else if (channels[i].getAdmin(fd)) {
      channels[i].removeAdmin(fd);
      if (channels[i].getClientCount() == 0)
        channels.erase(channels.begin() + i);
      else {
        std::string rpl = ":" + getClientFromFd(fd)->getNickname() + "!~" +
                          getClientFromFd(fd)->getUsername() +
                          "@localhost QUIT " + reason + "\r\n";
        channels[i].sendToAll(rpl);
      }
    }
  }
  std::cout << "Client <" << fd << "> Disconnected" << std::endl;
  removeChannel(fd);
  removeClient(fd);
  removeFd(fd);
  close(fd);
}

std::string Server::getTopicTime() {
  std::time_t current = std::time(NULL);
  std::stringstream res;
  res << current;
  return res.str();
}

std::string Server::getTopic(std::string &input) {
  size_t pos = input.find(":");
  if (pos == std::string::npos) return "";
  return input.substr(pos);
}

int Server::getColonPos(std::string &cmd) {
  for (int i = 0; i < (int)cmd.size(); i++)
    if (cmd[i] == ':' && (cmd[i - 1] == 32)) return i;
  return -1;
}

void Server::topicCommand(std::string &cmd, int &fd) {
  if (cmd == "TOPIC :") {
    sendError(461, getClientFromFd(fd)->getNickname(), fd,
              " :Not enough parameters\r\n");
    return;
  }
  std::vector<std::string> scmd = splitCommand(cmd);
  if (scmd.size() == 1) {
    sendError(461, getClientFromFd(fd)->getNickname(), fd,
              " :Not enough parameters\r\n");
    return;
  }
  std::string nmch = scmd[1].substr(1);
  if (!getChannel(nmch)) {
    sendError(403, "#" + nmch, fd, " :No such channel\r\n");
    return;
  }
  if (!(getChannel(nmch)->getClientFromFd(fd)) &&
      !(getChannel(nmch)->getAdmin(fd))) {
    sendError(442, "#" + nmch, fd, " :You're not on that channel\r\n");
    return;
  }
  if (scmd.size() == 2) {
    if (getChannel(nmch)->getTopicName() == "") {
      sendResponse(": 331 " + getClientFromFd(fd)->getNickname() + " " + "#" +
                       nmch + " :No topic is set\r\n",
                   fd);
      return;
    }
    size_t pos = getChannel(nmch)->getTopicName().find(":");
    if (getChannel(nmch)->getTopicName() != "" && pos == std::string::npos) {
      sendResponse(": 332 " + getClientFromFd(fd)->getNickname() + " " + "#" +
                       nmch + " " + getChannel(nmch)->getTopicName() + "\r\n",
                   fd);
      sendResponse(": 333 " + getClientFromFd(fd)->getNickname() + " " + "#" +
                       nmch + " " + getClientFromFd(fd)->getNickname() + " " +
                       getChannel(nmch)->getTime() + "\r\n",
                   fd);
      return;
    } else {
      size_t pos = getChannel(nmch)->getTopicName().find(" ");
      if (pos == 0) getChannel(nmch)->getTopicName().erase(0, 1);
      sendResponse(": 332 " + getClientFromFd(fd)->getNickname() + " " + "#" +
                       nmch + " " + getChannel(nmch)->getTopicName() + "\r\n",
                   fd);
      sendResponse(": 333 " + getClientFromFd(fd)->getNickname() + " " + "#" +
                       nmch + " " + getClientFromFd(fd)->getNickname() + " " +
                       getChannel(nmch)->getTime() + "\r\n",
                   fd);
      return;
    }
  }

  if (scmd.size() >= 3) {
    std::vector<std::string> tmp;
    int pos = getColonPos(cmd);
    if (pos == -1 || scmd[2][0] != ':') {
      tmp.push_back(scmd[0]);
      tmp.push_back(scmd[1]);
      tmp.push_back(scmd[2]);
    } else {
      tmp.push_back(scmd[0]);
      tmp.push_back(scmd[1]);
      tmp.push_back(cmd.substr(getColonPos(cmd)));
    }

    if (tmp[2][0] == ':' && tmp[2][1] == '\0') {
      sendError(331, "#" + nmch, fd, " :No topic is set\r\n");
      return;
    }

    if (getChannel(nmch)->getTopicRestriction() &&
        getChannel(nmch)->getClientFromFd(fd)) {
      sendError(482, "#" + nmch, fd, " :You're Not a channel operator\r\n");
      return;
    } else if (getChannel(nmch)->getTopicRestriction() &&
               getChannel(nmch)->getAdmin(fd)) {
      getChannel(nmch)->setTime(getTopicTime());
      getChannel(nmch)->setTopicName(tmp[2]);
      std::string rpl;
      size_t pos = tmp[2].find(":");
      if (pos == std::string::npos)
        rpl = ":" + getClientFromFd(fd)->getNickname() + "!" +
              getClientFromFd(fd)->getUsername() + "@localhost TOPIC #" + nmch +
              " :" + getChannel(nmch)->getTopicName() + "\r\n";
      else
        rpl = ":" + getClientFromFd(fd)->getNickname() + "!" +
              getClientFromFd(fd)->getUsername() + "@localhost TOPIC #" + nmch +
              " " + getChannel(nmch)->getTopicName() + "\r\n";
      getChannel(nmch)->sendToAll(rpl);
    } else {
      std::string rpl;
      size_t pos = tmp[2].find(":");
      if (pos == std::string::npos) {
        getChannel(nmch)->setTime(getTopicTime());
        getChannel(nmch)->setTopicName(tmp[2]);
        rpl = ":" + getClientFromFd(fd)->getNickname() + "!" +
              getClientFromFd(fd)->getUsername() + "@localhost TOPIC #" + nmch +
              " " + getChannel(nmch)->getTopicName() + "\r\n";
      } else {
        size_t poss = tmp[2].find(" ");
        getChannel(nmch)->setTopicName(tmp[2]);
        if (poss == std::string::npos && tmp[2][0] == ':' && tmp[2][1] != ':')
          tmp[2] = tmp[2].substr(1);
        getChannel(nmch)->setTopicName(tmp[2]);
        getChannel(nmch)->setTime(getTopicTime());
        rpl = ":" + getClientFromFd(fd)->getNickname() + "!" +
              getClientFromFd(fd)->getUsername() + "@localhost TOPIC #" + nmch +
              " " + getChannel(nmch)->getTopicName() + "\r\n";
      }
      getChannel(nmch)->sendToAll(rpl);
    }
  }
}
