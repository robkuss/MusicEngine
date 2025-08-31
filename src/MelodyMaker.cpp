#include "MelodyMaker.h"

using namespace std;


void MelodyMaker::initMarkovChain(const int markovChainOrder, const Melody& melody) {
	mc	   = MarkovChain(markovChainOrder);
	buffer = FixedQueue(markovChainOrder + 1);  // + 1 for next note (only for training)

	// Initialize buffer with START token events
	for (int i = 0; i < buffer.maxSize; i++)
		buffer.push(make_shared<SimpleEvent>(START));

	// Train the Markov model using the provided MIDI file
	cout << "[MelodyMaker] Training order " << markovChainOrder << " Markov chain...\n";
	for (const auto& note : melody.events) {
		buffer.push(note);
		mc.iatp(buffer, note);
	}
	cout << "[MelodyMaker] Markov chain training done.\n";

	// Reset play buffer for playback mode
	buffer = FixedQueue(markovChainOrder);  // No + 1 needed for playback mode

	// Initialize buffer with START token events
	for (int i = 0; i < buffer.maxSize; i++)
		buffer.push(make_shared<SimpleEvent>(START));
}


shared_ptr<Event> MelodyMaker::handleNoResult() {
	// No valid continuation, reset buffer with START tokens
	buffer = FixedQueue(buffer.size());
	for (int i = 0; i < buffer.maxSize; ++i)
		buffer.push(make_shared<SimpleEvent>(START));

	const auto result = mc.getNext(buffer.getSnapshot());
	if (!result.has_value()) {
		cout << "[MelodyMaker] No valid continuation found. The input MIDI is likely empty.\n";
		exit(EXIT_FAILURE);
	}

	return result.value();  // Definitely exists at this point
}


shared_ptr<Event> MelodyMaker::pollNextEvent() {
	const auto context = buffer.getSnapshot();
	auto result = mc.getNextWithFallback(context);
	if (!result.has_value()) result = handleNoResult();
	shared_ptr<Event> event = *result;
	buffer.push(event);
	return event;
}
