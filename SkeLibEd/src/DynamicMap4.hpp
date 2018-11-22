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
		size_t sizeOfWork;
		std::thread **allThreads;
		bool isInitialised;
		Elemental<EL> elemental;
		std::chrono::high_resolution_clock::time_point tstart;
		std::chrono::high_resolution_clock::time_point tend;
		double duration = 0.0f;

		template<typename IN, typename OUT>
		class Scoreboard {
		public:
			// constructor
			Scoreboard() {
				this->isFinished = false;
				this->curIndex = 0;
			}
			~Scoreboard() {}
			void addWork(std::vector<IN> *in, std::vector<OUT> *out) {
				this->input = in;
				this->output = out;
				isFinished = false;
				inputSize = in->size();
				curIndex = 0;
				itemsCount = 0;
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

			// analysis
			double meanTime;
			size_t startItems;

		};
		void* scoreboard;


		// ThreadMap - function applied to each thread
		// --------------------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(Scoreboard<IN, OUT> *scoreboard, ARGs... args) {
			size_t elementsCount;
			size_t elementIndex;
			double scoreTime;
			double workTime;
			std::chrono::high_resolution_clock::time_point wstart;
			std::chrono::high_resolution_clock::time_point wend;
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
				scoreTime = scoreboard->meanTime;
				// unlock scoreboard
				scoreboard->scoreboardInUse.unlock();


				// Process the data block
				// ----------------------
				for (int elementsFinished = 0; elementsFinished < elementsCount; elementsFinished++) {
					wstart = std::chrono::high_resolution_clock::now();
					scoreboard->output->at(elementIndex + elementsFinished) = elemental.elemental(scoreboard->input->at(elementIndex + elementsFinished), args...);
					wend = std::chrono::high_resolution_clock::now();
					workTime = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(wend - wstart).count();
					//std::cout << workTime << std::endl;
					if (workTime > scoreTime * 1.50f && workTime < scoreTime * 2.00f ) {
						// lessen work
						while (!scoreboard->scoreboardInUse.try_lock());
						if (scoreboard->itemsCount != (scoreboard->startItems / 2)) {
							scoreboard->itemsCount = scoreboard->itemsCount / 2;
							std::cout << "Item count: " << scoreboard->itemsCount << std::endl;
							scoreboard->meanTime = workTime;
						}
						scoreboard->scoreboardInUse.unlock();
					} else if (workTime * 1.50f < scoreTime && workTime * 2.00f > scoreTime) {
						// increase work
						while (!scoreboard->scoreboardInUse.try_lock());
						if ( scoreboard->itemsCount != (scoreboard->startItems * 2)) {
							scoreboard->itemsCount = scoreboard->itemsCount * 2;
							std::cout << "Item count: " << scoreboard->itemsCount << std::endl;
							scoreboard->meanTime = workTime;
						}
						scoreboard->scoreboardInUse.unlock();
					}
				}
			}

		}

		// Constructor
		// -----------
		DynamicMapImplementation(Elemental<EL> elemental) : elemental(elemental) {
			this->nthreads = std::thread::hardware_concurrency();
			this->sizeOfWork = 1000;
			allThreads = new std::thread*[nthreads];
			this->isInitialised = false;
		}

	public:
		template <typename IN, typename OUT, typename ...ARGs>
		void init(std::vector<OUT> *output, std::vector<IN> *input, ARGs... args) {
			// init scoreboard
			((Scoreboard<IN, OUT>*)scoreboard)->addWork(input, output);
			((Scoreboard<IN, OUT>*)scoreboard)->itemsCount = sizeOfWork;
			((Scoreboard<IN, OUT>*)scoreboard)->curIndex = sizeOfWork;
			// create threads
			for (size_t t = 0; t < nthreads; t++) {
				allThreads[t] = new std::thread(&DynamicMapImplementation<EL>::threadMap<IN, OUT, ARGs...>, this, ((Scoreboard<IN, OUT>*)scoreboard), args...);
			}
		}

		template <typename IN, typename OUT, typename ...ARGs>
		void analyse(std::vector<OUT> *output, std::vector<IN> *input, ARGs... args) {
			std::cout << "ANALYSIS" << std::endl;
			size_t newWorkSize = 0;
			while (duration == 0.0f);
			duration = duration * nthreads;
			((Scoreboard<IN, OUT>*)scoreboard)->meanTime = duration;
			while (duration > 0.0f) {

				tstart = std::chrono::high_resolution_clock::now();
				output->at(newWorkSize) = elemental.elemental(input->at(newWorkSize), args...);
				tend = std::chrono::high_resolution_clock::now();
				std::cout << "Item Time: " << (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count() << "\n";
				duration -= (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count();
				newWorkSize++;
			}
			sizeOfWork = newWorkSize;

			// send work size

			((Scoreboard<IN, OUT>*)scoreboard)->meanTime /= sizeOfWork;
			if (sizeOfWork % 2 == 1)sizeOfWork--;
			((Scoreboard<IN, OUT>*)scoreboard)->startItems = sizeOfWork;
			std::cout << "Mean Time: " << ((Scoreboard<IN, OUT>*)scoreboard)->meanTime << "\n";
			((Scoreboard<IN, OUT>*)scoreboard)->curIndex = sizeOfWork;
			((Scoreboard<IN, OUT>*)scoreboard)->itemsCount = sizeOfWork;
		}


		// Paranthesis operator: call function
		// -----------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void operator()(std::vector<OUT> &output, std::vector<IN> &input, ARGs... args) {
			if (!isInitialised) {
				scoreboard = new Scoreboard<IN, OUT>();
				sizeOfWork = 0;
				duration = 0.0f;

				// USE THREADER
				// -----------------------------------------------------------------------------
				// start threader
				std::thread *threader;
				tstart = std::chrono::high_resolution_clock::now();
				threader = new std::thread(&DynamicMapImplementation<EL>::init<IN, OUT, ARGs...>, this, &output, &input, args...);
				tend = std::chrono::high_resolution_clock::now();
				duration = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count();
				// main thread analyses
				analyse(&output, &input, args...);


				// delete threader
				threader->join();
				delete threader;


				// USE ANALYSER
				// -----------------------------------------------------------------------------
				// start analyser
				//std::thread *analyser;
				//tstart = std::chrono::high_resolution_clock::now();
				////analyser = new std::thread(&DynamicMapImplementation<EL>::analyse<IN,OUT,ARGs...>, this, &output, &input, args...);
				//analyse(&output, &input, args...);
				//tend = std::chrono::high_resolution_clock::now();
				//duration = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count();

				// main thread initialises threads
				//init(output, input, args...);

				//analyser->join();
				//delete analyser;

				isInitialised = true;
			}

			// Join threads
			// ------------
			for (size_t t = 0; t < nthreads; ++t) {
				allThreads[t]->join(); delete allThreads[t];
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
