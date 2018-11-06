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

		class Scoreboard {
		public:
			bool isFinished;
			size_t inputSize;
			size_t curIndex;
			size_t itemsCount;
			std::queue <size_t> finishedWorkers;
			int finishedJobs;
			int sentJobs;
		};

		// ThreadArgument
		// --------------
		template<typename IN, typename OUT>
		class ThreadArgument {
		public:
			size_t threadInputIndex;
			size_t elementsCount;
			std::vector<IN> *input;
			std::vector<OUT> *output;
			bool isThereWork;

			// Constructors
			ThreadArgument() {}
			ThreadArgument(std::vector<OUT> &output, std::vector<IN> &input, size_t threadInputIndex, size_t elementsCount, bool isThereWork)
				: threadInputIndex(threadInputIndex), elementsCount(elementsCount), input(&input), output(&output),isThereWork(isThereWork) {}
			// Destructor
			~ThreadArgument() {
				//delete[] dataBlockIndices;
				//delete[] dataBlockFlags;
				//delete dataBlockMutex;
			}

		};

		// ThreadMap - function applied to each thread
		// --------------------------------------------
		//	THREADS[t] = new std::thread(&MapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, threadArguments, t, args...);
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(Scoreboard *scoreboard, ThreadArgument<IN, OUT> *threadArguments, size_t threadID, ARGs... args) {

			auto input = threadArguments[threadID].input;
			auto output = threadArguments[threadID].output;

			while (!scoreboard->isFinished) {
				if (threadArguments[threadID].isThereWork) {

					size_t elementsCount = threadArguments[threadID].elementsCount;
					size_t elementIndex = threadArguments[threadID].threadInputIndex;
					// Process the data block
					// ----------------------
					for (int elementsFinished = 0; elementsFinished < elementsCount; elementsFinished++) {
						output->at(elementIndex+elementsFinished) = elemental.elemental(input->at(elementIndex+elementsFinished), args...);
					}

					threadArguments[threadID].isThereWork = false;
					scoreboard->finishedWorkers.push(threadID);
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
			std::cout << "Thread count: " <<nthreads<< std::endl;

			Scoreboard *scoreboard = new Scoreboard;

			scoreboard->isFinished = false;
			scoreboard->inputSize = input.size();
			scoreboard->curIndex = 0;
			scoreboard->itemsCount = this->sizeOfChunk;
			scoreboard->sentJobs = 0;
			scoreboard->finishedJobs = 0;
			// Hardcoded for input sizes of 0 and 1
			// ------------------------------------
			if (input.size() == 0) { return; }
			if (input.size() == 1) { output[0] = elemental.elemental(input[0], args...); return; }


			// Generate Threads, Thread Arguments, Temporary Ouput Vector
			// ----------------------------------------------------------
			std::thread *THREADS[nthreads];

			ThreadArgument<IN, OUT> *threadArguments = new ThreadArgument<IN, OUT>[nthreads];
			std::vector<OUT> tempOutput(input.size());

			// Assign proper data chunks to thread arguments
			// ---------------------------------------------
			for (size_t t = 0; t < nthreads; t++) {

				threadArguments[t].input = &input;
				threadArguments[t].output = &tempOutput;
				threadArguments[t].isThereWork = true;
				threadArguments[t].elementsCount = scoreboard->itemsCount;
				threadArguments[t].threadInputIndex = scoreboard->curIndex;
				//chunkIndex += sizeOfChunk;
				scoreboard->curIndex += scoreboard->itemsCount;
				scoreboard->sentJobs ++;
			}
			// Run threads
			// -----------
			for (size_t t = 0; t< nthreads; t++)
				THREADS[t] = new std::thread(&DynamicMapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, scoreboard, threadArguments, t, args...);


			// Assign new jobs until work is done
			// ----------------------------------
			while (!scoreboard->isFinished) {

				// Check for free thread
				if (!scoreboard->finishedWorkers.empty()) {
					size_t threadID = scoreboard->finishedWorkers.front();
					scoreboard->finishedWorkers.pop();
					scoreboard->finishedJobs ++;
					if (scoreboard->curIndex == scoreboard->inputSize){

					} else if (scoreboard->curIndex + scoreboard->itemsCount < scoreboard->inputSize) {
						threadArguments[threadID].elementsCount = scoreboard->itemsCount;
						threadArguments[threadID].threadInputIndex = scoreboard->curIndex;
						scoreboard->curIndex += scoreboard->itemsCount;
						threadArguments[threadID].isThereWork = true;
					} else {
						size_t lastItemsCount = scoreboard->inputSize - scoreboard->curIndex;
						threadArguments[threadID].elementsCount = lastItemsCount;
						threadArguments[threadID].threadInputIndex = scoreboard->curIndex;
						scoreboard->curIndex += lastItemsCount;
						threadArguments[threadID].isThereWork = true;
					}
				}

				// Check if job will be finished
				if (scoreboard->finishedJobs == scoreboard->sentJobs)
					scoreboard->isFinished = true;

			}

			// Join threads
			// ------------
			for (size_t t = 0; t< nthreads; ++t) { THREADS[t]->join(); delete THREADS[t]; }

			// Tidy-up after finish
			// --------------------
			output = tempOutput;
			delete[] threadArguments;
			delete scoreboard;
		}

		// Utility functions for setting options
		// ------------------------------------
		//void setNumberOfBlocks(size_t nblocks) { this->nDataBlocks = nblocks; }
		void setNumberOfThreads(size_t nthreads) { this->nthreads = nthreads; }
		void setSizeOfChunk(size_t sizeOfChunk) { this->sizeOfChunk = sizeOfChunk;  }
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
