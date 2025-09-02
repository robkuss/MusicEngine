#pragma once

#include <data/MarkovChain.h>

#include "SimilarityAlgos.h"


/** @brief Generates a number of test sequences from a given Markov chain */
inline std::vector<Melody> generateMelodySamples(
	const MarkovChain& mc,
	const int order,
	const int count,
	const size_t length
) {
	std::vector<Melody> sequences;
	for (int i = 0; i < count; ++i) {
		FixedQueue genBuffer(order);

		// Initialize genBuffer with START token events
		for (int j = 0; j < genBuffer.maxSize; j++) genBuffer.push(std::make_shared<SimpleEvent>(START));

		Melody generated;

		for (size_t j = 0; j < length; ++j) {
			auto result= mc.getNext(genBuffer.getSnapshot());
			if (!result.has_value()) break;

			const auto& next = *result;

			genBuffer.push(next);
			generated.events.push_back(std::make_shared<SimpleEvent>(next->note));
		}
		sequences.push_back(generated);
	}
	return sequences;
}


/**
 * @brief Selects the most appropriate Markov chain order for the training melody.
 *
 * Tries different orders, generates melodies, compares them to the original,
 * and returns the order with the best balance between randomness and repetition.
 */
inline int determineBestOrder(const Melody& melody, const Mode mode) {
	using namespace std;

	cout << "\nRunning tests to find the most fitting Markov chain order for your MIDI..." << endl;

	const size_t melodyLength = melody.events.size() * 5;
	const size_t simFuncNum = similarityFunctions.size();

	int order = 0;
	int bestOrder = order;
	double bestScore = 0.0;

	int tooSimilarInARow = 0;	// will break the loop once there have been 3 "too similar" results in a row

    while (true) {
    	order++;

    	// Train a temporary Markov chain at this order
    	MarkovChain tempMc(order);
    	FixedQueue tempBuffer(order + 1);

    	// Initialize tempBuffer with START token events
    	for (int i = 0; i < tempBuffer.maxSize; i++) tempBuffer.push(make_shared<SimpleEvent>(START));

    	for (const auto& note : melody.events) {
    		tempBuffer.push(note);
    		tempMc.iatp(tempBuffer, note);
    	}

    	// Generate test sequences from the trained Markov chain
    	vector<Melody> sequences = generateMelodySamples(tempMc, order, 100, melodyLength);

    	vector results(simFuncNum, 0.0);

    	// Evaluate similarity between original and generated sequences using all metrics
    	for (const auto& sequence : sequences) {
    		for (size_t i = 0; i < simFuncNum; ++i) {
    			// Add them all up - average will be evaluated later
    			results[i] += similarityFunctions[i].second(melody, sequence);
    		}
    	}

    	// Calculate average score per similarity function
    	double score = 0.0;
    	vector weights = {0.2, 0.1, 0.3, 0.2, 0.2};	 // Weights for different similarity metrics
    	for (int i = 0; i < simFuncNum; ++i) {
    		results[i] /= static_cast<double>(sequences.size());	  // Average results
			score += weights[i] * (1 - 2 * abs(results[i] - 0.5));  // The closer a result is to 0.5, the better
    	}

    	// Print results for this order
        cout << "Order " << left << setw(2) << order << " | ";
    	for (size_t i = 0; i < simFuncNum; ++i) {
    		cout << similarityFunctions[i].first
    			<< ": " << fixed << setprecision(3) << results[i]
    			<< (i == simFuncNum - 1 ? "" : ", ");
    	}
    	cout << " => Score: " << score;

    	// Apply penalties if result is too similar or too random
    	int penaltyType = 0;
    	if (results[1] > 0.95     /* no 3-gram */  || results[3] > 0.99 || results[4] > 0.80) penaltyType |= 2;	 // too similar
    	if (results[1] < 0.20 || results[2] < 0.25 || results[3] < 0.20 || results[4] < 0.15) penaltyType |= 1;  // too random
    	if (penaltyType) {
    		const char* messages[] = { "", "too random", "too similar", "too similar AND too random" };
    		cout << " (" << messages[penaltyType] << ")";
    	}
    	cout << endl;

    	// Exit early if chain is repeatedly too similar
    	if ((tooSimilarInARow = penaltyType >= 2 ? tooSimilarInARow + 1 : 0) == 3) break;

    	// Track best performing order so far
        if (score > bestScore) {
            bestScore = score;
            bestOrder = order;
        }
    }

	// Accept or override the suggested order
    cout << "Suggested Markov chain order: " << bestOrder << endl;

	// Optionally let the user decide if they want to keep the suggested order or override it
	/*cout << "Press ENTER to accept or choose a different order: ";
	if (mode == Mode::IDLE && cin.peek() == '\n') cin.ignore();
	string userInput;
	getline(cin, userInput);
	if (!userInput.empty()) {
		try {
			if (const int manualOrder = stoi(userInput); manualOrder >= 1) {
				cout << "Using manually selected order: " << manualOrder << endl;
				return manualOrder;
			}
		} catch (...) {
			cout << "Invalid input. Using suggested order: " << bestOrder << endl;
		}
	}*/

    return bestOrder;
}