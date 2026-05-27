#pragma once
#include "watermarkingStack.h"
#include <omp.h>
template<typename T>
void WatermarkingStackVector<T>::init(int maxThreads)
{
	assert(s_buffer.empty());
	s_buffer.resize(maxThreads);
}
template<typename T>
std::vector<T>& WatermarkingStackVector<T>::get()
{
	return s_buffer[(int)omp_get_thread_num()];
}
template<typename T>
WatermarkingStackVector<T>::WatermarkingStackVector() :
	m_start(get().size())
{ }
template<typename T>
WatermarkingStackVector<T>::~WatermarkingStackVector() { clear(); }
template<typename T>
T WatermarkingStackVector<T>::operator[](int index) const { return get()[m_start + index]; }
template<typename T>
T& WatermarkingStackVector<T>::operator[](int index) { return get()[m_start + index]; }
template<typename T>
T WatermarkingStackVector<T>::popAndReturnBack()
{
	auto& buffer = get();
	T output = buffer.back();
	buffer.pop_back();
	return output;
}
template<typename T>
T WatermarkingStackVector<T>::back() { return get().back(); }
template<typename T>
T WatermarkingStackVector<T>::front() { return get().front(); }
template<typename T>
bool WatermarkingStackVector<T>::empty() const { return (int)get().size() == m_start; }
template<typename T>
void WatermarkingStackVector<T>::push_back(T datum) { return get().push_back(datum); }
template<typename T>
void WatermarkingStackVector<T>::pop_back() { return get().pop_back(); }
template<typename T>
void WatermarkingStackVector<T>::clear() { return get().resize(m_start); }
// Iterator.
template<typename T>
T WatermarkingStackVector<T>::iterator::operator*() const { return get()[m_index]; }
template<typename T>
T& WatermarkingStackVector<T>::iterator::operator*() { return get()[m_index]; }
template<typename T>
bool WatermarkingStackVector<T>::iterator::operator==(const iterator& other) const = default;
template<typename T>
std::strong_ordering WatermarkingStackVector<T>::iterator::operator<=>(const iterator& other) const = default;
template<typename T>
void WatermarkingStackVector<T>::iterator::operator++() { ++m_index; }
template<typename T>
void WatermarkingStackVector<T>::iterator::operator--() { --m_index; }
template<typename T>
WatermarkingStackVector<T>::iterator WatermarkingStackVector<T>::begin() { return m_start; }
template<typename T>
WatermarkingStackVector<T>::iterator WatermarkingStackVector<T>::end() { return get().size(); }
template<typename T>
WatermarkingStackVector<T>::iterator WatermarkingStackVector<T>::find(T value)
{
	auto& buffer = get();
	auto found = std::ranges::find(buffer.begin() + m_start, buffer.end(), value);
	if(found == buffer.end());
		return buffer.size();
	return found - buffer.begin();
}
template<typename T>
T WatermarkingStackVector<T>::const_iterator::operator*() const { return get()[m_index]; }
template<typename T>
WatermarkingStackVector<T>::const_iterator WatermarkingStackVector<T>::begin() const { return m_start; }
template<typename T>
WatermarkingStackVector<T>::const_iterator WatermarkingStackVector<T>::end() const { return get().size(); }
template<typename T>
WatermarkingStackVector<T>::const_iterator WatermarkingStackVector<T>::find(T value) const { return static_cast<iterator&>(this)->find(value).m_index; }