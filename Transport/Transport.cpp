#include "Transport.h"
#include <iostream>
#include <map>
#include <memory>


// --- --- --- Object --- --- ---
int CreateObject(const char* name, void** ptr) {
	if (std::string(name) == "Transport") {
		*ptr = new Transport::Factory{};
		return 0;
	}
	return 1;
}

int DestroyObject(const char* name, void* ptr) {
	if (std::string(name) == "Transport") {
		Transport::IFactory* myptr = (Transport::IFactory*)ptr;
		delete myptr;
		return 0;
	}
	return 1;
}
// --- --- ---  --- --- ---  --- --- ---


DWORD __stdcall Transport::Server::MainThreadFunction(LPVOID lpvParam) {
	// obtaining object
	Transport::Server* that = (Transport::Server*)lpvParam;

	// declarations and initializations
	DWORD openMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
	DWORD pipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
	DWORD nMaxInstances = PIPE_UNLIMITED_INSTANCES;
	DWORD nOutBufferSize = BUFFERSIZE;
	DWORD nInBufferSize = BUFFERSIZE;
	DWORD nDefaultTimeOut = 0;
	LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;
	BOOL connect;
	HANDLE hExitEvent, hNewConnEvent;
	int nrOfObjects = 2;
	DWORD dwWaitResult;
	std::map<DWORD, std::unique_ptr<Package>> myMap; // if Package is used and not std::unique_ptr<Package>, when the map elements are reallocated
													 // the pointers of the overlapped struct point to a deallocated memory area - best case: program fails
	OVERLAPPED ovrConn;
	
	// overlapped structure for connecting
	ZeroMemory(&ovrConn, sizeof(ovrConn));
	ovrConn.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// create events
	hExitEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("ExitEvent"));
	hNewConnEvent = ovrConn.hEvent;

	// prepare multiple waiting loop
	that->hEvents.push_back(hExitEvent); // hExitEvent : that->hEvents[0]
	that->hEvents.push_back(hNewConnEvent); // hNewConnEvent : that->hEvents[1]
	
	// connect
	connect = ConnectNamedPipe(that->hPipe[that->hPipe.size() - 1], &ovrConn);

	// loop
	while (true) {
		dwWaitResult = WaitForMultipleObjects(nrOfObjects, that->hEvents.data(), FALSE, INFINITE);

		if (dwWaitResult == WAIT_OBJECT_0) {
			return 0;
		} else if (dwWaitResult == WAIT_OBJECT_0 + 1) {
			// create new pipe
			that->hPipe.push_back(CreateNamedPipe(that->pipeName, openMode, pipeMode, nMaxInstances, nOutBufferSize, 
												  nInBufferSize, nDefaultTimeOut, lpSecurityAttributes));

			// create new overllaped structure
			OVERLAPPED ovrOp;
			ZeroMemory(&ovrConn, sizeof(ovrConn));
			ovrConn.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			ZeroMemory(&ovrOp, sizeof(ovrOp));
			ovrOp.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

			// connection event changes
			that->hEvents[1] = ovrConn.hEvent;

			// insert package in map
			std::unique_ptr<Package> p(new Package{}); 
			p.get()->flag = 0;
			p.get()->ovr = ovrOp; 
			p.get()->pipe = that->hPipe[that->hPipe.size() - 2]; // 2 not 1 /// previous pipe
			myMap.insert(std::pair<DWORD, std::unique_ptr<Package>>(nrOfObjects, p.get()));
			p.release();
					
			// the new operation event in inserted in hEvents vector
			that->hEvents.push_back(ovrOp.hEvent); 
			nrOfObjects++;
			if (nrOfObjects == NR_MAX_SIMUL_CONN) { // wait for multiple objects does'n support more than 64 events
				SetEvent(hExitEvent);
			}

			// connect
			connect = ConnectNamedPipe(that->hPipe[that->hPipe.size() - 1], &ovrConn);

		} else {
			Package* p = myMap.at(dwWaitResult).get();

			// preliminary check - the last read/write operation completed successfully
			GetOverlappedResult(p->pipe, &p->ovr, &p->nBytesDone, FALSE);
			if (p->flag == 0) {
				if (p->nBytesDone != p->nBytesToDo) {
					p->offset += p->nBytesDone;
					p->nBytesDone -= p->nBytesToDo;
				} else {
					p->offset = 0;
				}
			}
			
			ResetEvent(p->ovr.hEvent);

			// another way here is to use a 4-cases switch which are:
			//									1. reading message size, 2. then reading message
			//									3. writing result size, 4. then writing result
			if (p->flag == 0) {
				p->nBytesDone = 0; 
				p->nBytesToDo = BUFFERSIZE; 
				p->flag = 1;
				// read message
				ReadFile(p->pipe, p->buffer, p->nBytesToDo, NULL, &p->ovr);
			} else {
				if (p->nBytesDone == 0) {
					p = nullptr;
					continue;
				}

				// do onMessage operation
				Buffer rBuffer = Buffer(); 
				Buffer wBuffer = Buffer();
				rBuffer.content.resize(p->nBytesDone);
				memcpy(rBuffer.content.data(), p->buffer, p->nBytesDone);
				that->handler->OnMessage(rBuffer, wBuffer);
				DWORD nBytesToWrite = (DWORD)wBuffer.content.size();
				memcpy(p->buffer, wBuffer.content.data(), nBytesToWrite);

				p->flag = 0;
				// write result
				WriteFile(p->pipe, p->buffer, nBytesToWrite, NULL, &p->ovr);
			}
			p = nullptr;
		}
	}

	return 0;
}

int Transport::Server::Init(const char* path) {
	this->pipeName = path;

	// first pipe must be created before anyone tries to connect
	this->hPipe.push_back(CreateNamedPipe(this->pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
														  PIPE_UNLIMITED_INSTANCES, BUFFERSIZE, BUFFERSIZE, 0, NULL));
	if (INVALID_HANDLE_VALUE == this->hPipe.back()) { 
		return 1; 
	}

	this->hMainThread = CreateThread(NULL, 0, &MainThreadFunction, (LPVOID)(this), 0, NULL);
	if (this->hMainThread == NULL) {
		return 1;
	}

	return 0;
}

int Transport::Server::SetHandler(IHandler* Handler) {
	this->handler = Handler;
	return 0;
}

int Transport::Server::Uninit() {
	SetEvent(this->hEvents[0]);
	DWORD dwWaitResult = WaitForSingleObject(this->hMainThread, EXIT_WAIT_TIME); // if EXIT_WAIT_TIME passed, something went wrong and
																				 // we forcefully close the main thread and return a status error
	switch (dwWaitResult) {
	case WAIT_OBJECT_0:
		CloseHandle(this->hMainThread);
		break;
	case WAIT_TIMEOUT:
		CloseHandle(this->hMainThread);
		return 1;
	}
	return 0;
}

int Transport::Client::Connect(const char* remote) {
	this->myPipe = CreateFile(remote, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (this->myPipe != INVALID_HANDLE_VALUE) {
		DWORD pipeMode = PIPE_READMODE_MESSAGE;
		if (SetNamedPipeHandleState(myPipe, &pipeMode, NULL, NULL)) {
			return 0;
		}
	}
	return 1;
}

int Transport::Client::Send(const Buffer& request, Buffer& reply) {
	// declarations and initializations
	DWORD nBytesRead = 0, nBytesWritten = 0, nBytesToRead = BUFFERSIZE;
	DWORD nBytesToWrite, offset = 0;
	TCHAR readBuffer[BUFFERSIZE];
	BOOL bResult = FALSE;

	// write
	nBytesToWrite = (DWORD)request.content.size();
	while (nBytesWritten != nBytesToWrite) {
		bResult = WriteFile(this->myPipe, request.content.data() + offset, nBytesToWrite, &nBytesWritten, NULL);
		offset += nBytesWritten;
		if (nBytesWritten != nBytesToWrite) {
			nBytesToWrite -= nBytesWritten;
		}
	} 
	if (!bResult) { 
		return 1; 
	}

	// read
	bResult = ReadFile(this->myPipe, readBuffer, nBytesToRead, &nBytesRead, NULL);
	if (!bResult || nBytesRead == 0) { 
		return 1; 
	}

	// build the reply
	reply.content.resize(nBytesRead);
	memcpy(reply.content.data(), readBuffer, nBytesRead);
	return 0;
}

int Transport::Client::Disconnect() {
	try {
		CloseHandle(this->myPipe);
	} catch (...) {
		return 0;
	}
	return 0;
}

Transport::IServer* Transport::Factory::CreateServer() {
	return new Transport::Server();
}

void Transport::Factory::DestroyServer(IServer* iServer) {
	delete iServer;
}

Transport::IClient* Transport::Factory::CreateClient() {
	return new Transport::Client();
}

void Transport::Factory::DestroyClient(IClient* iClient) {
	delete iClient;
}
// --- --- ---  --- --- ---  --- --- ---