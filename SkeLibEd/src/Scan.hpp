#ifndef SKELETONS_SCAN_HPP
#define SKELETONS_SCAN_HPP
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

class ScanSkeleton {

private:
	ScanSkeleton() {}
	
	// Successor: function used on separate items
	// ------------------------------------------
	template<typename SU>
	class Successor {
	public:
		Successor(SU su) : successor(su) {}
		SU successor;
	};

public:
	// ScanImplementation: functionality of the Scan pattern
	// -----------------------------------------------------
	template<typename SU>
	class ScanImplementation {

	private:
		unsigned char BLOCK_FLAG_INITIAL_VALUE;
		size_t nthreads;
		size_t nDataBlocks;
		Successor<SU> successor;

		// ScanTreeNode: ...
		// ---------------------------
		template<typename IN>
		class ScanTreeNode {
		public:

			IN sum;
			IN* leftSum;
			size_t range;

			bool deleteLeftSumContent;

			ScanTreeNode<IN>* leftChild;
			ScanTreeNode<IN>* rightChild;

			std::mutex *nodeLock;
			std::condition_variable *nodeCond_var;

			ScanTreeNode() {

				deleteLeftSumContent = false;

				leftChild = NULL;
				rightChild = NULL;

				leftSum = NULL;

				nodeCond_var = new std::condition_variable();
			}

			~ScanTreeNode() {

				deleteIfPointer(sum);

				if (leftSum) {
					if (deleteLeftSumContent) deleteIfPointer(*leftSum);
					delete leftSum;
				}

				if (leftChild) leftChild->leftSum = NULL;

				if (leftChild) delete leftChild;
				if (rightChild) delete rightChild;

				delete nodeCond_var;
			}

			void nodeWait(std::unique_lock<std::mutex> &lck) {

				nodeCond_var->wait(lck);

			}

			void nodeSignal() {

				nodeCond_var->notify_one();
			}

		};

		// ThreadArgument: keeps all data for each thread
		// ----------------------------------------------
		template<typename IN>
		class ThreadArgument {

		public:
			size_t * threadsArrived;
			std::mutex *barrierLock;
			std::condition_variable *cond_var;

			size_t *treeThreadsArrived;
			std::mutex *treeBarrierLock;
			std::condition_variable *treeCond_var;

			size_t *carryThreadsArrived;
			std::mutex *carryBarrierLock;
			std::condition_variable *carryCond_var;

			std::mutex *nodeLock;
			std::condition_variable *rootCond_var;
			size_t* threadsCompletedTheirUpTreeTask;

			size_t threadId;
			size_t threadInputIndex;
			size_t chunkSize;

			size_t nDataBlocks;
			std::mutex *dataBlockMutex;
			unsigned char *dataBlockFlags;
			size_t *dataBlockIndices;


			std::vector<IN> *input;
			std::vector<IN> *output;

			ScanTreeNode<IN> *node;
			size_t *carry;

			// Constructors
			ThreadArgument() {}
			ThreadArgument(std::vector<IN> &output, std::vector<IN> &input, size_t threadId, size_t threadInputIndex, size_t chunkSize)
				: threadInputIndex(threadInputIndex), chunkSize(chunkSize), threadId(threadId), input(&input), output(&output) {}

			// Destructors
			~ThreadArgument() {

				delete[] dataBlockFlags;
				delete[] dataBlockIndices;
				delete dataBlockMutex;

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

			void treeBarrier(size_t numberOfThreadsToBarrier) {

				auto lck = std::unique_lock<std::mutex>(*treeBarrierLock);

				(*treeThreadsArrived)++;
				if (*treeThreadsArrived == numberOfThreadsToBarrier) {

					treeCond_var->notify_all();
					*treeThreadsArrived = 0;
				}
				else {
					treeCond_var->wait(lck);
				}
			}

			void carryBarrier(size_t numberOfThreadsToBarrier) {

				auto lck = std::unique_lock<std::mutex>(*carryBarrierLock);

				(*carryThreadsArrived)++;
				if (*carryThreadsArrived == numberOfThreadsToBarrier) {

					carryCond_var->notify_all();
					*carryThreadsArrived = 0;
				}
				else {
					carryCond_var->wait(lck);
				}
			}
		};

		// Constructor
		// -----------
		ScanImplementation(Successor<SU> successor, size_t threads) : successor(successor), nthreads(threads) {
			this->nDataBlocks = 10;
			this->BLOCK_FLAG_INITIAL_VALUE = 1;
		}

		// Utility functions
		// -----------------
		template<typename T>
		static inline void deleteIfPointer(T* &p) { delete p; };

		template<typename T>
		static inline void deleteIfPointer(T) {	return; };

		template<typename T>
		static inline T* makeACopy(T* &p) {	return new T(*p); }

		template<typename T> 
		static inline T makeACopy(T &p) { return p; }

		// ThreadScan: functionality of the Scan pattern
		// ---------------------------------------------
		template<typename IN, typename ...ARGs>
		void threadScan(ThreadArgument<IN> *threadArguments, size_t threadID, ARGs... args) {

			auto input = threadArguments[threadID].input;
			auto output = threadArguments[threadID].output;
			auto chunkStart = threadArguments[threadID].threadInputIndex;
			auto chunkEnd = chunkStart + threadArguments[threadID].chunkSize;

			/*
			*
			* Phase 1
			*
			* Scan thread chunk
			*/
			output->at(chunkStart) = makeACopy(input->at(chunkStart));

			for (size_t elementIndex = chunkStart + 1; elementIndex < chunkEnd; ++elementIndex) {

				output->at(elementIndex) = successor.successor(output->at(elementIndex - 1), input->at(elementIndex), args...);
			}

			/*
			* Sync
			*/
			threadArguments[threadID].barrier(nthreads);

			/*
			*
			* Phase 2
			* Construct the prefix-sum tree
			*/
			ScanTreeNode<IN>* root;

			size_t numberOfThreadsInStage = nthreads >> 1;
			size_t offset = 2; //TODO: hardcoded?
			size_t threadPair = 1;

			auto &threadsNode = threadArguments[threadID].node;
			threadsNode = new ScanTreeNode<IN>();
			threadsNode->sum = makeACopy(output->at(chunkEnd - 1));
			threadsNode->range = threadID + threadPair;
			threadsNode->nodeLock = threadArguments[threadID].nodeLock;

			auto carry = threadArguments[threadID].carry;

			threadArguments[threadID].treeBarrier(nthreads);

			/*
			* Work on "Up-pass" of ScanTree
			*/
			while (threadID % offset == 0 && threadsNode->range < nthreads) {

				ScanTreeNode <IN> *parentNode = new ScanTreeNode<IN>();
				parentNode->nodeLock = threadArguments[threadID].nodeLock;
				parentNode->leftChild = threadArguments[threadID].node;
				parentNode->rightChild = threadArguments[threadID + threadPair].node;
				parentNode->sum = successor.successor(parentNode->leftChild->sum, parentNode->rightChild->sum, args...);

				parentNode->range = parentNode->rightChild->range;
				threadsNode = parentNode;

				if (threadID == 0 && *carry == 0) *carry = numberOfThreadsInStage % 2 ? (2 * numberOfThreadsInStage - 2)*threadPair : 0;

				threadArguments[threadID].treeBarrier(numberOfThreadsInStage);

				//Handle Carrys
				auto numberOfThreadsInCarry = numberOfThreadsInStage;
				while (*carry) {

					if (threadsNode->range == *carry) {

						ScanTreeNode <IN> *parentNode = new ScanTreeNode<IN>();

						parentNode->nodeLock = threadArguments[threadID].nodeLock;
						parentNode->leftChild = threadArguments[threadID].node;

						parentNode->rightChild = threadArguments[*carry].node;

						parentNode->sum = successor.successor(parentNode->leftChild->sum, parentNode->rightChild->sum, args...);

						parentNode->range = parentNode->rightChild->range;
						threadsNode = parentNode;
					}

					threadArguments[threadID].carryBarrier(numberOfThreadsInCarry);

					if (numberOfThreadsInCarry % 2) {

						auto newCarry = (2 * numberOfThreadsInCarry - 2) * threadPair;

						if (threadID == 0) *carry = newCarry;
						threadArguments[threadID].carryBarrier(numberOfThreadsInCarry);

						numberOfThreadsInCarry--;
						if (threadID == newCarry) break;
					}
					else {

						if (threadID == 0) *carry = 0;
						threadArguments[threadID].carryBarrier(numberOfThreadsInCarry);

					}
				}

				threadArguments[threadID].treeBarrier(numberOfThreadsInStage);



				numberOfThreadsInStage >>= 1;
				threadPair <<= 1;
				offset <<= 1;
			}

			if (threadID == 0) {

				root = threadsNode;
				root->leftSum = NULL;
			}

			auto nodeLck = std::unique_lock<std::mutex>(*threadArguments[threadID].nodeLock);
			if (++(*threadArguments[threadID].threadsCompletedTheirUpTreeTask) == nthreads) {

				if (threadID != 0) { // If you are NOT thread 0 and you are the "last" tree-thread

					threadArguments[threadID].rootCond_var->notify_one();//Wake-up "root" thread
				}
				// if you are thread 0, it moves on, as all other threads are waiting in nodes
			}
			else {

				if (threadID == 0) { // if you are thread 0, and you are NOT the "last" tree-thread

									 //wait for other tree-threads, were the last one will notify you.
					threadArguments[threadID].rootCond_var->wait(nodeLck);
				}
			}

			if (threadID != 0) {
				threadsNode->nodeWait(nodeLck); // Wait for "down-pass" of ScanTree
			}
			nodeLck.unlock();

			/*
			* Work on "Down-pass" of ScanTree
			*/
			while (threadsNode->range - threadID > 1) {

				threadsNode->leftChild->leftSum = threadsNode->leftSum;

				if (threadsNode->leftSum) {

					auto successorOutput = successor.successor(threadsNode->leftChild->sum, *threadsNode->leftSum, args...);

					threadsNode->rightChild->leftSum = new IN(successorOutput);
					threadsNode->rightChild->deleteLeftSumContent = true;
				}
				else {
					threadsNode->rightChild->leftSum = new IN(threadsNode->leftChild->sum);
				}

				threadsNode->rightChild->nodeSignal(); //Wake-up rightChild thread

				threadsNode = threadsNode->leftChild;
			}

			threadArguments[threadID].treeBarrier(nthreads);

			if (threadID != 0) {
				auto temp = output->at(chunkEnd - 1);
				output->at(chunkEnd - 1) = successor.successor(temp, *threadsNode->leftSum, args...);
				deleteIfPointer(temp);
			}

			/*
			* Sync
			*/
			threadArguments[threadID].barrier(nthreads);

			/*
			* Phase 3
			*
			* Update each chunk's value with the last value of the previous chunk - A Map
			*
			* Optimised with "task-stealing"
			*/

			size_t assistedThreadID = threadID;

			if (nthreads>1)
				do {

					assistedThreadID = (assistedThreadID + 1) % nthreads;

					if (assistedThreadID == 0) { // first chunk is ready
						continue;
					}

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

							auto prevChunkLastValue = output->at(dataBlockIndices[0] - 1);

							//the last chunk's element is ready, we need to avoid modifying it
							auto dataBlockEnd =
								dataBlock == nDataBlocks - 1 ? dataBlockIndices[dataBlock + 1] - 1 : dataBlockIndices[dataBlock + 1];

							for (size_t elementIndex = dataBlockIndices[dataBlock]; elementIndex < dataBlockEnd; ++elementIndex) {

								IN temp = output->at(elementIndex); // In order to delete if it is a pointer

								output->at(elementIndex) = successor.successor(prevChunkLastValue, temp, args...);

								deleteIfPointer(temp);
							}
						}
						else { // Just in case after the first if, the flag changes its value to 0 from another thread
							dataBlockMutex->unlock();
						}
						++dataBlock;
					}

				} while (assistedThreadID != threadID);

				if (threadID == 0) {
					delete root;
				}
		}

	public:
		// Paranthesis operator: call function
		// -----------------------------------
		template<typename IN, typename ...ARGs>
		void operator()(std::vector<IN> &output, std::vector<IN> input, ARGs... args) {

			// Optimization to best number of threads
			// --------------------------------------
			nthreads = nthreads ? nthreads : std::thread::hardware_concurrency();
			nthreads = nthreads * 2 > input.size() ? input.size() / 2 : nthreads;		// x2 because we need atleast 2 items per chunk

			// Hardcoded for input sizes of 0 and 1
			// ------------------------------------
			if (input.size() == 0) { return; }
			if (input.size() == 1) { output = input; return; }

			// When input size is 3 we need only 1 thread for reduction
			// --------------------------------------------------------
			if (input.size() < 4) {
				output[0] = input[0];
				output[1] = successor.successor(input[0], input[1], args...);
				output[2] = successor.successor(output[1], input[2], args...);
				return;
			}

			// Generate Threads, Thread Arguments, Thread Output Vector
			// --------------------------------------------------------
			std::thread *THREADS[nthreads];
			ThreadArgument<IN> *threadArguments = new ThreadArgument<IN>[nthreads];
			std::vector<IN> tempOutput(input.size());

			// Generate synchronization variables
			// ----------------------------------
			size_t threadsArrived = 0;
			std::mutex barrierLock;
			std::condition_variable cond_var;

			size_t treeThreadsArrived = 0;
			std::mutex treeBarrierLock;
			std::condition_variable treeCond_var;

			size_t carryThreadsArrived = 0;
			std::mutex carryBarrierLock;
			std::condition_variable carryCond_var;

			size_t threadsCompletedTheirUpTreeTask = 0;
			std::mutex nodeLock;
			std::condition_variable rootCond_var;

			// Assign proper data chunks to thread arguments
			// ---------------------------------------------
			size_t chunkIndex = 0;
			size_t carry = nthreads % 2 ? nthreads - 1 : 0; // We are using a tree structure so a carry is possible at any depth
			for (size_t t = 0; t< nthreads; ++t) {

				// Calculate size of chunks			// When data can't be divided in equal chunks we must increase the
				// ------------------------			// data processed by the first DataSize(mod ThreadCount) threads.
				if (t < (input.size() % nthreads)) threadArguments[t].chunkSize = 1 + input.size() / nthreads;
				else threadArguments[t].chunkSize = input.size() / nthreads;

				threadArguments[t].threadId = t;

				// Assign synchronization variables
				// --------------------------------
				threadArguments[t].threadsArrived = &threadsArrived;
				threadArguments[t].barrierLock = &barrierLock;
				threadArguments[t].cond_var = &cond_var;

				threadArguments[t].treeThreadsArrived = &treeThreadsArrived;
				threadArguments[t].treeBarrierLock = &treeBarrierLock;
				threadArguments[t].treeCond_var = &treeCond_var;

				threadArguments[t].carryThreadsArrived = &carryThreadsArrived;
				threadArguments[t].carryBarrierLock = &carryBarrierLock;
				threadArguments[t].carryCond_var = &carryCond_var;

				threadArguments[t].threadsCompletedTheirUpTreeTask = &threadsCompletedTheirUpTreeTask;
				threadArguments[t].nodeLock = &nodeLock;
				threadArguments[t].rootCond_var = &rootCond_var;

				// Assign general variables
				// ------------------------
				threadArguments[t].input = &input;
				threadArguments[t].output = &tempOutput;
				threadArguments[t].threadInputIndex = chunkIndex;
				threadArguments[t].carry = &carry;

				// Shift chunk index for next thread argument
				// ------------------------------------------
				chunkIndex += threadArguments[t].chunkSize;

				// ------------------------------------------------------------------------------------------
				// DATA BLOCKS
				// ------------------------------------------------------------------------------------------

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
				size_t blockSize, blockStart = threadArguments[t].threadInputIndex, blockEnd;
				for (size_t block = 0; block < nDataBlocks; ++block) {

					if (block < (threadArguments[t].chunkSize % nDataBlocks)) blockSize = 1 + threadArguments[t].chunkSize / nDataBlocks;
					else blockSize = threadArguments[t].chunkSize / nDataBlocks;

					blockEnd = blockStart + blockSize;
					threadArguments[t].dataBlockIndices[block] = blockStart;
					blockStart = blockEnd;
				}
				threadArguments[t].dataBlockIndices[nDataBlocks] = blockEnd;
			}

			// Run threads
			// -----------
			for (size_t t = 0; t< nthreads; ++t) {
				THREADS[t] = new std::thread(&ScanImplementation<SU>::threadScan<IN, ARGs...>, this, threadArguments, t, args...);
			}
			
			// Join threads
			// ------------
			for (size_t t = 0; t< nthreads; ++t) {
				THREADS[t]->join();
				delete THREADS[t];
			}

			// Tidy-up after finish
			// --------------------
			delete[] threadArguments;
			output = tempOutput;
		}

		// Friend functions for the Scan Implementation
		// --------------------------------------------
		template<typename SU2>
		friend ScanSkeleton::ScanImplementation<SU2> __ScanWithAccess(SU2 su, const size_t &threads);
	};

	// Friend functions for ScanSkeleton
	// ---------------------------------
	template<typename SU2>
	friend ScanSkeleton::ScanImplementation<SU2> __ScanWithAccess(SU2 su, const size_t &threads);
};

// Wrapper
// -------
template<typename SU>
ScanSkeleton::ScanImplementation<SU> __ScanWithAccess(SU su, const size_t &threads) {

	ScanSkeleton::Successor<SU> successor(su);
	ScanSkeleton::ScanImplementation<SU> scan(successor, threads);

	return scan;
}

template<typename SU>
ScanSkeleton::ScanImplementation<SU> Scan(SU su, const size_t &threads = 0) {
	return __ScanWithAccess(su, threads);
}


#endif //SKELETONS_SCAN_HPP
