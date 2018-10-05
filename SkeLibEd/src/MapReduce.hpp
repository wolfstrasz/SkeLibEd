#pragma once
/*
* To change this license header, choose License Headers in Project Properties.
* To change this template file, choose Tools | Templates
* and open the template in the editor.
*/

/*
* File:   MapReduce.hpp
* Author: tonio
*
* Created on May 24, 2018, 1:06 PM
*/

#ifndef MAPREDUCE_HPP
#define MAPREDUCE_HPP


#include <cstdlib>
#include <iostream>

#include <vector>
#include <map>
#include <list>
#include <tuple>


#include <type_traits>
#include <functional>
#include <stdarg.h>
#include <typeinfo>

#include <utility>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <algorithm>

class MapReduceSkeleton {

private:

	// Elemental: Function used by the Map phase
	// -----------------------------------------
	template<typename EL>
	class MapReduceElemental {
	public:
		MapReduceElemental(EL el) : elemental(el) {}

		EL elemental;
	};

	// Reducer: Function used by the Reduce phase
	// ------------------------------------------
	template<typename RE>
	class MapReduceReducer {
	public:
		MapReduceReducer(RE re) : reducer(re) {}

		RE reducer;
	};

	// Hasher: Function for the hash mapping
	// -------------------------------------
	template<typename HA>
	class MapReduceHasher {
	public:
		MapReduceHasher(HA ha) : hasher(ha) {}

		HA hasher;
	};

public:

	// MapReduceImplementation: Main functional class of Map Reduce
	// ------------------------------------------------------------
	template<typename EL, typename RE, typename HA>
	class MapReduceImplementation {

	private:
		short int BLOCK_FLAG_INITIAL_VALUE;
		size_t nthreads;
		size_t nDataBlocks;

		MapReduceElemental<EL> elemental;
		MapReduceReducer<RE> reducer;
		MapReduceHasher<HA> hasher;

		// ThreadArgument: Keeps data for each thread
		// ------------------------------------------
		template<typename IN, typename K2, typename V2>
		class ThreadArgument {

			using MAP_OUTPUT = std::list<std::pair<K2, V2>>;
			using REDUCE_OUTPUT = std::list<V2>;

		public:
			size_t * threadsArrived1;
			std::mutex *barrierLock1;
			std::condition_variable *cond_var1;

			size_t *threadsArrived2;
			std::mutex *barrierLock2;
			std::condition_variable *cond_var2;

			std::vector<IN> *input;

			size_t threadMapInputIndex;
			size_t mapChunkSize;

			size_t mapDataBlocks;
			size_t *dataBlockIndices;
			std::mutex *dataBlockMutexes;
			size_t mapChunkSignedThreads;
			std::mutex *signUpMutex;
			short int *dataBlockFlags;
			MAP_OUTPUT *mapOutputValues;

			std::unordered_map<size_t, std::pair<K2, std::list<V2> *> > *reduceHashTable;

			std::list<std::pair<size_t, unsigned char>> reduceKeys;
			std::mutex *reduceKeysMutex;

			// Constructors
			ThreadArgument() {
				reduceHashTable = new std::unordered_map<size_t, std::pair<K2, std::list<V2> *>>();
				reduceKeysMutex = new std::mutex();
				mapChunkSignedThreads = 1;
			}
			ThreadArgument(std::vector<IN> &input, size_t threadInputIndex, size_t chunkSize)
				: threadMapInputIndex(threadInputIndex), mapChunkSize(chunkSize), input(&input) {
				reduceHashTable = new std::unordered_map<size_t, std::pair<K2, std::list<V2> *>>();

				reduceKeysMutex = new std::mutex();

				mapChunkSignedThreads = 1;
			}
			// Destructors
			~ThreadArgument() {

				delete reduceHashTable;

				delete reduceKeysMutex;

				delete[] dataBlockIndices;
				delete[] dataBlockFlags;
				delete[] dataBlockMutexes;

				delete signUpMutex;

			}


			// Sync barrier lock 1: Used for sync between phases
			// -------------------------------------------------
			void barrier1(size_t numberOfThreadsToBarrier) {

				auto lck = std::unique_lock<std::mutex>(*barrierLock1);

				(*threadsArrived1)++;
				if (*threadsArrived1 == numberOfThreadsToBarrier) {

					*threadsArrived1 = 0;
					cond_var1->notify_all();
				}
				else {
					cond_var1->wait(lck);
				}
			}

			// Sync barrier lock 2: used while tree reduction of hash maps occurs
			// ------------------------------------------------------------------
			void barrier2(size_t numberOfThreadsToBarrier) {

				auto lck = std::unique_lock<std::mutex>(*barrierLock2);

				(*threadsArrived2)++;
				if (*threadsArrived2 == numberOfThreadsToBarrier) {

					*threadsArrived2 = 0;
					cond_var2->notify_all();
				}
				else {
					cond_var2->wait(lck);
				}
			}

		};

		// Utility functions
		// -----------------
		template<typename P>
		inline bool isPointer(P *&p) { return true; };

		template<typename P>
		inline bool isPointer(P) { return false; };

		template<typename P>
		inline void deleteIfPointer(P *&p) { delete p; };

		template<typename P>
		inline void deleteIfPointer(P) { return; };

		// ThreadMapReduce: functionality of the MapReduce pattern
		// -------------------------------------------------------
		template<typename IN, typename K2, typename V2, typename ...ARGs>
		void threadMapReduce(ThreadArgument<IN, K2, V2> *threadArguments, size_t threadID, ARGs... args) {

			using MAP_OUTPUT = std::list<std::pair<K2, V2>>;
			using REDUCE_OUTPUT = std::list<V2>;

			// ---------------------------------------------------------------------
			// MAP PHASE:
			// ---------------------------------------------------------------------
			auto mapInput = threadArguments[threadID].input;
			auto mapOutputValues = threadArguments[threadID].mapOutputValues;

			size_t assignedThreadID = threadID; // Assigns first job to be helping itself
			do {

				// Lock for sign-up and look for work
				// ----------------------------------
				threadArguments[assignedThreadID].signUpMutex->lock();

				// Zero signed-threads means this chunk's work is completed.
				// --------------------------------------------------------
				if (threadArguments[assignedThreadID].mapChunkSignedThreads == 0) {
					threadArguments[assignedThreadID].signUpMutex->unlock();
					assignedThreadID = (assignedThreadID + 1) % nthreads;
					continue;
				}

				// If thread is assisting another thread => mark as signed for primary thread
				// --------------------------------------------------------------------------
				if (assignedThreadID != threadID)
					threadArguments[assignedThreadID].mapChunkSignedThreads++;
				threadArguments[assignedThreadID].signUpMutex->unlock();

				// Assign new variables for data for easier reading
				// ------------------------------------------------
				auto mapDataBlockMutexes = threadArguments[assignedThreadID].dataBlockMutexes;
				auto mapDataBlockFlags = threadArguments[assignedThreadID].dataBlockFlags;
				auto mapDataBlockIndices = threadArguments[assignedThreadID].dataBlockIndices;
				auto mapDataBlocks = threadArguments[assignedThreadID].mapDataBlocks;

				// Starts to iterate over data blocks of given thread (assisting / not)
				// --------------------------------------------------------------------
				size_t dataBlock = 0;
				while (dataBlock < mapDataBlocks) {

					// Lock and check if block is has not been processed already
					// ---------------------------------------------------------
					mapDataBlockMutexes[dataBlock].lock();

					if (mapDataBlockFlags[dataBlock] == 1) {
						mapDataBlockFlags[dataBlock] = 0;		// Set as processed
						mapDataBlockMutexes[dataBlock].unlock();

						// Process the data block
						// ----------------------
						for (size_t inputIndex = mapDataBlockIndices[dataBlock];
							 	inputIndex < mapDataBlockIndices[dataBlock + 1]; inputIndex++ ){
							mapOutputValues[inputIndex] = elemental.elemental(mapInput->at(inputIndex), args...);
						}
					} else mapDataBlockMutexes[dataBlock].unlock();

					dataBlock++;
				}
				// ---------------------------------------------------------------------
				// REDUCE PHASE 1:
				// ---------------------------------------------------------------------

				// Thread will combine dataBlockReducedValues if it is the last "signed-up" thread
				// -------------------------------------------------------------------------------
				threadArguments[assignedThreadID].signUpMutex->lock();
				threadArguments[assignedThreadID].mapChunkSignedThreads--;
				if (threadArguments[assignedThreadID].mapChunkSignedThreads == 0) {
					threadArguments[assignedThreadID].signUpMutex->unlock();


					auto &reduceHashTable = threadArguments[assignedThreadID].reduceHashTable;

					// Go through data blocks of current chunl and fill the hash tables
					// ----------------------------------------------------------------
					for (size_t inputIndex = mapDataBlockIndices[0]; inputIndex < mapDataBlockIndices[mapDataBlocks]; ++inputIndex) {
						// For each map output, assign it to a possition in the hash table
						// ---------------------------------------------------------------	// key_value = pair(key,value)
						for (auto &key_value : mapOutputValues[inputIndex]) {				// key_vlaue.first  = key
							auto hashValue = hasher.hasher(key_value.first);				// key_value.second = value

							// If we have entries with given hash code -> push in data value
							// -------------------------------------------------------------
							if (reduceHashTable->count(hashValue)) {
								reduceHashTable->at(hashValue).second->push_back(key_value.second);
								deleteIfPointer(key_value.first);
							}
							// else create a new entry in the hash table with the given data value
							// -------------------------------------------------------------------
							else {
								REDUCE_OUTPUT *keyList = new REDUCE_OUTPUT{ key_value.second };
								reduceHashTable->insert(std::make_pair(hashValue, std::make_pair(key_value.first, keyList)));
							}
						}
					}
				} else threadArguments[assignedThreadID].signUpMutex->unlock();

				// When finished working on the available data blocks of a given thread
				// -> continue to search for other work
				// -------------------------------------
				assignedThreadID = (assignedThreadID + 1) % nthreads;
			} while (assignedThreadID != threadID);

			// -----------------------------------------------------------------------------------------
			// PHASE 2 - For every iteration, half threads from the previous one combine the hashTables
			//           of two threads. The logic is the same with Reduce Skeleton.
			// -----------------------------------------------------------------------------------------

			// Check for synchronization before continuing
			// -------------------------------------------
			threadArguments[threadID].barrier1(nthreads);

			// Arguments for first stage of Phase 2
			// ------------------------------------
			size_t offset = 2; 													//TODO: hardcoded?
			size_t numberOfThreadsInStage = nthreads >> 1; 						//TODO: hardcoded?
			size_t carry = nthreads % 2 ? nthreads - 1 : 0; 					//TODO: hardcoded?
			size_t threadPair = 1;

			// Start tree reduction
			// --------------------
			while (threadID < numberOfThreadsInStage) { // Chooses if thread is going to be active -> dependent on how much are needed as active

				auto reduceHashTable = threadArguments[threadID * offset].reduceHashTable;

				for (auto &hashKey_Bpair : *threadArguments[threadID * offset + threadPair].reduceHashTable) {

					if (reduceHashTable->count(hashKey_Bpair.first)) {

						auto &Apair = reduceHashTable->operator[](hashKey_Bpair.first);

						Apair.second->splice(Apair.second->end(), *hashKey_Bpair.second.second);

						delete hashKey_Bpair.second.second;
						deleteIfPointer(hashKey_Bpair.second.first);

					}
					else {
						reduceHashTable->operator[](hashKey_Bpair.first) = hashKey_Bpair.second;
					}
				}

				// If uneven amount of values are present -> we have a carry that has to be added
				// ------------------------------------------------------------------------------
				if (threadID == 0 && carry) {

					for (auto &hashKey_Bpair : *threadArguments[carry].reduceHashTable) {

						if (reduceHashTable->count(hashKey_Bpair.first)) {

							auto &Apair = reduceHashTable->operator[](hashKey_Bpair.first);

							Apair.second->splice(Apair.second->end(), *hashKey_Bpair.second.second);

							delete hashKey_Bpair.second.second;
							deleteIfPointer(hashKey_Bpair.second.first);
						} else {

							reduceHashTable->operator[](hashKey_Bpair.first) = hashKey_Bpair.second;
						}
					}
					carry = 0;
				}

				// Check for synchronization before continuing
				// -------------------------------------------
				threadArguments[threadID].barrier2(numberOfThreadsInStage);

				// Calculate if carry is present in next stage
				// -------------------------------------------
				if (threadID == 0 && numberOfThreadsInStage % 2)
					carry = (2 * numberOfThreadsInStage - 2) * threadPair;

				// Update next stage arguments
				// ---------------------------
				offset <<= 1;
				threadPair <<= 1;
				numberOfThreadsInStage >>= 1;
			}

			// Check for synchronization before continuing
			// -------------------------------------------
			threadArguments[threadID].barrier1(nthreads);

			// -----------------------------------------------------------------------------------------
			// PHASE 3 - Thread 0 assigns keys to each thread.
			// -----------------------------------------------------------------------------------------
			if (threadID == 0) {

				auto &reduceHashTable = threadArguments[0].reduceHashTable;
				unsigned char reduceKeyFlag = 1;
				size_t threadIndex = 0;
				for (const auto &p : *reduceHashTable) {

					threadArguments[threadIndex].reduceKeys.push_back({ p.first, reduceKeyFlag });

					threadIndex = (threadIndex + 1) % nthreads;
				}
			}

			// Check for synchronization before continuing
			// -------------------------------------------
			threadArguments[threadID].barrier1(nthreads);

			// -----------------------------------------------------------------------------------------
			// PHASE 4 - Each thread applies its keys to the reducer
			// -----------------------------------------------------------------------------------------
			size_t assistedThreadID = threadID;
			do {

				std::mutex *reduceKeysMutex = threadArguments[assistedThreadID].reduceKeysMutex;

				for (auto &p : threadArguments[assistedThreadID].reduceKeys) {

					auto &reduceKeyFlag = p.second;

					// Lock and check if block is has not been processed already
					// ---------------------------------------------------------
					reduceKeysMutex->lock();
					if (reduceKeyFlag == 1) {
						reduceKeyFlag = 0;			// Set as processed
						reduceKeysMutex->unlock();

						auto reducePair = threadArguments[0].reduceHashTable->at(p.first);

						// Proccess reduce of pairs
						REDUCE_OUTPUT reduceRes = reducer.reducer(reducePair.first, *reducePair.second, args...);

						// Tidy up
						if (isPointer(reducePair.second->front())) {
							for (auto reduceInputValue : *reducePair.second) {
								deleteIfPointer(reduceInputValue);
							}
						}
						*reducePair.second = reduceRes;

					} else reduceKeysMutex->unlock();
					
				}

				assistedThreadID = (assistedThreadID + 1) % nthreads;

			} while (assistedThreadID != threadID);
		}

		// Constructor
		// -----------
		MapReduceImplementation(MapReduceElemental<EL> &elemental, MapReduceReducer<RE> &reducer, MapReduceHasher<HA> &hasher, size_t threads)
			: elemental(elemental), reducer(reducer), hasher(hasher), nthreads(threads) {
			this->nDataBlocks = 10;
			this->BLOCK_FLAG_INITIAL_VALUE = 1;
		}

	public:

		template<typename IN, typename K2, typename V2, typename ...ARGs>
		void operator()(std::vector<std::pair<K2, V2>> &output, std::vector<IN> &input, ARGs... args) {

			std::vector<std::pair<K2, std::vector<V2>>> tempOutput;

			this->operator()(tempOutput, input, args...);

			// Undo mapping from tempOutput to output
			// --------------------------------------
			output.resize(tempOutput.size());
			size_t outputIndex = 0;
			for (auto &pair : tempOutput) {
				output[outputIndex++] = { pair.first, pair.second[0] };
			}
		}

		// Overide the paranthesis operator
		// --------------------------------
		template<typename IN, typename K2, typename V2, typename ...ARGs>
		void operator()(std::vector<std::pair<K2, std::vector<V2>>> &output, std::vector<IN> &input, ARGs... args) {

			// Optimization to best number of threads
			// --------------------------------------
			nthreads = nthreads ? nthreads : std::thread::hardware_concurrency();

			using MAP_OUTPUT = std::list<std::pair<K2, V2>>;
			using REDUCE_OUTPUT = std::list<V2>;

			// Hardcoded for input sizes of 0 and 1
			// ------------------------------------
			if (input.size() == 0) { return; }
			//                if( input.size() == 1 ) {
			//                  TODO: Handle this case?
			//                }

			// Generate Threads, Thread Arguments
			// ----------------------------------
			std::thread *THREADS[nthreads];
			ThreadArgument<IN, K2, V2> *threadArguments = new ThreadArgument<IN, K2, V2>[nthreads];

			// Generate communication variables
			// --------------------------------
			size_t threadsArrived1 = 0;
			std::mutex barrierLock1;
			std::condition_variable cond_var1;

			size_t threadsArrived2 = 0;
			std::mutex barrierLock2;
			std::condition_variable cond_var2;

			auto mapOutputValues = new MAP_OUTPUT[input.size()];

			// Assign proper data chunks to thread arguments
			// ---------------------------------------------
			size_t chunkIndex = 0;
			for (size_t t = 0; t < nthreads; ++t) {

				// Calculate size of chunks			// When data can't be divided in equal chunks we must increase the
				// ------------------------			// data processed by the first DataSize(mod ThreadCount) threads.
				if (t < (input.size() % nthreads)) threadArguments[t].mapChunkSize = 1 + input.size() / nthreads;
				else threadArguments[t].mapChunkSize = input.size() / nthreads;

				// Assign communication variables
				// ------------------------------
				threadArguments[t].threadsArrived1 = &threadsArrived1;
				threadArguments[t].barrierLock1 = &barrierLock1;
				threadArguments[t].cond_var1 = &cond_var1;

				threadArguments[t].threadsArrived2 = &threadsArrived2;
				threadArguments[t].barrierLock2 = &barrierLock2;
				threadArguments[t].cond_var2 = &cond_var2;

				// Assign general variables
				// ------------------------
				threadArguments[t].input = &input;
				threadArguments[t].threadMapInputIndex = chunkIndex;
				threadArguments[t].mapOutputValues = mapOutputValues;

				// Shift chunk index for next thread argument
				// ------------------------------------------
				chunkIndex += threadArguments[t].mapChunkSize;

				// ------------------------------------------------------------------------------------------
				// DATA BLOCKS
				// ------------------------------------------------------------------------------------------

				// Calculate number of data blocks for the thread
				// ----------------------------------------------
				nDataBlocks = nDataBlocks * 2 > threadArguments[t].mapChunkSize ? std::max(threadArguments[t].mapChunkSize / 2, (size_t)1) : nDataBlocks;

				// Generate data blocks arguments for the thread
				// ---------------------------------------------
				threadArguments[t].mapDataBlocks = nDataBlocks;
				threadArguments[t].signUpMutex = new std::mutex;
				threadArguments[t].dataBlockMutexes = new std::mutex[nDataBlocks];
				threadArguments[t].dataBlockIndices = new size_t[nDataBlocks + 1]();
				threadArguments[t].dataBlockFlags = new short int[nDataBlocks]();
				std::fill_n(threadArguments[t].dataBlockFlags, nDataBlocks, BLOCK_FLAG_INITIAL_VALUE);

				// Assign data block info to thread argument
				// -----------------------------------------
				size_t blockSize;
				size_t blockStart = threadArguments[t].threadMapInputIndex;

				for (size_t block = 0; block < nDataBlocks; block++) {
					// Assign block index
					threadArguments[t].dataBlockIndices[block] = blockStart;

					// Calculate block size for the current block
					if (block < (threadArguments[t].mapChunkSize % nDataBlocks)) blockSize = 1 + threadArguments[t].mapChunkSize / nDataBlocks;
					else blockSize = threadArguments[t].mapChunkSize / nDataBlocks;

					// Shift block start index for next iteration
					blockStart += blockSize;
				}
				// Assign index for last block
				threadArguments[t].dataBlockIndices[nDataBlocks] = blockStart;
			}

			// Run threads
			// -----------
			for (size_t t = 0; t < nthreads; ++t)
				THREADS[t] = new std::thread(&MapReduceImplementation<EL, RE, HA>::threadMapReduce<IN, K2, V2, ARGs...>, this, threadArguments, t, args...);

			// Join threads
			// ------------
			for (size_t t = 0; t < nthreads; ++t) { THREADS[t]->join(); delete THREADS[t]; }

			// Tidy up after finishing
			// -----------------------

			/* CAUTION!!! */    output.clear();
			output.resize(threadArguments[0].reduceHashTable->size());

			size_t outputIndex = 0;
			for (auto &hashKey_pair : *(threadArguments[0].reduceHashTable)) {

				auto &pair = hashKey_pair.second;

				std::vector<V2> values(pair.second->size());

				values.assign(pair.second->begin(), pair.second->end());

				output[outputIndex++] = std::make_pair( /*Keys*/ pair.first,/*Key-Values*/values);

				delete pair.second;
			}

			delete[] threadArguments;
			delete[] mapOutputValues;

		}

		// Friend Functions for MapReduce Implementation Class
		// ---------------------------------------------------
		template<typename HA2, typename EL2, typename RE2>
		friend MapReduceSkeleton::MapReduceImplementation<EL2, RE2, HA2> __MapReduceWithAccess(EL2 el, RE2 re, HA2 hasher, size_t threads);
	};
	// Friend Functions for MapReduce Skeleton Class
	// ---------------------------------------------
	template<typename HA2, typename EL2, typename RE2>
	friend MapReduceSkeleton::MapReduceImplementation<EL2, RE2, HA2> __MapReduceWithAccess(EL2 el, RE2 re, HA2 hasher, size_t threads);
};

/*
* We cannot define a friend function with default argument
* that needs access to inner class on latest g++ compiler versions.
* We need a wrapper!
*/
template<typename HA, typename EL, typename RE>
MapReduceSkeleton::MapReduceImplementation<EL, RE, HA> __MapReduceWithAccess(EL el, RE re, HA ha, size_t threads) {

	MapReduceSkeleton::MapReduceElemental<EL> elemental(el);
	MapReduceSkeleton::MapReduceReducer<RE> reducer(re);
	MapReduceSkeleton::MapReduceHasher<HA> hasher(ha);
	return MapReduceSkeleton::MapReduceImplementation<EL, RE, HA> (elemental, reducer, hasher, threads);
}

template<typename HA, typename EL, typename RE>
MapReduceSkeleton::MapReduceImplementation<EL, RE, HA> MapReduce(EL el, RE re, HA hasher, size_t threads = 0) {
	return __MapReduceWithAccess(el, re, hasher, threads);
}

#endif /* MAPREDUCE_HPP */
