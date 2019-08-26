/*
 * container.h
 *
 *  Created on: 2017年4月21日
 *      Author: Jugo
 */

#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>

using namespace std;

template<typename T>
class create_set
{
private:
	set<T> m_set;
public:
	create_set(const T& key)
	{
		m_set.insert(key);
	}

	create_set<T>& operator()(const T& key)
	{
		m_set.insert(key);
		return *this;
	}

	operator set<T>()
	{
		return m_set;
	}
};
// static set<string> setSample = create_set<string>("");

template<typename T>
class create_vector
{
private:
	vector<T> m_vector;
public:
	create_vector(const T& key)
	{
		m_vector.push_back(key);
	}

	create_vector<T>& operator()(const T& key)
	{
		m_vector.push_back(key);
		return *this;
	}

	operator vector<T>()
	{
		return m_vector;
	}
};
// static vector<string> vecSample = create_vector<string>("");

template<typename T, typename U>
class create_map
{
private:
	map<T, U> m_map;
public:
	create_map(const T& key, const U& val)
	{
		m_map[key] = val;
	}

	create_map<T, U>& operator()(const T& key, const U& val)
	{
		m_map[key] = val;
		return *this;
	}

	operator map<T, U>()
	{
		return m_map;
	}
};
// static map<string, string> mapSample = create_map<string, string>("","");

template<typename T, typename U, typename V, typename W>
class create_map_muliti
{
private:
	map<T, map<U, map<V, W> > > m_map;
public:
	create_map_muliti(const T& key1, const U& key2, const V& key3, const W& val)
	{
		map<V, W> m1;
		m1[key3] = val;
		map<U, map<V, W> > m2;
		m2[key2] = m1;
		m_map[key1] = m2;
		m1.clear();
		m2.clear();
	}

	create_map_muliti<T, U, V, W>& operator()(const T& key1, const U& key2, const V& key3, const W& val)
	{
		map<V, W> m1;
		m1[key3] = val;
		map<U, map<V, W> > m2;
		m2[key2] = m1;
		m_map[key1] = m2;
		m1.clear();
		m2.clear();
		return *this;
	}

	operator std::map<T, map<U, map<V, W> > >()
	{
		return m_map;
	}

	string find(const T& key1, const U& key2, const V& key3)
	{
		return m_map.find(key1).find(key2).find(key2);
	}
};

//static map<int, map<int, map<int, string> > > mapCommand = create_map_muliti<int, int, int, string>(1, 0, 1, "CTL_SYSTEM_ON");
