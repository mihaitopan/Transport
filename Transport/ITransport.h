#pragma once
#include <vector>

namespace Transport {
	struct Buffer {
		std::vector<unsigned char> content;
	};

	struct IServer {
		struct IHandler {
			virtual int OnMessage(const Buffer& request, Buffer& response) = 0;
		};

		virtual int Init(const char* pipeName) = 0;
		virtual int SetHandler(IHandler* handler) = 0;
		virtual int Uninit() = 0;
		virtual ~IServer(){}
	};

	struct IClient {
		virtual int Connect(const char* remote) = 0;
		virtual int Send(const Buffer& request, Buffer& reply) = 0;
		virtual int Disconnect() = 0;
		virtual ~IClient(){}
	};

	struct IFactory {
		virtual IServer* CreateServer() = 0;
		virtual void DestroyServer(IServer* ) = 0;
		virtual IClient* CreateClient() = 0;
		virtual void DestroyClient(IClient* ) = 0;
	};
}
