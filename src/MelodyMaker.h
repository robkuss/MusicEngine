#pragma once

#include "data/MarkovChain.h"


class MelodyMaker {
public:
	void initMarkovChain(int markovChainOrder, const Melody& melody);
	std::shared_ptr<Event> pollNextEvent();

private:
	MarkovChain mc;
	FixedQueue buffer;

	std::shared_ptr<Event> handleNoResult();
};
