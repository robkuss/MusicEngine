#pragma once

#include "Event.h"

#include <deque>
#include <random>
#include <ranges>
#include <unordered_map>


using T = std::shared_ptr<Event>;


// RNG
static std::mt19937 gen(std::random_device{}());


/** Stores relevant data for a given transition within a Markov chain */
struct TransitionData {
	int note = -1;
	int count = 0;
	std::vector<double> durations;
	int downbeatCount = 0;	// how often this transition lands on a downbeat

	void update(const int noteValue, const double duration, const bool isDownbeat = false) {
		note = noteValue;
		count++;
		durations.push_back(duration);
		if (isDownbeat) ++downbeatCount;
	}

	[[nodiscard]] double sampleDuration() const {
		std::uniform_int_distribution<size_t> dist(0, durations.size() - 1);
		return durations[dist(gen)];
	}

	[[nodiscard]] double getDownbeatProbability() const {
		return count > 0 ? static_cast<double>(downbeatCount) / count : 0.0;
	}
};


/** Deque buffer to store note transitions while training on live performance MIDI data */
class FixedQueue {
public:
	size_t maxSize{};

	FixedQueue() = default;
	explicit FixedQueue(const size_t maxSize) : maxSize(maxSize) {}

	void push(const T& value) {
		if (size() == maxSize) {
			queue.pop_front();
		}
		queue.push_back(value);
	}

	[[nodiscard]] std::vector<T> getSnapshot() const {
		return {queue.begin(), queue.end()};
	}

	[[nodiscard]] size_t size() const {
		return queue.size();
	}

private:
	std::deque<T> queue;
};


/**
 * Data structure for a Markov chain of any given order.
 * The order is equivalent to the size of the "lookbehind" for the chain.
 * Therefore, a classic Markov chain for random walks has order 1.
 * Higher orders take sensible voice leading into account and hence result in better musicality.
 * However, they make the data structure exponentially grow in size.
 */
class MarkovChain {
public:
	MarkovChain() = default;
	explicit MarkovChain(const int order) : order(order) {}

	/** Increment Absolute Transition Probability for the current transition */
	void iatp(
		const FixedQueue& buffer,
		const T& event
	) {
		auto snapshot = buffer.getSnapshot();
		const std::vector prev(snapshot.begin(), snapshot.end() - 1);
		const T next = snapshot.back();

		if (const FixedEvent* full = dynamic_cast<FixedEvent*>(event.get())) {
			const bool isDownbeat = std::abs(full->mtp.offset) < 1e-3;
			transitions[prev][next].update(full->note, full->duration, isDownbeat);
		} else {
			transitions[prev][next].update(event->note, 0.0, false);
		}
	}

	[[nodiscard]] std::optional<T> getNext(const std::vector<T>& context) const {
		if (transitions.empty())
			return std::nullopt;

		const auto it = transitions.find(context);
		if (it == transitions.end())
			return std::nullopt;

		const auto& nextMap = it->second;

		int total = 0;
		for (const auto& data: nextMap | std::views::values) {
			total += data.count;
		}

		std::uniform_int_distribution dist(1, total);
		int choice = dist(gen);

		for (const auto& data: nextMap | std::views::values) {
			if ((choice -= data.count) > 0) continue;

			// Construct a FullEvent using stored note and sampled duration
			return std::make_shared<FixedEvent>(
				data.note,
				MusicTimePoint(),	// will be overwritten
				data.sampleDuration()
			);
		}

		// Should not reach here if total > 0
		return std::nullopt;
	}

	/** @brief Fall back to shorter Markov chain if no continuation was found to prevent the program from halting */
	[[nodiscard]] std::optional<T> getNextWithFallback(const std::vector<T>& context) const {
		// Try progressively shorter contexts
		for (size_t len = context.size(); len >= 1; --len) {
			std::vector subcontext(context.end() - static_cast<int>(len), context.end());
			auto result = getNext(subcontext);
			if (result.has_value()) return result;
		}

		// No valid continuation found at any context size -> Reset buffer and start over
		return std::nullopt;
	}

	/** @brief Get all possible next transitions for a given context */
	const std::unordered_map<T, TransitionData>* getTransitionsForContextRef(const std::vector<T>& context) const {
		const auto it = transitions.find(context);
		if (it != transitions.end())
			return& it->second;
		return nullptr;
	}

private:
	int order{};

	std::unordered_map<
		std::vector<T>,							// Key: context of "order" previous events
		std::unordered_map<T, TransitionData>,	// Value: next event
		VectorHash<T>							// Custom hash for event vectors
	> transitions;
};
