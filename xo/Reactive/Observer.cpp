#include "pch.h"
#include "Observer.h"

/*

In all of this code, you'll notice a pattern:
When removing an item, we do not shorten the list. We just make the slot null.
This is not necessary for the use cases that I envisage with the reactive system here.
The reason I say that, is because the reactive system is not supposed to be doing
re-rendering in immediate response to an ObservableTouched event. Instead, the
Control that received the ObservableTouched event should rather be setting Dirty = true,
and then waiting for the next DocProcess message, and only during that message,
perform re-rendering.

Anyway, I've been bitten by this kind of thing in the past, so I think it's a reasonable
approach to make this kind of code resilient to callees modifying caller's state
unexpectedly. It's also likely to be faster, since you get rid of the memory bumps
associated with erasing from an array, and there's no risk for allocation thrashing.

*/

namespace xo {
namespace rx {

Observer::~Observer() {
	std::lock_guard<std::mutex> lock(WatchingLock);
	for (size_t i = 0; i < Watching.size(); i++) {
		if (Watching[i])
			Watching[i]->RemoveWatcher(this);
	}
}

void Observer::Watch(Observable* target) {
	{
		std::lock_guard<std::mutex> lock(WatchingLock);
		size_t empty = -1;
		for (size_t i = 0; i < Watching.size(); i++) {
			if (Watching[i] == target)
				return;
			else if (empty == -1 && Watching[i] == nullptr)
				empty = i;
		}

		if (empty != -1)
			Watching[empty] = target;
		else
			Watching.push_back(target);
	}

	target->AddWatcher(this);
}

void Observer::ObservableDestroyed(Observable* target) {
	std::lock_guard<std::mutex> lock(WatchingLock);
	for (size_t i = 0; i < Watching.size(); i++) {
		if (Watching[i] == target) {
			Watching[i] = nullptr;
			return;
		}
	}
	XO_DIE_MSG("observable not found in Observer::ObservableDestroyed");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Observable::~Observable() {
	std::lock_guard<std::mutex> lock(ObserversLock);
	for (size_t i = 0; i < Observers.size(); i++) {
		if (Observers[i])
			Observers[i]->ObservableDestroyed(this);
	}
}

void Observable::Touch() {
	std::lock_guard<std::mutex> lock(ObserversLock);
	TouchInternal();
}

void Observable::TouchInternal() {
	for (size_t i = 0; i < Observers.size(); i++) {
		if (Observers[i])
			Observers[i]->ObservableTouched(this);
	}
}

void Observable::AddWatcher(Observer* watcher) {
	std::lock_guard<std::mutex> lock(ObserversLock);
	for (size_t i = 0; i < Observers.size(); i++) {
		if (!Observers[i]) {
			Observers[i] = watcher;
			return;
		}
	}
	Observers.push_back(watcher);
}

void Observable::RemoveWatcher(Observer* watcher) {
	std::lock_guard<std::mutex> lock(ObserversLock);
	for (size_t i = 0; i < Observers.size(); i++) {
		if (Observers[i] == watcher) {
			Observers[i] = nullptr;
			return;
		}
	}
	XO_DIE_MSG("watcher not found in Observable::RemoveWatcher");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MutateLock::MutateLock(Observable& obs) : Obs(&obs) {
	Obs->ObserversLock.lock();
}

MutateLock::~MutateLock() {
	Obs->TouchInternal();
	Obs->ObserversLock.unlock();
}

ObserveLock::ObserveLock(Observable& obs) : Obs(&obs) {
	Obs->ObserversLock.lock();
}

ObserveLock::~ObserveLock() {
	Obs->ObserversLock.unlock();
}

} // namespace rx
} // namespace xo