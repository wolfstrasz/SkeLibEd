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
		size_t sizeOfWork;
	//	bool isInitialised;
		Elemental<EL> elemental;

		template<typename IN, typename OUT>
		class Scoreboard {
		public:
			// constructor
			Scoreboard() {
				this->isFinished = false;
				this->curIndex = 0;
			}
			void addWork(std::vector<IN> *in, std::vector<OUT> *out) {
				this->input = in;
				this->output = out;
				isFinished = false;
				inputSize = in->size();
				curIndex = 0;
			}

			// input output
			std::vector<OUT> *output;
			std::vector<IN> *input;

			// detect global work
			bool isFinished;

			// detect next work
			size_t inputSize;
			size_t curIndex;

			// scoreboard worksize
			size_t itemsCount;

			// guard
			std::mutex scoreboardInUse;

		};

		// ThreadMap - function applied to each thread
		// --------------------------------------------
		//	THREADS[t] = new std::thread(&MapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, threadArguments, t, args...);
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(Scoreboard<IN, OUT> *scoreboard, size_t threadID, ARGs... args) {

			size_t elementsCount;
			size_t elementIndex;
			while (!scoreboard->isFinished) {

				// Lock scoreboard
				while (!scoreboard->scoreboardInUse.try_lock());

				if (scoreboard->isFinished) {
					scoreboard->scoreboardInUse.unlock();
					break;
				}

				// get new data
				if (scoreboard->curIndex + scoreboard->itemsCount < scoreboard->inputSize) {
					elementsCount = scoreboard->itemsCount;
					elementIndex = scoreboard->curIndex;
					scoreboard->curIndex += scoreboard->itemsCount;
				}
				else {
					elementsCount = scoreboard->inputSize - scoreboard->curIndex;
					elementIndex = scoreboard->curIndex;
					scoreboard->curIndex += elementsCount;
					scoreboard->isFinished = true;
				}
				// unlock scoreboard
				scoreboard->scoreboardInUse.unlock();

				// Process the data block
				// ----------------------
				for (int elementsFinished = 0; elementsFinished < elementsCount; elementsFinished++) {
					scoreboard->output->at(elementIndex + elementsFinished) = elemental.elemental(scoreboard->input->at(elementIndex + elementsFinished), args...);
				}
			}

		}

		// Constructor
		// -----------
		DynamicMapImplementation(Elemental<EL> elemental) : elemental(elemental) {
			this->nthreads = std::thread::hardware_concurrency();
			this->sizeOfWork = 1000;
	//		this->isInitialised = false;
		}

	public:
		// Paranthesis operator: call function
		// -----------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void operator()(std::vector<OUT> &output, std::vector<IN> &input, ARGs... args) {

		//	if (!isInitialised) {

			sizeOfWork = input.size() / (nthreads * 16);
			std::thread *THREADS[nthreads];
			Scoreboard<IN, OUT> *scoreboard = new Scoreboard<IN, OUT>();
			scoreboard->addWork(&input, &output);
			scoreboard->itemsCount = sizeOfWork;

			// Run threads
			// -----------
			//std::cout << "RUNNING THREADS" << std::endl;
			for (size_t t = 0; t < nthreads; t++)
				THREADS[t] = new std::thread(&DynamicMapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, scoreboard, t, args...);
			
		//		isInitialised = true;
	//		}
				// Join threads
			// ------------
			for (size_t t = 0; t < nthreads; ++t) { THREADS[t]->join(); delete THREADS[t]; }

			delete scoreboard;

		}

		bool stop() {
			// Join threads
			// ------------
//			for (size_t t = 0; t < nthreads; ++t) { THREADS[t]->join(); delete THREADS[t]; }

		//	delete scoreboard;
		}

		// Utility functions for setting options
		// ------------------------------------
		void setNumberOfThreads(size_t nthreads) { this->nthreads = nthreads; }
		void setSizeOfChunk(size_t sizeOfChunk) { this->sizeOfChunk = sizeOfChunk; }

		// Friend Functions for Dynamic Map Implementation Class
		// -----------------------------------------------------
		template<typename EL2>
		friend DynamicMapImplementation<EL2> __DynamicMapWithAccess(EL2 el);
	};

	// Friend Functions for Dynamic Map Skeleton Class
	// -----------------------------------------------
	template<typename EL2>
	friend DynamicMapImplementation<EL2> __DynamicMapWithAccess(EL2 el);
};

/*
* We cannot define a friend function with default argument
* that needs access to inner class on latest g++ compiler versions.
* We need a wrapper!
*/
template<typename EL>
DynamicMapSkeleton::DynamicMapImplementation<EL> __DynamicMapWithAccess(EL el) {
	return DynamicMapSkeleton::DynamicMapImplementation<EL>(el);
}

template<typename EL>
DynamicMapSkeleton::DynamicMapImplementation<EL> DynamicMap(EL el) {
	return __DynamicMapWithAccess(el);
}

#endif // !SLEDMAP_H