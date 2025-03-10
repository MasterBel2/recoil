/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONTAINER_UTIL_H
#define CONTAINER_UTIL_H

#include <algorithm>
#include <cassert>
#include <vector>

namespace spring {
	template<typename T, typename TV>
	static auto find(T& c, const TV& v) -> decltype(c.end())
	{
		return std::find(c.begin(), c.end(), v);
	}

	template<typename T, typename UnaryPredicate>
	static void MapEraseIf(T& c, UnaryPredicate p)
	{
		for (auto it = c.begin(); it != c.end(); ) {
			if (p(*it)) it = c.erase(it);
			else ++it;
		}
	}

	template<typename ForwardIt, typename T, typename Compare>
	ForwardIt BinarySearch(ForwardIt first, ForwardIt last, const T& value, Compare comp)
	{
		first = std::lower_bound(first, last, value, comp);
		return (!(first == last) && !(comp(value, *first))) ? first : last;
	}

	template<typename ForwardIt, typename T>
	ForwardIt BinarySearch(ForwardIt first, ForwardIt last, const T& value)
	{
		first = std::lower_bound(first, last, value);
		return (!(first == last) && !(value < *first)) ? first : last;
	}

	template<typename T, typename UnaryPredicate>
	static bool VectorEraseAllIf(std::vector<T>& v, UnaryPredicate p)
	{
		bool b = false;
		for (size_t i = 0; i < v.size(); /*NOOP*/) {
			if (p(v[i])) {
				v[i] = std::move(v.back());
				v.pop_back();
				b = true;
			}
			else {
				++i;
			}
		}

		return b;
	}


	template<typename T, typename UnaryPredicate>
	static bool VectorEraseIf(std::vector<T>& v, UnaryPredicate p)
	{
		auto it = std::find_if(v.begin(), v.end(), p);

		if (it == v.end())
			return false;

		*it = v.back();
		v.pop_back();
		return true;
	}

	template<typename T>
	static bool VectorErase(std::vector<T>& v, const T& e)
	{
		auto it = std::find(v.begin(), v.end(), e);

		if (it == v.end())
			return false;

		*it = v.back();
		v.pop_back();
		return true;
	}

	template<typename T, typename C>
	static bool VectorEraseUniqueSorted(std::vector<T>& v, const T& e, const C& c)
	{
		const auto iter = std::lower_bound(v.begin(), v.end(), e, c);

		if ((iter == v.end()) || (*iter != e))
			return false;

		for (size_t n = (iter - v.begin()); n < (v.size() - 1); n++) {
			std::swap(v[n], v[n + 1]);
		}

		v.pop_back();
		return true;
	}

	template<typename T, typename UniqPred>
	static void VectorUnique(std::vector<T>& v) {
		for (size_t i = 0; i < v.size(); i++) {
			for (size_t j = i + 1; j < v.size(); /*NOOP*/) {
				if (v[i] == v[j]) {
					v[j] = std::move(v.back());
					v.pop_back();
				}
				else {
					j++;
				}
			}
		}
	}

	template<typename T, typename UniqPred>
	static void VectorUnique(std::vector<T>& v, UniqPred uniqPred) {
		for (size_t i = 0; i < v.size(); i++) {
			for (size_t j = i + 1; j < v.size(); /*NOOP*/) {
				if (uniqPred(v[i], v[j])) {
					v[j] = std::move(v.back());
					v.pop_back();
				}
				else {
					j++;
				}
			}
		}
	}

	template<typename T, typename SortPred>
	static void VectorSortUnique(std::vector<T>& v, SortPred sortPred)
	{
		std::sort(v.begin(), v.end(), sortPred);
		auto last = std::unique(v.begin(), v.end());
		v.erase(last, v.end());
	}

	template<typename T, typename SortPred, typename UniqPred>
	static void VectorSortUnique(std::vector<T>& v, SortPred sortPred, UniqPred uniqPred)
	{
		std::sort(v.begin(), v.end(), sortPred);
		auto last = std::unique(v.begin(), v.end(), uniqPred);
		v.erase(last, v.end());
	}

	template<typename T>
	static void VectorSortUnique(std::vector<T>& v)
	{
		std::sort(v.begin(), v.end());
		auto last = std::unique(v.begin(), v.end());
		v.erase(last, v.end());
	}

	template<typename T>
	static bool VectorInsertUnique(std::vector<T>& v, const T& e, bool b = false)
	{
		// do not assume uniqueness, test for it
		if (b && std::find(v.begin(), v.end(), e) != v.end())
			return false;

		// assume caller knows best, skip the test
		assert(b || std::find(v.begin(), v.end(), e) == v.end());
		v.push_back(e);
		return true;
	}

	template<typename T, typename Pred>
	typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T&& item, Pred pred)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, pred), item);
	}

	template<
		typename T, typename Pred,
		typename = typename std::enable_if_t<std::is_pointer_v<std::remove_cv_t<T>>>
	>
	typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T item, Pred pred)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, pred), item);
	}

	template<typename T>
	typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T&& item)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
	}

	template<
		typename T,
		typename = typename std::enable_if_t<std::is_pointer_v<std::remove_cv_t<T>>>
	>
	typename std::vector<T>::iterator VectorInsertSorted(std::vector<T>& vec, T item)
	{
		return vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
	}

	template<typename T, typename Pred>
	static bool VectorInsertUniqueSorted(std::vector<T>& v, const T& e, Pred pred)
	{
		const auto iter = std::lower_bound(v.begin(), v.end(), e, pred);

		if ((iter != v.end()) && (*iter == e))
			return false;

		v.insert(iter, e);
		return true;
	}

	template<typename T>
	static T VectorBackPop(std::vector<T>& v) { T e = std::move(v.back()); v.pop_back(); return e; }
};

#endif

