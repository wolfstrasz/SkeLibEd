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
	// --------
	template<typename CO>
	class Combiner {
	public:
		Combiner(CO co) : combiner(co) {}

		CO combiner;
	};


public:
	// ReduceImplementation
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
			// ------------
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

			void barrier(size_t numberOfThreadsToBarrier) {

				auto lck = std::unique_lock<std::mutex>(*barrierLock);

				(*threadsArrived)++;
				if (*threadsArrived == numberOfThreadsToBarrier) {

					cond_var->notify_all();
					*threadsArrived = 0;
				}
				else {
					cond_var->wait(lck);
				}
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
		template<typename IN, typename ...ARGs>
		void threadReduce(ThreadArgument<IN> *threadArguments, size_t threadID, ARGs... args) {

			auto input = threadArguments[threadID].input;

			/**
			*
			* Phase 1
			*
			*/

			size_t assignedThreadID = threadID;
			do {

				threadArguments[assignedThreadID].signUpMutex->lock();
				if (threadArguments[assignedThreadID].chunkSignedThreads ==
					0) {// Zero signed-threads means this chunk's work is completed.
					threadArguments[assignedThreadID].signUpMutex->unlock();
					assignedThreadID = (assignedThreadID + 1) % nthreads;
					continue;
				}

				// thread is "signed" by default to its own chunck
				if (assignedThreadID != threadID) threadArguments[assignedThreadID].chunkSignedThreads++;
				threadArguments[assignedThreadID].signUpMutex->unlock();


				auto dataBlockMutexes = threadArguments[assignedThreadID].dataBlockMutexes;
				auto dataBlockFlags = threadArguments[assignedThreadID].dataBlockFlags;
				auto dataBlockIndices = threadArguments[assignedThreadID].dataBlockIndices;
				auto dataBlockReducedValues = threadArguments[assignedThreadID].dataBlockReducedValues;
				auto nDataBlocks = threadArguments[assignedThreadID].nDataBlocks;

				//why shouldn't we 'steal' even the first data block? :P
				size_t dataBlock = 0;

				while (dataBlock < nDataBlocks) {

					if (dataBlockFlags[dataBlock] ==
						0) {// if the data block has been, or being processed by another thread...
						++dataBlock;
						continue; //move on to the next data block.
					}

					dataBlockMutexes[dataBlock].lock();
					if (dataBlockFlags[dataBlock] == 1) {
						dataBlockFlags[dataBlock] = 0;
						dataBlockMutexes[dataBlock].unlock();

						IN accumulator, tempAccumulator1;

						accumulator = combiner.combiner(input->at(dataBlockIndices[dataBlock]),
							input->at(dataBlockIndices[dataBlock] + 1), args...);

						for (size_t elementIndex = dataBlockIndices[dataBlock] + 2;
							elementIndex < dataBlockIndices[dataBlock + 1]; ++elementIndex) {

							tempAccumulator1 = accumulator;

							accumulator = combiner.combiner(tempAccumulator1, input->at(elementIndex), args...);

							deleteIfPointer(tempAccumulator1);
						}

						dataBlockReducedValues[dataBlock] = accumulator;
					}
					else { // Just in case after the first if, the flag changes its value to 0 from another thread
						dataBlockMutexes[dataBlock].unlock();
					}

					++dataBlock;
				}

				/*
				* Thread will combine dataBlockReducedValues if it is the last "signed-up" thread
				*/
				threadArguments[assignedThreadID].signUpMutex->lock();
				threadArguments[assignedThreadID].chunkSignedThreads--;
				if (threadArguments[assignedThreadID].chunkSignedThreads == 0) {
					threadArguments[assignedThreadID].signUpMutex->unlock();

					if (nDataBlocks == 1) {
						threadArguments[assignedThreadID].stageOutput->at(assignedThreadID) = dataBlockReducedValues[0];
					}
					else {

						IN accumulator, tempAccumulator1;

						accumulator = combiner.combiner(dataBlockReducedValues[0], dataBlockReducedValues[1], args...);

						deleteIfPointer(dataBlockReducedValues[0]);
						deleteIfPointer(dataBlockReducedValues[1]);

						for (dataBlock = 2; dataBlock < nDataBlocks; ++dataBlock) {

							tempAccumulator1 = accumulator;

							accumulator = combiner.combiner(tempAccumulator1, dataBlockReducedValues[dataBlock],
								args...);

							deleteIfPointer(tempAccumulator1);
							deleteIfPointer(dataBlockReducedValues[dataBlock]);
						}

						threadArguments[assignedThreadID].stageOutput->at(assignedThreadID) = accumulator;
					}

				}
				else {
					threadArguments[assignedThreadID].signUpMutex->unlock();
				}

				assignedThreadID = (assignedThreadID + 1) % nthreads;

			} while (assignedThreadID != threadID);

			threadArguments[threadID].barrier(nthreads);

			/**
			*
			* Phase 2
			*
			*/
			size_t numberOfThreadsInStage = nthreads >> 1; //TODO: hardcoded?
			size_t offeset = 2; //TODO: hardcoded?
			size_t threadPair = 1;
			size_t carry = nthreads % 2 ? nthreads - 1 : 0; //TODO: hardcoded?
			IN accumulator, tempAccumulator1, tempAccumulator2;

			while (threadID < numberOfThreadsInStage) {

				tempAccumulator1 = threadArguments[threadID].stageOutput->at(threadID * offeset);
				tempAccumulator2 = threadArguments[threadID].stageOutput->at((threadID * offeset) + threadPair);

				accumulator = combiner.combiner(tempAccumulator1, tempAccumulator2, args...);

				deleteIfPointer(tempAccumulator1);
				deleteIfPointer(tempAccumulator2);

				threadArguments[threadID].stageOutput->at(threadID * offeset) = accumulator;

				if (threadID == 0 && carry) {

					tempAccumulator1 = threadArguments[threadID].stageOutput->at(threadID * offeset);
					tempAccumulator2 = threadArguments[threadID].stageOutput->at(carry);

					accumulator = combiner.combiner(tempAccumulator1, tempAccumulator2, args...);

					deleteIfPointer(tempAccumulator1);
					deleteIfPointer(tempAccumulator2);

					threadArguments[threadID].stageOutput->at(threadID * offeset) = accumulator;
					carry = 0;
				}

				threadArguments[threadID].barrier(numberOfThreadsInStage);

				if (threadID == 0 && numberOfThreadsInStage % 2) carry = (2 * numberOfThreadsInStage - 2) * threadPair;

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
			nthreads = nthreads * 2 > input.size() ? input.size() / 2 : nthreads;
			//std::cout << "nthreads is " << nthreads << std::endl;

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
			for (size_t t = 0; t < nthreads; ++t) {

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

				/*
				* Data Blocks
				*/
				/*times 2 so we can have at least 2 items per data block*/
				nDataBlocks =
					nDataBlocks * 2 > threadArguments[t].chunkSize ? threadArguments[t].chunkSize / 2 : nDataBlocks;

				threadArguments[t].nDataBlocks = nDataBlocks;

				threadArguments[t].signUpMutex = new std::mutex;

				threadArguments[t].dataBlockFlags = new unsigned char[nDataBlocks]();
				std::fill_n(threadArguments[t].dataBlockFlags, nDataBlocks, BLOCK_FLAG_INITIAL_VALUE);

				threadArguments[t].dataBlockMutexes = new std::mutex[nDataBlocks];

				threadArguments[t].dataBlockIndices = new size_t[nDataBlocks + 1]();

				size_t blockSize, blockStart = threadArguments[t].threadInputIndex, blockEnd;

				for (size_t block = 0; block < nDataBlocks; ++block) {

					if (block < (threadArguments[t].chunkSize % nDataBlocks)) blockSize = 1 + threadArguments[t].chunkSize / nDataBlocks;
					else blockSize = threadArguments[t].chunkSize / nDataBlocks;

					blockEnd = blockStart + blockSize;
					threadArguments[t].dataBlockIndices[block] = blockStart;
					blockStart = blockEnd;
				}
				threadArguments[t].dataBlockIndices[nDataBlocks] = blockEnd;

				threadArguments[t].dataBlockReducedValues = new IN[nDataBlocks];
			}

			// Run threads
			// -----------
			for (size_t t = 0; t < nthreads; ++t) {
				THREADS[t] = new std::thread(&ReduceImplementation<CO>::threadReduce<IN, ARGs...>, this, threadArguments, t, args...);
			}

			// Join threads
			// ------------
			for (size_t t = 0; t < nthreads; ++t) {
				THREADS[t]->join();
				delete THREADS[t];
			}

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

	// Friend Functions for Resude Skeleton CLass
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

	ReduceSkeleton::Combiner<CO> combiner(co);
	ReduceSkeleton::ReduceImplementation<CO> reduceImplementation(co, threads);

	return reduceImplementation;
}

template<typename CO>
ReduceSkeleton::ReduceImplementation<CO> Reduce(CO co, const size_t &threads = 0) { return __ReduceWithAccess(co, threads); }

#endif /* REDUCE_HPP */