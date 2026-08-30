#pragma once
#include <thread>
#include <mutex>
#include "types.hpp"

// unique lock -> controlled
// lock gueard -> raii

// forward declaration
extern StepScratch stepScratch;
void step(const std::unordered_set<pos_t>& cells, StepScratch& stepScratch);

class SimWorker {
	std::thread worker;
	std::mutex mtx;
	bool workDone = true;
	bool startWork = false;
	bool killThread = false;
	std::unordered_set<pos_t>* cells_ptr = nullptr;
	std::condition_variable cv;

	void run() {
		std::unique_lock lock(mtx);
		while (true) {
			cv.wait(lock, [&]() {
				return startWork || killThread;
			});

			if (killThread) break;
			
			startWork = false;

			step(*cells_ptr, stepScratch);

			workDone = true;
			cv.notify_one();
		}
	}
public:
	SimWorker() : worker(&SimWorker::run, this) {};

	~SimWorker() {
		{
			std::lock_guard lock(mtx);
			killThread = true;
		}
		cv.notify_one();
		worker.join();
	}

	void submitWork(std::unordered_set<pos_t>& cells) {
		{
			std::lock_guard lock(mtx);
			cells_ptr = &cells;
			workDone = false;
			startWork = true;
		}
		cv.notify_one();
	}

	void waitForWork() {
		std::unique_lock lock(mtx);

		cv.wait(lock, [&] {
			return workDone;
		});
	}
};
