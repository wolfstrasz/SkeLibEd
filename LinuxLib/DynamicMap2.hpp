#ifndef DYNAMICMAP_H
#define DYNAMICMAP_H

#include <cstdlib>
#include <iostream>
#include <vector>
#include <type_traits>
#include <functional>
#include <stdarg.h>
#include <typeinfo>
#include <queue>
#include <utility>
#include <thread>
#include <mutex>

class DynamicMapSkeleton {

private:
	DynamicMapSkeleton() {}

	// Elemental - function used in mapping
	// ------------------------------------
	template<typename EL>
	class Elemental {
	public:
		Elemental(EL el) : elemental(el) {}
		EL elemental;
	};

public:




	// MapImplementation
	// ------------------
	template<typename EL>
	class DynamicMapImplementation {

	private:
		//unsigned char BLOCK_FLAG_INITIAL_VALUE;
		size_t nthreads;
		size_t sizeOfChunk;
		Elemental<EL> elemental;

        template<typename IN, typename OUT>
		class Scoreboard {
		public:
            // constructor
            Scoreboard(){
                this->isFinished = false;
                this->curIndex = 0;
            }
            // input output
            std::vector<OUT> *output;
            std::vector<IN> *input;
            // detect global work
			bool isFinished;
            // detect next work
			size_t inputSize;
			size_t curIndex;
			size_t jobSize;
            // guard
			std::mutex scoreboardLock;

		};

		// ThreadMap - function applied to each thread
		// --------------------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(Scoreboard<IN,OUT> *scoreboard, size_t threadID, ARGs... args) {

			size_t elementsCount;
			size_t elementIndex;
			while (!scoreboard->isFinished) {

                    // Lock scoreboard
                    while (!scoreboard->scoreboardLock.try_lock());

					if(scoreboard->isFinished) {
						scoreboard->scoreboardLock.unlock();
						break;
					}

                    // get new data
                    if (scoreboard->curIndex + scoreboard->jobSize < scoreboard->inputSize){
				        elementsCount = scoreboard->jobSize;
					    elementIndex = scoreboard->curIndex;
                        scoreboard->curIndex += scoreboard->jobSize;
                    } else {
                        elementsCount = scoreboard->inputSize - scoreboard->curIndex;
                        elementIndex = scoreboard->curIndex;
                        scoreboard->curIndex += elementsCount;
						scoreboard->isFinished = true;
                    }
                    // unlock scoreboard
                    scoreboard->scoreboardLock.unlock();

					// Process the data block
					// ----------------------
					for (int elementsFinished = 0; elementsFinished < elementsCount; elementsFinished++) {
						scoreboard->output->at(elementIndex+elementsFinished) = elemental.elemental(scoreboard->input->at(elementIndex+elementsFinished), args...);
					}
			}

		}

		// Constructor
		// -----------
		DynamicMapImplementation(Elemental<EL> elemental, size_t threads, size_t sizeOfChunk) : elemental(elemental) {
			this->nthreads = (threads != 0) ? threads : std::thread::hardware_concurrency();
			this->sizeOfChunk = sizeOfChunk ? sizeOfChunk : 100000;
			//this->BLOCK_FLAG_INITIAL_VALUE = 1;
		}

	public:
		// Paranthesis operator: call function
		// -----------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void operator()(std::vector<OUT> &output, std::vector<IN> &input, ARGs... args) {

			// Hardcoded for input sizes of 0 and 1
			// ------------------------------------
			if (input.size() == 0) { return; }
			if (input.size() == 1) { output[0] = elemental.elemental(input[0], args...); return; }



			std::thread *THREADS[nthreads];
			std::vector<OUT> tempOutput(input.size());
            Scoreboard<IN,OUT> *scoreboard = new Scoreboard<IN, OUT>();
			scoreboard->input = &input;
			scoreboard->output = &tempOutput;
			scoreboard->inputSize = input.size();
			scoreboard->jobSize = sizeOfChunk;

			// Run threads
			// -----------
			//std::cout << "RUNNING THREADS" << std::endl;
			for (size_t t = 0; t< nthreads; t++)
				THREADS[t] = new std::thread(&DynamicMapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, scoreboard, t, args...);

			// Join threads
			// ------------
			for (size_t t = 0; t< nthreads; ++t) { THREADS[t]->join(); delete THREADS[t]; }

			// Tidy-up after finish
			// --------------------
			output = tempOutput;
			delete scoreboard;
		}

		// Friend Functions for Dynamic Map Implementation Class
		// -----------------------------------------------------
		template<typename EL2>
		friend DynamicMapImplementation<EL2> __DynamicMapWithAccess(EL2 el, const size_t &threads, const size_t &sizeOfChunk);
	};

	// Friend Functions for Dynamic Map Skeleton Class
	// -----------------------------------------------
	template<typename EL2>
	friend DynamicMapImplementation<EL2> __DynamicMapWithAccess(EL2 el, const size_t &threads, const size_t &sizeOfChunk);
};

/*
* We cannot define a friend function with default argument
* that needs access to inner class on latest g++ compiler versions.
* We need a wrapper!
*/
template<typename EL>
DynamicMapSkeleton::DynamicMapImplementation<EL> __DynamicMapWithAccess(EL el, const size_t &threads, const size_t &sizeOfChunk) {
	return DynamicMapSkeleton::DynamicMapImplementation<EL>(el, threads, sizeOfChunk);
}

template<typename EL>
DynamicMapSkeleton::DynamicMapImplementation<EL> DynamicMap(EL el, const size_t &threads = 0, const size_t &sizeOfChunk = 0) {
	return __DynamicMapWithAccess(el, threads, sizeOfChunk);
}

#endif // !SLEDMAP_H
