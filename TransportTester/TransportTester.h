#pragma once
#include "Exceptions.h"
#include "IBdBaseIbject.h"
#include "Transport\ITransport.h"
#include <vector>
#include <string>


class TransportTester {
private:
	std::string inputFile;
	std::string outputFile;
	Transport::IClient* client;
	char* pipeName;
public:
	void setInputFile(std::string InputFile) { this->inputFile = InputFile; }
	void setOutputFile(std::string OutputFile) { this->outputFile = OutputFile; }
	void setClient(Transport::IClient* Client) { this->client = Client; }
	void setPipe(char* PipeName) { this->pipeName = PipeName; }
private:
#define BUFFERSIZE 4096
public:
	TransportTester() {}
	void run();
	~TransportTester() {}
};

class Handler : public Transport::IServer::IHandler {
public:
	int OnMessage(const Transport::Buffer& request, Transport::Buffer& response) override;
};

// OnMessage must be an inline function so that no linking problems appear
inline int Handler::OnMessage(const Transport::Buffer& request, Transport::Buffer& response) {
	try {
		response.content.resize(request.content.size());
		memcpy(response.content.data(), request.content.data(), request.content.size());
	} catch (...) {
		return 1;
	}
	return 0;
}
