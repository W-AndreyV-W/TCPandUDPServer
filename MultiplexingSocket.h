#pragma once

#include <sys/types.h>
#include <sys/socket.h>
//#include <sys/time.h>
#include <sys/epoll.h>
#include <netinet/in.h>
//#include <stdio.h>
//#include <unistd.h>
#include <fcntl.h>
//#include <ctime>

#include <deque>
//#include <vector>
#include <map>
//#include <string>
#include <variant>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstddef>

class MultiplexingSocket {

public:

	MultiplexingSocket();
	MultiplexingSocket(const MultiplexingSocket&) = default;
	MultiplexingSocket(MultiplexingSocket&&) = default;
	~MultiplexingSocket();

	MultiplexingSocket& operator=(const MultiplexingSocket&) = default;
	MultiplexingSocket& operator=(MultiplexingSocket&&) = default;

	enum ConnectionProtocol {

		ip,
		Unix,
	};

	enum portProtocol {

		TCP,
		UDP,
	};

	struct internetMessage 	{

	public:

		internetMessage();
		internetMessage(const internetMessage& other);
		internetMessage(internetMessage&& other);
		~internetMessage();
				
		internetMessage& operator=(internetMessage& other);
		internetMessage& operator=(const internetMessage& other);
		internetMessage& operator=(internetMessage&& other);
		bool operator==(internetMessage& eother);
		
		int port = 0;
		int socket = 0;
		size_t  messageSize = 0;
		portProtocol protocol;
		sockaddr address;
		socklen_t addressSize;
		std::time_t dateMessage;

		void setMessage(std::string* mssg);
		void setMessage(std::string mssg);
		std::string* getMessage();

	private:

		std::string* message = nullptr;
	};

	bool setPort(int portNumber, ConnectionProtocol protocol = ip, portProtocol portProt = TCP, int connections = 50);
	void deletePort(int portNumber);
	int checkingIncomingMessages(bool blockingThread = false);
	internetMessage getIncomingMessage();
	void setOutgoingMessage(internetMessage& outgoingMessage);
	int numberClient();
	void deleteClient(internetMessage delClient);

private:

	struct addressClient {

		addressClient(internetMessage& add, std::time_t dateVst);

		std::time_t dateVisit;
		internetMessage address;
	};

	struct dateVisit {

		std::time_t dateVst;
		std::map<long long, std::deque<addressClient>>::iterator iterMapAddress;
		std::deque<addressClient>::iterator iterAddress;
	};

	bool threadOperation = true;
	int messageConnector = 0;
	int maxEvents = 20;
	std::deque<const internetMessage> threadInternetMessage;
	std::deque<const sockaddr> socketAddress;
	std::deque<internetMessage>* incomingDataBuffer = nullptr;
	std::deque<internetMessage>* incomingMessage = nullptr;
	std::deque<dateVisit>* poolTimerOff = nullptr;
	std::deque<std::jthread>* threadClass = nullptr;
	std::map<int, std::deque<internetMessage>>* outgoingDataBuffer = nullptr;
	std::map<long long, std::deque<addressClient>>* poolAddressClient = nullptr;
	std::map<int, std::deque<int>> bufferDeletingPort;
	std::mutex mutexIncomingDataBuffer;
	std::mutex mutexOutgoingDataBuffer;
	std::mutex mutexIncomingMessage;
	std::mutex mutexDeleteSocket;
	std::mutex mutexAddressClient;
	std::mutex mutexEnd;
	std::condition_variable messagesThread;
	std::condition_variable messagesOut;

	bool openPortTCP(int portNumber, ConnectionProtocol protocol, portProtocol portProt, int connections);
	bool openPortUDP(int portNumber, ConnectionProtocol protocol, portProtocol portProt, int connections);
	bool connectSocket(int portNumber, ConnectionProtocol protocol, portProtocol portProt, int connections, int listener);
	void newSocketAddress(int portNumber, ConnectionProtocol protocol);
	void threadSocketTCP();
	void threadSocketUDP();
	void blockingAddressClient();
	void threadAddressClient();
	void blockingMessageProcessing();
};
