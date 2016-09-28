#include "TransportTester.h"
#include <windows.h>


void TransportTester::run() {
	WIN32_FIND_DATA InputDataFile;
	HANDLE hInFind, hReadFile, hWriteFile;
	DWORD nBytesRead, nBytesWriten, nBytesToRead, nBytesToWrite;
	BOOL bReadResult, bWriteResult;
	char ReadBuffer[BUFFERSIZE], WriteBuffer[BUFFERSIZE];

	// find input file
	hInFind = FindFirstFile(this->inputFile.c_str(), &InputDataFile);
	if (INVALID_HANDLE_VALUE == hInFind) {
		throw FileException("input file not found");
	}

	// open files for read and write
	hReadFile = CreateFile(this->inputFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hReadFile) {
		throw FileException("error opening input file for reading");
	}
	hWriteFile = CreateFile(this->outputFile.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hWriteFile) {
		throw FileException("error opening output file for writing");
	}

	nBytesToRead = BUFFERSIZE;
	nBytesRead = 1;
	while (nBytesRead != 0) {
		// read input file
		bReadResult = ReadFile(hReadFile, ReadBuffer, nBytesToRead, &nBytesRead, NULL);
		if (nBytesRead < nBytesToRead) break;
		if (false == bReadResult) {
			throw FileException("error reading input file");
		}

		// save data into inBuffer
		Transport::Buffer inBuffer = Transport::Buffer();
		Transport::Buffer outBuffer = Transport::Buffer();
		inBuffer.content.resize(nBytesRead);
		memcpy(inBuffer.content.data(), ReadBuffer, nBytesRead);

		// connect. send message, obtain result, disconnect
		if (0 != this->client->Connect(this->pipeName)) { 
			throw LibException("error connecting pipe"); 
		}
		if (0 != this->client->Send(inBuffer, outBuffer)) { 
			throw LibException("error sharing data"); 
		}
		if (0 != this->client->Disconnect()) { 
			throw LibException("error disconnecting pipe"); 
		}

		// prepare data in WriteBuffer to be written in the output file
		nBytesToWrite = (DWORD)outBuffer.content.size();
		memcpy(WriteBuffer, outBuffer.content.data(), nBytesToWrite);

		// write output file
		bWriteResult = WriteFile(hWriteFile, WriteBuffer, nBytesToWrite, &nBytesWriten, NULL);
		if (false == bReadResult || nBytesWriten != nBytesToWrite) {
			throw FileException("error writing output file");
		}
	}
	if (nBytesRead != 0) {			// read now the last less than 4kb bytes from the input file
		// save data into inBuffer
		Transport::Buffer inBuffer = Transport::Buffer();
		Transport::Buffer outBuffer = Transport::Buffer();
		inBuffer.content.resize(nBytesRead);
		memcpy(inBuffer.content.data(), ReadBuffer, nBytesRead);

		// connect. send message, obtain result, disconnect
		if (0 != this->client->Connect(this->pipeName)) { 
			throw LibException("error connecting pipe"); 
		}
		if (0 != this->client->Send(inBuffer, outBuffer)) { 
			throw LibException("error sharing data"); }
		if (0 != this->client->Disconnect()) { throw LibException("error disconnecting pipe"); 
		}

		// prepare data in WriteBuffer to be written in the output file
		nBytesToWrite = (DWORD)outBuffer.content.size();
		memcpy(WriteBuffer, outBuffer.content.data(), nBytesToWrite);

		// write output file
		bWriteResult = WriteFile(hWriteFile, ReadBuffer, nBytesToWrite, &nBytesWriten, NULL);
		if (false == bReadResult || nBytesWriten != nBytesToWrite) {
			throw FileException("error writing output file");
		}

		// --- check end ---	// if we didn't reach by now the end of the input file, something went wrong
		bReadResult = ReadFile(hReadFile, WriteBuffer, nBytesToRead, &nBytesRead, NULL);
		if (nBytesRead != 0) {
			throw FileException("error reading input file");
		}
	}

	// close handles
	CloseHandle(hReadFile);
	CloseHandle(hWriteFile);
	FindClose(hInFind);
}
