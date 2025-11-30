#include "MultiplexingSocket.h"
#include "MessageProcessing.h"

int main(int argc, char* argv[]) {
;
	MultiplexingSocket MultiplexingSocket;
	MessageProcessing messageProcessing(&MultiplexingSocket);

	return 0;
}