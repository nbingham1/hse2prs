/*
 * path_space.cpp
 *
 *  Created on: Mar 14, 2013
 *      Author: nbingham
 */

#include "path_space.h"
#include "message.h"

path_space::path_space(int s) : total(s)
{

}

path_space::path_space(path p) : total(p.size(), p.from, p.to)
{
	if (!p.empty())
		push_back(p);
}

path_space::path_space(int s, int f, int t) : total(s, f, t)
{

}

path_space::path_space(int s, int f, vector<int> t) : total(s, f, t)
{

}

path_space::path_space(int s, vector<int> f, int t) : total(s, f, t)
{

}

path_space::path_space(int s, vector<int> f, vector<int> t) : total(s, f, t)
{

}


path_space::~path_space()
{

}

int path_space::size()
{
	return paths.size();
}

void path_space::merge(path_space s)
{
	paths.merge(s.paths);

	for (int i = 0; i < s.total.size(); i++)
		total[i] += s.total[i];

	total.from.insert(total.from.end(), s.total.from.begin(), s.total.from.end());
	total.to.insert(total.to.end(), s.total.to.begin(), s.total.to.end());
}

void path_space::push_back(path p)
{
	paths.push_back(p);

	for (int i = 0; i < p.size(); i++)
		total[i] += p[i];

	total.from.insert(total.from.end(), p.from.begin(), p.from.end());
	total.to.insert(total.to.end(), p.to.begin(), p.to.end());
}

list<path>::iterator path_space::erase(list<path>::iterator i)
{
	for (int j = 0; j < i->size(); j++)
		total[j] -= (*i)[j];

	return paths.erase(i);
}

list<path>::iterator path_space::begin()
{
	return paths.begin();
}

list<path>::iterator path_space::end()
{
	return paths.end();
}

void path_space::clear()
{
	paths.clear();
	total.clear();
}

void path_space::zero(int i)
{
	list<path>::iterator pi;
	for (pi = paths.begin(); pi != paths.end(); pi++)
	{
		(*pi)[i] = 0;
		if (pi->empty())
			pi = paths.erase(pi);
	}
	total[i] = 0;
}

void path_space::zero(vector<int> i)
{
	list<path>::iterator pi;
	int j;
	for (pi = paths.begin(); pi != paths.end(); pi++)
	{
		for (j = 0; j < (int)i.size(); j++)
			(*pi)[i[j]] = 0;
		if (pi->empty())
			pi = paths.erase(pi);
	}
	for (j = 0; j < (int)i.size(); j++)
		total[i[j]] = 0;
}

void path_space::inc(int i, int v)
{
	for (list<path>::iterator pi = paths.begin(); pi != paths.end(); pi++)
		(*pi)[i] += v;

	total[i] += paths.size()*v;
}

void path_space::dec(int i, int v)
{
	for (list<path>::iterator pi = paths.begin(); pi != paths.end(); pi++)
	{
		(*pi)[i] -= v;
		if (pi->empty())
			pi = paths.erase(pi);
	}
	total[i] -= paths.size()*v;
}

void path_space::inc(list<path>::iterator i, int j, int v)
{
	i->nodes[j] += v;
	total.nodes[j] += v;
}

void path_space::dec(list<path>::iterator i, int j, int v)
{
	total.nodes[j] -= i->nodes[j];
	i->nodes[j] -= v;

	if (i->nodes[j] < 0)
		i->nodes[j] = 0;

	total.nodes[j] += i->nodes[j];
}

void path_space::repair()
{
	total.nodes.resize(0, 0);
	total.from.clear();
	total.to.clear();
	for (list<path>::iterator p = paths.begin(); p != paths.end(); p++)
	{
		total.from.insert(total.from.end(), p->from.begin(), p->from.end());
		total.to.insert(total.to.end(), p->to.begin(), p->to.end());

		if (total.nodes.size() < p->nodes.size())
			total.nodes.resize(p->nodes.size(), 0);

		for (size_t i = 0; i < p->nodes.size(); i++)
			total.nodes[i] += p->nodes[i];
	}

	sort(total.from.begin(), total.from.end());
	total.from.resize(unique(total.from.begin(), total.from.end()) - total.from.begin());
	sort(total.to.begin(), total.to.end());
	total.to.resize(unique(total.to.begin(), total.to.end()) - total.to.begin());
}

int path_space::coverage_count(int n)
{
	return total[n];
}

int path_space::coverage_count(vector<int> n)
{
	int i;
	int m = 0;
	for (i = 0; i < (int)n.size(); i++)
		if (total[n[i]] > m)
			m = total[n[i]];
	return m;
}

int path_space::length()
{
	int result = 999999999;
	list<path>::iterator pi;
	for (pi = paths.begin(); pi != paths.end(); pi++)
		result = min(result, pi->length());
	return result;
}

vector<int> path_space::coverage_maxes()
{
	return total.maxes();
}

int path_space::coverage_max()
{
	return total.max();
}

path path_space::get_mask()
{
	path result(total.size(), total.from, total.to);
	for (int i = 0; i < (int)total.nodes.size(); i++)
		result.nodes[i] = (total[i] > 0);
	return result;
}

void path_space::apply_mask(path m)
{
	list<path>::iterator pi;
	int i;

	for (pi = paths.begin(); pi != paths.end(); pi++)
	{
		for (i = 0; i < (int)pi->size(); i++)
			(*pi)[i] *= m[i];
		if (pi->empty())
			pi = paths.erase(pi);
	}

	for (int i = 0; i < (int)total.nodes.size(); i++)
		total[i] *= m[i];
}

path_space path_space::inverse()
{
	list<path>::iterator i;
	path_space result(total.size());

	for (i = paths.begin(); i != paths.end(); i++)
		result.push_back(i->inverse());

	return result;
}

path_space path_space::coverage(int n)
{
	list<path>::iterator i;
	path_space result(total.size());

	for (i = paths.begin(); i != paths.end(); i++)
		if (i->contains(n))
			result.push_back(*i);

	return result;
}

path_space path_space::avoidance(int n)
{
	list<path>::iterator i;
	path_space result(total.size());

	for (i = paths.begin(); i != paths.end(); i++)
		if (!i->contains(n))
			result.push_back(*i);

	return result;
}

path_space &path_space::operator=(path_space s)
{
	paths.clear();
	total.clear();
	paths = s.paths;
	total = s.total;
	return *this;
}

void path_space::print_bounds(string name)
{
	log("", name + " " + to_string(total.from) + " -> " + to_string(total.to), __FILE__, __LINE__);
}

int &path_space::operator[](int i)
{
	return total[i];
}

path &path_space::operator()(int i)
{
	list<path>::iterator j;
	for (j = paths.begin(); j != paths.end() && i > 0; j++, i--);

	return *j;
}

ostream &operator<<(ostream &os, path_space p)
{
	list<path>::iterator i;
	for (i = p.paths.begin(); i != p.paths.end(); i++)
		os << *i << endl;
	return os;
}
