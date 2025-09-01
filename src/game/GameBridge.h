#pragma once

#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>


class GameBridge {
public:
	GameBridge();
	~GameBridge();

	void startGameStateListener();
	void closeGameStateListener();

	void waitForConnection();
	[[nodiscard]] bool isClientConnected() const;

	std::string receiveJsonPayload();

private:
	std::mutex connMutex;
	std::condition_variable connCV;
	std::atomic<bool> connected{false};

	uintptr_t listenSocket = 0;
	uintptr_t clientSocket = 0;

	std::string recvBuffer;								// persistent buffer for line framing
	static constexpr size_t kMaxBuffer = 256 * 1024;	// 256 KB protection

	bool setupSocket();
};
