#include "MessageProcessing.h"

MessageProcessing::MessageProcessing() {

	incomingClients = new std::deque<addressClient>;
	poolTimerOff = new std::deque<dateVisit>;
	poolAddressClient = new std::map<long long, std::list<addressClient>>;
	multiplexingSocket = new std::deque<MultiplexingSocket*>;
	threadClass = new std::deque<std::jthread>;
	threadClass->push_back(std::jthread(&MessageProcessing::threadAddressClient, this));
}

MessageProcessing::~MessageProcessing() {

	mutexEnd.lock();
	threadOperation = false;
	mutexEnd.unlock();

	for (auto& thrdClss : *threadClass) {

		thrdClss.join();
	}

	for (auto& mltplxngSckt : *multiplexingSocket) {

		if (mltplxngSckt != nullptr) {

			delete mltplxngSckt;
		}
	}

	delete multiplexingSocket;
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

MessageProcessing::addressClient::addressClient(const addressClient& other) {

	port = other.port;
	socket = other.socket;
	address = other.address;
	protocol = other.protocol;
	dateVisit = other.dateVisit;
	multiplexingSocket = other.multiplexingSocket;
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

void MessageProcessing::deleteClient(MultiplexingSocket* mltplxngSckt, MultiplexingSocket::internetMessage delClient) {

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

	if (delClient.protocol == MultiplexingSocket::TCP) {

		mltplxngSckt->deleteSocket(delClient.port, delClient.socket);
	}
}

void MessageProcessing::threadProcessing() {

	bool threadOn = true;
	mutexMultiplexingSocket.lock();
	MultiplexingSocket* mltplxngSckt = multiplexingSocket->back();
	mutexMultiplexingSocket.unlock();

	while (threadOn && mltplxngSckt != nullptr) {

		mltplxngSckt->checkingIncomingMessages(true);
		MultiplexingSocket::internetMessage intrntMsag = std::move(mltplxngSckt->getIncomingMessage());
		std::string* intMes = intrntMsag.getMessage();
		addingClient(mltplxngSckt, &intrntMsag);

		if (intMes->at(0) == '/') {

			if (intMes->find_first_of("/time") != std::string::npos) {

				preparingDate(intMes);
				intrntMsag.setMessage(intMes);
				mltplxngSckt->setOutgoingMessage(intrntMsag);
			}
			else if (intMes->find_first_of("/stats") != std::string::npos) {

				preparationNumberClients(intMes);
				intrntMsag.setMessage(intMes);
				mltplxngSckt->setOutgoingMessage(intrntMsag);
			}
			else if (intMes->find_first_of("/shutdown") != std::string::npos) {

				deleteClient(mltplxngSckt, intrntMsag);
			}
			else if (intMes->find_first_of("/close") != std::string::npos && intrntMsag.conProtocol == mltplxngSckt->Unix) {

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
			mltplxngSckt->setOutgoingMessage(intrntMsag);
		}

		threadOn = endThread();
	}
}

void MessageProcessing::addingClient(MultiplexingSocket* mltplxngSckt, MultiplexingSocket::internetMessage* intrntMsag) {

	mutexAddressClient.lock();
	incomingClients->push_back(*intrntMsag);
	incomingClients->back().multiplexingSocket = mltplxngSckt;
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
	std::deque<addressClient>* incmngClnts = new std::deque<addressClient>;

	while (threadOn) {

		blockingAddressClient();
		addingClientAddress(incmngClnts);
		deletingClientsTime(TIMEOUT_ADDRESS);
		threadOn = endThread();
	}

	delete incmngClnts;
}

void MessageProcessing::addingClientAddress(std::deque<addressClient>* incmngClnts) {

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

void MessageProcessing::searchClientAddress(std::deque<addressClient>* incmngClnts) {

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

void MessageProcessing::updatingVisitTime(std::deque<addressClient>* incmngClnts, addressClient* plAdd) {

	plAdd->dateVisit = incmngClnts->front().dateVisit;
}

void MessageProcessing::addingClient(std::map<long long, std::list<addressClient>>::iterator& plAddrssClnt, std::deque<addressClient>* incmngClnts) {

	plAddrssClnt->second.emplace_back(addressClient(incmngClnts->front()));
	poolTimerOff->emplace_back(dateVisit(incmngClnts->front().dateVisit, plAddrssClnt, --plAddrssClnt->second.end()));
}

void MessageProcessing::addingNewClientAddress(std::deque<addressClient>* incmngClnts, long long hashAddress) {

	poolAddressClient->emplace(hashAddress, std::list<addressClient>{incmngClnts->front()});

	if (auto plAddClnt = poolAddressClient->find(hashAddress); plAddClnt != poolAddressClient->end()) {

		poolTimerOff->emplace_back(dateVisit(incmngClnts->front().dateVisit, plAddClnt, plAddClnt->second.begin()));
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
	
	if (iterTO->protocol == MultiplexingSocket::TCP) {

		iterTO->multiplexingSocket->deleteSocket(iterTO->port, iterTO->socket);
	}

	poolTimerOff->front().iterMapAddress->second.erase(iterTO);
}

void MessageProcessing::newThread(MultiplexingSocket* newMultiplexingSocket) {

	mutexMultiplexingSocket.lock();
	multiplexingSocket->push_back(newMultiplexingSocket);
	mutexMultiplexingSocket.unlock();
	newMultiplexingSocket = nullptr;
	threadClass->push_back(std::jthread(&MessageProcessing::threadProcessing, this));
}

void MessageProcessing::startProcessing() {

	bool threadOn = true;

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