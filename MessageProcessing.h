#pragma once

#include <thread>
#include <deque>
#include <condition_variable>
#include <utility>

#include "MultiplexingSocket.h"
#include <bits/socket.h>

class MessageProcessing {

public:

	MessageProcessing(MultiplexingSocket* multiplexSocket);
	MessageProcessing(const MessageProcessing&) = default;
	MessageProcessing(MessageProcessing&&) = default;
	~MessageProcessing();

	MessageProcessing& operator=(const MessageProcessing&) = default;
	MessageProcessing& operator=(MessageProcessing&&) = default;

private:

	struct addressClient {

		addressClient(MultiplexingSocket::internetMessage& add);

		int port = 0;
		int socket = 0;
		sockaddr address;
		portProtocol protocol;
		std::time_t dateVisit;

		bool operator==(MultiplexingSocket::internetMessage& other);
		bool operator==(addressClient& other);
	};

	struct dateVisit {

		dateVisit(std::time_t dtVst, std::map<long long, std::deque<addressClient>>::iterator itrMpAdd, std::deque<addressClient>::iterator itrAddr);

		std::time_t dateVst;
		std::map<long long, std::deque<addressClient>>::iterator iterMapAddress;
		std::deque<addressClient>::iterator iterAddress;
	};

	MultiplexingSocket* multiplexingSocket;

	bool threadOperation = true;
	std::deque<dateVisit>* poolTimerOff = nullptr;
	std::map<long long, std::deque<addressClient>>* poolAddressClient = nullptr;
	std::deque<MultiplexingSocket::internetMessage>* incomingClients = nullptr;
	std::deque<std::jthread>* threadClass = nullptr;
	std::mutex mutexAddressClient;
	std::mutex mutexEnd;
	std::condition_variable messagesAddress;

	int numberClient();
	void deleteClient(MultiplexingSocket::internetMessage delClient);
	void threadProcessing();
	void blockingAddressClient();
	void threadAddressClient();
	void startProcessing();
};