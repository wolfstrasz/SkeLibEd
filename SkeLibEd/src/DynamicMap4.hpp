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
#include <chrono>
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
		std::thread **allThreads;
		bool isInitialised;
		Elemental<EL> elemental;
		std::chrono::high_resolution_clock::time_point tstart;
		std::chrono::high_resolution_clock::time_point tend;
		double duration = 0.0f;

		template<typename IN, typename OUT>
		class Scoreboard {
		public:
			// input output
			std::vector<OUT> *output;
			std::vector<IN> *input;
			// detect global work
			bool isFinished;
			bool isInitialised;
			// detect next work
			size_t inputSize;
			size_t curIndex;
			// scoreboard worksize
			size_t jobSize;
			// guard
			std::mutex scoreboardLock;

			// analysis
			//double meanTime;
			size_t startItems;
			void switchWorkload(size_t newMeanWork, double workTime) {
				std::cout << "SWITCH FROM: " << jobSize << "\t TO: "<< newMeanWork 
					<< "\t CUZ: " << workTime <<"\t" << workTime/1000 << "\t" << workTime / 1000 / 1000 << "\n";
					this->jobSize = (this->jobSize + newMeanWork) / 2;
					this->jobSize = this->jobSize == 0 ? 1 : jobSize;
			}
			// timing
			//std::vector<double>* scoretiming;
			// constructor
			Scoreboard(std::vector<IN> *in, std::vector<OUT> *out, size_t nthreads) {
				this->output = out;
				this->input = in;
				isFinished = false;
				isInitialised = false;
				inputSize = in->size();
				curIndex = 0;
				jobSize = 0;
				//meanTime = 1000 * 1000; // 1milisec

				//scoretiming = new std::vector<double>(nthreads);
			}
			~Scoreboard() {}
		};
		void* scoreboard;


		// ThreadMap - function applied to each thread
		// --------------------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(Scoreboard<IN, OUT> *scoreboard,size_t id, ARGs... args) {
			//std::chrono::high_resolution_clock::time_point thstart;
			//std::chrono::high_resolution_clock::time_point thend;
			//double timeForScore = 0.0f;
			//size_t jobsDone = 0;
			
			std::chrono::high_resolution_clock::time_point wstart;
			std::chrono::high_resolution_clock::time_point wend;
			double workTime = 0.0f;
			double meanTime = 0.0f;
			size_t meanElements;
			size_t elementsCount;
			size_t elementIndex;
		//	double scoreTime;
			
			while (!scoreboard->isInitialised);
			meanElements = scoreboard->jobSize;

			while (!scoreboard->isFinished) {
				// Lock scoreboard
				//thstart = std::chrono::high_resolution_clock::now();
				while (!scoreboard->scoreboardLock.try_lock());
				//	thend = std::chrono::high_resolution_clock::now();
				//	timeForScore += (double)std::chrono::duration_cast<std::chrono::nanoseconds>(thend - thstart).count();
				if (scoreboard->isFinished) {
					scoreboard->scoreboardLock.unlock();
					break;
				}
				// set new jobSize
				//scoreboard->printWork(id, workTime);
				//scoreboard->jobSize = (scoreboard->jobSize + meanElements) / 2;
				// get new data
				if (scoreboard->curIndex + scoreboard->jobSize < scoreboard->inputSize) {
					scoreboard->switchWorkload(meanElements, workTime);
					elementsCount = scoreboard->jobSize;
					elementIndex = scoreboard->curIndex;
					scoreboard->curIndex += scoreboard->jobSize;
				}
				else {
					elementsCount = scoreboard->inputSize - scoreboard->curIndex;
					elementIndex = scoreboard->curIndex;
					scoreboard->curIndex += elementsCount;
					scoreboard->isFinished = true;
				}
				// unlock scoreboard
				scoreboard->scoreboardLock.unlock();

				// Process the data block
				// ----------------------
				wstart = std::chrono::high_resolution_clock::now();
				for (int elementsFinished = 0; elementsFinished < elementsCount; elementsFinished++) {
					scoreboard->output->at(elementIndex + elementsFinished) = elemental.elemental(scoreboard->input->at(elementIndex + elementsFinished), args...);
				}
				wend = std::chrono::high_resolution_clock::now();
				workTime = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(wend - wstart).count();
				
				//if (workTime > 1.25f || workTime < 0.75f) // more thant 1.25 milisecs work weight has increased
				//{
				//	meanTime = workTime / elementsCount;
				//	meanElements = 1.00f / meanTime;
				//}
			}

		}

		// Constructor
		// -----------
		DynamicMapImplementation(Elemental<EL> elemental) : elemental(elemental) {
			this->nthreads = std::thread::hardware_concurrency();
			//this->sizeOfWork = 1000;
		//	allThreads = new std::thread*[nthreads];
			this->duration = 0.0f;
			//this->isInitialised = false;
		}

	public:
		template <typename IN, typename OUT, typename ...ARGs>
		void start_init(std::vector<OUT> *output, std::vector<IN> *input, ARGs... args) {

			// create threads
			for (size_t t = 0; t < nthreads; t++) {
				allThreads[t] = new std::thread(&DynamicMapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, ((Scoreboard<IN, OUT>*)scoreboard),t, args...);
			}
		}

		template <typename IN, typename OUT, typename ...ARGs>
		void start_analysis(std::vector<OUT> *output, std::vector<IN> *input, ARGs... args) {

			size_t newJobSize = 0;
			duration = 10000000.0f / nthreads;
			double durationAtStart = duration;
			// analyse worksize
			while (duration > 0.0f && newJobSize < input->size()) {
				tstart = std::chrono::high_resolution_clock::now();
				output->at(newJobSize) = elemental.elemental(input->at(newJobSize), args...);
				tend = std::chrono::high_resolution_clock::now();
				duration -= (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count();
				newJobSize++;
			}
			((Scoreboard<IN, OUT>*)scoreboard)->curIndex = newJobSize;
			newJobSize *= nthreads;

				// send work size
			((Scoreboard<IN, OUT>*)scoreboard)->jobSize = newJobSize;
			((Scoreboard<IN, OUT>*)scoreboard)->isInitialised = true;

		}


		// Paranthesis operator: call function
		// -----------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void operator()(std::vector<OUT> &output, std::vector<IN> &input, ARGs... args) {
			this->allThreads = new std::thread*[nthreads];
			scoreboard = new Scoreboard<IN, OUT>(&input, &output, nthreads);

			//std::cout << "THREADER INIT\n";
			// USE THREADER
			// -----------------------------------------------------------------------------------
			// start threader
			std::thread *threader;
			threader = new std::thread(&DynamicMapImplementation<EL>::start_init<IN, OUT, ARGs...>, this, &output, &input, args...);
			// main thread analyses
			//std::cout << "MAIN THREAD ANALYSIS\n";
			start_analysis(&output, &input, args...);


			// delete threader
			threader->join();
			delete threader;


			// Join threads
			// -----------------------------------------------------------------------------------
			for (size_t t = 0; t < nthreads; ++t) {
				allThreads[t]->join();
				//	std::cout << "THREAD ID:  " << t << "\n";
				//	std::cout << "SCORE TIME: " << ((Scoreboard<IN, OUT>*)scoreboard)->scoretiming->at(t) / 1000.0f / 1000.0f << "\n";
				//	std::cout << "INIT  TIME: " << ((Scoreboard<IN, OUT>*)scoreboard)->inittiming->at(t) << "\n";

				delete allThreads[t];
			}
			delete allThreads;
			delete ((Scoreboard<IN, OUT>*)scoreboard);

		}

		void stop() {

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
