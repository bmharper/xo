#pragma once

#include <string.h> // memset()
#include <stdint.h>
#include <initializer_list>
#include "Alloc.h"
#include "Asserts.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4345) // POD type is default-initialized with () syntax
#endif

namespace xo {

/* cheapvec is a cut-down vector<>, that gets around the need to implement
move semantics, for specific cases.

It is 'cheap' because you don't need to implement the plethora of functions
that are usually required to satisfy move semantic requirements.

cheapvec assumes that it's members can be moved with memcpy. Note that this
requirement fails for some implementations of std::string, which store a pointer
that points to memory inside itself.

Why not just use move semantics and std::vector?

Move semantics require a lot of boilerplate code. Generally, you need to write
a copy constructor, an operator=, a move constructor, and move operator=. These
four functions are very similar in nature, but they all need to be implemented
independently. By using cheapvec, and classes that don't store internal pointers,
you get the benefit of move semantics without all the boilerplate.

cheapvec implements iterators that make it usable with most STL code,
such as sort().

*/

// Return true if your type has no constructor or destructor.
template <typename T>
bool cheapvec_ispod() {
	return false;
}

template <typename T>
class cheapvec {
public:
	typedef T value_type;
	typedef T TElem;

	size_t count    = 0;
	size_t capacity = 0;
	T*     data     = nullptr;

	static bool ispod() { return cheapvec_ispod<T>(); }
	static void panic() { XO_DIE(); }
	cheapvec() {
	}
	cheapvec(const cheapvec& b) {
		*this = b;
	}
	cheapvec(cheapvec&& b) {
		std::swap(count, b.count);
		std::swap(capacity, b.capacity);
		std::swap(data, b.data);
	}
	cheapvec(std::initializer_list<T> list) {
		if (ispod()) {
			resize_uninitialized(list.size());
			memcpy(data, list.begin(), sizeof(T) * list.size());
		} else {
			resize(list.size());
			for (size_t i = 0; i < list.size(); i++)
				data[i] = list.begin()[i];
		}
	}

	~cheapvec() {
		clear();
	}

	size_t size() const { return count; }
	// We are consistent here with the stl's erase, which takes [start,last), where last is one beyond the last element to erase.
	void erase(size_t start, size_t end = -1) {
		if (end == -1)
			end = start + 1;

		// on an empty set, you must be able to do erase(0,0), and have it be a nop
		if (start == end)
			return;

		// erase is rare enough that I feel it's worth it to pay this branch penalty for an early crash.
		if (start >= count)
			panic();

		if (end > count)
			panic();

		size_t remain = count - end;
		for (size_t i = start; i < end; i++)
			dtor(data[i]);
		memmove(data + start, data + end, remain * sizeof(T));
		memset(data + start + remain, 0, (end - start) * sizeof(T));
		count = start + remain;
	}

	// written originally for adding bytes to a cheapvec<u8>. Will grow the buffer in powers of 2.
	void addn(const T* arr, size_t n) {
		if (isinternal(arr))
			panic();
		if (count + n > capacity)
			growfor(count + n);
		if (ispod()) {
			memcpy(data + count, arr, n * sizeof(T));
			count += n;
		} else {
			for (size_t i = 0; i < n; i++)
				data[count++] = arr[i];
		}
	}

	T& add() {
		push(T());
		return back();
	}

	void resize(size_t newsize) {
		resize_internal(newsize, true);
	}

	// This is intended to be the equivalent of a raw malloc(), for the cases where you don't
	// want to pay the cost of zero-initializing the memory that you're about to fill up anyway.
	void resize_uninitialized(size_t newsize) {
		resize_internal(newsize, false);
	}

	void reserve(size_t newcap) {
		// reserve may not alter count, therefore we do nothing in this case
		if (newcap <= count)
			return;
		resizeto(newcap, true);
	}

	// This is intended to be the equivalent of a raw malloc(), for the cases where you don't
	// want to pay the cost of zero-initializing the memory that you're about to fill up anyway.
	void reserve_uninitialized(size_t newcap) {
		// reserve may not alter count, therefore we do nothing in this case
		if (newcap <= count)
			return;
		resizeto(newcap, false);
	}

	size_t remaining_capacity() const {
		return capacity - count;
	}

	void fill(const T& v) {
		for (size_t i = 0; i < count; i++)
			data[i] = v;
	}

	void insert(size_t pos, const T& v) {
		if (pos > count)
			panic();
		if (isinternal(&v)) {
			T copy = v;
			insert(pos, copy);
		} else {
			grow();
			if (count != capacity)
				dtorz_block(1, data + count);
			memmove(data + pos + 1, data + pos, (count - pos) * sizeof(T));
			ctor(data[pos]);
			data[pos] = v;
		}
		count++;
	}

	void push_back(const T& v) { push(v); }
	void pop_back() { count--; }
	void pop() { pop_back(); }
	T    rpop() {
        T v = back();
        pop();
        return v;
	}

	void push(const T& v) {
		if (count == capacity) {
			if (isinternal(&v)) {
				T copy = v;
				push(copy);
			} else {
				grow();
				data[count++] = v;
			}
		} else {
			data[count++] = v;
		}
	}

	void clear_noalloc() { count = 0; }
	void clear() {
		dtorz_block(capacity, data);
		free(data);
		count    = 0;
		capacity = 0;
		data     = nullptr;
	}

	size_t find(const T& v) const {
		for (size_t i = 0; i < count; i++) {
			if (data[i] == v)
				return i;
		}
		return -1;
	}
	bool      contains(const T& v) const { return find(v) != -1; }
	T&        get(size_t i) { return data[i]; }
	const T&  get(size_t i) const { return data[i]; }
	void      set(size_t i, const T& v) { data[i] = v; }
	T&        operator[](size_t i) { return data[i]; }
	const T&  operator[](size_t i) const { return data[i]; }
	T&        front() { return data[0]; }
	const T&  front() const { return data[0]; }
	T&        back() { return data[count - 1]; }
	const T&  back() const { return data[count - 1]; }
	cheapvec& operator+=(const T& v) {
		push(v);
		return *this;
	}
	cheapvec& operator+=(const cheapvec& b) {
		addn(b.data, b.count);
		return *this;
	}
	void operator=(const cheapvec& b) {
		if (count != b.count) {
			clear();
			resizeto(b.count, !ispod());
		}
		count = b.count;
		if (ispod()) {
			memcpy(data, b.data, count * sizeof(T));
		} else {
			for (size_t i = 0; i < b.count; i++)
				data[i] = b.data[i];
		}
	}

	void operator=(cheapvec&& b) {
		clear();
		std::swap(count, b.count);
		std::swap(capacity, b.capacity);
		std::swap(data, b.data);
	}

	void grow() { growfor(count + 1); }
	class iterator {
	private:
		cheapvec* vec;
		size_t    pos;

	public:
		iterator(cheapvec* _vec, size_t _pos) : vec(_vec), pos(_pos) {}
		bool      operator!=(const iterator& b) const { return pos != b.pos; }
		T&        operator*() const { return vec->data[pos]; }
		iterator& operator++() {
			pos++;
			return *this;
		}
	};
	friend class iterator;

	class const_iterator {
	private:
		const cheapvec* vec;
		size_t          pos;

	public:
		const_iterator(const cheapvec* _vec, size_t _pos) : vec(_vec), pos(_pos) {}
		bool            operator!=(const const_iterator& b) const { return pos != b.pos; }
		const T&        operator*() const { return vec->data[pos]; }
		const_iterator& operator++() {
			pos++;
			return *this;
		}
	};
	friend class const_iterator;

	iterator       begin() { return iterator(this, 0); }
	iterator       end() { return iterator(this, count); }
	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end() const { return const_iterator(this, count); }

protected:
	void dtor(T& v) {
		if (!ispod())
			v.T::~T();
	}
	void dtorz_block(size_t n, T* block) {
		if (ispod())
			return;
		for (size_t i = 0; i < n; i++)
			block[i].T::~T();
		// memset(block, 0, sizeof(T) * n); - BMH 2016-09-12
	}
	void ctor(T& v) {
		new (&v) T();
	}
	void ctor_block(size_t n, T* block) {
		if (ispod()) {
			memset(block, 0, sizeof(T) * n);
		} else {
			for (size_t i = 0; i < n; i++)
				new (&block[i]) T();
		}
	}

	bool isinternal(const T* p) const {
		return (size_t)(p - data) < capacity;
	}

	void growfor(size_t target) {
		// Regular growth rate is 2.0, which is what most containers (.NET, STL) use.
		// There is no theoretical optimal. It's simply a trade-off between memcpy time and wasted VM.
		size_t ncap = capacity ? capacity : 1;
		while (ncap < target)
			ncap = ncap * 2;
		resizeto(ncap, true);
	}

	void resize_internal(size_t newsize, bool initmem) {
		if (newsize == count)
			return;
		if (newsize == 0) {
			clear();
			return;
		}
		if (newsize < capacity)
			dtorz_block(capacity - newsize, data + newsize);
		resizeto(newsize, initmem);
		count = newsize;
	}

	void resizeto(size_t newcap, bool initmem) {
		if (newcap == 0)
			return;
		data = (T*) ReallocOrDie(data, newcap * sizeof(T));
		if (newcap > capacity && initmem)
			ctor_block(newcap - capacity, data + capacity);
		capacity = newcap;
	}
};

template <typename T>
bool operator==(const cheapvec<T>& a, const cheapvec<T>& b) {
	size_t asize = a.size();
	if (asize != b.size())
		return false;
	if (cheapvec_ispod<T>()) {
		return memcmp(a.data, b.data, asize * sizeof(T)) == 0;
	} else {
		for (size_t i = 0; i < asize; i++) {
			if (a.data[i] != b.data[i])
				return false;
		}
		return true;
	}
}
template <typename T>
bool operator!=(const cheapvec<T>& a, const cheapvec<T>& b) {
	return !(a == b);
}

template <>
inline bool cheapvec_ispod<int8_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<int16_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<int32_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<int64_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<uint8_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<uint16_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<uint32_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<uint64_t>() {
	return true;
}
template <>
inline bool cheapvec_ispod<float>() {
	return true;
}
template <>
inline bool cheapvec_ispod<double>() {
	return true;
}

template <>
inline bool cheapvec_ispod<bool>() {
	return true;
}
template <>
inline bool cheapvec_ispod<void*>() {
	return true;
}
} // namespace xo

namespace std {
template <typename T>
inline void swap(xo::cheapvec<T>& a, xo::cheapvec<T>& b) {
	char tmp[sizeof(xo::cheapvec<T>)];
	memcpy(tmp, &a, sizeof(a));
	memcpy(&a, &b, sizeof(a));
	memcpy(&b, tmp, sizeof(a));
}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
