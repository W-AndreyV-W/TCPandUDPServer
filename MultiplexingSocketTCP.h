#pragma once

#include "MultiplexingSocket.h"

class MultiplexingSocketTCP : public MultiplexingSocket {

public:

	bool setPort(int portNumber, connectionProtocol protocol = ip, int connections = 50);

private:

	bool openPort(int portNumber, connectionProtocol protocol, int connections) override;
	bool connectSocket(int portNumber, connectionProtocol protocol, int connections, int listener);
	void newSocketAddress(int portNumber, connectionProtocol protocol);
	void threadSocket();
	void deleteSocketPort(std::map<int, internetMessage>* plThrdSckt, const internetMessage* incomingMessage, bool* scktOn, int scktDscrptr);
	void copyingOutgoingMessagesPort(std::map<int, std::deque<internetMessage>>* mssgBffr, const internetMessage* incomingMessage);
	bool newSocketPoet(std::map<int, internetMessage>* plThrdSckt, epoll_event* evntSckts, const internetMessage* incomingMessage, int scktDscrptr);
	void receivingMessagePort(std::map<int, internetMessage>* plThrdSckt, std::deque<internetMessage>* incmngDtThrd, epoll_event* evnts, char* chrBf);
	void saveReceivingMessagePort(std::map<int, internetMessage>::iterator plThrdSckt, std::deque<internetMessage>* incmngDtThrd);
	void sendingMessagePort(std::map<int, std::deque<internetMessage>>* mssgBffr, epoll_event* evnts, int MaxSizeMessage);
	void copyingIncomingMessages(std::deque<internetMessage>* incmngDtThrd);
};