#include "MultiplexingSocket.h"
#include "MultiplexingSocketTCP.h"
#include "MultiplexingSocketUDP.h"
#include "MessageProcessing.h"

int main(int argc, char* argv[]) {

	MultiplexingSocket* multiplexingSocketTCP = new MultiplexingSocketTCP();;
	MultiplexingSocket* multiplexingSocketUDP = new MultiplexingSocketUDP();
	MessageProcessing messageProcessing;;
	multiplexingSocketTCP->setPort(8088);
	multiplexingSocketUDP->setPort(8089, MultiplexingSocket::ip);
	multiplexingSocketTCP->setPort(8090, MultiplexingSocket::Unix);
	messageProcessing.newThread(multiplexingSocketTCP);
	messageProcessing.newThread(multiplexingSocketUDP);
	messageProcessing.startProcessing();

	return 0;
}