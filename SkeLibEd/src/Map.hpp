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

		// ThreadMap - function applied to each thread
		// --------------------------------------------
		//	THREADS[t] = new std::thread(&MapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, threadArguments, t, args...);
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(ThreadArgument<IN, OUT> *threadArguments, size_t threadID, ARGs... args) {

			auto input = threadArguments[threadID].input;
			auto output = threadArguments[threadID].output;

			size_t assistedThreadID = threadID; // Assigns first job to be helping itself
			do {

				// Assign new variables for data for easier reading
				// ------------------------------------------------
				size_t			nDataBlocks		 = threadArguments[assistedThreadID].nDataBlocks;
				std::mutex*		dataBlockMutex	 = threadArguments[assistedThreadID].dataBlockMutex;
				unsigned char*	dataBlockFlags	 = threadArguments[assistedThreadID].dataBlockFlags;
				std::size_t*	dataBlockIndices = threadArguments[assistedThreadID].dataBlockIndices;


				// Starts to iterate over data blocks of given thread (assisting / not)
				// --------------------------------------------------------------------
				size_t dataBlock = 0;
				while (dataBlock < nDataBlocks) {

					// Lock and check if block is has not been processed already
					// ---------------------------------------------------------
					dataBlockMutex->lock();

					if (dataBlockFlags[dataBlock] == 1) {
						dataBlockFlags[dataBlock] = 0;	// Set as processed
						dataBlockMutex->unlock();

						// Process the data block
						// ----------------------
						for (size_t elementIndex = dataBlockIndices[dataBlock];
								elementIndex < dataBlockIndices[dataBlock + 1];
									elementIndex++)
							output->at(elementIndex) = elemental.elemental(input->at(elementIndex), args...);

					} else dataBlockMutex->unlock();

					dataBlock++;
				}

				// When finished working on the available data blocks of a given thread
				// -> continue to search for other work
				// -------------------------------------
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
			for (size_t t = 0; t < nthreads; t++) {

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

				// Assign data block info to thread argument
				// -----------------------------------------
				size_t blockStart = threadArguments[t].threadInputIndex;
				size_t blockSize;

				for (size_t block = 0; block < nDataBlocks; block++) {
					// Assign block index
					threadArguments[t].dataBlockIndices[block] = blockStart;

					// Calculate block size for the current block
					if (block < (threadArguments[t].chunkSize % nDataBlocks)) blockSize = 1 + threadArguments[t].chunkSize / nDataBlocks;
					else blockSize = threadArguments[t].chunkSize / nDataBlocks;

					// Shift block start index for next iteration
					blockStart += blockSize;
				}
				// Assign index for last block
				threadArguments[t].dataBlockIndices[nDataBlocks] = blockStart;
			}

			// Run threads
			// -----------
			for (size_t t = 0; t< nthreads; ++t)
				THREADS[t] = new std::thread(&MapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, threadArguments, t, args...);

			// Join threads
			// ------------
			for (size_t t = 0; t< nthreads; ++t) { THREADS[t]->join(); delete THREADS[t]; }

			// Tidy up after finishing
			// -----------------------
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
	return MapSkeleton::MapImplementation<EL> (el, threads);
}

template<typename EL>
MapSkeleton::MapImplementation<EL> Map(EL el, const size_t &threads = 0) {
	return __MapWithAccess(el, threads);
}

#endif // !SLEDMAP_H
