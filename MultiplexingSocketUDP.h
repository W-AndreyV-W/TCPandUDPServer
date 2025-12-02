#pragma once

#include "MultiplexingSocket.h"

class MultiplexingSocketUDP : public MultiplexingSocket {

public:

	bool setPort(int portNumber, connectionProtocol protocol = ip, int connections = 50);
	void deleteSocket(int portNumber, int socketNumber) = delete;

private:
	
	bool openPort(int portNumber, connectionProtocol protocol, int connections)  override;
	bool connectSocket(int portNumber, connectionProtocol protocol, int connections, int listener);
	void newSocketAddress(int portNumber, connectionProtocol protocol);
	void threadSocket();
	void deletePort(const internetMessage* incomingMessage, bool* scktOn);
	void copyingOutgoingMessagesPort(std::deque<internetMessage>* mssgBffr, const internetMessage* incomingMessage);
	void receivingMessagePort(internetMessage* nwMssg, const internetMessage* incomingMessage, std::deque<internetMessage>* incmngDtThrd, char* chrBf);
	void saveReceivingMessagePort(internetMessage* nwMssg, std::deque<internetMessage>* incmngDtThrd);
	void sendingMessagePort(std::deque<internetMessage>* mssgBffr, int MaxSizeMessage);
	void copyingIncomingMessages(std::deque<internetMessage>* incmngDtThrd);
};