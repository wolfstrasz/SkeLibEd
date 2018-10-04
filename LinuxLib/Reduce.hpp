#ifndef REDUCE_HPP
#define REDUCE_HPP
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
#include <condition_variable>
#include <stack>

class ReduceSkeleton {

private:
	ReduceSkeleton() {}

	// Combiner - function used in reducing
	// ------------------------------------
	template<typename CO>
	class Combiner {
	public:
		Combiner(CO co) : combiner(co) {}
		CO combiner;
	};

public:
	// ReduceImplementation
	// --------------------
	template<typename CO>
	class ReduceImplementation {

	private:
		unsigned char BLOCK_FLAG_INITIAL_VALUE;
		size_t nthreads;
		size_t nDataBlocks;
		Combiner<CO> combiner;

		// ThreadArgument
		// --------------
		template<typename IN>
		class ThreadArgument {
			public:
			size_t * threadsArrived;
			std::mutex *barrierLock;
			std::condition_variable *cond_var;

			size_t threadInputIndex;
			size_t chunkSize;

			size_t nDataBlocks;
			size_t *dataBlockIndices;
			std::mutex *dataBlockMutexes;

			size_t chunkSignedThreads;
			std::mutex *signUpMutex;

			unsigned char *dataBlockFlags;
			IN *dataBlockReducedValues;

			std::vector<IN> *input;
			std::vector<IN> *stageOutput;

			// Constructors
			ThreadArgument() {
				this->chunkSignedThreads = 1;
			}
			ThreadArgument(std::vector<IN> &stageOutput, std::vector<IN> &input, size_t threadInputIndex, size_t chunkSize)
				: threadInputIndex(threadInputIndex), chunkSize(chunkSize), input(&input), stageOutput(&stageOutput) {
				this->chunkSignedThreads = 1;
			}
			// Destructor
			~ThreadArgument() {
				delete[] dataBlockIndices;
				delete[] dataBlockFlags;
				delete[] dataBlockReducedValues;
				delete[] dataBlockMutexes;
				delete signUpMutex;
			}

			// Synchronization barrier lock - sends notification
			// to all threads when they can continue after Phase 1
			// ---------------------------------------------------
			void barrier(size_t numberOfThreadsToBarrier) {
				auto lock = std::unique_lock<std::mutex>(*barrierLock);

				(*threadsArrived)++;
				if (*threadsArrived == numberOfThreadsToBarrier) {
					cond_var->notify_all();
					*threadsArrived = 0;
				}
				else cond_var->wait(lock);

			}

		};

		// Utility functions to destroy pointers
		// -------------------------------------
		template<typename T>
		inline void deleteIfPointer(T *&p) { delete p; };
		template<typename T>
		inline void deleteIfPointer(T) { return; };

		// ThreadReduce - functionality of the reduce pattern
		// --------------------------------------------------
		// THREADS[t] = new std::thread(&ReduceImplementation<CO>::threadReduce<IN, ARGs...>, this, threadArguments, t, args...);
		template<typename IN, typename ...ARGs>
		void threadReduce(ThreadArgument<IN> *threadArguments, size_t threadID, ARGs... args) {

			auto input = threadArguments[threadID].input;


			// -----------------------------------------------------------------------------------------
			// PHASE 1 - Reducing first level data to number of threads data items called reduced values
			// -----------------------------------------------------------------------------------------
			size_t assignedThreadID = threadID; // Assigns first job to be helping itself
			do {

				// Lock for sign-up and look for work
				// ----------------------------------
				threadArguments[assignedThreadID].signUpMutex->lock();

				// Zero signed-threads means this chunk's work is completed.
				// --------------------------------------------------------
				if (threadArguments[assignedThreadID].chunkSignedThreads ==	0) {
					threadArguments[assignedThreadID].signUpMutex->unlock();
					assignedThreadID = (assignedThreadID + 1) % nthreads;
					continue;
				}

				// If thread is assisting another thread => mark as signed for primary thread
				// --------------------------------------------------------------------------
				if (assignedThreadID != threadID)
					threadArguments[assignedThreadID].chunkSignedThreads++;
				threadArguments[assignedThreadID].signUpMutex->unlock();

				// Assign new variables for data for easier reading
				// ------------------------------------------------
				auto dataBlockMutexes = threadArguments[assignedThreadID].dataBlockMutexes;
				auto dataBlockFlags = threadArguments[assignedThreadID].dataBlockFlags;
				auto dataBlockIndices = threadArguments[assignedThreadID].dataBlockIndices;
				auto dataBlockReducedValues = threadArguments[assignedThreadID].dataBlockReducedValues;
				auto nDataBlocks = threadArguments[assignedThreadID].nDataBlocks;

				// Starts to iterate over data blocks of given thread (assisting / not)
				// --------------------------------------------------------------------
				size_t dataBlock = 0;
				while (dataBlock < nDataBlocks) {

					// Lock and check if block is has not been processed already
					// ---------------------------------------------------------
					dataBlockMutexes[dataBlock].lock();

					if (dataBlockFlags[dataBlock] == 1) {
						dataBlockFlags[dataBlock] = 0; // Set as processed
						dataBlockMutexes[dataBlock].unlock();

						// Assign first input to accumulating variable (accumulator)
						// --------------------------------------------------------
						IN accumulator = input->at(dataBlockIndices[dataBlock]);

						// Process the data block
						// ----------------------
						for (size_t elementIndex = dataBlockIndices[dataBlock] + 1;
								elementIndex < dataBlockIndices[dataBlock + 1];
									elementIndex++)
							accumulator = combiner.combiner(accumulator, input->at(elementIndex), args...);

						// Assign final reduced variable of block to dataBlockReducedValues
						// ----------------------------------------------------------------
						dataBlockReducedValues[dataBlock] = accumulator;
					} else dataBlockMutexes[dataBlock].unlock();

					dataBlock++;
				}

				// Thread will combine dataBlockReducedValues if it is the last "signed-up" thread
				// -------------------------------------------------------------------------------
				threadArguments[assignedThreadID].signUpMutex->lock();
				threadArguments[assignedThreadID].chunkSignedThreads--;
				if (threadArguments[assignedThreadID].chunkSignedThreads == 0) {
					threadArguments[assignedThreadID].signUpMutex->unlock();
					// Assign first reduced value to accumulating variable (accumulator)
					// -----------------------------------------------------------------
					IN accumulator = dataBlockReducedValues[0];
					deleteIfPointer(dataBlockReducedValues[0]);

					// Process the reduced values
					// ----------------------------
					for (dataBlock = 1; dataBlock < nDataBlocks; dataBlock++){
						accumulator = combiner.combiner(accumulator,
							 							dataBlockReducedValues[dataBlock],
														args...
														);
						deleteIfPointer(dataBlockReducedValues[dataBlock]);
					}

					// Assign final stageOutput
					// ------------------------
					threadArguments[assignedThreadID].stageOutput->at(assignedThreadID) = accumulator;
				} else threadArguments[assignedThreadID].signUpMutex->unlock();

				// When finished working on the available data blocks of a given thread
				// -> continue to search for other work
				// -------------------------------------
				assignedThreadID = (assignedThreadID + 1) % nthreads;
			} while (assignedThreadID != threadID);

			// -----------------------------------------------------------------------------------------
			// PHASE 2 - Reducing the reduced values with a tree
			// -----------------------------------------------------------------------------------------

			// Check for synchronization before continuing
			// -------------------------------------------
			threadArguments[threadID].barrier(nthreads);

			// Arguments for each stage of Phase 2
			// -----------------------------------
			size_t numberOfThreadsInStage = nthreads >> 1; 						//TODO: hardcoded?
			size_t offeset = 2; 												//TODO: hardcoded?
			size_t carry = nthreads % 2 ? nthreads - 1 : 0; 					//TODO: hardcoded?
			size_t threadPair = 1;
			IN accumulator;
			// Start tree reduction
			// --------------------
			while (threadID < numberOfThreadsInStage) {	// Chooses if thread is going to be active -> dependent on how much are needed as active

				// Combine every pair of data items (store them in the first's data item place)
				// ----------------------------------------------------------------------------
				accumulator = combiner.combiner(threadArguments[threadID].stageOutput->at(threadID * offeset),
				 								threadArguments[threadID].stageOutput->at((threadID * offeset) + threadPair),
												args...
											 	);
				threadArguments[threadID].stageOutput->at(threadID * offeset) = accumulator;

				// If uneven amount of values are present -> we have a carry that has to be added
				// ------------------------------------------------------------------------------
				if (threadID == 0 && carry) {
					accumulator = combiner.combiner(threadArguments[threadID].stageOutput->at(threadID * offeset),
					 								threadArguments[threadID].stageOutput->at(carry),
													args...);
					threadArguments[threadID].stageOutput->at(threadID * offeset) = accumulator;
					carry = 0;
				}

				//
				threadArguments[threadID].barrier(numberOfThreadsInStage);

				if (threadID == 0 && numberOfThreadsInStage % 2)
					carry = (2 * numberOfThreadsInStage - 2) * threadPair;

				offeset <<= 1;
				threadPair <<= 1;
				numberOfThreadsInStage >>= 1;
			}
		}

		// Constructor
		// -----------
		ReduceImplementation(Combiner<CO> combiner, size_t threads) : combiner(combiner), nthreads(threads) {
			this->nDataBlocks = 10;
			this->BLOCK_FLAG_INITIAL_VALUE = 1;
		}

	public:
		// Overide the paranthesis operator
		// --------------------------------
		template<typename IN, typename ...ARGs>
		void operator()(IN &output, std::vector<IN> &input, ARGs... args) {

			// Optimization to best number of threads
			// --------------------------------------
			nthreads = nthreads ? nthreads : std::thread::hardware_concurrency();
			nthreads = nthreads * 2 > input.size() ? input.size() / 2 : nthreads;		// x2 because we need atleast 2 items per junk

			// Hardcoded for input sizes of 0 and 1
			// ------------------------------------
			if (input.size() == 0) { return; }
			if (input.size() == 1) { output = input[0]; return; }

			// When input size is 3 we need only 1 thread for reduction
			// --------------------------------------------------------
			if (input.size() < 4) {
				IN accumulator = combiner.combiner(input[0], input[1], args...);
				output = combiner.combiner(accumulator, input[2], args...);
				deleteIfPointer(accumulator);
				return;
			}

			// Generate Threads, Thread Arguments, Temporary Output Vector
			// -------------------------------------
			std::thread *THREADS[nthreads];
			ThreadArgument<IN> *threadArguments = new ThreadArgument<IN>[nthreads];
			std::vector<IN> stageOutput(nthreads);

			// Generate communications variables
			size_t threadsArrived = 0;
			std::mutex barrierLock;
			std::condition_variable cond_var;

			// Assign proper data chunks to thread arguments
			// ---------------------------------------------
			size_t chunkIndex = 0;
			for (size_t t = 0; t < nthreads; t++) {

				// Calculate size of chunks			// When data can't be divided in equal chunks we must increase the
				// ------------------------			// data processed by the first DataSize(mod ThreadCount) threads.
				if (t < (input.size() % nthreads)) threadArguments[t].chunkSize = 1 + input.size() / nthreads;
				else threadArguments[t].chunkSize = input.size() / nthreads;

				// Assign communication variables
				// ------------------------------
				threadArguments[t].threadsArrived = &threadsArrived;
				threadArguments[t].barrierLock = &barrierLock;
				threadArguments[t].cond_var = &cond_var;

				// Assign general variables
				// ------------------------
				threadArguments[t].input = &input;
				threadArguments[t].stageOutput = &stageOutput;
				threadArguments[t].threadInputIndex = chunkIndex;

				// Shift chunk index for next thread argument
				// ------------------------------------------
				chunkIndex += threadArguments[t].chunkSize;

				// Calculate number of data blocks for the thread
				// ----------------------------------------------
				nDataBlocks = nDataBlocks * 2 > threadArguments[t].chunkSize ? threadArguments[t].chunkSize / 2 : nDataBlocks; // times 2 so we can have at least 2 items per data block

				// Generate data blocks arguments for the thread
				// ---------------------------------------------
				threadArguments[t].nDataBlocks = nDataBlocks;
				threadArguments[t].signUpMutex = new std::mutex;
				threadArguments[t].dataBlockMutexes = new std::mutex[nDataBlocks];
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
				// Generate new partial reduced values for the thread argument
				threadArguments[t].dataBlockReducedValues = new IN[nDataBlocks];
			}

			// Run threads
			// -----------
			for (size_t t = 0; t < nthreads; ++t)
				THREADS[t] = new std::thread(&ReduceImplementation<CO>::threadReduce<IN, ARGs...>, this, threadArguments, t, args...);

			// Join threads
			// ------------
			for (size_t t = 0; t < nthreads; ++t) { THREADS[t]->join(); delete THREADS[t]; }

			// Tidy up after finishing
			// -----------------------
			output = threadArguments[0].stageOutput->at(0);
			delete[] threadArguments;
		}

		// Friend Functions for Reduce Implementation Class
		// ------------------------------------------------
		template<typename CO2>
		friend ReduceSkeleton::ReduceImplementation<CO2> __ReduceWithAccess(CO2 co, const size_t &threads);
	};

	// Friend Functions for Reduce Skeleton CLass
	// ------------------------------------------
	template<typename CO2>
	friend ReduceSkeleton::ReduceImplementation<CO2> __ReduceWithAccess(CO2 co, const size_t &threads);
};

/*
* We cannot define a friend function with default argument
* that needs access to inner class on latest g++ compiler versions.
* We need a wrapper!
*/
template<typename CO>
ReduceSkeleton::ReduceImplementation<CO> __ReduceWithAccess(CO co, const size_t &threads) {
	return ReduceSkeleton::ReduceImplementation<CO> (co, threads);
}

template<typename CO>
ReduceSkeleton::ReduceImplementation<CO> Reduce(CO co, const size_t &threads = 0) {
	return __ReduceWithAccess(co, threads);
}

#endif /* REDUCE_HPP */
