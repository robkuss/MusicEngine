#include "GameBridge.h"

#include <iostream>

// Platform-specific includes/macros
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#pragma comment(lib, "Ws2_32.lib")
	typedef SOCKET SocketType;
	#define CLOSESOCKET(s) closesocket(s)
	#define IS_VALID_SOCKET(s) ((s) != INVALID_SOCKET)
#else
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	typedef int SocketType;
	#define INVALID_SOCKET (-1)
	#define CLOSESOCKET(s) close(s)
	#define IS_VALID_SOCKET(s) ((s) >= 0)
#endif

using namespace std;


// Globals
constexpr u_short PORT = 5555;

constexpr bool DEBUG_GB = false;


GameBridge::GameBridge() = default;

GameBridge::~GameBridge() {
	closeGameStateListener();
}


bool GameBridge::setupSocket() {
	#ifdef _WIN32
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			cerr << "[GameBridge] WSAStartup failed" << endl;
			return false;
		}
	#endif

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (!IS_VALID_SOCKET(listenSocket)) {
		cerr << "[GameBridge] Failed to create socket" << endl;
		return false;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
		cerr << "[GameBridge] Failed to bind socket" << endl;
		return false;
	}

	if (listen(listenSocket, 1) < 0) {
		cerr << "[GameBridge] Failed to listen" << endl;
		return false;
	}

	return true;
}


void GameBridge::startGameStateListener() {
	if (!setupSocket()) {
		return;
	}

	thread([this]() {
		cout << "[GameBridge] Waiting for client on port 5555..." << endl;
		clientSocket = accept(listenSocket, nullptr, nullptr);
		if (!IS_VALID_SOCKET(clientSocket)) {
			cerr << "[GameBridge] Failed to accept client" << endl;
			return;
		}

		{
			lock_guard lock(connMutex);
			connected = true;
		}

		connCV.notify_all();
		cout << "[GameBridge] Client connected" << endl;

	}).detach();
}


void GameBridge::waitForConnection() {
	unique_lock lock(connMutex);
	connCV.wait(lock, [this] {
		return connected.load();
	});
}

bool GameBridge::isClientConnected() const {
	return connected.load();
}


string GameBridge::receiveJsonPayload() {
	if (!isClientConnected()) return "";

	char chunk[4096];

	while (true) {
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(clientSocket, &readSet);

		timeval timeout{};
		timeout.tv_sec = 0;
		timeout.tv_usec = 150000;  // Wait 150 ms

		const int sel = select(static_cast<int>(clientSocket) + 1, &readSet, nullptr, nullptr, &timeout);
		if (sel == 0) return "";  // Timeout: No data received for 150 ms
		if (sel < 0) {
			cerr << "[GameBridge] select() error.\n";
			connected = false;
			return "";
		}

		const int rec = recv(clientSocket, chunk, sizeof(chunk) - 1, 0);
		if (rec <= 0) {
			cout << "[GameBridge] Client disconnected.\n";
			connected = false;
			return "";
		}

		recvBuffer.append(chunk, rec);

		// Size limit for protection
		if (recvBuffer.size() > kMaxBuffer) {
			cerr << "[GameBridge] Dropping oversized buffer > " << kMaxBuffer << " bytes\n";
			recvBuffer.clear();
			return "";
		}

		// Handle newline-delimited JSON
		const size_t nl = recvBuffer.find('\n');
		if (nl != string::npos) {
			string line = recvBuffer.substr(0, nl);
			recvBuffer.erase(0, nl + 1);
			if (DEBUG_GB) cout << "[GameBridge] Received JSON: " << line << endl;
			return line;
		}
	}
}


void GameBridge::closeGameStateListener() {
	if (IS_VALID_SOCKET(clientSocket)) {
		CLOSESOCKET(clientSocket);
		clientSocket = INVALID_SOCKET;
	}

	if (IS_VALID_SOCKET(listenSocket)) {
		CLOSESOCKET(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	connected = false;

	#ifdef _WIN32
		WSACleanup();
	#endif
}
