#pragma once

#include <aegis.hpp>

class DelayedTasker {
	std::thread tasking;

	std::mutex delayed_tasks_m;
	std::vector<std::pair<std::chrono::system_clock::duration, std::function<void(void)>>> delayed_tasks;

	bool keep = true;

	void loop();
public:
	DelayedTasker();
	~DelayedTasker();

	void close(const bool = true);

	// ms, func
	void delay(const unsigned long long, std::function<void(void)>);

	size_t delayed_tasks_size() const;
};