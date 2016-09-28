# Transport
Dynamic library which facilitates the communication between server and clients. It is designed to execute asynchronous operations (using overlapped structures).

It has an abstract class IHandler and a virtual method OnMessage which are to be overwritten in order to perform what the user desires with the given data.

The library exports 2 functions: CreateObject and DestroyObject, which are use to instantiate the factory of the transport. 
The factory creates the server and clients.
The clients connect to server throgh named pipes, send packages of information, wait for the expected result and disconnect.

A C++ tester is given as an example of how to use the library.
There is also a tester in Python which uses a testbed to test the Transport library.
