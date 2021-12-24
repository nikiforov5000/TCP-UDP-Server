#include <chrono>
#include <iostream>
#include <filesystem>
#include <future>
#include <fstream>
#include <map>
#include <queue>
#include <sstream>
#include <thread>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

std::map<int, std::string> ConfQueue{};

class TCPServer {
	std::string m_ipAddress{};
	int m_tcpPort{};
	std::string m_folderName{};
	SOCKET m_listening{};
	SOCKET m_clientSocket{};
public:
	TCPServer(std::string& ip, int port) : m_ipAddress(ip), m_tcpPort(port) {
		// Initialize winsock
		WSADATA wsData;
		WORD ver = MAKEWORD(2, 2);
		int wsOk = WSAStartup(ver, &wsData);
		if (wsOk != 0) {
			std::cerr << "Can't initialize Winsock" << std::endl;
			return;
		}
		// Create socket
		m_listening = socket(AF_INET, SOCK_STREAM, 0);
		if (m_listening == INVALID_SOCKET) {
			std::cerr << "Can' create socket" << std::endl;
			return;
		}
		// Bind ip and port to socket
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(m_tcpPort);
		inet_pton(AF_INET, m_ipAddress.c_str(), &hint.sin_addr);
		bind(m_listening, (sockaddr*)&hint, sizeof(hint));
		listen(m_listening, SOMAXCONN);
		// Wait for connection
		sockaddr_in client;
		int clientSize = sizeof(client);
		m_clientSocket = accept(m_listening, (sockaddr*)&client, &clientSize);
		if (m_clientSocket == INVALID_SOCKET) {
			std::cerr << "Invalid socket" << std::endl;
			return;
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
		closesocket(m_listening);
	}
	std::string receiveInfo() {
		// accept 
		char buf[128];
		
		ZeroMemory(buf, 128);
		int bytesRecv = recv(m_clientSocket, buf, 128, 0);
		if (bytesRecv == SOCKET_ERROR) {
			std::cerr << "Error in recv" << std::endl;
		}
		if (bytesRecv == 0) {
			std::cout << "Client disconnected" << std::endl;
		}
		if (bytesRecv == -1) {

		}
		return std::string(buf);
	}
	void sendInfo(std::string info) {
		int bytesSent = send(m_clientSocket, info.c_str(), info.length() + 1, 0); // Send to client a response
		//std::cout << "tcpConfSent:" << info << std::endl;
	}
	void sendConf() {	
		size_t count{};
		while (true) {													  
			std::this_thread::sleep_for(std::chrono::milliseconds(1));	  
			if (!ConfQueue.empty()) {									  
				sendInfo((*ConfQueue.begin()).second);							  
				ConfQueue.erase(ConfQueue.begin());
				count = 0;
			}			
			if (count++ > 1000) {
				return;
			}
		}																  
	}
	void closeConn() { 
		closesocket(m_clientSocket);
		WSACleanup();
	}
};

class UDPServer {
	std::map<int, std::string> m_Storage{};
	std::string m_ipAddress{};
	int m_udpPort{};
	SOCKET m_in{};
public:
	UDPServer(std::string& ip, int port) 
		: m_ipAddress(ip), m_udpPort(port){
		WSADATA data;
		WORD version = MAKEWORD(2, 2);
		int wsOk = WSAStartup(version, &data);
		if (wsOk != 0) {
			std::cout << "Can't start Winsock! " << wsOk;
			return;
		}
		// Create a socket, notice that it is a user datagram socket (UDP)
		m_in = socket(AF_INET, SOCK_DGRAM, 0);
		// Create a server hint structure for the server
		sockaddr_in serverHint;
		serverHint.sin_addr.S_un.S_addr = ADDR_ANY; 
		inet_pton(AF_INET, m_ipAddress.c_str(), &serverHint.sin_addr);
		serverHint.sin_family = AF_INET; 
		serverHint.sin_port = htons(m_udpPort);
		// Try and bind the socket to the IP and port
		if (bind(m_in, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR) {
			std::cout << "Can't bind socket! " << WSAGetLastError() << std::endl;
			return;
		}
	}
	int recvFile() {
		//		receive a pack and store into temp map
		const size_t bufSize = 65536;
		char buf[bufSize];
		sockaddr_in client; // Use to hold the client information (port / ip address)
		int clientLength = sizeof(client); // The size of the client information
		while (true) {
			ZeroMemory(&client, clientLength); // Clear the client structure
			ZeroMemory(buf, bufSize); // Clear the receive buffer
			// Wait for message
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			int bytesIn = recvfrom(m_in, buf, bufSize, 0, (sockaddr*)&client, &clientLength);
			if (bytesIn == SOCKET_ERROR) {
				std::cout << "Error receiving from client " << WSAGetLastError() << std::endl;
				continue;
			}
			// store pack
			std::string stringBuf(buf);
			if (stringBuf == "-1") {
				return 0;
			}
			int id{ std::stoi(stringBuf.substr(0, stringBuf.find(' '))) };
			std::string pack{ stringBuf.substr(stringBuf.find(' ') + 1, stringBuf.length()) };
			m_Storage.emplace(id, pack);
//		push confirmation
			ConfQueue.emplace(id, std::to_string(id) + " " + std::to_string(bytesIn));
		}
		return -1;
	}
	void saveFile(std::string& folderName, std::string& fileName) {
		std::filesystem::create_directories("./" + folderName);
		std::ofstream ofs(folderName + "/" + fileName);
		if (ofs.good()) {
			for (auto pack : m_Storage) { ofs << pack.second; }
		}
		ofs.close();
	}
	void closeConn() {
		closesocket(m_in);
		WSACleanup();
	}

};
int main(int argc, char* argv[]) {
	std::string ipAddress{ argv[1] };	//127.0.0.1
	int tcpPort{ atoi(argv[2]) };		//5555 
	std::string folderName{ argv[3] };	//temp
//	tcp server starts and accepts call from client
	TCPServer tcpServer(ipAddress, tcpPort);
//	tcp recv
	std::string info{ tcpServer.receiveInfo() }; // 1st udp and filename // 2nd confirmation to close
	int udpPort{ std::stoi(info.substr(0, info.find(' '))) };
	std::string fileName{ info.substr(info.find(' ') + 1, info.length()) };
//	start udpserver
	UDPServer udpServer(ipAddress, udpPort);
//	new thread RECV udp
	std::future resultUdpRecv{ std::async(std::launch::async, &UDPServer::recvFile, &udpServer) };

	tcpServer.sendConf();
	udpServer.saveFile(folderName, fileName);

	std::string disc = "disconnect";
	tcpServer.sendInfo(disc);

	udpServer.closeConn();
	tcpServer.closeConn();

	return 0;
}