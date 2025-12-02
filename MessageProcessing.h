#pragma once

#include <thread>
#include <deque>
#include <list>
#include <condition_variable>
#include <utility>

#include "MultiplexingSocket.h"
#include <bits/socket.h>

class MessageProcessing {

public:

	MessageProcessing();
	MessageProcessing(const MessageProcessing&) = default;
	MessageProcessing(MessageProcessing&&) = default;
	~MessageProcessing();

	MessageProcessing& operator=(const MessageProcessing&) = default;
	MessageProcessing& operator=(MessageProcessing&&) = default;
	
	void newThread(MultiplexingSocket* newMultiplexingSocket);
	void startProcessing();

private:

	struct addressClient {

		addressClient(MultiplexingSocket::internetMessage& add);
		addressClient(const addressClient& other);

		int port = 0;
		int socket = 0;
		sockaddr address;
		MultiplexingSocket::portProtocol protocol;
		std::time_t dateVisit;
		MultiplexingSocket* multiplexingSocket;

		bool operator==(MultiplexingSocket::internetMessage& other);
		bool operator==(addressClient& other);
	};

	struct dateVisit {

		dateVisit(std::time_t dtVst, std::map<long long, std::list<addressClient>>::iterator itrMpAdd, std::list<addressClient>::iterator itrAddr);

		std::time_t dateVst;
		std::map<long long, std::list<addressClient>>::iterator iterMapAddress;
		std::list<addressClient>::iterator iterAddress;
	};

	bool threadOperation = true;
	std::deque<dateVisit>* poolTimerOff = nullptr;
	std::map<long long, std::list<addressClient>>* poolAddressClient = nullptr;
	std::deque<addressClient>* incomingClients = nullptr;
	std::deque<MultiplexingSocket*>* multiplexingSocket = nullptr;
	std::deque<std::jthread>* threadClass = nullptr;
	std::mutex mutexAddressClient;
	std::mutex mutexMultiplexingSocket;
	std::mutex mutexEnd;
	std::condition_variable messagesAddress;

	int numberClient();
	void deleteClient(MultiplexingSocket* mltplxngSckt, MultiplexingSocket::internetMessage delClient);
	void threadProcessing();
	void addingClient(MultiplexingSocket* mltplxngSckt, MultiplexingSocket::internetMessage* intrntMsag);
	void preparingDate(std::string* intMes);
	void preparationNumberClients(std::string* intMes);
	void blockingAddressClient();
	void threadAddressClient();
	void addingClientAddress(std::deque<addressClient>* incmngClnts);
	void searchClientAddress(std::deque<addressClient>* incmngClnts);
	void updatingVisitTime(std::deque<addressClient>* incmngClnts, addressClient* plAdd);
	void addingClient(std::map<long long, std::list<addressClient>>::iterator& plAddrssClnt, std::deque<addressClient>* incmngClnts);
	void addingNewClientAddress(std::deque<addressClient>* incmngClnts, long long hashAddress);
	void deletingClientsTime(double timeoutAddress);
	void checkingTimeLastSession(double timeoutAddress);
	void deleteAddress(std::list<addressClient>::iterator& iterTO);
	bool endThread();
};