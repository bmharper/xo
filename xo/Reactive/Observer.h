#pragma once

namespace xo {
namespace rx {

// NOTE: When I first wrote Observer and Observable, I intended for them to be single-threaded, and usable
// for a purpose other than listening for reactive events. However, since adding a mutex to them,
// and intentionally making them usable from multiple threads, I think it's a good idea if one nails down
// clearly what the intention was for these two interfaces, and that we don't try and use them for
// anything other than the present use.
// The danger in other uses, is that of deadlock. It's quite conceivable that you end up with circular
// references of observers and observables, and they call each other, which ends up with either double
// entry into a mutex, or deadlock.

// Something that watches for changes.
// This is implemented by Control, in order to be notified when data changes.
// At the end of the event dispatch, that control will re-render itself.
class XO_API Observer {
public:
	friend class Observable;

	virtual ~Observer();

	// Called by an Observable when it has been modified
	virtual void ObservableTouched(Observable* target) = 0;

	// Add a target to the watch list. If the target is already on the list, then this is a no-op.
	void Watch(Observable* target);

private:
	std::mutex               WatchingLock;
	std::vector<Observable*> Watching;

	void ObservableDestroyed(Observable* target);
};

// Implement this interface in order to notify Controls that your state has changed
class XO_API Observable {
public:
	friend class Observer;
	friend class MutatorLock;
	friend class ObserverLock;

	virtual ~Observable();

	// Inform observers that your state has changed
	void Touch();

private:
	std::mutex             ObserversLock;
	std::vector<Observer*> Observers;

	void TouchInternal();
	void AddWatcher(Observer* watcher);    // Intended to be called only from Observer.Watch
	void RemoveWatcher(Observer* watcher); // Intended to be called only from Observer.~Observer
};

// Scoped lock around Observable, which automatically calls Touch() when it is destroyed.
// This is intended to be used as a catch-all mechanism for mutating an Observable object,
// especially from a background thread (ie not the UI thread).
// Usage of this is identical in spirit to std::lock_guard.
class XO_API MutatorLock {
public:
	MutatorLock(Observable& obs);
	~MutatorLock();

private:
	Observable* Obs;
};

// Scoped lock around Observable, which is used by DOM elements when they render.
// This is intended to be a simple mechanism for ensuring that the state of an
// Observable is always mutated and read atomically.
// Usage of this is identical in spirit to std::lock_guard.
class XO_API ObserverLock {
public:
	ObserverLock(Observable& obs);
	~ObserverLock();

private:
	Observable* Obs;
};

} // namespace rx
} // namespace xo