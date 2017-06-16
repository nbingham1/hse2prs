/*
 * path.cpp
 *
 *  Created on: Mar 14, 2013
 *      Author: nbingham
 */

#include "path.h"
#include "common.h"

path::path(int s)
{
	nodes.resize(s, 0);
}

path::path(int s, int f, int t)
{
	nodes.resize(s, 0);
	from.push_back(f);
	to.push_back(t);
}

path::path(int s, int f, vector<int> t)
{
	nodes.resize(s, 0);
	from.push_back(f);
	to.insert(to.end(), t.begin(), t.end());
}

path::path(int s, vector<int> f, int t)
{
	nodes.resize(s, 0);
	from.insert(from.end(), f.begin(), f.end());
	to.push_back(t);
}

path::path(int s, vector<int> f, vector<int> t)
{
	nodes.resize(s, 0);
	from.insert(from.end(), f.begin(), f.end());
	to.insert(to.end(), t.begin(), t.end());
}

path::~path()
{

}

int path::size()
{
	return nodes.size();
}

void path::clear()
{
	nodes.assign(nodes.size(), 0);
	from.clear();
	to.clear();
}

bool path::contains(int n)
{
	return (nodes[n] > 0);
}

void path::set(int n)
{
	nodes[n] = 1;
}

bool path::empty()
{
	for (int i = 0; i < (int)nodes.size(); i++)
		if (nodes[i] > 0)
			return false;
	return true;
}

vector<int> path::maxes()
{
	vector<int> r;
	int t = -1;
	for (size_t i = 0; i < nodes.size(); i++)
		if (nodes[i] > t)
			t = nodes[i];

	for (size_t i = 0; i < nodes.size() && t > 0; i++)
		if (nodes[i] == t)
			r.push_back(i);

	return r;
}

int path::max()
{
	int r = -1;
	int t = -1;
	int i;
	for (i = 0; i < (int)nodes.size(); i++)
		if (nodes[i] > t)
		{
			t = nodes[i];
			r = i;
		}

	return r;
}

path path::inverse()
{
	path result(nodes.size());
	int i;

	for (i = 0; i < (int)nodes.size(); i++)
		result.nodes[i] = 1 - nodes[i];

	result.from = from;
	result.to = to;

	return result;
}

path path::mask()
{
	path result(nodes.size());
	int i;

	for (i = 0; i < (int)nodes.size(); i++)
		result.nodes[i] = (nodes[i] > 0);

	result.from = from;
	result.to = to;

	return result;
}

int path::length()
{
	int result = 0;
	for (size_t i = 0; i < nodes.size(); i++)
		result += nodes[i];
	return result;
}

vector<int>::iterator path::begin()
{
	return nodes.begin();
}

vector<int>::iterator path::end()
{
	return nodes.end();
}

path &path::operator=(path p)
{
	nodes = p.nodes;
	from = p.from;
	to = p.to;
	return *this;
}

int &path::operator[](int i)
{
	return nodes[i];
}

ostream &operator<<(ostream &os, path p)
{
	vector<int>::iterator i;
	for (i = p.begin(); i != p.end(); i++)
		os << *i << " ";
	return os;
}

path operator+(path p1, path p2)
{
	path result(max(p1.size(), p2.size()));
	for (int i = 0; i < result.size(); i++)
		result[i] = p1[i] + p2[i];

	return result;
}

path operator/(path p1, int n)
{
	for (int i = 0; i < p1.size(); i++)
		p1[i] /= n;
	return p1;
}

path operator*(path p1, int n)
{
	for (int i = 0; i < p1.size(); i++)
		p1[i] *= n;
	return p1;
}

bool operator==(path p1, path p2)
{
	return p1.nodes == p2.nodes;
}

bool operator<(path p1, path p2)
{
	return p1.nodes < p2.nodes;
}


bool operator>(path p1, path p2)
{
	return p1.nodes > p2.nodes;
}
