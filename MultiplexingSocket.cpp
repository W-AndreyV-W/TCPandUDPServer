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

bool MultiplexingSocket::setPort(int portNumber, connectionProtocol protocol, int connections) {

	if (!openPort(portNumber, protocol, connections)) {

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

void MultiplexingSocket::deleteSocket(int portNumber, int socketNumber) {

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

bool MultiplexingSocket::openPort(int portNumber, connectionProtocol protocol, int connections)	{

	return false;
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