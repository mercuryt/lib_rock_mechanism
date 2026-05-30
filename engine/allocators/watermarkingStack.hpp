#pragma once
#include "watermarkingStack.h"
#include <omp.h>
template<typename T>
void ThreadStripedWatermarkingStack<T>::init(int maxThreads)
{
	assert(s_buffer.empty());
	s_buffer.resize(maxThreads);
}
template<typename T>
std::vector<T>& ThreadStripedWatermarkingStack<T>::get()
{
	return s_buffer[(int)omp_get_thread_num()];
}
template<typename T>
ThreadStripedWatermarkingStack<T>::ThreadStripedWatermarkingStack() :
	m_start(get().size())
{ }
template<typename T>
ThreadStripedWatermarkingStack<T>::~ThreadStripedWatermarkingStack() { clear(); }
template<typename T>
T ThreadStripedWatermarkingStack<T>::operator[](int index) const { return get()[m_start + index]; }
template<typename T>
T& ThreadStripedWatermarkingStack<T>::operator[](int index) { return get()[m_start + index]; }
template<typename T>
T ThreadStripedWatermarkingStack<T>::popAndReturnBack()
{
	auto& buffer = get();
	T output = buffer.back();
	buffer.pop_back();
	return output;
}
template<typename T>
T ThreadStripedWatermarkingStack<T>::back() { return get().back(); }
template<typename T>
T ThreadStripedWatermarkingStack<T>::front() { return get().front(); }
template<typename T>
bool ThreadStripedWatermarkingStack<T>::empty() const { return (int)get().size() == m_start; }
template<typename T>
bool ThreadStripedWatermarkingStack<T>::contains(const T& value) const { return find(value) != end(); }
template<typename T>
int ThreadStripedWatermarkingStack<T>::size() const { return (int)get().size() - m_start; }
template<typename T>
void ThreadStripedWatermarkingStack<T>::insert(T datum) { return get().push_back(datum); }
template<typename T>
void ThreadStripedWatermarkingStack<T>::popBack() { return get().pop_back(); }
template<typename T>
void ThreadStripedWatermarkingStack<T>::clear() { return get().resize(m_start); }
// Iterator.
template<typename T>
T ThreadStripedWatermarkingStack<T>::iterator::operator*() const { return {ThreadStripedWatermarkingStack<T>::get()[m_index] }; }
template<typename T>
T& ThreadStripedWatermarkingStack<T>::iterator::operator*() { return {ThreadStripedWatermarkingStack<T>::get()[m_index] }; }
template <typename T>
void ThreadStripedWatermarkingStack<T>::iterator::operator++() { ++m_index; }
template<typename T>
void ThreadStripedWatermarkingStack<T>::iterator::operator--() { --m_index; }
template<typename T>
ThreadStripedWatermarkingStack<T>::iterator ThreadStripedWatermarkingStack<T>::begin() { return {m_start}; }
template<typename T>
ThreadStripedWatermarkingStack<T>::iterator ThreadStripedWatermarkingStack<T>::end() { return {(int)ThreadStripedWatermarkingStack<T>::get().size()}; }
template<typename T>
ThreadStripedWatermarkingStack<T>::iterator ThreadStripedWatermarkingStack<T>::find(T value)
{
	auto& buffer = ThreadStripedWatermarkingStack<T>::get();
	auto found = std::ranges::find(buffer.begin() + m_start, buffer.end(), value);
	if(found == buffer.end())
		return {(int)buffer.size()};
	return {(int)(found - buffer.begin())};
}
template<typename T>
const T ThreadStripedWatermarkingStack<T>::const_iterator::operator*() const { return ThreadStripedWatermarkingStack<T>::get()[static_cast<const iterator*>(this)->m_index]; }
template<typename T>
ThreadStripedWatermarkingStack<T>::const_iterator ThreadStripedWatermarkingStack<T>::begin() const { return {m_start}; }
template<typename T>
ThreadStripedWatermarkingStack<T>::const_iterator ThreadStripedWatermarkingStack<T>::end() const { return {(int)ThreadStripedWatermarkingStack<T>::get().size() }; }
template<typename T>
ThreadStripedWatermarkingStack<T>::const_iterator ThreadStripedWatermarkingStack<T>::find(T value) const { return {const_cast<ThreadStripedWatermarkingStack<T>*>(this)->find(value).m_index }; }