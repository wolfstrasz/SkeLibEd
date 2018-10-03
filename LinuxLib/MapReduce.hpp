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

	template<typename EL>
	class MapReduceElemental {
	public:
		MapReduceElemental(EL el) : elemental(el) {}

		EL elemental;
	};

	template<typename RE>
	class MapReduceReducer {
	public:
		MapReduceReducer(RE re) : reducer(re) {}

		RE reducer;
	};

	template<typename HA>
	class MapReduceHasher {
	public:
		MapReduceHasher(HA ha) : hasher(ha) {}

		HA hasher;
	};

public:


	template<typename EL, typename RE, typename HA>
	class MapReduceImplementation {

	private:
		short int BLOCK_FLAG_INITIAL_VALUE;
		size_t nthreads;
		size_t nDataBlocks;

		/*
		*
		*ThreadArgument
		*
		*/
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

			~ThreadArgument() {

				delete reduceHashTable;

				delete reduceKeysMutex;

				delete[] dataBlockIndices;
				delete[] dataBlockFlags;
				delete[] dataBlockMutexes;

				delete signUpMutex;

			}

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

		template<typename P>
		inline bool isPointer(P *&p) {
			return true;
		};

		template<typename P>
		inline bool isPointer(P) {
			return false;
		};

		template<typename P>
		inline void deleteIfPointer(P *&p) {
			delete p;
		};

		template<typename P>
		inline void deleteIfPointer(P) {
			return;
		};

		template<typename IN, typename K2, typename V2, typename ...ARGs>
		void threadMapReduce(ThreadArgument<IN, K2, V2> *threadArguments, size_t threadID, ARGs... args) {

			using MAP_OUTPUT = std::list<std::pair<K2, V2>>;
			using REDUCE_OUTPUT = std::list<V2>;

			/*
			* Map
			*
			* Each thread
			*/
			auto mapInput = threadArguments[threadID].input;
			auto mapOutputValues = threadArguments[threadID].mapOutputValues;

			size_t assignedThreadID = threadID;
			do {

				threadArguments[assignedThreadID].signUpMutex->lock();
				if (threadArguments[assignedThreadID].mapChunkSignedThreads == 0) {// Zero signed-threads means this chunk's work is completed.
					threadArguments[assignedThreadID].signUpMutex->unlock();
					assignedThreadID = (assignedThreadID + 1) % nthreads;
					continue;
				}

				// thread is "signed" by default to its own chunck
				if (assignedThreadID != threadID) threadArguments[assignedThreadID].mapChunkSignedThreads++;
				threadArguments[assignedThreadID].signUpMutex->unlock();


				auto mapDataBlockMutexes = threadArguments[assignedThreadID].dataBlockMutexes;
				auto mapDataBlockFlags = threadArguments[assignedThreadID].dataBlockFlags;
				auto mapDataBlockIndices = threadArguments[assignedThreadID].dataBlockIndices;
				auto mapDataBlocks = threadArguments[assignedThreadID].mapDataBlocks;

				//why shouldn't we 'steal' even the first data block? :P
				size_t dataBlock = 0;

				while (dataBlock < mapDataBlocks) {

					if (mapDataBlockFlags[dataBlock] == 0) {// if the data block has been, or being processed by another thread...
						++dataBlock;
						continue; //move on to the next data block.
					}

					mapDataBlockMutexes[dataBlock].lock();
					if (mapDataBlockFlags[dataBlock] == 1) {
						mapDataBlockFlags[dataBlock] = 0;
						mapDataBlockMutexes[dataBlock].unlock();

						for (size_t inputIndex = mapDataBlockIndices[dataBlock]; inputIndex < mapDataBlockIndices[dataBlock + 1]; ++inputIndex) {

							mapOutputValues[inputIndex] = elemental.elemental(mapInput->at(inputIndex), args...);
						}
					}
					else { // Just in case after the first if, the flag changes its value to 0 from another thread
						mapDataBlockMutexes[dataBlock].unlock();
					}

					++dataBlock;
				}

				/*
				* Thread will combine dataBlockReducedValues if it is the last "signed-up" thread
				*/
				threadArguments[assignedThreadID].signUpMutex->lock();
				threadArguments[assignedThreadID].mapChunkSignedThreads--;
				if (threadArguments[assignedThreadID].mapChunkSignedThreads == 0) {
					threadArguments[assignedThreadID].signUpMutex->unlock();

					auto &reduceHashTable = threadArguments[assignedThreadID].reduceHashTable;

					for (size_t inputIndex = mapDataBlockIndices[0]; inputIndex < mapDataBlockIndices[mapDataBlocks]; ++inputIndex) {

						for (auto &key_value : mapOutputValues[inputIndex]) {

							auto hashValue = hasher.hasher(key_value.first);

							if (reduceHashTable->count(hashValue)) {

								reduceHashTable->at(hashValue).second->push_back(key_value.second);

								deleteIfPointer(key_value.first);

							}
							else {
								REDUCE_OUTPUT *keyList = new REDUCE_OUTPUT{ key_value.second };

								reduceHashTable->insert(std::make_pair(hashValue, std::make_pair(key_value.first, keyList)));
							}
						}
					}
				}
				else {
					threadArguments[assignedThreadID].signUpMutex->unlock();
				}

				assignedThreadID = (assignedThreadID + 1) % nthreads;

			} while (assignedThreadID != threadID);

			/*
			* sync
			*/
			threadArguments[threadID].barrier1(nthreads);


			/*
			* Phase: For every iteration, half threads from the previous one combine the hashTables
			*        of two threads. The logic is the same with Reduce Skeleton.
			*
			* Each pair of threads
			*/
			size_t numberOfThreadsInStage = nthreads >> 1; //TODO: hardcoded?
			size_t offset = 2; //TODO: hardcoded?
			size_t threadPair = 1;
			size_t carry = nthreads % 2 ? nthreads - 1 : 0; //TODO: hardcoded?

			while (threadID < numberOfThreadsInStage) {

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

				if (threadID == 0 && carry) {

					for (auto &hashKey_Bpair : *threadArguments[carry].reduceHashTable) {

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

					carry = 0;
				}

				threadArguments[threadID].barrier2(numberOfThreadsInStage);

				if (threadID == 0 && numberOfThreadsInStage % 2) carry = (2 * numberOfThreadsInStage - 2) * threadPair;

				offset <<= 1;
				threadPair <<= 1;
				numberOfThreadsInStage >>= 1;
			}

			/*
			* sync
			*/
			threadArguments[threadID].barrier1(nthreads);

			/*
			* Phase: Thread 0 assigns keys to each thread.
			*
			* Thread 0
			*/
			if (threadID == 0) {

				auto &reduceHashTable = threadArguments[0].reduceHashTable;
				unsigned char reduceKeyFlag = 1;
				size_t threadIndex = 0;
				for (const auto &p : *reduceHashTable) {

					threadArguments[threadIndex].reduceKeys.push_back({ p.first, reduceKeyFlag });

					threadIndex = (threadIndex + 1) % nthreads;
				}
			}


			/*
			* sync
			*/
			threadArguments[threadID].barrier1(nthreads);

			/*
			* Phase: Each thread applies its keys to the reducer
			*
			* Each thread
			*/
			size_t assistedThreadID = threadID;
			do {

				std::mutex *reduceKeysMutex = threadArguments[assistedThreadID].reduceKeysMutex;

				for (auto &p : threadArguments[assistedThreadID].reduceKeys) {

					auto &reduceKeyFlag = p.second;

					if (reduceKeyFlag == 0) {
						continue;
					}

					reduceKeysMutex->lock();
					if (reduceKeyFlag == 1) {
						reduceKeyFlag = 0;
						reduceKeysMutex->unlock();

						auto reducePair = threadArguments[0].reduceHashTable->at(p.first);

						REDUCE_OUTPUT reduceRes = reducer.reducer(reducePair.first, *reducePair.second, args...);

						if (isPointer(reducePair.second->front())) {
							for (auto reduceInputValue : *reducePair.second) {
								deleteIfPointer(reduceInputValue);
							}
						}

						*reducePair.second = reduceRes;
					}
					else { // Just in case after the first if, the flag changes its value to 0 from another thread
						reduceKeysMutex->unlock();
					}
				}

				assistedThreadID = (assistedThreadID + 1) % nthreads;

			} while (assistedThreadID != threadID);
		}

		MapReduceElemental<EL> elemental;
		MapReduceReducer<RE> reducer;
		MapReduceHasher<HA> hasher;

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

			output.resize(tempOutput.size());
			size_t outputIndex = 0;
			for (auto &pair : tempOutput) {

				output[outputIndex++] = { pair.first, pair.second[0] };
			}
		}

		template<typename IN, typename K2, typename V2, typename ...ARGs>
		void operator()(std::vector<std::pair<K2, std::vector<V2>>> &output, std::vector<IN> &input, ARGs... args) {

			nthreads = nthreads ? nthreads : std::thread::hardware_concurrency();

			using MAP_OUTPUT = std::list<std::pair<K2, V2>>;
			using REDUCE_OUTPUT = std::list<V2>;
			/*
			* TODO: Handle input.size == 0 or 1
			* Hardcoded for now...
			*/
			if (input.size() == 0) {
				return;
			}
			//                if( input.size() == 1 ) {
			//                  TODO: Handle this case?
			//                }

			std::thread *THREADS[nthreads];
			ThreadArgument<IN, K2, V2> *threadArguments = new ThreadArgument<IN, K2, V2>[nthreads];

			size_t threadsArrived1 = 0;
			std::mutex barrierLock1;
			std::condition_variable cond_var1;

			size_t threadsArrived2 = 0;
			std::mutex barrierLock2;
			std::condition_variable cond_var2;

			auto mapOutputValues = new MAP_OUTPUT[input.size()];

			size_t chunkIndex = 0;
			for (size_t t = 0; t < nthreads; ++t) {

				if (t < (input.size() % nthreads)) threadArguments[t].mapChunkSize = 1 + input.size() / nthreads;
				else threadArguments[t].mapChunkSize = input.size() / nthreads;


				threadArguments[t].threadsArrived1 = &threadsArrived1;
				threadArguments[t].barrierLock1 = &barrierLock1;
				threadArguments[t].cond_var1 = &cond_var1;

				threadArguments[t].threadsArrived2 = &threadsArrived2;
				threadArguments[t].barrierLock2 = &barrierLock2;
				threadArguments[t].cond_var2 = &cond_var2;

				threadArguments[t].input = &input;
				threadArguments[t].threadMapInputIndex = chunkIndex;

				chunkIndex += threadArguments[t].mapChunkSize;
				threadArguments[t].mapOutputValues = mapOutputValues;

				/*
				* Data Blocks
				*/
				/*times 2 so we can have at least 2 items per data block*/
				nDataBlocks = nDataBlocks * 2 > threadArguments[t].mapChunkSize ? std::max(threadArguments[t].mapChunkSize / 2, (size_t)1) : nDataBlocks;

				threadArguments[t].mapDataBlocks = nDataBlocks;

				threadArguments[t].signUpMutex = new std::mutex;

				threadArguments[t].dataBlockFlags = new short int[nDataBlocks]();
				std::fill_n(threadArguments[t].dataBlockFlags, nDataBlocks, BLOCK_FLAG_INITIAL_VALUE);

				threadArguments[t].dataBlockMutexes = new std::mutex[nDataBlocks];

				threadArguments[t].dataBlockIndices = new size_t[nDataBlocks + 1]();

				size_t blockSize, blockStart = threadArguments[t].threadMapInputIndex, blockEnd;

				for (size_t block = 0; block < nDataBlocks; ++block) {

					if (block < (threadArguments[t].mapChunkSize % nDataBlocks)) blockSize = 1 + threadArguments[t].mapChunkSize / nDataBlocks;
					else blockSize = threadArguments[t].mapChunkSize / nDataBlocks;

					blockEnd = blockStart + blockSize;
					threadArguments[t].dataBlockIndices[block] = blockStart;
					blockStart = blockEnd;
				}
				threadArguments[t].dataBlockIndices[nDataBlocks] = blockEnd;
			}

			for (size_t t = 0; t < nthreads; ++t) {
				THREADS[t] = new std::thread(&MapReduceImplementation<EL, RE, HA>::threadMapReduce<IN, K2, V2, ARGs...>, this, threadArguments, t, args...);
			}

			for (size_t t = 0; t < nthreads; ++t) {
				THREADS[t]->join();
				delete THREADS[t];
			}

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


		template<typename HA2, typename EL2, typename RE2>
		friend MapReduceSkeleton::MapReduceImplementation<EL2, RE2, HA2> __MapReduceWithAccess(EL2 el, RE2 re, HA2 hasher, size_t threads);
	};

	template<typename HA2, typename EL2, typename RE2>
	friend MapReduceSkeleton::MapReduceImplementation<EL2, RE2, HA2> __MapReduceWithAccess(EL2 el, RE2 re, HA2 hasher, size_t threads);
};

template<typename HA, typename EL, typename RE>
MapReduceSkeleton::MapReduceImplementation<EL, RE, HA> __MapReduceWithAccess(EL el, RE re, HA ha, size_t threads) {

	MapReduceSkeleton::MapReduceElemental<EL> elemental(el);
	MapReduceSkeleton::MapReduceReducer<RE> reducer(re);
	MapReduceSkeleton::MapReduceHasher<HA> hasher(ha);

	MapReduceSkeleton::MapReduceImplementation<EL, RE, HA> mr(elemental, reducer, hasher, threads);

	return mr;
}

template<typename HA, typename EL, typename RE>
MapReduceSkeleton::MapReduceImplementation<EL, RE, HA> MapReduce(EL el, RE re, HA hasher, size_t threads = 0) {

	return __MapReduceWithAccess(el, re, hasher, threads);
}

#endif /* MAPREDUCE_HPP */

