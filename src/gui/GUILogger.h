#pragma once

#include <mutex>
#include <deque>
#include <chrono>
#include <iomanip>
#include <sstream>


// Thread-safe line logger
class GuiLogger {
public:
	void clear() {
		std::lock_guard lk(mu);
		lines.clear();
	}

	void pushLine(const std::string &line) {
		using namespace std::chrono;

		const auto now = system_clock::now();
		const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
		std::time_t t = system_clock::to_time_t(now);
		std::tm tm{};

		#ifdef _WIN32
			localtime_s(&tm, &t);
		#else
			localtime_r(&t, &tm);
		#endif

		std::ostringstream oss;
		oss << '[' << std::setfill('0')
			<< std::setw(2) << tm.tm_hour << ':'
			<< std::setw(2) << tm.tm_min  << ':'
			<< std::setw(2) << tm.tm_sec  << '.'
			<< std::setw(3) << ms.count() << "] "
			<< line;

		std::lock_guard lk(mu);
		if (lines.size() >= maxLines) lines.pop_front();
		lines.push_back(std::move(oss).str());
		++version;
	}

	// Copy out snapshot (cheap enough; keeps UI simple)
	void snapshot(std::vector<std::string>& out) const {
		std::lock_guard lk(mu);
		out.assign(lines.begin(), lines.end());
	}

	size_t getVersion() const {
		std::lock_guard lk(mu);
		return version;
	}

private:
	mutable std::mutex mu;
	std::deque<std::string> lines;
	size_t version{0};
	size_t maxLines{5000};  // ring buffer cap
};


// A streambuf that writes to GuiLogger line-by-line
class GuiLogStreamBuf final : public std::streambuf {
public:
	explicit GuiLogStreamBuf(GuiLogger& logger, const char* tag = nullptr)
		: logger(logger), tag(tag ? tag : "") {}

protected:
	int overflow(const int ch) override {
		if (ch == EOF) return 0;
		buffer.push_back(static_cast<char>(ch));
		if (ch == '\n') flushBuffer();
		return ch;
	}

	std::streamsize xsputn(const char* s, std::streamsize n) override {
		for (std::streamsize i = 0; i < n; ++i) {
			const char c = s[i];
			buffer.push_back(c);
			if (c == '\n') flushBuffer();
		}
		return n;
	}

	int sync() override {
		flushBuffer();
		return 0;
	}

private:
	void flushBuffer() {
		if (buffer.empty()) return;

		// Strip trailing '\n'
		if (!buffer.empty() && buffer.back() == '\n') buffer.pop_back();

		if (!buffer.empty()) {
			if (!tag.empty())
				logger.pushLine(tag + std::string(": ") + buffer);
			else logger.pushLine(buffer);
		}
		buffer.clear();
	}

	GuiLogger& logger;
	std::string tag;
	std::string buffer;
};
