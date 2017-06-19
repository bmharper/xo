#pragma once

namespace xo {
namespace rx {

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
	std::vector<Observable*> Watching;

	void ObservableDestroyed(Observable* target);
};

// Implement this interface in order to notify Controls that your state has changed
class XO_API Observable {
public:
	friend class Observer;
	
	virtual ~Observable();

	// Inform observers that your state has changed
	void Touch();

private:
	std::vector<Observer*> Observers;

	void AddWatcher(Observer* watcher);    // Intended to be called only from Observer.Watch
	void RemoveWatcher(Observer* watcher); // Intended to be called only from Observer.~Observer
};

} // namespace rx
} // namespace xo