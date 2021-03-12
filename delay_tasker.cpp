#include "delay_tasker.h"

void DelayedTasker::loop()
{
	while (keep)
	{
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		std::lock_guard<std::mutex> l(delayed_tasks_m);
		const auto now = std::chrono::system_clock::now().time_since_epoch();

		for (size_t p = 0; p < delayed_tasks.size() && keep; p++)
		{
			auto& i = delayed_tasks[p];
			if (now >= i.first)
			{
				try {
					if (i.second) i.second();
					else std::cout << "Empty delayed task?! Exception?! Skipped..." << std::endl;
				}
				catch (const std::exception& e) {
					std::cout << "Unhandled error at delayed task: " << e.what() << "!" << std::endl;
				}
				catch (...) {
					std::cout << "Unhandled error at delayed task!" << std::endl;
				}
				delayed_tasks.erase(delayed_tasks.begin() + p--);
			}

		}
	}


}

DelayedTasker::DelayedTasker()
{
	keep = true;
	tasking = std::thread([&] { loop(); });
}

DelayedTasker::~DelayedTasker()
{
	if (tasking.joinable())
	{
		if (keep) {
			keep = false;
		}
		tasking.join();

		std::lock_guard<std::mutex> l(delayed_tasks_m);
		delayed_tasks.clear();
	}
}

void DelayedTasker::close(const bool terminate_tasks)
{
	if (tasking.joinable())
	{
		if (keep) {
			keep = false;
		}
		tasking.join();

		std::lock_guard<std::mutex> l(delayed_tasks_m);
		if (terminate_tasks) {
			for (auto& i : delayed_tasks) if (i.second) i.second();
		}
		delayed_tasks.clear();
	}
}

void DelayedTasker::delay(const unsigned long long plust, std::function<void(void)> f)
{
	std::lock_guard<std::mutex> l(delayed_tasks_m);
	const auto timet = (std::chrono::system_clock::now().time_since_epoch() + std::chrono::milliseconds(plust));
	delayed_tasks.push_back({timet, std::move(f)});
}

size_t DelayedTasker::delayed_tasks_size() const
{
	return delayed_tasks.size();
}
