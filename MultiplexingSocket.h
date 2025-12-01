#pragma once

#include <fcntl.h>

#include <deque>
#include <map>
#include <variant>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

enum connectionProtocol {

	ip,
	Unix,
};

enum portProtocol {

	TCP,
	UDP,
};

class MultiplexingSocket {

public:

	MultiplexingSocket();
	MultiplexingSocket(const MultiplexingSocket&) = default;
	MultiplexingSocket(MultiplexingSocket&&) = default;
	~MultiplexingSocket();

	MultiplexingSocket& operator=(const MultiplexingSocket&) = default;
	MultiplexingSocket& operator=(MultiplexingSocket&&) = default;



	struct internetMessage 	{

	public:

		internetMessage();
		internetMessage(const internetMessage& other);
		internetMessage(internetMessage&& other);
		~internetMessage();
				
		internetMessage& operator=(internetMessage& other);
		internetMessage& operator=(const internetMessage& other);
		internetMessage& operator=(internetMessage&& other);
		
		int port = 0;
		int socket = 0;
		size_t  messageSize = 0;
		portProtocol protocol;
		connectionProtocol conProtocol;
		socklen_t addressSize;
		sockaddr address;
		std::time_t dateMessage;

		void setMessage(std::string* mssg);
		void setMessage(std::string mssg);
		std::string* getMessage();
		bool operator!=(struct sockaddr& other);

	private:

		std::string* message = nullptr;
	};

	bool setPort(int portNumber, connectionProtocol protocol = ip, portProtocol portProt = TCP, int connections = 50);
	void deletePort(int portNumber);
	void deleteSocketTCP(int portNumber, int socketNumber);
	int checkingIncomingMessages(bool blockingThread = false);
	internetMessage getIncomingMessage();
	void setOutgoingMessage(internetMessage& outgoingMessage);

private:

	bool threadOperation = true;
	int messageConnector = 0;
	int maxEvents = 20;
	std::deque<internetMessage> threadInternetMessage;
	std::deque<sockaddr> socketAddress;
	std::deque<internetMessage>* incomingDataBuffer = nullptr;
	std::deque<std::jthread>* threadClass = nullptr;
	std::map<int, std::deque<internetMessage>>* outgoingDataBuffer = nullptr;
	std::map<int, std::deque<int>> bufferDeletingPort;
	std::mutex mutexIncomingDataBuffer;
	std::mutex mutexOutgoingDataBuffer;
	std::mutex mutexDeleteSocket;
	std::mutex mutexEnd;
	std::condition_variable messagesThread;


	bool openPortTCP(int portNumber, connectionProtocol protocol, portProtocol portProt, int connections);
	bool openPortUDP(int portNumber, connectionProtocol protocol, portProtocol portProt, int connections);
	bool connectSocket(int portNumber, connectionProtocol protocol, portProtocol portProt, int connections, int listener);
	void newSocketAddress(int portNumber, connectionProtocol protocol);
	void threadSocketTCP();	 
	void deleteSocketPortTCP(std::map<int, internetMessage>* plThrdSckt, const internetMessage* incomingMessage, bool* scktOn, int scktDscrptr);
	void copyingOutgoingMessagesPortTCP(std::map<int, std::deque<internetMessage>>* mssgBffr, const internetMessage* incomingMessage);
	bool newSocketPoetTCP(std::map<int, internetMessage>* plThrdSckt, epoll_event* evntSckts, const internetMessage* incomingMessage, int scktDscrptr);
	void receivingMessagePortTCP(std::map<int, internetMessage>* plThrdSckt, std::deque<internetMessage>* incmngDtThrd, epoll_event* evnts, char* chrBf);
	void saveReceivingMessagePortTCP(std::map<int, internetMessage>::iterator plThrdSckt, std::deque<internetMessage>* incmngDtThrd);
	void sendingMessagePortTCP(std::map<int, std::deque<internetMessage>>* mssgBffr, epoll_event* evnts, int MaxSizeMessage);
	void copyingIncomingMessages(std::deque<internetMessage>* incmngDtThrd);
	void threadSocketUDP();
	void deletePortUDP(const internetMessage* incomingMessage, bool* scktOn);
	void copyingOutgoingMessagesPortUDP(std::deque<internetMessage>* mssgBffr, const internetMessage* incomingMessage);
	void receivingMessagePortUDP(internetMessage* nwMssg, const internetMessage* incomingMessage, std::deque<internetMessage>* incmngDtThrd, char* chrBf);
	void saveReceivingMessagePortUDP(internetMessage* nwMssg, std::deque<internetMessage>* incmngDtThrd);
	void sendingMessagePortUDP(std::deque<internetMessage>* mssgBffr, int MaxSizeMessage);
	void blockingMessageProcessing();
	bool endThread();
};