#pragma once
#include <exception>
#include <string>


class FileException : public std::exception {
private:
	std::string message;
public:
	FileException(std::string msg) : message(msg) {}
	const char* what() const override { return message.c_str(); }
};

class LibException : public std::exception {
private:
	std::string message;
public:
	LibException(std::string msg) : message(msg) {}
	const char* what() const override { return message.c_str(); }
};
