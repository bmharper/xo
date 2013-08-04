#ifndef SCOPEOVERRIDE_H
#define SCOPEOVERRIDE_H

/** A small scope-limited value overrider.
The original value is reset when the object goes out of scope.
**/
template < typename TTarget >
class ScopeOverride
{
public:
	/** Remembers original value and applies override value, if apply = true.
	**/
	ScopeOverride( TTarget& source, TTarget overrideValue, bool apply = true )
	{
		TargetLocation = &source;
		OriginalValue = source;
		if ( apply )
		{
			source = overrideValue;
		}
	}
	/// Passive- does not change current value- only resets when exiting scope.
	ScopeOverride( TTarget& source )
	{
		TargetLocation = &source;
		OriginalValue = source;
	}
	~ScopeOverride()
	{
		*TargetLocation = OriginalValue;
	}

private:
	TTarget* TargetLocation;
	TTarget OriginalValue;
};

typedef ScopeOverride< bool > ScopeOverrideBool;
typedef ScopeOverride< int >	ScopeOverrideInt;

/** A small scope-limited lock-count incrementer.
The lock count is decremented when the objects goes out of scope.
**/
template < typename TCounter >
class ScopeLock
{
public:
	/** Remembers original value and applies override value, if apply = true.
	**/
	ScopeLock( TCounter& counter )
	{
		CounterLocation = &counter;
		(*CounterLocation)++;
	}
	~ScopeLock()
	{
		(*CounterLocation)--;
	}

private:
	TCounter* CounterLocation;
};

typedef ScopeLock< int > ScopeLockInt;

#endif
