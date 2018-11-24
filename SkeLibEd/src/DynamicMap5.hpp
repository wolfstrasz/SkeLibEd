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
		Elemental<EL> elemental;
		size_t nthreads;
		std::thread **allThreads;

		std::chrono::high_resolution_clock::time_point tstart;
		std::chrono::high_resolution_clock::time_point tend;
		double duration = 0.0f;
		//	size_t sizeOfWork;
		void* scoreboard;

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

			// timing
			std::vector<double>* scoretiming;
			// constructor
			Scoreboard(std::vector<IN> *in, std::vector<OUT> *out, size_t nthreads) {
				this->input = in;
				this->output = out;
				isFinished = false;
				isInitialised = false;
				inputSize = in->size();
				curIndex = 0;
				jobSize = 0;
					scoretiming = new std::vector<double>(nthreads);
			}
			~Scoreboard() {}
		};



		// ThreadMap - function applied to each thread
		// --------------------------------------------
		template<typename IN, typename OUT, typename ...ARGs>
		void threadMap(Scoreboard<IN, OUT> *scoreboard, size_t id, ARGs... args) {
				std::chrono::high_resolution_clock::time_point thstart;
				std::chrono::high_resolution_clock::time_point thend;
			size_t elementsCount;
			size_t elementIndex;
			double timeForScore = 0.0f;

			while (!scoreboard->isInitialised);


			while (!scoreboard->isFinished) {
				// Lock scoreboard
				thstart = std::chrono::high_resolution_clock::now();
				while (!scoreboard->scoreboardLock.try_lock());
					thend = std::chrono::high_resolution_clock::now();
					timeForScore += (double)std::chrono::duration_cast<std::chrono::nanoseconds>(thend - thstart).count();
				if (scoreboard->isFinished) {
					scoreboard->scoreboardLock.unlock();
					break;
				}

				// get new data
				if (scoreboard->curIndex + scoreboard->jobSize < scoreboard->inputSize) {
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
				for (int elementsFinished = 0; elementsFinished < elementsCount; elementsFinished++) {
					//if (elementIndex + elementsFinished >= scoreboard->inputSize)
					//	std::cout << "ACCESS OUT OF RANGE AT THREAD: " << id << "\n";
					scoreboard->output->at(elementIndex + elementsFinished) = elemental.elemental(scoreboard->input->at(elementIndex + elementsFinished), args...);
				}
			}
				scoreboard->scoretiming->at(id) = timeForScore;
				//std::cout << "Time for score : " << timeForScore << "\n";

		}

		// Constructor
		// -----------
		DynamicMapImplementation(Elemental<EL> elemental) : elemental(elemental) {
			this->nthreads = std::thread::hardware_concurrency();
			//	this->sizeOfWork = 0;
			this->duration = 0.0f;
			//this->allThreads = new std::thread*[nthreads];
		}

	public:
		template <typename IN, typename OUT, typename ...ARGs>
		void start_init(std::vector<OUT> *output, std::vector<IN> *input, ARGs... args) {
			// create threads
			for (size_t t = 0; t < nthreads; t++) {
				allThreads[t] = new std::thread(&DynamicMapImplementation<EL>::threadMap<IN, OUT, ARGs...>,
					this, ((Scoreboard<IN, OUT>*)scoreboard), t, args...);
			}
		}

		template <typename IN, typename OUT, typename ...ARGs>
		void start_analysis(std::vector<OUT> *output, std::vector<IN> *input, ARGs... args) {

			size_t newJobSize = 0;
			while (duration == 0.0f);			// guard if we are using analyser
			//duration = duration * nthreads;
			duration = 1000000.0f; // 1milisec
			double durationAtStart = duration;
			// analyse worksize
			while (duration > 0.0f && newJobSize < input->size()) {
				tstart = std::chrono::high_resolution_clock::now();
				//	if (newJobSize >= input->size())
				//		std::cout << "ACCESS OUT OF RANGE AT THREAD: MAIN: " << newJobSize << "\n";
				output->at(newJobSize) = elemental.elemental(input->at(newJobSize), args...);
				tend = std::chrono::high_resolution_clock::now();
				duration -= (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count();
				newJobSize++;
			}
			std::cout << "JOBSIZE: " << newJobSize << "\n";
			std::cout << "TIME:  " << durationAtStart << "\n";
			std::cout << "MICRO: " << durationAtStart / 1000.0f << "\n";
			std::cout << "MILLI: " << durationAtStart / 1000.0f / 1000.0f << "\n";
			//sizeOfWork = newJobSize;
			//factor = duration / (1000.0f 1000.0f); //micro
			//duration = duration / 1000.0f // milli
			//if(factor < 1.0f)
			// send work size
			while (!((Scoreboard<IN, OUT>*)scoreboard)->scoreboardLock.try_lock());
			if (newJobSize == input->size())((Scoreboard<IN, OUT>*)scoreboard)->isFinished = true;
			((Scoreboard<IN, OUT>*)scoreboard)->curIndex = newJobSize;
			((Scoreboard<IN, OUT>*)scoreboard)->jobSize = newJobSize;
			((Scoreboard<IN, OUT>*)scoreboard)->isInitialised = true;
			((Scoreboard<IN, OUT>*)scoreboard)->scoreboardLock.unlock();
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
			tstart = std::chrono::high_resolution_clock::now();
			threader = new std::thread(&DynamicMapImplementation<EL>::start_init<IN, OUT, ARGs...>, this, &output, &input, args...);
			tend = std::chrono::high_resolution_clock::now();
			duration = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count();
			// main thread analyses
		//	std::cout << "MAIN THREAD ANALYSIS\n";
			start_analysis(&output, &input, args...);


			// delete threader
			threader->join();
			delete threader;
			//	std::cout << "THREADER DELETED\n";

				// USE ANALYSER
				// -----------------------------------------------------------------------------------
				// start analyser
				//std::thread *analyser;
				//tstart = std::chrono::high_resolution_clock::now();
				////analyser = new std::thread(&DynamicMapImplementation<EL>::start_analysis<IN,OUT,ARGs...>, this, &output, &input, args...);
				//analyse(&output, &input, args...);
				//tend = std::chrono::high_resolution_clock::now();
				//duration = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(tend - tstart).count();

				// main thread initialises threads
				//start_init(output, input, args...);

				//analyser->join();
				//delete analyser;


				// Join threads
				// -----------------------------------------------------------------------------------
			for (size_t t = 0; t < nthreads; ++t) {
				allThreads[t]->join();
				//	std::cout << "THREAD ID:  " << t << "\n";
				std::cout << "SCORE TIME: " << ((Scoreboard<IN, OUT>*)scoreboard)->scoretiming->at(t) / 1000.0f / 1000.0f << "\n";
				//	std::cout << "INIT  TIME: " << ((Scoreboard<IN, OUT>*)scoreboard)->inittiming->at(t) << "\n";

				delete allThreads[t];
			}
			delete allThreads;
			delete ((Scoreboard<IN, OUT>*)scoreboard);

		}

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
