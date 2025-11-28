#include "MultiplexingSocket.h"

MultiplexingSocket::MultiplexingSocket() {

	incomingDataBuffer = new std::deque<internetMessage>;
	incomingMessage = new std::deque<internetMessage>;
	poolTimerOff = new std::deque<dateVisit>;
	outgoingDataBuffer = new std::map<int, std::deque<internetMessage>>;
	poolAddressClient = new std::map<long long, std::deque<addressClient>>;
	
	threadClass = new std::deque<std::jthread>;
	threadClass->push_back(std::jthread(&MultiplexingSocket::threadAddressClient, this));
}

MultiplexingSocket::~MultiplexingSocket() {
	
	mutexEnd.lock();
	threadOperation = false;
	mutexEnd.unlock();

	for (auto& thrdClss : *threadClass) {

		thrdClss.join();
	}

	delete threadClass;
	delete incomingMessage;
	delete outgoingDataBuffer;
	delete incomingDataBuffer;
	delete poolTimerOff;
	delete poolAddressClient;
	delete threadClass;
}

MultiplexingSocket::internetMessage::internetMessage() {

}

MultiplexingSocket::internetMessage::internetMessage(const internetMessage& other) { 

	port = other.port;
	protocol = other.protocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	dateMessage = other.dateMessage;
}

MultiplexingSocket::internetMessage::internetMessage(internetMessage&& other) {

	port = other.port;
	protocol = other.protocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	messageSize = other.messageSize;
	dateMessage = other.dateMessage;
	setMessage(other.getMessage());
	other.messageSize = 0;
}

MultiplexingSocket::internetMessage::~internetMessage() {

	if (message != nullptr) {

		delete message;
	}
}

MultiplexingSocket::internetMessage& MultiplexingSocket::internetMessage::operator=(internetMessage& other) {

	port = other.port;
	protocol = other.protocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	dateMessage = other.dateMessage;
	other.messageSize = 0; 

	return *this;
}

MultiplexingSocket::internetMessage& MultiplexingSocket::internetMessage::operator=(const internetMessage& other) {

	port = other.port;
	protocol = other.protocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	dateMessage = other.dateMessage;

	return *this;
}

MultiplexingSocket::internetMessage& MultiplexingSocket::internetMessage::operator=(internetMessage&& other) {

	port = other.port;
	protocol = other.protocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	messageSize = other.messageSize;
	dateMessage = other.dateMessage;
	setMessage(other.getMessage());
	other.messageSize = 0;

	return *this;
}

bool MultiplexingSocket::internetMessage::operator==(internetMessage& other) {

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

void MultiplexingSocket::internetMessage::setMessage(std::string* mssg) {

	if (message != nullptr) {

		delete message;
	}

	message = mssg;
	mssg = nullptr;
}

void MultiplexingSocket::internetMessage::setMessage(std::string mssg) {

	if (message == nullptr) {

		message = new std::string(std::move(mssg));
	}
	else {

		*message = std::move(mssg);
	}
}

std::string* MultiplexingSocket::internetMessage::getMessage() {

	std::string* mssg = nullptr;

	if (message != nullptr) {
	
		mssg = message;
		message = nullptr;
	}
	else {

		mssg = new std::string(0);
	}
	
	return mssg;
}

MultiplexingSocket::addressClient::addressClient(internetMessage& add, std::time_t dateVst) {

	address = internetMessage(add);
	dateVisit = dateVst;
}

bool MultiplexingSocket::setPort(int portNumber, ConnectionProtocol protocol, portProtocol portProt, int connections) {

	if (portProt == TCP) {

		if (!openPortTCP(portNumber, protocol, portProt, connections)) {
		
			return false;
		}
	}
	else if (portProt == UDP) {

		if (!openPortUDP(portNumber, protocol, portProt, connections)) {

			return false;
		}
	}
	else {
	
		return false;
	}
	
	return true;
}

bool MultiplexingSocket::openPortTCP(int portNumber, ConnectionProtocol protocol, portProtocol portProt, int connections) {

	int listener = 0;

	if (protocol == ip) {

		listener = socket(AF_INET, SOCK_STREAM, 0);
	}
	else if (protocol == Unix) {

		listener = socket(AF_UNIX, SOCK_STREAM, 0);
	}

	if (listener < 0) {

		return false;
	}

	if (!connectSocket(portNumber, protocol, portProt, connections, listener)) {

		return false;
	}

	return true;
}

bool MultiplexingSocket::openPortUDP(int portNumber, ConnectionProtocol protocol, portProtocol portProt, int connections) {

	int listener = 0;

	if (protocol == ip) {

		listener = socket(AF_INET, SOCK_DGRAM, 0);
	}
	else if (protocol == Unix) {

		listener = socket(AF_UNIX, SOCK_DGRAM, 0);
	}

	if (listener < 0) {

		return false;
	}

	if (!connectSocket(portNumber, protocol, portProt, connections, listener)) {

		return false;
	}

	return true;
}

bool MultiplexingSocket::connectSocket(int portNumber, ConnectionProtocol protocol, portProtocol portProt, int connections, int listener) {

	fcntl(listener, F_SETFL, O_NONBLOCK);
	newSocketAddress(portNumber, protocol);
	auto& sckt = --socketAddress.end();

	if (bind(listener, &*sckt, sizeof(*sckt)) < 0) {

		return false;
	}

	if (portProt == TCP) {

		listen(listener, connections);
	}

	internetMessage threadIM;
	threadIM.address = socketAddress.back();
	threadIM.addressSize = sizeof(socketAddress.back());
	threadIM.socket = listener;
	threadIM.port = portNumber;
	threadIM.protocol = portProt;
	threadIM.dateMessage = std::time(nullptr);
	threadInternetMessage.push_back(std::move(threadIM));

	if (portProt == TCP) {

		threadClass->emplace_back(std::jthread(&MultiplexingSocket::threadSocketTCP, this));
	}
	else if (portProt == UDP) {

		threadClass->emplace_back(std::jthread(&MultiplexingSocket::threadSocketUDP, this));
	}
	else {

		return false;
	}

	return true;
}

void MultiplexingSocket::deletePort(int portNumber) {

	for (auto const& thrdIM : threadInternetMessage) {

		if (thrdIM.port == portNumber) {

			std::unique_lock lock(mutexDeleteSocket);

			if (auto iterDlt = bufferDeletingPort.find(portNumber); iterDlt != bufferDeletingPort.end()) {

				iterDlt->second.push_back(thrdIM.socket);
			}
			else {

				bufferDeletingPort.emplace(portNumber, thrdIM.socket);
			}
		}
	}
}

int MultiplexingSocket::checkingIncomingMessages(bool blockingThread) {

	if (blockingThread) {

		blockingMessageProcessing();
	}

	std::unique_lock lock(mutexIncomingMessage);
	int memberMessanges = static_cast<int>(incomingMessage->size());

	return memberMessanges;
}

MultiplexingSocket::internetMessage MultiplexingSocket::getIncomingMessage() {

	internetMessage mssg;
	std::unique_lock lock (mutexIncomingMessage);

	if (!incomingMessage->empty()) {

		mssg = std::move(incomingMessage->front());
		incomingMessage->pop_front();
	}

	return std::move(mssg);
}

void MultiplexingSocket::setOutgoingMessage(MultiplexingSocket::internetMessage& outgoingMessage) {

	std::unique_lock lock(mutexOutgoingDataBuffer);

	if (auto iterMssg = outgoingDataBuffer->find(outgoingMessage.port); iterMssg != outgoingDataBuffer->end()) {

		iterMssg->second.push_back(std::move(outgoingMessage));
	}
	else {

		outgoingDataBuffer->emplace(outgoingMessage.port, std::deque<internetMessage>{std::move(outgoingMessage)});
	}
}

int MultiplexingSocket::numberClient() {

	int numClnt = 0;
	std::unique_lock lock(mutexAddressClient);
	
	for (const auto& plAddClnt : *poolAddressClient) {

		numClnt += static_cast<int>(plAddClnt.second.size());
	}

	return numClnt;
}

void MultiplexingSocket::deleteClient(internetMessage delClient) {

	long long hashAddress = std::hash<std::string_view>{}(delClient.address.sa_data);
	mutexAddressClient.lock();

	if (auto plAddrssClnt = poolAddressClient->find(hashAddress); plAddrssClnt != poolAddressClient->end()) {

		for (auto plAdd = plAddrssClnt->second.begin(); plAdd != plAddrssClnt->second.end(); ++plAdd) {

			if (plAdd->address == delClient) {

				plAddrssClnt->second.erase(plAdd);
			}
		}
	}

	mutexAddressClient.unlock();
}

void MultiplexingSocket::newSocketAddress(int portNumber, ConnectionProtocol protocol) {

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(static_cast<int>(portNumber));

	if (protocol == ip) {

		addr.sin_addr.s_addr = INADDR_ANY;
	} 
	else if (protocol == Unix) {

		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	socketAddress.push_back(*(sockaddr*)&addr);
}

void MultiplexingSocket::threadSocketTCP() {

	const int MAX_EVENT = maxEvents;
	const int MAX_SIZE_MESSAGE = 65535;
	const internetMessage INCOMING_MESSAGE(threadInternetMessage.front());
	epoll_event eventSockets{};
	epoll_event events[MAX_EVENT];
	bool socketOn = true;
	int pauseTime = 50;
	int numberSocket = 0;
	int socketDescriptor = 0;
	char* charBuf = new char[MAX_SIZE_MESSAGE];
	std::map<int, internetMessage>* poolThreadSocket = new std::map<int, internetMessage>;
	std::map<int, std::deque<internetMessage>>* messageBuffer = new std::map<int, std::deque<internetMessage>>;
	std::deque<internetMessage>* incomingDataThread = new std::deque<internetMessage>;
	std::deque<int> bufferDeletingSocket;

	socketDescriptor = epoll_create(10);

	if (socketDescriptor == -1) {

		socketOn = false;
	}

	eventSockets.events = EPOLLIN;
	eventSockets.data.fd = INCOMING_MESSAGE.socket;

	if (epoll_ctl(socketDescriptor, EPOLL_CTL_ADD, INCOMING_MESSAGE.socket, &eventSockets) == -1) {

		socketOn = false;
	}

	while (socketOn) {

		mutexDeleteSocket.lock();

		if (auto iterBDP = bufferDeletingPort.find(INCOMING_MESSAGE.port); iterBDP == bufferDeletingPort.end()) {

			while (!iterBDP->second.empty()) {

				if (iterBDP->second.front() == INCOMING_MESSAGE.socket) {

					socketOn = false;
				}
				else {

					bufferDeletingSocket.push_back(iterBDP->second.front());
					iterBDP->second.pop_front();
				}
			}
		}

		mutexDeleteSocket.unlock();

		while (!bufferDeletingSocket.empty()) {

			close(bufferDeletingSocket.front());
			epoll_ctl(socketDescriptor, EPOLL_CTL_DEL, bufferDeletingSocket.front(), NULL);
			poolThreadSocket->erase(poolThreadSocket->find(bufferDeletingSocket.front()));
			bufferDeletingSocket.pop_front();
		}

		mutexOutgoingDataBuffer.lock();

		if (auto iterMssg = outgoingDataBuffer->find(INCOMING_MESSAGE.port); iterMssg != outgoingDataBuffer->end()) {

			while (!iterMssg->second.empty()) {

				if (auto iterBffr = messageBuffer->find(iterMssg->second.front().socket); iterBffr != messageBuffer->end()) {

					iterBffr->second.push_back(std::move(iterMssg->second.front()));
				}
				else {

					messageBuffer->emplace(iterMssg->second.front().socket, std::deque<internetMessage>{std::move(iterMssg->second.front())});
				}

				iterMssg->second.pop_front();
			}
		}

		mutexOutgoingDataBuffer.unlock();
		numberSocket = epoll_wait(socketDescriptor, events, MAX_EVENT, pauseTime);
		
		if (numberSocket == -1) {
			
			break;
		}

		for (int i = 0; i < numberSocket; i++) {

			if (events[i].data.fd == INCOMING_MESSAGE.socket) {

				internetMessage newSM = INCOMING_MESSAGE;
				newSM.socket = accept(newSM.socket, &newSM.address, &newSM.addressSize);

				if (newSM.socket == -1) {
					
					continue;
				}

				fcntl(newSM.socket, F_SETFL, O_NONBLOCK);
				eventSockets.events = EPOLLIN | EPOLLET;
				eventSockets.data.fd = newSM.socket;

				if (epoll_ctl(socketDescriptor, EPOLL_CTL_ADD, newSM.socket,&eventSockets) == -1) {

					continue;
				}

				poolThreadSocket->emplace(newSM.socket, std::move(newSM));
			}
			else {

				if (auto iterThrSoc = poolThreadSocket->find(events[i].data.fd); iterThrSoc != poolThreadSocket->end()) {

					while (true) {

						int sizeMssg = recv(iterThrSoc->first, charBuf, sizeof(charBuf), 0);

						if (sizeMssg > 0) {

							std::string* mssg = iterThrSoc->second.getMessage();
							mssg->insert(iterThrSoc->second.messageSize, charBuf, sizeMssg);
							iterThrSoc->second.setMessage(mssg);
							iterThrSoc->second.messageSize += sizeMssg;
						}
						else if (sizeMssg == 0) {

							if (iterThrSoc->second.messageSize > 0) {

								iterThrSoc->second.dateMessage = std::time(nullptr);
								incomingDataThread->push_back(std::move(iterThrSoc->second));
							}

							break;
						}
						else {

							break;
						}
					}

					if (auto iterMssgBffr = messageBuffer->find(events[i].data.fd); iterMssgBffr != messageBuffer->end()) {

						while (!iterMssgBffr->second.empty()) {
					
							int sizeMssg = 0;
							int sizeItrMssg = 0;

							std::string* mssg = iterMssgBffr->second.front().getMessage();
							sizeItrMssg = mssg->size();
							const char* mssgOut = mssg->substr(iterMssgBffr->second.front().messageSize, MAX_SIZE_MESSAGE).data();
							iterMssgBffr->second.front().setMessage(mssg);
							
							if (mssgOut == nullptr) {

								iterMssgBffr->second.pop_front();
								break;
							}

							if (iterMssgBffr->second.size() > 1) {
							
								sizeMssg = send(iterMssgBffr->second.front().socket, mssgOut, sizeof(*mssgOut), MSG_MORE);
							}
							else {

								sizeMssg = send(iterMssgBffr->second.front().socket, mssgOut, sizeof(*mssgOut), 0);
							}
		
							delete mssgOut;

							if (sizeMssg > 0) {

								iterMssgBffr->second.front().messageSize += sizeMssg;
							}
							else if (sizeMssg < 0) {

								break;
							}

							if (iterMssgBffr->second.front().messageSize >= sizeItrMssg) {

								iterMssgBffr->second.pop_front();
							}
						}
					}
				}
			}
		}

		if (!incomingDataThread->empty()) {

			mutexIncomingDataBuffer.lock();

			while (!incomingDataThread->empty()) {

				incomingDataBuffer->push_back(std::move(incomingDataThread->front()));
				incomingDataThread->pop_front();
			}

			mutexIncomingDataBuffer.unlock();
			messagesThread.notify_all();
			mutexEnd.lock();
			socketOn = threadOperation;
			mutexEnd.unlock();
		}
	}

	for (const auto& [key, value] : *poolThreadSocket) {

		close(key);
	}

	close(INCOMING_MESSAGE.socket);

	delete charBuf;
	delete poolThreadSocket;
	delete messageBuffer;
	delete incomingDataThread;
}
	
void MultiplexingSocket::threadSocketUDP() {

	const int MAX_EVENT = maxEvents;
	const int MAX_SIZE_MESSAGE = 508;
	const internetMessage INCOMING_MESSAGE(threadInternetMessage.front());
	epoll_event eventSockets{};
	epoll_event events[MAX_EVENT];
	bool socketOn = true;
	int pauseTime = 20;
	int numberSocket = 0;
	int socketDescriptor = 0;
	internetMessage newMssg = INCOMING_MESSAGE;
	char* charBuf = new char[MAX_SIZE_MESSAGE];
	std::deque<internetMessage>* incomingDataThread = new std::deque<internetMessage>;
	std::deque<internetMessage>* messageBuffer = new std::deque<internetMessage>;

	socketDescriptor = epoll_create(10);

	if (socketDescriptor == -1) {

		socketOn = false;
	}

	eventSockets.events = EPOLLIN | EPOLLET;
	eventSockets.data.fd = INCOMING_MESSAGE.socket;

	if (epoll_ctl(socketDescriptor, EPOLL_CTL_ADD, INCOMING_MESSAGE.socket, &eventSockets) == -1) {

		socketOn = false;
	}

	while (socketOn) {

		mutexDeleteSocket.lock();

		if (auto iterBDP = bufferDeletingPort.find(INCOMING_MESSAGE.port); iterBDP == bufferDeletingPort.end()) {

			while (!iterBDP->second.empty()) {

				if (iterBDP->second.front() == INCOMING_MESSAGE.socket) {

					socketOn = false;
				}
				else {

					iterBDP->second.pop_front();
				}
			}
		}

		mutexDeleteSocket.unlock();
		mutexOutgoingDataBuffer.lock();

		if (auto iterMssg = outgoingDataBuffer->find(INCOMING_MESSAGE.port); iterMssg != outgoingDataBuffer->end()) {

			while (!iterMssg->second.empty()) {

				messageBuffer->push_back(std::move(iterMssg->second.front()));
				iterMssg->second.pop_front();
			}
		}

		mutexOutgoingDataBuffer.unlock();
		numberSocket = epoll_wait(socketDescriptor, events, MAX_EVENT, pauseTime);

		if (numberSocket == -1) {

			break;
		}

		for (int i = 0; i < numberSocket; i++) {

			if (events[i].data.fd == INCOMING_MESSAGE.socket) {

				while (true) {

					sockaddr addrss = INCOMING_MESSAGE.address;
					socklen_t addrssSz = INCOMING_MESSAGE.addressSize;
					std::string* mssg = newMssg.getMessage();
					int sizeItrMssg = mssg->size();
					int sizeMssg = recvfrom(newMssg.socket, charBuf, sizeof(charBuf), 0, &addrss, &addrssSz);
					newMssg.setMessage(mssg);

					if (sizeMssg > 0) {

						if (newMssg.address.sa_data != addrss.sa_data) {

							if (sizeItrMssg > 0) {

								newMssg.dateMessage = std::time(nullptr);
								incomingDataThread->push_back(std::move(newMssg));
							}
						}

						newMssg.address = addrss;
						newMssg.addressSize = addrssSz;
						mssg = newMssg.getMessage();
						mssg->insert(newMssg.messageSize, charBuf, sizeMssg);
						newMssg.setMessage(mssg);
						newMssg.messageSize += sizeMssg;
					}
					else if (sizeMssg == 0) {

						if (sizeItrMssg > 0) {

							newMssg.dateMessage = std::time(nullptr);
							incomingDataThread->push_back(std::move(newMssg));
						}

						break;
					}
					else {

						break;
					}
				}

				while (!messageBuffer->empty()) {

					std::string* mssg = messageBuffer->front().getMessage();
					int sizeItrMssg = mssg->size();
					int sizeMssg = 0;
					const char* mssgOut = mssg->substr(messageBuffer->front().messageSize, MAX_SIZE_MESSAGE).data();
					messageBuffer->front().setMessage(mssg);
					
					if (mssgOut == nullptr) {

						messageBuffer->pop_front();
						break;
					}

					sizeMssg = sendto(messageBuffer->front().socket, mssgOut, sizeof(*mssgOut), 0, &messageBuffer->front().address, messageBuffer->front().addressSize);
					delete mssgOut;

					if (sizeMssg > 0) {

						messageBuffer->front().messageSize += sizeMssg;
					}
					else if (sizeMssg < 0) {

						break;
					}

					if (messageBuffer->front().messageSize >= sizeItrMssg) {

						messageBuffer->pop_front();
					}
				}
			}
		}

		if (!incomingDataThread->empty()) {

			mutexIncomingDataBuffer.lock();

			while (!incomingDataThread->empty()) {

				incomingDataBuffer->push_back(std::move(incomingDataThread->front()));
				incomingDataThread->pop_front();
			}

			mutexIncomingDataBuffer.unlock();
			messagesThread.notify_all();
		}

		mutexEnd.lock();
		socketOn = threadOperation;
		mutexEnd.unlock();
	}

	close(INCOMING_MESSAGE.socket);

	delete charBuf;
	delete messageBuffer; 
	delete incomingDataThread;
}

void MultiplexingSocket::blockingAddressClient() {

	const long long TIMEOUT = 100;
	std::unique_lock lock(mutexIncomingDataBuffer);
	messagesThread.wait_for(lock, std::chrono::duration<long long, std::milli>(TIMEOUT));
}

void MultiplexingSocket::threadAddressClient() {

	bool threadOn = true;
	double TIMEOUT_ADDRESS = 300;
	std::deque<internetMessage>* incomingDataThread = new std::deque<internetMessage>;

	while (threadOn) {

		blockingAddressClient();
		mutexIncomingDataBuffer.lock();
		
		if (!incomingDataBuffer->empty()) {
			
			while (!incomingDataBuffer->empty()) {

				incomingDataThread->push_back(std::move(incomingDataBuffer->front()));
				incomingDataBuffer->pop_front();
			}
			
			mutexIncomingDataBuffer.unlock();
			mutexIncomingMessage.lock();
			
			for (auto& iterIncomingData : *incomingDataThread) {

				incomingMessage->push_back(std::move(iterIncomingData));
			}

			mutexIncomingMessage.unlock();
			messagesOut.notify_all();

			while (!incomingDataThread->empty()) {

				bool addressSearch = true;
				long long hashAddress = std::hash<std::string_view>{}(incomingDataThread->front().address.sa_data);
				mutexAddressClient.lock();

				if (auto plAddrssClnt = poolAddressClient->find(hashAddress); plAddrssClnt != poolAddressClient->end()) {

					for (auto& plAdd : plAddrssClnt->second) {

						if (plAdd.address == incomingDataThread->front()) {

							addressSearch = false;
							plAdd.dateVisit = incomingDataThread->front().dateMessage;
						}
					}

					if (addressSearch) {

						addressSearch = false;
						plAddrssClnt->second.emplace_back(addressClient(incomingDataThread->front(), incomingDataThread->front().dateMessage));
						poolTimerOff->emplace_back(dateVisit());
						poolTimerOff->back().dateVst = incomingDataThread->front().dateMessage;
						poolTimerOff->back().iterMapAddress = plAddrssClnt;
						poolTimerOff->back().iterAddress = --plAddrssClnt->second.end();
					}
				}

				if (addressSearch) {

					poolAddressClient->emplace(hashAddress, std::deque<addressClient>{addressClient(incomingDataThread->front(), incomingDataThread->front().dateMessage)});

					if (auto plAddClnt = poolAddressClient->find(hashAddress); plAddClnt != poolAddressClient->end()) {

						poolTimerOff->emplace_back(dateVisit());
						poolTimerOff->back().dateVst = incomingDataThread->front().dateMessage;
						poolTimerOff->back().iterMapAddress = plAddClnt;
						poolTimerOff->back().iterAddress = --plAddClnt->second.end();
					}
				}

				mutexAddressClient.unlock();
				incomingDataThread->pop_front();
			}
		}
		else {

			mutexIncomingDataBuffer.unlock();
			break;
		}
	}

	while (!poolTimerOff->empty()) 	{

		if (difftime(std::time(nullptr), poolTimerOff->front().dateVst) > TIMEOUT_ADDRESS) {

			mutexAddressClient.lock();

			for (auto iterTO = poolTimerOff->front().iterMapAddress->second.begin(); iterTO != poolTimerOff->front().iterMapAddress->second.end(); ++iterTO) {
			
				if (poolTimerOff->front().iterAddress == iterTO) {
			
					if (difftime(std::time(nullptr), iterTO->dateVisit) > TIMEOUT_ADDRESS) {

						if (iterTO->address.protocol == TCP) {

							mutexDeleteSocket.lock();

							if (auto iterDlt = bufferDeletingPort.find(iterTO->address.port); iterDlt != bufferDeletingPort.end()) {

								iterDlt->second.push_back(iterTO->address.socket);
							}
							else {

								bufferDeletingPort.emplace(iterTO->address.port, iterTO->address.socket);
							}

							mutexDeleteSocket.unlock();
						}

						poolTimerOff->front().iterMapAddress->second.erase(iterTO);
						break;
					}
				}
			}

			mutexAddressClient.unlock();
			poolTimerOff->pop_front();
		}
		else {

			break;
		}

		mutexEnd.lock();
		threadOn = threadOperation;
		mutexEnd.unlock();
	}

	delete incomingDataThread;
}

void MultiplexingSocket::blockingMessageProcessing() {

	std::unique_lock lock(mutexIncomingMessage);
	messagesOut.wait(lock, [&]() {return !incomingMessage->empty(); });
}