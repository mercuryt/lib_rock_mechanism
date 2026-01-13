#pragma once
#include <cstdint>
#include <cassert>
#include <limits>
#include <atomic>
#ifndef NDEBUG
	#define DEBUG_ASSERT_REFERENCE_COUNT
#endif
template<class T>
class AssertReference;
template<class T, typename Count = int8_t, Count Max = std::numeric_limits<Count>::max()>
class AssertReferenceCount
{
	T m_data;
#ifdef DEBUG_ASSERT_REFERENCE_COUNT
	std::atomic<Count> m_count = 0;
	[[nodiscard]] Count getMax() const { return Max; }
public:
	// On create reference increment count.
	AssertReference<T> getRef() { assert(m_count < Max); ++m_count; return {this}; }
	// Validate no dangling references.
	~AssertReferenceCount() { assert(m_count == 0); }
#else
public:
	// Get reference without count.
	AssertReference<T> getRef() { return {this}; }
#endif
	AssertReferenceCount(T data) : m_data(data) { }
	// On move: object being moved is destroyed and will implicitly assert count is 0.
	AssertReferenceCount(const AssertReferenceCount&& other) = default;
	// On copy: copy data but allow count to remain at 0.
	AssertReferenceCount(const AssertReferenceCount& other) : m_data(other.m_data) { }
	friend class AssertReference<T>;
};
template<class T>
class AssertReference
{
	AssertReferenceCount<T>* m_data;
public:
#ifdef DEBUG_ASSERT_REFERENCE_COUNT
	// On copy increment count.
	AssertReference(const AssertReference& other) { assert(m_data->m_count < m_data->getMax()); m_data = other.m_data; ++m_data->m_count; }
	// On destroy decrement count.
	~AssertReference() { assert(m_data->m_count != 0); --m_data->m_count; }
#else
	// Copy without count.
	AssertReference(const AssertReference& other) { m_data = other.m_data; }
#endif
	AssertReference(AssertReferenceCount<T>* data) : m_data(data) { }
	// On move the old one is destroyed and a new one is created, no need to change count.
	AssertReference(const AssertReference&&) = default;
	void clear() { ~this(); m_data = nullptr; }
	[[nodiscard]] bool exists() const { return m_data != nullptr; }
	[[nodiscard]] bool empty() const { return m_data == nullptr; }
	[[nodiscard]] T& operator*() { return *m_data->m_data; }
	[[nodiscard]] T* operator->() { return m_data->m_data; }
	[[nodiscard]] T& get() { return *m_data->m_data; }
};
