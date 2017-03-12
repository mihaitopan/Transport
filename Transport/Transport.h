#pragma once
#include "ITransport.h"
#include <windows.h>

extern "C" int __declspec(dllexport) CreateObject(const char* name, void** ptr);
extern "C" int __declspec(dllexport) DestroyObject(const char* name, void* ptr);


namespace Transport {
#define BUFFERSIZE 4096
#define NR_MAX_SIMUL_CONN 63
#define EXIT_WAIT_TIME 9000

	class Server : public IServer {
	public:
		const char* pipeName;
		IHandler* handler;
		std::vector<HANDLE> hEvents, hPipe;
		HANDLE hMainThread;
	public:
		int Init(const char* path) override;
		int SetHandler(IHandler* handler) override;
		int Uninit() override;
		~Server() {}

		DWORD static __stdcall MainThreadFunction(LPVOID lpvParam);
		// !!! - another way to call a static function where I need the current object is to have a non-static function
		// which is only called by the static one with "static_cast"
		// DWORD MainThreadFunction() { .. the code goes here .. }
		// DWORD static __stdcall MainThreadFunction(LPVOID lpvParam) { return static_cast<cj::Transport::Server*>(lpvParam)->MainThreadFunction(); }
	};

	struct Package {
		bool flag; // read = 0, write = 1
		HANDLE pipe;
		DWORD offset = 0; // just for preliminary checking
		char buffer[BUFFERSIZE];
		DWORD nBytesDone, nBytesToDo;
		OVERLAPPED ovr;
	};

	class Client : public IClient {
	private:
		HANDLE myPipe;
	public:
		int Connect(const char* remote) override;
		int Send(const Buffer& request, Buffer& reply) override;
		int Disconnect() override;
		~Client() {}
	};

	class Factory : public IFactory {
	public:
		IServer* CreateServer() override;
		void DestroyServer(IServer* iServer) override;

		IClient* CreateClient() override;
		void DestroyClient(IClient* iClient) override;
	};
}
