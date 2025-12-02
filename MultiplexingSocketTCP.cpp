#include "MultiplexingSocketTCP.h"

bool MultiplexingSocketTCP::setPort(int portNumber, connectionProtocol protocol, int connections) {

	if (!openPort(portNumber, protocol, connections)) {

		return false;
	}

	return true;
}

bool MultiplexingSocketTCP::openPort(int portNumber, connectionProtocol protocol, int connections) {

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

	if (!connectSocket(portNumber, protocol, connections, listener)) {

		return false;
	}

	return true;
}

bool MultiplexingSocketTCP::connectSocket(int portNumber, connectionProtocol protocol, int connections, int listener) {

	fcntl(listener, F_SETFL, O_NONBLOCK);
	newSocketAddress(portNumber, protocol);
	auto& sckt = --socketAddress.end();

	if (bind(listener, &*sckt, sizeof(*sckt)) < 0) {

		return false;
	}

	listen(listener, connections);
	internetMessage threadIM;
	threadIM.address = socketAddress.back();
	threadIM.addressSize = sizeof(socketAddress.back());
	threadIM.socket = listener;
	threadIM.port = portNumber;
	threadIM.protocol = MultiplexingSocketTCP::TCP;
	threadIM.conProtocol = protocol;
	threadIM.dateMessage = std::time(nullptr);
	threadInternetMessage.push_back(std::move(threadIM));
	threadClass->emplace_back(std::jthread(&MultiplexingSocketTCP::threadSocket, this));

	return true;
}

void MultiplexingSocketTCP::newSocketAddress(int portNumber, connectionProtocol protocol) {

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

void MultiplexingSocketTCP::threadSocket() {

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

		deleteSocketPort(poolThreadSocket, &INCOMING_MESSAGE, &socketOn, socketDescriptor);
		copyingOutgoingMessagesPort(messageBuffer, &INCOMING_MESSAGE);
		numberSocket = epoll_wait(socketDescriptor, events, MAX_EVENT, pauseTime);

		if (numberSocket == -1) {

			break;
		}

		for (int i = 0; i < numberSocket; i++) {

			if (events[i].data.fd == INCOMING_MESSAGE.socket) {

				newSocketPoet(poolThreadSocket, &eventSockets, &INCOMING_MESSAGE, socketDescriptor);
			}
			else {

				receivingMessagePort(poolThreadSocket, incomingDataThread, &events[i], charBuf);
				sendingMessagePort(messageBuffer, &events[i], MAX_SIZE_MESSAGE);
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

void MultiplexingSocketTCP::deleteSocketPort(std::map<int, internetMessage>* plThrdSckt, const internetMessage* incomingMessage, bool* scktOn, int scktDscrptr) {

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

void MultiplexingSocketTCP::copyingOutgoingMessagesPort(std::map<int, std::deque<internetMessage>>* mssgBffr, const internetMessage* incomingMessage) {

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

bool MultiplexingSocketTCP::newSocketPoet(std::map<int, internetMessage>* plThrdSckt, epoll_event* evntSckts, const internetMessage* incomingMessage, int scktDscrptr) {

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

void MultiplexingSocketTCP::receivingMessagePort(std::map<int, internetMessage>* plThrdSckt, std::deque<internetMessage>* incmngDtThrd, epoll_event* evnts, char* chrBf) {

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

					saveReceivingMessagePort(iterThrSoc, incmngDtThrd);
				}

				break;
			}
			else {

				break;
			}
		}
	}
}

void MultiplexingSocketTCP::saveReceivingMessagePort(std::map<int, internetMessage>::iterator iterThrSoc, std::deque<internetMessage>* incmngDtThrd) {

	iterThrSoc->second.dateMessage = std::time(nullptr);
	incmngDtThrd->push_back(std::move(iterThrSoc->second));
}

void MultiplexingSocketTCP::sendingMessagePort(std::map<int, std::deque<internetMessage>>* mssgBffr, epoll_event* evnts, int MaxSizeMessage) {

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

void MultiplexingSocketTCP::copyingIncomingMessages(std::deque<internetMessage>* incmngDtThrd) {

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