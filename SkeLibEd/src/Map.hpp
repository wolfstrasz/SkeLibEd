#ifndef MAP_H
#define MAP_H
#include <cstdlib>
#include <iostream>
#include <vector>
#include <type_traits>
#include <functional>
#include <stdarg.h>
#include <typeinfo>

#include <utility>
#include <thread>
#include <mutex>

class MapSkeleton {

private:
	MapSkeleton() {}

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
	class MapImplementation {

	private:
		unsigned char BLOCK_FLAG_INITIAL_VALUE;
		size_t nthreads;
		size_t nDataBlocks;
		Elemental<EL> elemental;

		// ThreadArgument
		// --------------
		template<typename IN, typename OUT>
		class ThreadArgument {

		public:
			size_t threadInputIndex;
			size_t chunkSize;

			size_t nDataBlocks;
			std::mutex *dataBlockMutex;
			unsigned char *dataBlockFlags;
			size_t *dataBlockIndices;

			std::vector<IN> *input;
			std::vector<OUT> *output;

			// Constructors
			ThreadArgument() {}
			ThreadArgument(std::vector<OUT> &output, std::vector<IN> &input, size_t threadInputIndex, size_t chunkSize)
				: threadInputIndex(threadInputIndex), chunkSize(chunkSize), input(&input), output(&output) {}
			// Destructor
			~ThreadArgument() {
				delete[] dataBlockIndices;
				delete[] dataBlockFlags;
				delete dataBlockMutex;
			}

		};

		// ThreadMap - functionality of the Map pattern
		// --------------------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(ThreadArgument<IN, OUT> *threadArguments, size_t threadID, ARGs... args) {

			auto input = threadArguments[threadID].input;
			auto output = threadArguments[threadID].output;

			std::cout << "Thread here " << threadID << std::endl;

			size_t assistedThreadID = threadID;
			do {

				std::mutex* dataBlockMutex = threadArguments[assistedThreadID].dataBlockMutex;
				unsigned char* dataBlockFlags = threadArguments[assistedThreadID].dataBlockFlags;
				std::size_t* dataBlockIndices = threadArguments[assistedThreadID].dataBlockIndices;
				size_t nDataBlocks = threadArguments[assistedThreadID].nDataBlocks;

				size_t dataBlock = 0;

				// why shouldn't we 'steal' even the first dataBlock? :P
				while (dataBlock < nDataBlocks) {

					if (dataBlockFlags[dataBlock] == 0 // if the data block has been, or being processed by another thread...
						|| dataBlockIndices[dataBlock] == dataBlockIndices[dataBlock + 1]) {
						++dataBlock;
						continue; //as we iterate in reverse -> continue. In case there are no more dataBlocks, as chunkSize might be less than NDATABLOCKS
					}

					dataBlockMutex->lock();
					if (dataBlockFlags[dataBlock] == 1) { // were the flag is zero, the following flags are zero too.
						dataBlockFlags[dataBlock] = 0;
						dataBlockMutex->unlock();

						for (size_t elementIndex = dataBlockIndices[dataBlock]; elementIndex < dataBlockIndices[dataBlock + 1]; ++elementIndex) {
							output->at(elementIndex) = elemental.elemental(input->at(elementIndex), args...);
						}
					}
					else { // Just in case after the first if, the flag changes its value to 0 from another thread
						dataBlockMutex->unlock();
					}
					++dataBlock;
				}

				assistedThreadID = (assistedThreadID + 1) % nthreads;
			} while (assistedThreadID != threadID);
		}

		// Constructor
		// -----------
		MapImplementation(Elemental<EL> elemental, size_t threads) : elemental(elemental), nthreads(threads) {
			this->nDataBlocks = 3000;
			// this->nDataBlocks = 1; MIC! was 10
			this->BLOCK_FLAG_INITIAL_VALUE = 1;
		}

	public:
		// Overide the paranthesis operator
		// --------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void operator()(std::vector<OUT> &output, std::vector<IN> &input, ARGs... args) {

			// Optimization to best number of threads
			// --------------------------------------
			nthreads = nthreads ? nthreads : std::thread::hardware_concurrency();
			nthreads = nthreads >= input.size() ? input.size() / 2 : nthreads;
			//std::cout << "nthreads is " << nthreads << std::endl;

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
			size_t chunkIndex = 0;
			for (size_t t = 0; t< nthreads; ++t) {

				// Calculate size of chunks			// When data can't be divided in equal chunks we must increase the
				// ------------------------			// data processed by the first DataSize(mod ThreadCount) threads.
				if (t < (input.size() % nthreads)) threadArguments[t].chunkSize = 1 + input.size() / nthreads;
				else threadArguments[t].chunkSize = input.size() / nthreads;

				// Assign general variables
				// ------------------------
				threadArguments[t].input = &input;
				threadArguments[t].output = &tempOutput;
				threadArguments[t].threadInputIndex = chunkIndex;

				// Shift chunk index for next thread argument
				// ------------------------------------------
				chunkIndex += threadArguments[t].chunkSize;

				/*
				* Data Blocks
				*/
				// Calculate number of data blocks for the thread
				// ----------------------------------------------
				nDataBlocks = nDataBlocks > threadArguments[t].chunkSize ? threadArguments[t].chunkSize / 2 : nDataBlocks;

				// Generate data blocks arguments for the thread
				// ---------------------------------------------
				threadArguments[t].nDataBlocks = nDataBlocks;
				threadArguments[t].dataBlockMutex = new std::mutex;
				threadArguments[t].dataBlockIndices = new size_t[nDataBlocks + 1]();
				threadArguments[t].dataBlockFlags = new unsigned char[nDataBlocks]();
				std::fill_n(threadArguments[t].dataBlockFlags, nDataBlocks, BLOCK_FLAG_INITIAL_VALUE);

				// Assign data block info to thread
				// --------------------------------
				size_t blockStart = threadArguments[t].threadInputIndex;
				size_t blockSize;
				for (size_t block = 0; block < nDataBlocks; ++block) {

					// Calculate block size for the current data block 
					if (block < (threadArguments[t].chunkSize % nDataBlocks)) blockSize = 1 + threadArguments[t].chunkSize / nDataBlocks;
					else blockSize = threadArguments[t].chunkSize / nDataBlocks;

					// Assign info
					threadArguments[t].dataBlockIndices[block] = blockStart;

					// Shift block start index
					blockStart += blockSize;
				}
				threadArguments[t].dataBlockIndices[nDataBlocks] = blockStart;
			}

			// Run threads
			// -----------
			for (size_t t = 0; t< nthreads; ++t) {
				THREADS[t] = new std::thread(&MapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, threadArguments, t, args...);
			}

			// Join threads
			// ------------
			for (size_t t = 0; t< nthreads; ++t) {
				THREADS[t]->join();
				delete THREADS[t];
			}

			// Tidy up after finish
			// --------------------
			output = tempOutput;
			delete[] threadArguments;
		}

		// Friend Functions for Map Implementation Class
		// ---------------------------------------------
		template<typename EL2>
		friend MapImplementation<EL2> __MapWithAccess(EL2 el, const size_t &threads);
	};

	// Friend Functions for Map Skeleton Class
	// ---------------------------------------
	template<typename EL2>
	friend MapImplementation<EL2> __MapWithAccess(EL2 el, const size_t &threads);
};

/*
* We cannot define a friend function with default argument
* that needs access to inner class on latest g++ compiler versions.
* We need a wrapper!
*/
template<typename EL>
MapSkeleton::MapImplementation<EL> __MapWithAccess(EL el, const size_t &threads) {

	MapSkeleton::Elemental<EL> elemental(el);
	MapSkeleton::MapImplementation<EL> map(elemental, threads);

	return map;
}

template<typename EL>
MapSkeleton::MapImplementation<EL> Map(EL el, const size_t &threads = 0) {

	return __MapWithAccess(el, threads);
}

#endif // !SLEDMAP_H