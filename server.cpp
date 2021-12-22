#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <thread>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


class UDPServer {
	std::map<int, std::string> m_Storage{};
	int UDPPort{};
	std::string ipAddress{};
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	SOCKET in = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in serverHint;
	std::string fileName;

public:
	std::map<int, std::string>& getMap() { return m_Storage; }
	// Start WinSock
	UDPServer(std::string& file, int udpPort, std::string& folderName, std::string ip) : fileName(file), UDPPort(udpPort), ipAddress(ip) {
		int wsOk = WSAStartup(version, &data);
		if (wsOk != 0) {
			std::cout << "Can't start Winsock! " << wsOk;
			return;
		}
		// Create a socket, notice that it is a user datagram socket (UDP)
		// SOCKET in = socket(AF_INET, SOCK_DGRAM, 0);
		////////////////////////////////// SOME ERROR HERE
		// Create a server hint structure for the server
		//serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
		inet_pton(AF_INET, ipAddress.c_str(), &serverHint.sin_addr);
		serverHint.sin_family = AF_INET; // Address format is IPv4
		serverHint.sin_port = htons(UDPPort); // Convert from little to big endian
		// Try and bind the socket to the IP and port
		if (bind(in, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR) {
			std::cout << "Can't bind socket! " << WSAGetLastError() << std::endl;
			//return;
		}
	}
	// Enter a loop
	int receive() {
		char buf[4096];
		sockaddr_in client; // Use to hold the client information (port / ip address)
		int clientLength = sizeof(client); // The size of the client information
		while (true) {

			ZeroMemory(&client, clientLength); // Clear the client structure
			ZeroMemory(buf, 4096); // Clear the receive buffer

			// Wait for message
			std::this_thread::sleep_for(std::chrono::milliseconds(200));

			std::cout << "about to  recvfrom" << std::endl;
			int bytesIn = recvfrom(in, buf, 4096, 0, (sockaddr*)&client, &clientLength);
			std::cout << "recv buf " << buf << " " << std::endl;
			if (bytesIn == SOCKET_ERROR) {
				std::cout << "Error receiving from client " << WSAGetLastError() << std::endl;
				continue;
			}
			if (buf == "EXIT") {
				return 0;
			}
			//////////// store pack

			std::string stringBuf{ std::string(buf) };
			int id{ std::stoi(stringBuf.substr(0, stringBuf.find(' '))) };
			std::string pack{ stringBuf.substr(stringBuf.find(' ') + 1, stringBuf.length()) };

			m_Storage.emplace(id, pack);

			return stringBuf.length();
			// Display message and client info
			char clientIp[256]; // Create enough space to convert the address byte array
			ZeroMemory(clientIp, 256); // to string of characters

			// Convert from byte array to chars
			inet_ntop(AF_INET, &client.sin_addr, clientIp, 256);

			// Display the message / who sent it
			std::cout << "Message recv from " << clientIp << " : " << buf << std::endl;
		}
	}
	void close() {
		// Close socket
		closesocket(in);
		// Shutdown winsock
		WSACleanup();

		//////////////////////   Save FILE

		std::ofstream ofs(fileName);
		for (auto pack : m_Storage) {
			ofs << pack.second;
		}
	}
};


///   ^      ^  ^      ^  ^      ^
///   00    00	000000	  000000
///	  00	00	00	  00  00	00
///	  00	00	00	  00  00	00
///	  00	00	00	  00  000000
///	  00	00  00	  00  00
///	  00	00  00	  00  00
///		0000	000000	  00

////////////////////////////////////////////////////////////////////////////

//		  0000		00000000    000000		00	  00    00000000	000000	
//		00	  00	00		    00	  00	00	  00    00			00	  00
//		00			00		    00	  00	00	  00    00			00	  00
//		  0000		000000	    000000		00	  00    000000		000000	
//			  00	00		    00	  00	00	  00    00			00	  00
//		00	  00	00		    00	  00	 00  00	    00			00	  00
//		  0000		00000000    00	  00	   00	    00000000	00	  00
int main(int argc, char* argv[]) {
	//int main() {

	/////////// Receive args: ipAdress, TCPport, folder name
	std::string ipAddress{ argv[1] };
	int port{ atoi(argv[2]) };
	std::string folderName{ argv[3] };
	//std::cin >> ipAddress >> port >> folderName;
	std::filesystem::create_directories("./" + folderName);

	///////// Create TCP socket

	// Initialize winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		std::cerr << "Can't initialize Winsock" << std::endl;
		return 0;
	}

	// Create socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		std::cerr << "Can' create socket" << std::endl;
		return 0;
	}

	// Bind ip and port to socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	//hint.sin_addr.S_un.S_addr = INADDR_ANY;
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);


	//////////// listen...

	bind(listening, (sockaddr*)&hint, sizeof(hint));
	
	// Set sock to be listening sock
	listen(listening, SOMAXCONN);
	

	// Wait for connection
	sockaddr_in client;
	int clientSize = sizeof(client);
	SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Invalid socket" << std::endl;
		return 0;
	}
	char host[NI_MAXHOST];
	char service[NI_MAXHOST];

	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXHOST);

	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
		std::cout << host << " connected on port " << service << std::endl;
	}
	else {
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
	}
	
	// Close listening sock
	closesocket(listening);
	

	//////////// Receive fileName and UDPport


	// accept 
	char buf[4096];

	ZeroMemory(buf, 4096);
	int bytesRecv = recv(clientSocket, buf, 4096, 0);
	if (bytesRecv == SOCKET_ERROR) {
		std::cerr << "Error in recv" << std::endl;
	}
	if (bytesRecv == 0) {
		std::cout << "Client disconnected" << std::endl;
	}
	if (bytesRecv == -1) {

	}
	std::string stringBuf{ std::string(buf) };
	std::string fileName{ stringBuf.substr(0, stringBuf.find(' '))};
	int UDPport{ std::stoi(stringBuf.substr(stringBuf.find(' ') + 1, stringBuf.length())) };

//////////// Create UDP socket

	UDPServer udpServer(fileName, UDPport, folderName, ipAddress);

	
	//////////// While
	////////////		Receive id and pack
	int conf{};
	while (conf = udpServer.receive()) {
	////////////		Send confirmation (id of pack) via TCP
		// Echo message to client
		send(clientSocket, std::to_string(conf).c_str(), std::to_string(conf).length() + 1, 0); // Send to client a response
	}
	

//////////// on EXIT close both sockets

	//ZeroMemory(buf, 4096);
	//bytesRecv = recv(clientSocket, buf, 4096, 0);
	//if (buf == "EXIT") {
		// Close socket
		closesocket(clientSocket);
		// Shutdown winsock
		WSACleanup();
	//}
	//if (bytesRecv == SOCKET_ERROR) {
	//	std::cerr << "Error in recv" << std::endl;
	//}
	//if (bytesRecv == 0) {
	//	std::cout << "Client disconnected" << std::endl;
	//}
	//if (bytesRecv == -1) {
	//
	//}
		std::ofstream ofs(folderName + "/" + fileName);
		std::for_each(udpServer.getMap().begin(), udpServer.getMap().end(), [&ofs](std::pair<int, std::string>&& pair) {
			ofs << pair.second;
			}
		);
		
//////////// save file in Folder
//////////// 


	

	return 0;

}