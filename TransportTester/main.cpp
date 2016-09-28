#include "Debug.h"
#include "TransportTester.h"
#include "Transport\Transport.h"
#include <iostream>
#include <windows.h>
#include <fstream>
#include <sstream>


int __cdecl main(int argc, char *argv[]) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// validating arguments
	if (argc < 2 || argc > 3) {
		std::cout << "Invalid arguments\n";
		std::cout << "\t see -help\n";
		return 0;
	}
	
	// help
	if (argc == 2 && std::string(argv[1]) == "-help") {
		std::cout << "\t-help					possible arguments\n";
		std::cout << "\t<nume_pipe> <fisier_de_configurare>	run tester for config file\n";
		return 0;
	}

	// run
	if (argc == 3) {
		try {
			// load transport lib
			HINSTANCE tlib = LoadLibrary("Transport.dll");
			if (tlib == NULL) {
				throw LibException("Failed to load library.");
			}

			// load create and destroy functions
			p_CreateObject transport = (p_CreateObject)GetProcAddress(tlib, CREATE_OBJECT_FNAME);
			p_DestroyObject transportD = (p_DestroyObject)GetProcAddress(tlib, DESTROY_OBJECT_FNAME);
			if (!transport || !transportD) {
				throw LibException("Failed to load library functions.");
			}

			// create factory
			Transport::IFactory* factory;
			if ((transport("Transport", (void**)&factory) != 0)) {
				FreeLibrary(tlib);
				throw LibException("Failed to run library functions.");
			}

			// create server
			Transport::IServer* server = factory->CreateServer();

			// init server
			if (0 != server->Init(argv[1])) { throw LibException("error initiating server - check pipe name"); }
			Handler handler;
			if (0 != server->SetHandler(&handler)) { throw LibException("error setting handler"); }

			// do things
			std::ifstream myfile;
			myfile.open(std::string(argv[2]));
			if (!myfile.is_open()) {
				std::cout << "config file not found";
				return 1;
			}
			std::string line, inputFile, outputFile;
			while (std::getline(myfile, line)) { // for every file in config file

				inputFile = ""; outputFile = "";
				std::istringstream iss(line);
				iss >> inputFile >> outputFile;

				TransportTester tt = TransportTester{};
				tt.setPipe(argv[1]);
				tt.setInputFile(inputFile);
				tt.setOutputFile(outputFile);

				// create client
				Transport::IClient* client = factory->CreateClient();
				tt.setClient(client);

				tt.run();

				// destroy client
				factory->DestroyClient(client);
			}

			// uninit server
			if (0 != server->Uninit()) { throw LibException("server didn't exit correctly"); }

			// destroy server
			factory->DestroyServer(server);
			if ((transportD("Transport", (void*)factory) != 0)) {
				FreeLibrary(tlib);
				throw LibException("Failed to run library functions.");
			}

			// unload transport lib
			FreeLibrary(tlib);
			return 0;
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
			return 1;
		}
	}

	std::cout << "Invalid arguments\n";
	std::cout << "\t see -help\n";
	return 0;
}
