#include "MultiplexingSocketUDP.h"

bool MultiplexingSocketUDP::setPort(int portNumber, connectionProtocol protocol, int connections) {

	if (!openPort(portNumber, protocol, connections)) {

		return false;
	}

	return true;
}

bool MultiplexingSocketUDP::openPort(int portNumber, connectionProtocol protocol, int connections) {

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

	if (!connectSocket(portNumber, protocol, connections, listener)) {

		return false;
	}

	return true;
}

bool MultiplexingSocketUDP::connectSocket(int portNumber, connectionProtocol protocol, int connections, int listener) {

	fcntl(listener, F_SETFL, O_NONBLOCK);
	newSocketAddress(portNumber, protocol);
	auto& sckt = --socketAddress.end();

	if (bind(listener, &*sckt, sizeof(*sckt)) < 0) {

		return false;
	}

	internetMessage threadIM;
	threadIM.address = socketAddress.back();
	threadIM.addressSize = sizeof(socketAddress.back());
	threadIM.socket = listener;
	threadIM.port = portNumber;
	threadIM.protocol = MultiplexingSocketUDP::UDP;
	threadIM.conProtocol = protocol;
	threadIM.dateMessage = std::time(nullptr);
	threadInternetMessage.push_back(std::move(threadIM));
	threadClass->emplace_back(std::jthread(&MultiplexingSocketUDP::threadSocket, this));

	return true;
}

void MultiplexingSocketUDP::newSocketAddress(int portNumber, connectionProtocol protocol) {

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

void MultiplexingSocketUDP::threadSocket() {

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

		deletePort(&INCOMING_MESSAGE, &socketOn);
		copyingOutgoingMessagesPort(messageBuffer, &INCOMING_MESSAGE);
		numberSocket = epoll_wait(socketDescriptor, events, MAX_EVENT, pauseTime);

		if (numberSocket == -1) {

			break;
		}

		for (int i = 0; i < numberSocket; i++) {

			if (events[i].data.fd == INCOMING_MESSAGE.socket) {

				receivingMessagePort(&newMssg, &INCOMING_MESSAGE, incomingDataThread, charBuf);
				sendingMessagePort(messageBuffer, MAX_SIZE_MESSAGE);
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

void MultiplexingSocketUDP::deletePort(const internetMessage* incomingMessage, bool* scktOn) {

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

void MultiplexingSocketUDP::copyingOutgoingMessagesPort(std::deque<internetMessage>* mssgBffr, const internetMessage* incomingMessage) {

	mutexOutgoingDataBuffer.lock();

	if (auto iterMssg = outgoingDataBuffer->find(incomingMessage->port); iterMssg != outgoingDataBuffer->end()) {

		while (!iterMssg->second.empty()) {

			mssgBffr->push_back(std::move(iterMssg->second.front()));
			iterMssg->second.pop_front();
		}
	}

	mutexOutgoingDataBuffer.unlock();
}

void MultiplexingSocketUDP::receivingMessagePort(internetMessage* nwMssg, const internetMessage* incomingMessage, std::deque<internetMessage>* incmngDtThrd, char* chrBf) {

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

					saveReceivingMessagePort(nwMssg, incmngDtThrd);
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

				saveReceivingMessagePort(nwMssg, incmngDtThrd);
			}

			break;
		}
		else {

			break;
		}
	}
}

void MultiplexingSocketUDP::saveReceivingMessagePort(internetMessage* nwMssg, std::deque<internetMessage>* incmngDtThrd) {

	nwMssg->dateMessage = std::time(nullptr);
	incmngDtThrd->push_back(std::move(*nwMssg));
}

void MultiplexingSocketUDP::sendingMessagePort(std::deque<internetMessage>* mssgBffr, int MaxSizeMessage) {

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

void MultiplexingSocketUDP::copyingIncomingMessages(std::deque<internetMessage>* incmngDtThrd) {

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