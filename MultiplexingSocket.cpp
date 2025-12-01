#include "MultiplexingSocket.h"

MultiplexingSocket::MultiplexingSocket() {

	incomingDataBuffer = new std::deque<internetMessage>;
	outgoingDataBuffer = new std::map<int, std::deque<internetMessage>>;
	threadClass = new std::deque<std::jthread>;
}

MultiplexingSocket::~MultiplexingSocket() {
	
	mutexEnd.lock();
	threadOperation = false;
	mutexEnd.unlock();

	for (auto& thrdClss : *threadClass) {

		thrdClss.join();
	}
	 
	delete outgoingDataBuffer;
	delete incomingDataBuffer;
	delete threadClass;
}

MultiplexingSocket::internetMessage::internetMessage() {}

MultiplexingSocket::internetMessage::internetMessage(const internetMessage& other) { 

	port = other.port;
	protocol = other.protocol;
	conProtocol = other.conProtocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	dateMessage = other.dateMessage;
}

MultiplexingSocket::internetMessage::internetMessage(internetMessage&& other) {

	port = other.port;
	protocol = other.protocol;
	conProtocol = other.conProtocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
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
	conProtocol = other.conProtocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	dateMessage = other.dateMessage;

	return *this;
}

MultiplexingSocket::internetMessage& MultiplexingSocket::internetMessage::operator=(const internetMessage& other) {

	port = other.port;
	protocol = other.protocol;
	conProtocol = other.conProtocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	dateMessage = other.dateMessage;

	return *this;
}

MultiplexingSocket::internetMessage& MultiplexingSocket::internetMessage::operator=(internetMessage&& other) {

	port = other.port;
	protocol = other.protocol;
	conProtocol = other.conProtocol;
	socket = other.socket;
	address = other.address;
	addressSize = other.addressSize;
	dateMessage = other.dateMessage;
	setMessage(other.getMessage());
	other.messageSize = 0;

	return *this;
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

		mssg = new std::string;
	}
	
	return mssg;
}

bool MultiplexingSocket::internetMessage::operator!=(sockaddr& other) {
	
	if (address.sa_family == other.sa_family) {

		for (int i = 2; i < 8; i++) {

			if (address.sa_data[i] != other.sa_data[i]) {

				return true;
			}
		}
	}
	else {

		return true;
	}

	return false;
}

bool MultiplexingSocket::setPort(int portNumber, connectionProtocol protocol, portProtocol portProt, int connections) {

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

bool MultiplexingSocket::openPortTCP(int portNumber, connectionProtocol protocol, portProtocol portProt, int connections) {

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

bool MultiplexingSocket::openPortUDP(int portNumber, connectionProtocol protocol, portProtocol portProt, int connections) {

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

bool MultiplexingSocket::connectSocket(int portNumber, connectionProtocol protocol, portProtocol portProt, int connections, int listener) {

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
	threadIM.conProtocol = protocol;
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

	for (const auto& thrdIM : threadInternetMessage) {

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

void MultiplexingSocket::deleteSocketTCP(int portNumber, int socketNumber) {

	std::unique_lock lock(mutexDeleteSocket);

	if (auto iterDlt = bufferDeletingPort.find(portNumber); iterDlt != bufferDeletingPort.end()) {

		iterDlt->second.push_back(socketNumber);
	}
	else {

		bufferDeletingPort.emplace(portNumber, socketNumber);
	}
}


int MultiplexingSocket::checkingIncomingMessages(bool blockingThread) {

	if (blockingThread) {

		blockingMessageProcessing();
	}

	std::unique_lock lock(mutexIncomingDataBuffer);
	int memberMessanges = static_cast<int>(incomingDataBuffer->size());

	return memberMessanges;
}

MultiplexingSocket::internetMessage MultiplexingSocket::getIncomingMessage() {

	internetMessage mssg;
	std::unique_lock lock (mutexIncomingDataBuffer);

	if (!incomingDataBuffer->empty()) {

		mssg = std::move(incomingDataBuffer->front());
		incomingDataBuffer->pop_front();
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

void MultiplexingSocket::newSocketAddress(int portNumber, connectionProtocol protocol) {

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(static_cast<int>(portNumber));

	if (protocol == ip) {

		addr.sin_addr.s_addr = INADDR_ANY;
	} 
	else if (protocol == Unix) {

		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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
	int socketDescriptor = epoll_create(10);
	char* charBuf = new char[MAX_SIZE_MESSAGE];
	std::map<int, internetMessage>* poolThreadSocket = new std::map<int, internetMessage>;
	std::map<int, std::deque<internetMessage>>* messageBuffer = new std::map<int, std::deque<internetMessage>>;
	std::deque<internetMessage>* incomingDataThread = new std::deque<internetMessage>;

	if (socketDescriptor == -1) {

		socketOn = false;
	}

	eventSockets.events = EPOLLIN;
	eventSockets.data.fd = INCOMING_MESSAGE.socket;

	if (epoll_ctl(socketDescriptor, EPOLL_CTL_ADD, INCOMING_MESSAGE.socket, &eventSockets) == -1) {

		socketOn = false;
	}

	while (socketOn) {

		deleteSocketPortTCP(poolThreadSocket, &INCOMING_MESSAGE, &socketOn, socketDescriptor);
		copyingOutgoingMessagesPortTCP(messageBuffer, &INCOMING_MESSAGE);
		numberSocket = epoll_wait(socketDescriptor, events, MAX_EVENT, pauseTime);
		
		if (numberSocket == -1) {
			
			break;
		}

		for (int i = 0; i < numberSocket; i++) {

			if (events[i].data.fd == INCOMING_MESSAGE.socket) {

				newSocketPoetTCP(poolThreadSocket, &eventSockets, &INCOMING_MESSAGE, socketDescriptor);
			}
			else {

				receivingMessagePortTCP(poolThreadSocket, incomingDataThread, &events[i], charBuf);
				sendingMessagePortTCP(messageBuffer, &events[i], MAX_SIZE_MESSAGE);
			}
		}

		copyingIncomingMessages(incomingDataThread);
		socketOn = endThread();
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

void MultiplexingSocket::deleteSocketPortTCP(std::map<int, internetMessage>* plThrdSckt, const internetMessage* incomingMessage, bool* scktOn, int scktDscrptr) {

	mutexDeleteSocket.lock();
	std::deque<int> bufferDeletingSocket;

	if (auto iterBDP = bufferDeletingPort.find(incomingMessage->port); iterBDP == bufferDeletingPort.end()) {

		while (!iterBDP->second.empty()) {

			if (iterBDP->second.front() == incomingMessage->socket) {

				*scktOn = false;
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
		epoll_ctl(scktDscrptr, EPOLL_CTL_DEL, bufferDeletingSocket.front(), NULL);
		plThrdSckt->erase(plThrdSckt->find(bufferDeletingSocket.front()));
		bufferDeletingSocket.pop_front();
	}
}

void MultiplexingSocket::copyingOutgoingMessagesPortTCP(std::map<int, std::deque<internetMessage>>* mssgBffr, const internetMessage* incomingMessage) {

	mutexOutgoingDataBuffer.lock();

	if (auto iterMssg = outgoingDataBuffer->find(incomingMessage->port); iterMssg != outgoingDataBuffer->end()) {

		while (!iterMssg->second.empty()) {

			if (auto iterBffr = mssgBffr->find(iterMssg->second.front().socket); iterBffr != mssgBffr->end()) {

				iterBffr->second.push_back(std::move(iterMssg->second.front()));
			}
			else {

				mssgBffr->emplace(iterMssg->second.front().socket, std::deque<internetMessage>{std::move(iterMssg->second.front())});
			}

			iterMssg->second.pop_front();
		}
	}

	mutexOutgoingDataBuffer.unlock();
}

bool MultiplexingSocket::newSocketPoetTCP(std::map<int, internetMessage>* plThrdSckt, epoll_event* evntSckts, const internetMessage* incomingMessage, int scktDscrptr) {

	internetMessage newSM(*incomingMessage);
	newSM.socket = accept(newSM.socket, &newSM.address, &newSM.addressSize);

	if (newSM.socket == -1) {

		return false;
	}

	fcntl(newSM.socket, F_SETFL, O_NONBLOCK);
	evntSckts->events = EPOLLIN | EPOLLET;
	evntSckts->data.fd = newSM.socket;

	if (epoll_ctl(scktDscrptr, EPOLL_CTL_ADD, newSM.socket, evntSckts) == -1) {

		return false;
	}

	plThrdSckt->emplace(newSM.socket, newSM);

	return true;
}

void MultiplexingSocket::receivingMessagePortTCP(std::map<int, internetMessage>* plThrdSckt, std::deque<internetMessage>* incmngDtThrd, epoll_event* evnts, char* chrBf) {

	if (auto iterThrSoc = plThrdSckt->find(evnts->data.fd); iterThrSoc != plThrdSckt->end()) {

		while (true) {

			int sizeMssg = recv(iterThrSoc->first, chrBf, sizeof(*chrBf), 0);

			if (sizeMssg > 0) {

				std::string* mssg = iterThrSoc->second.getMessage();
				mssg->insert(iterThrSoc->second.messageSize, chrBf, sizeMssg);
				iterThrSoc->second.setMessage(mssg);
				iterThrSoc->second.messageSize += sizeMssg;
			}
			else if (sizeMssg == 0) {

				if (iterThrSoc->second.messageSize > 0) {

					saveReceivingMessagePortTCP(iterThrSoc, incmngDtThrd);
				}

				break;
			}
			else {

				break;
			}
		}
	}
}

void MultiplexingSocket::saveReceivingMessagePortTCP(std::map<int, internetMessage>::iterator iterThrSoc, std::deque<internetMessage>* incmngDtThrd) {

	iterThrSoc->second.dateMessage = std::time(nullptr);
	incmngDtThrd->push_back(std::move(iterThrSoc->second));
}

void MultiplexingSocket::sendingMessagePortTCP(std::map<int, std::deque<internetMessage>>* mssgBffr, epoll_event* evnts, int MaxSizeMessage) {

	if (auto iterMssgBffr = mssgBffr->find(evnts->data.fd); iterMssgBffr != mssgBffr->end()) {

		while (!iterMssgBffr->second.empty()) {

			int sizeMssg = 0;
			std::string* mssg = iterMssgBffr->second.front().getMessage();
			int sizeItrMssg = mssg->size();
			const char* mssgOut = mssg->substr(iterMssgBffr->second.front().messageSize, MaxSizeMessage).data();
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

void MultiplexingSocket::copyingIncomingMessages(std::deque<internetMessage>* incmngDtThrd) {

	if (!incmngDtThrd->empty()) {

		mutexIncomingDataBuffer.lock();

		while (!incmngDtThrd->empty()) {

			incomingDataBuffer->push_back(std::move(incmngDtThrd->front()));
			incmngDtThrd->pop_front();
		}

		mutexIncomingDataBuffer.unlock();
		messagesThread.notify_all();
	}
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
	internetMessage newMssg;
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

		deletePortUDP(&INCOMING_MESSAGE, &socketOn);
		copyingOutgoingMessagesPortUDP(messageBuffer, &INCOMING_MESSAGE);
		numberSocket = epoll_wait(socketDescriptor, events, MAX_EVENT, pauseTime);

		if (numberSocket == -1) {

			break;
		}

		for (int i = 0; i < numberSocket; i++) {

			if (events[i].data.fd == INCOMING_MESSAGE.socket) {

				receivingMessagePortUDP(&newMssg, &INCOMING_MESSAGE, incomingDataThread, charBuf);
				sendingMessagePortUDP(messageBuffer, MAX_SIZE_MESSAGE);
			}
		}

		copyingIncomingMessages(incomingDataThread);
		socketOn = endThread();
	}

	close(INCOMING_MESSAGE.socket);

	delete charBuf;
	delete messageBuffer; 
	delete incomingDataThread;
}

void MultiplexingSocket::deletePortUDP(const internetMessage* incomingMessage, bool* scktOn) {

	mutexDeleteSocket.lock();

	if (auto iterBDP = bufferDeletingPort.find(incomingMessage->port); iterBDP == bufferDeletingPort.end()) {

		while (!iterBDP->second.empty()) {

			if (iterBDP->second.front() == incomingMessage->socket) {

				*scktOn = false;
			}
			else {

				iterBDP->second.pop_front();
			}
		}
	}

	mutexDeleteSocket.unlock();
}

void MultiplexingSocket::copyingOutgoingMessagesPortUDP(std::deque<internetMessage>* mssgBffr, const internetMessage* incomingMessage) {

	mutexOutgoingDataBuffer.lock();

	if (auto iterMssg = outgoingDataBuffer->find(incomingMessage->port); iterMssg != outgoingDataBuffer->end()) {

		while (!iterMssg->second.empty()) {

			mssgBffr->push_back(std::move(iterMssg->second.front()));
			iterMssg->second.pop_front();
		}
	}

	mutexOutgoingDataBuffer.unlock();
}

void MultiplexingSocket::receivingMessagePortUDP(internetMessage* nwMssg, const internetMessage* incomingMessage, std::deque<internetMessage>* incmngDtThrd, char* chrBf) {

	while (true) {

		sockaddr addrss = incomingMessage->address;
		socklen_t addrssSz = incomingMessage->addressSize;
		std::string* mssg = nwMssg->getMessage();
		int sizeItrMssg = mssg->size();
		int sizeMssg = recvfrom(incomingMessage->socket, chrBf, sizeof(*chrBf), 0, &addrss, &addrssSz);
		nwMssg->setMessage(mssg);

		if (sizeMssg > 0) {

			if (*nwMssg != addrss) {

				if (sizeItrMssg > 0) {

					saveReceivingMessagePortUDP(nwMssg, incmngDtThrd);
				}
			}

			nwMssg->address = addrss;
			nwMssg->addressSize = addrssSz;
			mssg = nwMssg->getMessage();
			mssg->insert(nwMssg->messageSize, chrBf, sizeMssg);
			nwMssg->setMessage(mssg);
			nwMssg->messageSize += sizeMssg;
		}
		else if (sizeMssg == 0) {

			if (sizeItrMssg > 0) {

				saveReceivingMessagePortUDP(nwMssg, incmngDtThrd);
			}

			break;
		}
		else {

			break;
		}
	}
}

void MultiplexingSocket::saveReceivingMessagePortUDP(internetMessage* nwMssg, std::deque<internetMessage>* incmngDtThrd) {

	nwMssg->dateMessage = std::time(nullptr);
	incmngDtThrd->push_back(std::move(*nwMssg));
}

void MultiplexingSocket::sendingMessagePortUDP(std::deque<internetMessage>* mssgBffr, int MaxSizeMessage) {

	while (!mssgBffr->empty()) {

		std::string* mssg = mssgBffr->front().getMessage();
		int sizeItrMssg = mssg->size();
		int sizeMssg = 0;
		const char* mssgOut = mssg->substr(mssgBffr->front().messageSize, MaxSizeMessage).data();
		mssgBffr->front().setMessage(mssg);

		if (mssgOut == nullptr) {

			mssgBffr->pop_front();
			break;
		}

		sizeMssg = sendto(mssgBffr->front().socket, mssgOut, sizeof(*mssgOut), 0, &mssgBffr->front().address, mssgBffr->front().addressSize);
		delete mssgOut;

		if (sizeMssg > 0) {

			mssgBffr->front().messageSize += sizeMssg;
		}
		else if (sizeMssg < 0) {

			break;
		}

		if (mssgBffr->front().messageSize >= sizeItrMssg) {

			mssgBffr->pop_front();
		}
	}
}

void MultiplexingSocket::blockingMessageProcessing() {

	std::unique_lock lock(mutexIncomingDataBuffer);

	if (incomingDataBuffer->empty()) {

		messagesThread.wait(lock, [&]() {return !incomingDataBuffer->empty(); });
	}
}

bool MultiplexingSocket::endThread() {
	
	bool endThrd = true;
	mutexEnd.lock();
	endThrd = threadOperation;
	mutexEnd.unlock();

	return endThrd;
}