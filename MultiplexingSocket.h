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

class MultiplexingSocket {

public:

	MultiplexingSocket();
	MultiplexingSocket(const MultiplexingSocket&) = default;
	MultiplexingSocket(MultiplexingSocket&&) = default;
	~MultiplexingSocket();

	MultiplexingSocket& operator=(const MultiplexingSocket&) = default;
	MultiplexingSocket& operator=(MultiplexingSocket&&) = default;

	enum connectionProtocol {

		ip,
		Unix,
	};

	enum portProtocol {

		TCP,
		UDP,
	};

	struct internetMessage {

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
	
	bool setPort(int portNumber, connectionProtocol protocol = ip, int connections = 50);
	void deletePort(int portNumber);
	void deleteSocket(int portNumber, int socketNumber);
	int checkingIncomingMessages(bool blockingThread = false);
	internetMessage getIncomingMessage();
	void setOutgoingMessage(internetMessage& outgoingMessage);

protected:

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

	virtual bool openPort(int portNumber, connectionProtocol protocol, int connections);
	void blockingMessageProcessing();
	bool endThread();

private:

};