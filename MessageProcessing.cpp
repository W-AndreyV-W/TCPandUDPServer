#include "MessageProcessing.h"

MessageProcessing::MessageProcessing(MultiplexingSocket* multiplexSocket) {

	multiplexingSocket = multiplexSocket;

	incomingClients = new std::deque<MultiplexingSocket::internetMessage>;
	poolTimerOff = new std::deque<dateVisit>;
	poolAddressClient = new std::map<long long, std::list<addressClient>>;

	startProcessing();

	threadClass->push_back(std::jthread(&MessageProcessing::threadProcessing, this));
	threadClass->push_back(std::jthread(&MessageProcessing::threadAddressClient, this));
}

MessageProcessing::~MessageProcessing() {

	mutexEnd.lock();
	threadOperation = false;
	mutexEnd.unlock();

	for (auto& thrdClss : *threadClass) {

		thrdClss.join();
	}

	delete incomingClients;
	delete poolTimerOff;
	delete poolAddressClient;		 
	delete threadClass;
}

MessageProcessing::addressClient::addressClient(MultiplexingSocket::internetMessage& add) {

	port = add.port;
	socket = add.socket;
	address = add.address;
	protocol = add.protocol;
	dateVisit = add.dateMessage;
}

bool MessageProcessing::addressClient::operator==(MultiplexingSocket::internetMessage& other) {
	
	if (socket != other.socket) {

		return false;
	}
	else {

		for (int i = 2; i < 8; i++) {

			if (address.sa_data[i] != other.address.sa_data[i]) {

				return false;
			}
		}
	}

	return true;
}

bool MessageProcessing::addressClient::operator==(addressClient& other) {

	if (socket != other.socket) {

		return false;
	}
	else {

		for (int i = 2; i < 8; i++) {

			if (address.sa_data[i] != other.address.sa_data[i]) {

				return false;
			}
		}
	}

	return true;
}

MessageProcessing::dateVisit::dateVisit(std::time_t dtVst, std::map<long long, std::list<addressClient>>::iterator itrMpAdd, 
										std::list<addressClient>::iterator itrAddr) {

	dateVst = dtVst;
	iterMapAddress = itrMpAdd;
	iterAddress = itrAddr;
}

int MessageProcessing::numberClient() {

	int numClnt = 0;
	std::unique_lock lock(mutexAddressClient);

	for (const auto& plAddClnt : *poolAddressClient) {

		numClnt += static_cast<int>(plAddClnt.second.size());
	}

	return numClnt;
}

void MessageProcessing::deleteClient(MultiplexingSocket::internetMessage delClient) {

	long long hashAddress = std::hash<std::string_view>{}(delClient.address.sa_data);
	mutexAddressClient.lock();

	if (auto plAddrssClnt = poolAddressClient->find(hashAddress); plAddrssClnt != poolAddressClient->end()) {

		for (auto plAdd = plAddrssClnt->second.begin(); plAdd != plAddrssClnt->second.end(); ++plAdd) {

			if (*plAdd == delClient) {

				plAddrssClnt->second.erase(plAdd);
			}
		}
	}

	mutexAddressClient.unlock();

	if (delClient.protocol == TCP) {

		multiplexingSocket->deleteSocketTCP(delClient.port, delClient.socket);
	}
}

void MessageProcessing::threadProcessing() {

	bool threadOn = true;

	while (threadOn) {

		multiplexingSocket->checkingIncomingMessages(true);
		MultiplexingSocket::internetMessage intrntMsag = std::move(multiplexingSocket->getIncomingMessage());
		std::string* intMes = intrntMsag.getMessage();
		addingClient(&intrntMsag);

		if (intMes->at(0) == '/') {

			if (intMes->find_first_of("/time") != std::string::npos) {

				preparingDate(intMes);
				intrntMsag.setMessage(intMes);
				multiplexingSocket->setOutgoingMessage(intrntMsag);
			}
			else if (intMes->find_first_of("/stats") != std::string::npos) {

				preparationNumberClients(intMes);
				intrntMsag.setMessage(intMes);
				multiplexingSocket->setOutgoingMessage(intrntMsag);
			}
			else if (intMes->find_first_of("/shutdown") != std::string::npos) {

				deleteClient(intrntMsag);
			}
			else if (intMes->find_first_of("/close") != std::string::npos && intrntMsag.conProtocol == Unix) {

				threadOn = false;
				mutexEnd.lock();
				threadOperation = false;
				mutexEnd.unlock();
			}

			if (intMes == nullptr) {

				delete intMes;
			}
		}
		else {

			intrntMsag.setMessage(intMes);
			multiplexingSocket->setOutgoingMessage(intrntMsag);
		}
	}
}

void MessageProcessing::addingClient(MultiplexingSocket::internetMessage* intrntMsag) {

	mutexAddressClient.lock();
	incomingClients->push_back(*intrntMsag);
	mutexAddressClient.unlock();
	messagesAddress.notify_all();
}

void MessageProcessing::preparingDate(std::string* intMes) {

	char timeString[20];
	std::time_t timeServer = std::time(nullptr);
	std::strftime(std::data(timeString), std::size(timeString), "%F %T", std::gmtime(&timeServer));
	intMes->clear();
	*intMes = std::string(timeString);
}

void MessageProcessing::preparationNumberClients(std::string* intMes) {

	intMes->clear();
	*intMes = std::to_string(numberClient());
}

void MessageProcessing::blockingAddressClient() {

	const long long TIMEOUT = 100;
	std::unique_lock lock(mutexAddressClient);
	messagesAddress.wait_for(lock, std::chrono::duration<long long, std::milli>(TIMEOUT), [&]() {return !incomingClients->empty(); });
}

void MessageProcessing::threadAddressClient() {

	bool threadOn = true;
	double TIMEOUT_ADDRESS = 300;
	std::deque<MultiplexingSocket::internetMessage>* incmngClnts = new std::deque<MultiplexingSocket::internetMessage>;

	while (threadOn) {

		blockingAddressClient();
		addingClientAddress(incmngClnts);
		deletingClientsTime(TIMEOUT_ADDRESS);
		threadOn = endThread();
	}

	delete incmngClnts;
}

void MessageProcessing::addingClientAddress(std::deque<MultiplexingSocket::internetMessage>* incmngClnts) {

	mutexAddressClient.lock();

	if (!incomingClients->empty()) {

		while (!incomingClients->empty()) {

			incmngClnts->push_back(incomingClients->front());
			incomingClients->pop_front();
		}

		mutexAddressClient.unlock();
		searchClientAddress(incmngClnts);
	}
	else {

		mutexAddressClient.unlock();
	}
}

void MessageProcessing::searchClientAddress(std::deque<MultiplexingSocket::internetMessage>* incmngClnts) {

	while (!incmngClnts->empty()) {

		bool addressSearch = true;
		long long hashAddress = std::hash<std::string_view>{}(incmngClnts->front().address.sa_data);
		mutexAddressClient.lock();

		if (auto plAddrssClnt = poolAddressClient->find(hashAddress); plAddrssClnt != poolAddressClient->end()) {

			for (auto& plAdd : plAddrssClnt->second) {

				if (plAdd == incmngClnts->front()) {

					addressSearch = false;
					updatingVisitTime(incmngClnts, &plAdd);
				}
			}

			if (addressSearch) {

				addressSearch = false;
				addingClient(plAddrssClnt, incmngClnts);
			}
		}

		if (addressSearch) {

			addingNewClientAddress(incmngClnts, hashAddress);
		}

		mutexAddressClient.unlock();
		incmngClnts->pop_front();
	}
}

void MessageProcessing::updatingVisitTime(std::deque<MultiplexingSocket::internetMessage>* incmngClnts, addressClient* plAdd) {

	plAdd->dateVisit = incmngClnts->front().dateMessage;
}

void MessageProcessing::addingClient(std::map<long long, std::list<addressClient>>::iterator& plAddrssClnt, std::deque<MultiplexingSocket::internetMessage>* incmngClnts) {

	plAddrssClnt->second.emplace_back(addressClient(incmngClnts->front()));
	poolTimerOff->emplace_back(dateVisit(incmngClnts->front().dateMessage, plAddrssClnt, --plAddrssClnt->second.end()));
}

void MessageProcessing::addingNewClientAddress(std::deque<MultiplexingSocket::internetMessage>* incmngClnts, long long hashAddress) {

	poolAddressClient->emplace(hashAddress, std::list<addressClient>{incmngClnts->front()});

	if (auto plAddClnt = poolAddressClient->find(hashAddress); plAddClnt != poolAddressClient->end()) {

		poolTimerOff->emplace_back(dateVisit(incmngClnts->front().dateMessage, plAddClnt, plAddClnt->second.begin()));
	}
}

void MessageProcessing::deletingClientsTime(double timeoutAddress) {

	while (!poolTimerOff->empty()) {

		if (difftime(std::time(nullptr), poolTimerOff->front().dateVst) > timeoutAddress) {

			checkingTimeLastSession(timeoutAddress);
			poolTimerOff->pop_front();
		}
		else {

			break;
		}
	}
}

void MessageProcessing::checkingTimeLastSession(double timeoutAddress) {
	
	mutexAddressClient.lock();
	
	if (!poolTimerOff->front().iterMapAddress->second.empty()) {

		for (auto iterTO = poolTimerOff->front().iterMapAddress->second.begin(); iterTO != poolTimerOff->front().iterMapAddress->second.end(); ++iterTO) {

			if (poolTimerOff->front().iterAddress == iterTO) {

				if (difftime(std::time(nullptr), iterTO->dateVisit) > timeoutAddress) {

					deleteAddress(iterTO);
					break;
				}
			}
		}
	}

	mutexAddressClient.unlock();
}

void MessageProcessing::deleteAddress(std::list<addressClient>::iterator& iterTO) {
	
	if (iterTO->protocol == TCP) {

		multiplexingSocket->deleteSocketTCP(iterTO->port, iterTO->socket);
	}

	poolTimerOff->front().iterMapAddress->second.erase(iterTO);
}

void MessageProcessing::startProcessing() {

	bool threadOn = true;

	multiplexingSocket->setPort(8088);
	multiplexingSocket->setPort(8089, ip, UDP);
	multiplexingSocket->setPort(8090, Unix);

	while (threadOn) {
	
		std::this_thread::sleep_for(std::chrono::duration<long long, std::milli>(2000));
		mutexEnd.lock();
		threadOn = threadOperation;
		mutexEnd.unlock();
	}
}

bool MessageProcessing::endThread() {

	bool endThrd = true;
	mutexEnd.lock();
	endThrd = threadOperation;
	mutexEnd.unlock();

	return endThrd;
}