/*
 * petri.cpp
 *
 *  Created on: May 12, 2013
 *      Author: nbingham
 */

#include "petri.h"
#include "variable_space.h"
#include "message.h"

petri_index::petri_index()
{
	data = -1;
}

petri_index::petri_index(int idx, bool place)
{
	data = place ? idx : (idx | 0x80000000);
}

petri_index::petri_index(string str)
{
	data = strtol(str.substr(1).data(), NULL, 10);

	if (str[0] == 'T')
		data |= 0x80000000;
}

petri_index::~petri_index()
{

}

bool petri_index::is_place() const
{
	return data >= 0;
}

bool petri_index::is_trans() const
{
	return data < 0;
}

string petri_index::name() const
{
	return (data >= 0 ? "S" : "T") + to_string(data & 0x7FFFFFFF);
}

int petri_index::idx() const
{
	return data & 0x7FFFFFFF;
}

petri_index &petri_index::operator=(petri_index i)
{
	data = i.data;
	return *this;
}

petri_index &petri_index::operator--()
{
	--data;
	return *this;
}

petri_index &petri_index::operator++()
{
	++data;
	return *this;
}

petri_index &petri_index::operator--(int)
{
	data--;
	return *this;
}

petri_index &petri_index::operator++(int)
{
	data++;
	return *this;
}

bool operator==(petri_index i, petri_index j)
{
	return i.data == j.data;
}

bool operator!=(petri_index i, petri_index j)
{
	return i.data != j.data;
}

bool operator<(petri_index i, int j)
{
	return ((i.data & 0x7FFFFFFF) < j);
}

ostream &operator<<(ostream &os, petri_index i)
{
	os << i.name();
	return os;
}

bool operator>(petri_index i, petri_index j)
{
	return i.data > j.data;
}

bool operator<(petri_index i, petri_index j)
{
	return i.data < j.data;
}

bool operator>=(petri_index i, petri_index j)
{
	return i.data >= j.data;
}

bool operator<=(petri_index i, petri_index j)
{
	return i.data <= j.data;
}

petri_index operator+(petri_index i, int j)
{
	i.data += j;
	return i;
}

petri_index operator-(petri_index i, int j)
{
	i.data -= j;
	return i;
}

ostream &operator<<(ostream &os, petri_arc a)
{
	os << a.first << "->" << a.second;
	return os;
}

petri_node::petri_node()
{
	assumptions = 1;
}

petri_node::petri_node(canonical predicate, bool active)
{
	this->predicate = predicate;
	this->active = active;
	assumptions = 1;
}

petri_node::~petri_node()
{
}

pair<int, int> petri_node::sense_count()
{
	pair<int, int> result(0, 0);
	for (size_t i = 0; i < predicate.terms.size(); i++)
	{
		for (size_t j = 0; j < predicate.terms[i].size; j++)
		{
			if (predicate.terms[i].val(j) == 0)
				result.first++;
			else if (predicate.terms[i].val(j) == 1)
				result.second++;
		}
	}

	return result;
}

petri_net::petri_net()
{
	remote = false;
	elaborate = true;
	assumptions = 1;
}

petri_net::~petri_net()
{
}

petri_index petri_net::put_place(canonical root)
{
	S.push_back(petri_node(root, false));
	return petri_index(S.size()-1, true);
}

petri_index petri_net::put_transition(canonical root, bool active)
{
	T.push_back(petri_node(root, active));
	return petri_index(T.size()-1, false);
}

vector<petri_index> petri_net::put_places(vector<canonical> root)
{
	vector<petri_index> result;
	for (size_t i = 0; i < root.size(); i++)
		result.push_back(put_place(root[i]));
	return result;
}

vector<petri_index> petri_net::put_transitions(vector<canonical> root, bool active)
{
	vector<petri_index> result;
	for (size_t i = 0; i < root.size(); i++)
		result.push_back(put_transition(root[i], active));
	return result;
}

void petri_net::cut(petri_index node)
{
	for (size_t m = 0; m < M0.size(); )
	{
		if (M0[m] == node)
			M0.erase(M0.begin() + m);
		else
		{
			if (M0[m].is_place() == node.is_place() && M0[m] > node)
				M0[m]--;
			m++;
		}
	}

	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); )
	{
		if (arc->first == node || arc->second == node)
			arc = arcs.erase(arc);
		else
		{
			if (arc->first.is_place() == node.is_place() && arc->first > node)
				arc->first--;
			if (arc->second.is_place() == node.is_place() && arc->second > node)
				arc->second--;
			arc++;
		}
	}

	if (node.is_place())
		S.erase(S.begin() + node.data);
	else
		T.erase(T.begin() + (node.data & 0x7FFFFFFF));
}

void petri_net::cut(vector<petri_index> nodes)
{
	for (size_t i = 0; i < nodes.size(); i++)
	{
		cut(nodes[i]);
		for (size_t j = i+1; j < nodes.size(); j++)
			if (nodes[j].is_place() == nodes[i].is_place() && nodes[j] > nodes[i])
				nodes[j]--;
	}
}

vector<petri_index> petri_net::connect(vector<petri_index> from, vector<petri_index> to)
{
	for (size_t i = 0; i < from.size(); i++)
		for (size_t j = 0; j < to.size(); j++)
			connect(from[i], to[j]);

	return to;
}

petri_index petri_net::connect(vector<petri_index> from, petri_index to)
{
	for (size_t i = 0; i < from.size(); i++)
		connect(from[i], to);

	return to;
}

vector<petri_index> petri_net::connect(petri_index from, vector<petri_index> to)
{
	for (size_t j = 0; j < to.size(); j++)
		connect(from, to[j]);

	return to;
}

petri_index petri_net::connect(petri_index from, petri_index to)
{
	if (from.is_place() && to.is_trans())
	{
		arcs.push_back(petri_arc(from, to));
		at(from).active = at(from).active || at(to).active;
	}
	else if (from.is_trans() && to.is_place())
		arcs.push_back(petri_arc(from, to));
	else if (from.is_place() && to.is_place())
		error("", "illegal arc {" + from.name() + ", " + to.name() + "}", "", __FILE__, __LINE__);
	else if (from.is_trans() && to.is_trans())
		error("", "illegal arc {" + from.name() + ", " + to.name() + "}", "", __FILE__, __LINE__);

	return to;
}

petri_index petri_net::push_transition(petri_index from, canonical root, bool active)
{
	return connect(from, put_transition(root, active));
}

petri_index petri_net::push_transition(vector<petri_index> from, canonical root, bool active)
{
	return connect(from, put_transition(root, active));
}

petri_index petri_net::push_transition(petri_index from)
{
	return connect(from, put_transition(canonical(1), false));
}

petri_index petri_net::push_transition(vector<petri_index> from)
{
	return connect(from, put_transition(canonical(1), false));
}

vector<petri_index> petri_net::push_transitions(petri_index from, vector<canonical> root, bool active)
{
	return connect(from, put_transitions(root, active));
}

vector<petri_index> petri_net::push_transitions(vector<petri_index> from, vector<canonical> root, bool active)
{
	return connect(from, put_transitions(root, active));
}

petri_index petri_net::push_place(petri_index from)
{
	return connect(from, put_place(canonical(0)));
}

petri_index petri_net::push_place(vector<petri_index> from)
{
	return connect(from, put_place(canonical(0)));
}

vector<petri_index> petri_net::push_places(vector<petri_index> from)
{
	vector<petri_index> result;
	for (size_t i = 0; i < from.size(); i++)
		result.push_back(connect(from[i], put_place(canonical(0))));
	return result;
}

void petri_net::pinch_forward(petri_index n)
{
	vector<petri_index> n1 = next(n);
	vector<petri_index> n1p = prev(n1);
	vector<petri_index> from = prev(n), to = next(n1);
	n1.push_back(n);
	sort(n1.rbegin(), n1.rend());
	n1.resize(unique(n1.begin(), n1.end()) - n1.begin());

	vector<petri_index> intersect(max(n1.size(), M0.size()));
	vector<petri_index>::iterator last = set_intersection(M0.begin(), M0.end(), n1.begin(), n1.end(), intersect.begin());
	if (last - intersect.begin() > 0)
		M0.insert(M0.end(), from.begin(), from.end());

	connect(n1p, from);
	connect(from, to);

	cut(n1);
}

void petri_net::pinch_backward(petri_index n)
{
	vector<petri_index> n1 = prev(n);
	vector<petri_index> n1n = next(n1);
	vector<petri_index> from = prev(n1), to = next(n);
	n1.push_back(n);
	sort(n1.rbegin(), n1.rend());
	n1.resize(unique(n1.begin(), n1.end()) - n1.begin());

	bool done = false;
	for (size_t i = 0; !done && i < n1.size(); i++)
		if (find(M0.begin(), M0.end(), n1[i]) != M0.end())
		{
			M0.insert(M0.end(), to.begin(), to.end());
			done = true;
		}

	connect(to, n1n);
	connect(from, to);
	cut(n1);
}

void petri_net::insert(int a, canonical root, bool active)
{
	map<int, int> pbranch, cbranch;
	petri_index n0, n1;

	if (arcs[a].first.is_place())
	{
		n0 = put_transition(root, active);
		n1 = put_place(at(arcs[a].first).predicate);
	}
	else
	{
		n0 = put_place(at(arcs[a].second).predicate);
		n1 = put_transition(root, active);
	}

	connect(arcs[a].first, n0);
	connect(n0, n1);
	arcs[a].first = n1;
}

void petri_net::insert_alongside(petri_index from, petri_index to, canonical root, bool active)
{

}

petri_index petri_net::duplicate(petri_index n)
{
	petri_index result;
	if (n.is_place())
	{
		S.push_back(at(n));
		result = petri_index(S.size()-1, true);
	}
	else
	{
		T.push_back(at(n));
		result = petri_index(T.size()-1, false);
	}

	for (size_t i = 0; i < arcs.size(); i++)
	{
		if (arcs[i].first == n)
			arcs.push_back(petri_arc(result, arcs[i].second));
		if (arcs[i].second == n)
			arcs.push_back(petri_arc(arcs[i].first, result));
	}

	if (find(M0.begin(), M0.end(), n) != M0.end())
		M0.push_back(result);

	return result;
}

vector<petri_index> petri_net::duplicate(vector<petri_index> n)
{
	vector<petri_index> result;
	for (vector<petri_index>::iterator i = n.begin(); i != n.end(); i++)
		result.push_back(duplicate(*i));
	return result;
}

petri_index petri_net::merge(petri_index n0, petri_index n1)
{
	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
	{
		if (arc->second == n1)
			arcs.push_back(petri_arc(arc->first, n0));
		if (arc->first == n1)
			arcs.push_back(petri_arc(n0, arc->second));
	}

	at(n0).predicate |= at(n1).predicate;
	cut(n1);
	return n0;
}

petri_index petri_net::merge(vector<petri_index> n)
{
	petri_index n0 = n.back();
	n.pop_back();

	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
	{
		for (vector<petri_index>::iterator ni = n.begin(); ni != n.end(); ni++)
		{
			if (arc->second == *ni)
				arcs.push_back(petri_arc(arc->first, n0));
			if (arc->first == *ni)
				arcs.push_back(petri_arc(n0, arc->second));
		}
	}

	for (vector<petri_index>::iterator ni = n.begin(); ni != n.end(); ni++)
		at(n0).predicate |= at(*ni).predicate;
	cut(n);
	return n0;
}

vector<petri_index> petri_net::next(petri_index n)
{
	vector<petri_index> result;
	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		if (arc->first == n)
			result.push_back(arc->second);

	sort(result.rbegin(), result.rend());
	return result;
}

vector<petri_index> petri_net::next(vector<petri_index> n)
{
	vector<petri_index> result;
	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		for (vector<petri_index>::iterator ni = n.begin(); ni != n.end(); ni++)
			if (arc->first == *ni)
				result.push_back(arc->second);

	sort(result.rbegin(), result.rend());
	return result;
}

vector<petri_index> petri_net::prev(petri_index n)
{
	vector<petri_index> result;
	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		if (arc->second == n)
			result.push_back(arc->first);

	sort(result.rbegin(), result.rend());
	return result;
}

vector<petri_index> petri_net::prev(vector<petri_index> n)
{
	vector<petri_index> result;
	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		for (vector<petri_index>::iterator ni = n.begin(); ni != n.end(); ni++)
			if (arc->second == *ni)
				result.push_back(arc->first);

	sort(result.rbegin(), result.rend());
	return result;
}

vector<int> petri_net::outgoing(petri_index n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		if (arcs[a].first == n)
			result.push_back(a);

	return result;
}

vector<int> petri_net::outgoing(vector<petri_index> n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		for (vector<petri_index>::iterator ni = n.begin(); ni != n.end(); ni++)
			if (arcs[a].first == *ni)
				result.push_back(a);

	return result;
}

vector<int> petri_net::incoming(petri_index n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		if (arcs[a].second == n)
			result.push_back(a);

	return result;
}

vector<int> petri_net::incoming(vector<petri_index> n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		for (vector<petri_index>::iterator ni = n.begin(); ni != n.end(); ni++)
			if (arcs[a].second == *ni)
				result.push_back(a);

	return result;
}

vector<int> petri_net::next_arc(int n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		if (arcs[a].first == arcs[n].second)
			result.push_back(a);

	return result;
}

vector<int> petri_net::next_arc(vector<int> n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		for (vector<int>::iterator ni = n.begin(); ni != n.end(); ni++)
			if (arcs[a].first == arcs[*ni].second)
				result.push_back(a);

	return result;
}

vector<int> petri_net::prev_arc(int n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		if (arcs[a].second == arcs[n].first)
			result.push_back(a);

	return result;
}

vector<int> petri_net::prev_arc(vector<int> n)
{
	vector<int> result;
	for (size_t a = 0; a < arcs.size(); a++)
		for (vector<int>::iterator ni = n.begin(); ni != n.end(); ni++)
			if (arcs[a].second == arcs[*ni].first)
				result.push_back(a);

	return result;
}

bool petri_net::is_floating(petri_index n)
{
	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		if (arc->first == n || arc->second == n)
			return false;

	return true;
}

bool petri_net::are_connected(petri_index n0, petri_index n1)
{
	vector<petri_index> cf1, cf2;
	vector<petri_index> ct1, ct2;

	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
	{
		if ((arc->first == n0 && arc->second == n1) || (arc->first == n1 && arc->second == n0))
			return true;

		if (arc->first == n0)
			cf2.push_back(arc->second);
		else if (arc->second == n0)
			cf1.push_back(arc->first);
		if (arc->first == n1)
			ct2.push_back(arc->second);
		else if (arc->second == n1)
			ct1.push_back(arc->first);
	}

	sort(cf1.begin(), cf1.end());
	cf1.resize(unique(cf1.begin(), cf1.end()) - cf1.begin());
	sort(cf2.begin(), cf2.end());
	cf2.resize(unique(cf2.begin(), cf2.end()) - cf2.begin());
	sort(ct1.begin(), ct1.end());
	ct1.resize(unique(ct1.begin(), ct1.end()) - ct1.begin());
	sort(ct2.begin(), ct2.end());
	ct2.resize(unique(ct2.begin(), ct2.end()) - ct2.begin());

	for (vector<petri_index>::iterator i = cf2.begin(), j = ct1.begin(); i != cf2.end() && j != ct1.end();)
	{
		if (*i == *j)
			return true;
		else if (*i < *j)
			i++;
		else if (*i > *j)
			j++;
	}

	for (vector<petri_index>::iterator i = cf1.begin(), j = ct2.begin(); i != cf1.end() && j != ct2.end();)
	{
		if (*i == *j)
			return true;
		else if (*i < *j)
			i++;
		else if (*i > *j)
			j++;
	}

	return false;
}

bool petri_net::have_same_source(petri_index n0, petri_index n1)
{
	vector<petri_index> n0in;
	vector<petri_index> n1in;

	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
	{
		if (arc->second == n0)
			n0in.push_back(arc->first);
		if (arc->second == n1)
			n1in.push_back(arc->first);
	}

	sort(n0in.begin(), n0in.end());
	n0in.resize(unique(n0in.begin(), n0in.end()) - n0in.begin());
	sort(n1in.begin(), n1in.end());
	n1in.resize(unique(n1in.begin(), n1in.end()) - n1in.begin());

	bool diff = true;
	if (n0in.size() == n1in.size())
	{
		diff = false;
		for (vector<petri_index>::iterator n0i = n0in.begin(); n0i != n0in.end() && !diff; n0i++)
		{
			diff = true;
			for (vector<petri_index>::iterator n1i = n1in.begin(); n1i != n1in.end() && diff; n1i++)
				if (at(*n0i).predicate == at(*n1i).predicate)
					diff = false;
		}
	}

	return !diff;
}

bool petri_net::have_same_dest(petri_index n0, petri_index n1)
{
	vector<petri_index> n0out;
	vector<petri_index> n1out;

	for (vector<petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
	{
		if (arc->first == n0)
			n0out.push_back(arc->second);
		if (arc->first == n1)
			n1out.push_back(arc->second);
	}

	sort(n0out.begin(), n0out.end());
	n0out.resize(unique(n0out.begin(), n0out.end()) - n0out.begin());
	sort(n1out.begin(), n1out.end());
	n1out.resize(unique(n1out.begin(), n1out.end()) - n1out.begin());

	bool diff = true;
	if (n0out.size() == n1out.size())
	{
		diff = false;
		for (vector<petri_index>::iterator n0i = n0out.begin(); n0i != n0out.end() && !diff; n0i++)
		{
			diff = true;
			for (vector<petri_index>::iterator n1i = n1out.begin(); n1i != n1out.end() && diff; n1i++)
				if (at(*n0i).predicate == at(*n1i).predicate)
					diff = false;
		}
	}

	return !diff;
}

bool petri_net::are_parallel_siblings(petri_index p0, petri_index p1)
{
	if (p0 < p1)
		return binary_search(parallel_nodes.begin(), parallel_nodes.end(), pair<petri_index, petri_index>(p0, p1));
	else
		return binary_search(parallel_nodes.begin(), parallel_nodes.end(), pair<petri_index, petri_index>(p1, p0));
}

petri_node &petri_net::operator[](petri_index i)
{
	return i.is_place() ? S[i.data] : T[i.data & 0x7FFFFFFF];
}

petri_node &petri_net::at(petri_index i)
{
	return i.is_place() ? S[i.data] : T[i.data & 0x7FFFFFFF];
}

pair<int, int> petri_net::closest_input(vector<int> from, vector<int> to)
{
	vector<int> covered(arcs.size(), 0);
	list<pair<int, int> > nodes;
	for (size_t i = 0; i < to.size(); i++)
		nodes.push_back(pair<int, int>(0, to[i]));

	for (list<pair<int, int> >::iterator n = nodes.begin(); n != nodes.end(); n = nodes.erase(n))
	{
		if (covered[n->second] == 0)
		{
			if (find(from.begin(), from.end(), n->second) != from.end())
				return *n;

			for (size_t a = 0; a < arcs.size(); a++)
				if (arcs[a].second == arcs[n->second].first)
					nodes.push_back(pair<int, int>(n->first+1, a));
		}

		covered[n->second]++;
	}

	return pair<int, int>(arcs.size(), -1);
}

pair<int, int> petri_net::closest_output(vector<int> from, vector<int> to)
{
	vector<int> covered(arcs.size(), 0);
	list<pair<int, int> > nodes;
	for (size_t i = 0; i < from.size(); i++)
		nodes.push_back(pair<int, int>(0, from[i]));

	for (list<pair<int, int> >::iterator n = nodes.begin(); n != nodes.end(); n = nodes.erase(n))
	{
		if (covered[n->second] == 0)
		{
			if (find(to.begin(), to.end(), n->second) != to.end())
				return *n;

			for (size_t a = 0; a < arcs.size(); a++)
				if (arcs[a].first == arcs[n->second].second)
					nodes.push_back(pair<int, int>(n->first+1, a));
		}

		covered[n->second]++;
	}

	return pair<int, int>(arcs.size(), -1);
}

void petri_net::get_paths(vector<int> from, vector<int> to, path_space *result)
{
	for (size_t i = 0; i < from.size(); i++)
	{
		vector<int> ex = from;
		ex.erase(ex.begin() + i);

		result->paths.push_back(path(arcs.size(), from[i], from[i]));
		list<path>::iterator path = result->paths.end();
		path--;
		while (path != result->paths.end())
		{
			/* If we hit an arc that should be excluded or we
			 * found a loop then we kill this path and move on.
			 */
			if (find(ex.begin(), ex.end(), path->to.front()) != ex.end() || path->nodes[path->to.front()] > 0)
				path = result->paths.erase(path);
			/* If we have reached one of the designated ending nodes
			 * then we are done here and we can move on to the next path.
			 */
			else if (find(to.begin(), to.end(), path->to.front()) != to.end())
				path++;
			/* Otherwise we record this location in the path and
			 * increment to the next set of arcs.
			 */
			else
			{
				path->nodes[path->to.front()]++;

				vector<int> n = next_arc(path->to.front());
				for (int j = n.size()-1; j >= 0; j--)
				{
					if (j == 0)
						path->to.front() = n[j];
					else
					{
						result->paths.push_back(*path);
						result->paths.back().to.front() = n[j];
					}
				}
			}
		}
	}

	// Accumulate the resulting paths into the total count.
	result->total.nodes.resize(arcs.size(), 0);
	for (list<path>::iterator path = result->begin(); path != result->end(); path++)
	{
		result->total.from.insert(result->total.from.end(), path->from.begin(), path->from.end());
		result->total.to.insert(result->total.to.end(), path->to.begin(), path->to.end());
		for (size_t i = 0; i < arcs.size(); i++)
			result->total.nodes[i] += path->nodes[i];
	}
	sort(result->total.from.begin(), result->total.from.end());
	result->total.from.resize(unique(result->total.from.begin(), result->total.from.end()) - result->total.from.begin());
	sort(result->total.to.begin(), result->total.to.end());
	result->total.to.resize(unique(result->total.to.begin(), result->total.to.end()) - result->total.to.begin());
}

void petri_net::zero_paths(path_space *paths, petri_index from)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].first == from || arcs[i].second == from)
			paths->zero(i);
}

void petri_net::zero_paths(path_space *paths, vector<petri_index> from)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].first == from[i] || arcs[j].second == from[i])
				paths->zero(j);
}

void petri_net::zero_ins(path_space *paths, petri_index from)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].second == from)
			paths->zero(i);
}

void petri_net::zero_ins(path_space *paths, vector<petri_index> from)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].second == from[i])
				paths->zero(j);
}

void petri_net::zero_outs(path_space *paths, petri_index from)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].first == from)
			paths->zero(i);
}

void petri_net::zero_outs(path_space *paths, vector<petri_index> from)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].first == from[i])
				paths->zero(j);
}

vector<int> petri_net::start_path(int from, vector<int> ex)
{
	sort(ex.begin(), ex.end());

	vector<int> base = vector<int>(1, from);
	for (size_t i = 0; i < ex.size(); i++)
	{
		bool parallel = true;
		for (size_t j = 0; parallel && j < base.size(); j++)
			if (!are_parallel_siblings(arcs[ex[i]].first, arcs[base[j]].first) &&
				!are_parallel_siblings(arcs[ex[i]].first, arcs[base[j]].second) &&
				!are_parallel_siblings(arcs[ex[i]].second, arcs[base[j]].first) &&
				!are_parallel_siblings(arcs[ex[i]].second, arcs[base[j]].second))
				parallel = false;

		if (parallel)
			base.push_back(ex[i]);
	}

	vector<int> result = get_arc_cut(base, true, true);
	vector_symmetric_compliment(&result, &ex);

	return result;
}

vector<int> petri_net::start_path(vector<int> from, vector<int> ex)
{
	sort(ex.begin(), ex.end());

	vector<int> base = from;
	for (size_t i = 0; i < ex.size(); i++)
	{
		bool parallel = true;
		for (size_t j = 0; parallel && j < base.size(); j++)
			if (!are_parallel_siblings(arcs[ex[i]].first, arcs[base[j]].first) &&
				!are_parallel_siblings(arcs[ex[i]].first, arcs[base[j]].second) &&
				!are_parallel_siblings(arcs[ex[i]].second, arcs[base[j]].first) &&
				!are_parallel_siblings(arcs[ex[i]].second, arcs[base[j]].second))
				parallel = false;

		if (parallel)
			base.push_back(ex[i]);
	}

	vector<int> result = get_arc_cut(base, true, true);
	vector_symmetric_compliment(&result, &ex);

	return result;
}

void petri_net::check_assertions()
{
	for (size_t i = 0; i < S.size(); i++)
	{
		for (size_t j = 0; j < S[i].assertions.size(); j++)
			if ((S[i].predicate & S[i].assertions[j]) != 0)
				error("", "assertion " + (~S[i].assertions[j]).print(vars) + " fails at state " + to_string(petri_index(i, true)) + " with a state encoding of " + S[i].predicate.print(vars), "", __FILE__, __LINE__);

		for (size_t j = 0; j < assertions.size(); j++)
			if ((S[i].predicate & assertions[j]) != 0)
				error("", "requirement " + (~assertions[j]).print(vars) + " fails at state " + to_string(petri_index(i, true)) + " with a state encoding of " + S[i].predicate.print(vars), "", __FILE__, __LINE__);
	}
}

pair<int, int> petri_net::get_input_sense_count(petri_index idx)
{
	pair<int, int> result(0, 0), temp;
	vector<petri_index> input = prev(idx);
	for (size_t j = 0; j < input.size(); j++)
	{
		temp = at(input[j]).sense_count();
		result.first += temp.first;
		result.second += temp.second;
	}
	return result;
}

pair<int, int> petri_net::get_input_sense_count(vector<petri_index> idx)
{
	pair<int, int> result(0, 0), temp;
	vector<petri_index> input = prev(idx);
	for (size_t j = 0; j < input.size(); j++)
	{
		temp = at(input[j]).sense_count();
		result.first += temp.first;
		result.second += temp.second;
	}
	return result;
}

petri_index petri_net::get_split_place(petri_index merge_place, vector<bool> *covered)
{
	petri_index i = merge_place;
	vector<petri_index> ot, op, it, ip;
	vector<bool> c;
	bool loop;

	if ((*covered)[i.data])
		return petri_index(-1, false);

	loop = true;
	it = prev(i);
	if ((int)it.size() <= 0)
		return i;
	else if ((int)it.size() == 1)
	{
		ot = next(i);
		for (int j = 0; j < (int)ot.size() && loop; j++)
		{
			op = next(ot[j]);
			for (int k = 0; k < (int)op.size() && loop; k++)
				if (!(*covered)[op[k].data])
					loop = false;
		}
	}

	(*covered)[i.data] = true;

	while (loop)
	{
		it = prev(i);
		if ((int)it.size() <= 0)
			return i;
		else if ((int)it.size() == 1)
		{
			ip = prev(it[0]);
			if (ip.size() == 0)
				return i;
			i = ip[0];

			if ((*covered)[i.data])
				return petri_index(-1, false);
		}
		else
		{
			petri_index k(-1, true);
			int j = ip.size();
			for (int l = 0; l < (int)it.size() && k.data == -1; l++)
			{
				ip = prev(it[l]);
				for (j = 0; j < (int)ip.size() && k.data == -1; j++)
				{
					c = *covered;
					k = get_split_place(ip[j], &c);
				}
			}

			if (k.data == -1)
				return i;
			else
				i = ip[--j];
		}

		(*covered)[i.data] = true;

		loop = true;
		ot = next(i);
		for (int j = 0; j < (int)ot.size() && loop; j++)
		{
			op = next(ot[j]);
			for (int k = 0; k < (int)op.size() && loop; k++)
				if (!(*covered)[op[k].data])
					loop = false;
		}
	}

	return i;
}

void petri_net::remove_invalid_split_points(pair<int, int> up_sense_count, vector<petri_index> up_start, path_space *up_paths, pair<int, int> down_sense_count, vector<petri_index> down_start, path_space *down_paths)
{
	path_space up_inv = up_paths->inverse();
	path_space down_inv = down_paths->inverse();

	path up_mask = up_inv.get_mask();
	path down_mask = down_inv.get_mask();

	up_paths->apply_mask(down_mask);
	down_paths->apply_mask(up_mask);

	if (up_sense_count.first != 0)
		zero_paths(up_paths, up_start);

	if (down_sense_count.second != 0)
		zero_paths(down_paths, down_start);

	for (size_t j = 0; j < up_paths->total.to.size(); j++)
		if (arcs[up_paths->total.to[j]].second.is_place())
			up_paths->zero(up_paths->total.to[j]);

	for (size_t j = 0; j < down_paths->total.to.size(); j++)
		if (arcs[down_paths->total.to[j]].second.is_place())
			down_paths->zero(down_paths->total.to[j]);

	for (int j = 0; j < (int)arcs.size(); j++)
		if (arcs[j].first.is_place() && next(arcs[j].first).size() > 1)
		{
			up_paths->zero(j);
			down_paths->zero(j);
		}
}

vector<int> petri_net::choose_split_points(path_space *paths)
{
	vector<int> result;
	int max;

	for (map<petri_index, list<vector<petri_index> > >::iterator l = indistinguishable.begin(); l != indistinguishable.end(); l++)
	{
		for (int j = 0; j < (int)arcs.size(); j++)
			if (paths->total.nodes[j] > 0 && (arcs[j].first == l->first || arcs[j].second == l->first))
				paths->total.nodes[j] += max_indistinguishables - (int)l->second.size();
	}

	log("", to_string(paths->total), __FILE__, __LINE__);

	while (paths->size() > 0 && (max = closest_input(paths->total.maxes(), paths->total.to).second) != -1)
	{
		result.push_back(max);
		*paths = paths->avoidance(max);

		for (map<petri_index, list<vector<petri_index> > >::iterator l = indistinguishable.begin(); l != indistinguishable.end(); l++)
		{
			for (size_t j = 0; j < arcs.size(); j++)
				if (paths->total.nodes[j] > 0 && (arcs[j].first == l->first || arcs[j].second == l->first))
					paths->total.nodes[j] += max_indistinguishables - (int)l->second.size();
		}

		log("", to_string(paths->total), __FILE__, __LINE__);
	}
	return result;
}

void petri_net::add_conflict_pair(map<petri_index, list<vector<petri_index> > > *c, petri_index i, petri_index j)
{
	map<petri_index, list<vector<petri_index> > >::iterator ri;
	list<vector<petri_index> >::iterator li;
	vector<list<vector<petri_index> >::iterator > gi;
	vector<petri_index> group;

	ri = c->find(i);
	if (ri != c->end())
	{
		gi.clear();
		for (li = ri->second.begin(); li != ri->second.end(); li++)
			for (int k = 0; k < (int)li->size(); k++)
				if (are_connected(j, (*li)[k]))// || psiblings(j, (*li)[k]) != -1)
				{
					gi.push_back(li);
					k = (int)li->size();
				}

		group = vector<petri_index>(1, j);
		for (int k = 0; k < (int)gi.size(); k++)
		{
			group.insert(group.end(), gi[k]->begin(), gi[k]->end());
			ri->second.erase(gi[k]);
		}
		sort(group.begin(), group.end());
		group.resize(unique(group.begin(), group.end()) - group.begin());
		ri->second.push_back(group);
	}
	else
		c->insert(pair<petri_index, list<vector<petri_index> > >(i, list<vector<petri_index> >(1, vector<petri_index>(1, j))));
}

void petri_net::generate_paths(pair<int, int> *up_sense_count, vector<petri_index> up_start, path_space *up_paths, pair<int, int> *down_sense_count, vector<petri_index> down_start,  path_space *down_paths)
{
	down_paths->clear();
	up_paths->clear();
	vector<int> iin;
	for (size_t j = 0; j < up_start.size(); j++)
	{
		vector<int> inc = incoming(up_start[j]);
		iin.insert(iin.end(), inc.begin(), inc.end());
	}


	vector<int> jin;
	for (size_t j = 0; j < down_start.size(); j++)
	{
		vector<int> inc = incoming(down_start[j]);
		jin.insert(jin.end(), inc.begin(), inc.end());
	}

	vector<int> istart = start_path(iin, jin);
	vector<int> jstart = start_path(jin, iin);
	pair<int, int> swap_sense_count;
	path_space swap_path_space(arcs.size());

	*up_sense_count = get_input_sense_count(up_start);
	*down_sense_count = get_input_sense_count(down_start);

	if (up_sense_count->second > up_sense_count->first && down_sense_count->first > down_sense_count->second)
	{
		get_paths(istart, jin, up_paths);
		get_paths(jstart, iin, down_paths);
	}
	else if (up_sense_count->first > up_sense_count->second && down_sense_count->second > down_sense_count->first)
	{
		get_paths(istart, jin, down_paths);
		get_paths(jstart, iin, up_paths);
		swap_sense_count = *up_sense_count;
		*up_sense_count = *down_sense_count;
		*down_sense_count = swap_sense_count;
	}
	else
	{
		get_paths(istart, jin, up_paths);
		get_paths(jstart, iin, down_paths);

		if (up_sense_count->second > up_sense_count->first && down_sense_count->second > down_sense_count->first && up_paths->length() > down_paths->length())
		{
			swap_path_space = *up_paths;
			*up_paths = *down_paths;
			*down_paths = swap_path_space;
			swap_sense_count = *up_sense_count;
			*up_sense_count = *down_sense_count;
			*down_sense_count = swap_sense_count;
		}
		else if (up_sense_count->first > up_sense_count->second && down_sense_count->first > down_sense_count->second && down_paths->length() > up_paths->length())
		{
			swap_path_space = *up_paths;
			*up_paths = *down_paths;
			*down_paths = swap_path_space;
			swap_sense_count = *up_sense_count;
			*up_sense_count = *down_sense_count;
			*down_sense_count = swap_sense_count;
		}
	}
}

void petri_net::generate_conflicts()
{
	map<int, list<vector<int> > >::iterator ri;
	list<vector<int> >::iterator li;
	vector<list<vector<int> >::iterator > gi;
	map<int, int>::iterator bi, bj;
	vector<int> group;
	vector<int> temp;

	conflicts.clear();
	indistinguishable.clear();

	for (petri_index i(0, true); i < S.size(); i++)
	{
		//cout << i << "/" << S.size() << endl;
		vector<petri_index> oi = next(i);

		minterm ti = 1;
		for (size_t j = 0; j < oi.size(); j++)
			if (at(oi[j]).active)
				ti &= at(oi[j]).predicate.terms[0];

		canonical nti = ~ti;
		canonical si, st;
		for (size_t j = 0; j < at(i).predicate.terms.size(); j++)
		{
			si.terms.push_back(at(i).predicate.terms[j] & ti);
			st.terms.push_back(at(i).predicate.terms[j] & ti.inverse());
		}

		for (size_t k = 0, l = 0; k < si.terms.size() && l < at(i).predicate.terms.size(); l++)
		{
			if (si.terms[k] != 0 && st.terms[l] == 0)
				si.terms.erase(si.terms.begin() + k);
			else
			{
				si.terms[k] = si.terms[k].xoutnulls() | at(i).predicate.terms[l];
				k++;
			}
		}

		for (petri_index j(0, true); j < S.size(); j++)
		{
			if (i != j)
			{
				canonical sj = get_effective_place_encoding(j, vector<petri_index>(1, i));

				/* States are indistinguishable if:
				 *  - they are not the same state
				 *  	> i != j
				 *  - the two states do not exist in parallel
				 *  	> are_parallel_siblings(i, j) < 0
				 *  - the two state encodings are not mutually exclusive
				 *    taking into account the inactive firings between them
				 *  	> si & get_effective_place_encoding(j, vector<petri_index>(1, i)) != 0
				 */
				if (!are_parallel_siblings(i, j) && !is_mutex(&si, &sj))
				{
					vector<petri_index> oj = next(j);
					minterm tj = 1;
					for (size_t k = 0; k < oj.size(); k++)
						if (at(oj[k]).active)
							tj &= at(oj[k]).predicate.terms[0];
					canonical ntj = ~tj;
					canonical jtj = (sj >> tj);

					//cout << "CONFLICT " << i.name() << "=" << si.print(vars) << "," << ti.print(vars) << " " << j.name() << "=" << sj.print(vars) << "," << tj.print(vars) << endl;

					/* States are conflicting if:
					 *  - they are indistinguishable
					 *  - the conflict is not caused my non-mutually exclusive guards in a conditional
					 *  	> !are_sibling_guards(i, j)
					 *  - the transition which causes the conflict is not a vacuous firing in the other state
					 *  - the transition which causes the conflict would not normally happen anyways as a result of the other state
					 */
					if (!is_mutex(&nti, &at(i).predicate, &jtj))
					{
						if (!are_sibling_guards(i, j))
							add_conflict_pair(&conflicts, i, j);
						else
							warning("", "conditional near " + to_string(i) + " and " + to_string(j) + " has non mutually exclusive guards.", "", __FILE__, __LINE__);
					}

					add_conflict_pair(&indistinguishable, i, j);
				}
			}
		}
	}

	max_indistinguishables = 0;
	for (map<petri_index, list<vector<petri_index> > >::iterator l = indistinguishable.begin(); l != indistinguishable.end(); l++)
		if ((int)l->second.size() > max_indistinguishables)
			max_indistinguishables = l->second.size();
}

bool petri_net::solve_conflicts()
{
	vector<pair<vector<int>, vector<int> > > ip;

	if (conflicts.size() == 0)
		return false;

	log("", "process \'" + name + "\'", __FILE__, __LINE__);
	for (map<petri_index, list<vector<petri_index> > >::iterator i = conflicts.begin(); i != conflicts.end(); i++)
	{
		for (list<vector<petri_index> >::iterator lj = i->second.begin(); lj != i->second.end(); lj++)
		{
			path_space up_paths(arcs.size()), down_paths(arcs.size());
			pair<int, int> up_sense_count, down_sense_count;
			vector<int> uptrans, downtrans;

			generate_paths(&up_sense_count, vector<petri_index>(1, i->first), &up_paths, &down_sense_count, *lj, &down_paths);
			remove_invalid_split_points(up_sense_count, vector<petri_index>(1, i->first), &up_paths, down_sense_count, *lj, &down_paths);

			up_paths.print_bounds("Up");
			uptrans = choose_split_points(&up_paths);

			down_paths.print_bounds("Down");
			downtrans = choose_split_points(&down_paths);

			vector_symmetric_compliment(&uptrans, &downtrans);

			if (uptrans.size() == 0 || downtrans.size() == 0)
				error("", "no solution for the conflict set: " + to_string(i->first) + " -> " + to_string(*lj), "", __FILE__, __LINE__);
			else if (uptrans <= downtrans)
				ip.push_back(pair<vector<int>, vector<int> >(uptrans, downtrans));
			else if (downtrans <= uptrans)
				ip.push_back(pair<vector<int>, vector<int> >(downtrans, uptrans));
		}
	}

	if (ip.size() == 0)
		return false;

	vector<int> reset_arcs = outgoing(M0);
	sort(ip.begin(), ip.end());
	ip.resize(unique(ip.begin(), ip.end()) - ip.begin());
	for (size_t j = 0; j < ip.size(); j++)
	{
		string vname = vars.unique_name();
		vars.globals.push_back(variable(vname));
		vars.globals.back().written = true;

		canonical um = canonical(vars.globals.size()-1, 1);
		canonical dm = canonical(vars.globals.size()-1, 0);

		for (int k = 0; k < (int)ip[j].first.size(); k++)
			insert(ip[j].first[k], um, true);
		for (int k = 0; k < (int)ip[j].second.size(); k++)
			insert(ip[j].second[k], dm, true);

		path_space up_paths(arcs.size()), down_paths(arcs.size());
		get_paths(ip[j].first, ip[j].second, &up_paths);
		get_paths(ip[j].second, ip[j].first, &down_paths);

		int up_coverage = 0;
		int down_coverage = 0;

		for (size_t k = 0; k < reset_arcs.size(); k++)
		{
			up_coverage += up_paths.coverage_count(reset_arcs[k]);
			down_coverage += down_paths.coverage_count(reset_arcs[k]);
		}

		if (up_coverage > 0 && down_coverage == 0)
			reset &= minterm(vars.globals.size()-1, 1);
		else if (down_coverage > 0 && up_coverage == 0)
			reset &= minterm(vars.globals.size()-1, 0);

		log("", "inserting " + vname + " up arcs = " + to_string(ip[j].first) + " down arcs = " + to_string(ip[j].second), __FILE__, __LINE__);
	}

	return true;
}

canonical petri_net::base(vector<int> idx)
{
	canonical res(1);
	int i;
	for (i = 0; i < (int)idx.size(); i++)
		res = res & S[idx[i]].predicate;

	return res;
}

bool petri_net::are_sibling_guards(petri_index i, petri_index j)
{
	vector<petri_index> ii = prev(i);
	vector<petri_index> ij = prev(j);
	for (size_t k = 0; k < ii.size(); k++)
	{
		vector<petri_index> iii = prev(ii[k]);
		for (size_t l = 0; l < ij.size(); l++)
		{
			vector<petri_index> iij = prev(ij[l]);
			for (size_t m = 0; m < iii.size(); m++)
				for (size_t n = 0; n < iij.size(); n++)
					if (iii[m] == iij[n] && !at(ii[k]).active && !at(ij[l]).active)
						return true;
		}
	}
	return false;
}

canonical petri_net::apply_debug(int pc)
{
	return (assumptions & S[pc].assumptions);
}

void petri_net::compact()
{
	bool change = true;
	while (change)
	{
		change = false;
		for (petri_index i(0, true); i < S.size();)
		{
			vector<int> ia = incoming(i);
			vector<petri_index> oa = next(i);

			if (ia.size() == 0)
			{
				if (find(M0.begin(), M0.end(), i) != M0.end())
				{
					bool all_active = true;
					for (size_t j = 0; all_active && j < oa.size(); j++)
						if (!at(oa[j]).active || is_mutex(reset, ~at(oa[j]).predicate))
							all_active = false;

					if (all_active)
					{
						M0.insert(M0.end(), oa.begin(), oa.end());
						cut(i);
						change = true;
					}
					else
						i++;
				}
				else
				{
					cut(i);
					change = true;
				}
			}
			else
				i++;
		}

		for (petri_index i(0, false); i < T.size();)
		{
			vector<petri_index> n = next(i), p = prev(i), np = prev(n), pn = next(p), nn = next(n), pp = prev(p);

			if (at(i).predicate == 0 || p.size() == 0)
			{
				if (find(M0.begin(), M0.end(), i) != M0.end())
				{
					M0.insert(M0.end(), n.begin(), n.end());
					if (at(i).predicate != 0)
						reset = reset >> at(i).predicate;
				}

				cut(i);
				change = true;
			}
			else if (at(i).predicate == 1 && (pp.size() == 1 || n.size() == 1) && p.size() == 1 && pn.size() == 1)
			{
				pinch_backward(i);
				change = true;
			}
			else
				i++;
		}

		for (petri_index i(0, false); i < T.size(); i++)
			for (petri_index j = i+1; j < T.size(); )
			{
				if (!at(i).active && !at(j).active && prev(i) == prev(j) && next(i) == next(j))
				{
					at(i).predicate |= at(j).predicate;
					if (find(M0.begin(), M0.end(), j) != M0.end())
						M0.push_back(i);
					cut(j);
					change = true;
				}
				else
					j++;
			}
	}
}

void petri_net::generate_observed()
{
	for (size_t i = 0; i < S.size(); i++)
		S[i].observed.clear();
	for (size_t i = 0; i < T.size(); i++)
		T[i].observed.clear();

	vector<pair<petri_index, bool> > counters;
	for (size_t i = 0; i < M0.size(); i++)
	{
		size_t input_size = prev(M0[i]).size();
		if (input_size == 0)
			input_size = 1;

		for (size_t j = 0; j < input_size; j++)
			counters.push_back(pair<petri_index, bool>(M0[i], false));
	}

	bool done = false;
	while (!done)
	{
		//for (size_t i = 0; i < counters.size(); i++)
		//	cout << counters[i].first << ":" << counters[i].second << " ";
		//cout << endl;

		vector<int> movable;
		for (size_t i = 0; i < counters.size(); i++)
		{
			vector<petri_index> p = prev(counters[i].first);
			size_t total = 1;
			for (size_t j = i+1; j < counters.size(); j++)
				if (counters[j].first == counters[i].first)
					total++;

			if (total >= p.size())
			{
				for (size_t j = i+1; j < counters.size(); )
				{
					if (counters[j].first == counters[i].first)
					{
						counters[i].second = counters[i].second && counters[j].second;
						counters.erase(counters.begin() + j);
					}
					else
						j++;
				}

				movable.push_back(i);
			}
		}

		for (size_t i = 0; i < movable.size(); i++)
		{
			map<vector<petri_index>, canonical> old = at(counters[movable[i]].first).observed;
			at(counters[movable[i]].first).observed.clear();
			if (!counters[movable[i]].second && counters[movable[i]].first.is_place())
			{
				vector<petri_index> s;
				s.push_back(counters[movable[i]].first);
				at(counters[movable[i]].first).observed.insert(pair<vector<petri_index>, canonical>(s, canonical()));

				vector<petri_index> inputs = prev(counters[movable[i]].first);
				for (size_t j = 0; j < inputs.size(); j++)
				{
					for (map<vector<petri_index>, canonical>::iterator k = at(inputs[j]).observed.begin(); k != at(inputs[j]).observed.end(); k++)
					{
						map<vector<petri_index>, canonical>::iterator l = at(counters[movable[i]].first).observed.find(k->first);
						canonical c = (k->second | ~at(inputs[j]).predicate);
						if (l != at(counters[movable[i]].first).observed.end())
							l->second &= c;
						else
							at(counters[movable[i]].first).observed.insert(pair<vector<petri_index>, canonical>(k->first, c));
					}
				}
			}
			else if (!counters[movable[i]].second && counters[movable[i]].first.is_trans())
			{
				vector<petri_index> inputs = prev(counters[movable[i]].first);
				vector<map<vector<petri_index>, canonical> > in_observed;
				vector<vector<petri_index> > state_set;
				for (size_t j = 0; j < inputs.size(); j++)
				{
					in_observed.push_back(at(inputs[j]).observed);
					for (map<vector<petri_index>, canonical>::iterator k = at(inputs[j]).observed.begin(); k != at(inputs[j]).observed.end(); k++)
						state_set.push_back(k->first);
				}
				sort(state_set.begin(), state_set.end());
				state_set.resize(unique(state_set.begin(), state_set.end()) - state_set.begin());

				//cout << "\tCommon States ";
				for (size_t j = 0; j < state_set.size(); j++)
				{
					canonical c = 0;
					bool found = true;
					for (size_t k = 0; found && k < in_observed.size(); k++)
					{
						map<vector<petri_index>, canonical>::iterator l = in_observed[k].find(state_set[j]);
						if (l == in_observed[k].end())
							found = false;
						else
							c |= l->second;
					}

					if (found)
					{
						//cout << state_set[j] << " ";
						at(counters[movable[i]].first).observed.insert(pair<vector<petri_index>, canonical>(state_set[j], c));
						for (size_t k = 0; k < in_observed.size(); k++)
							in_observed[k].erase(in_observed[k].find(state_set[j]));
					}
				}
				//cout << endl;

				vector<map<vector<petri_index>, canonical>::iterator> j;
				for (size_t k = 0; k < in_observed.size(); )
				{
					if (in_observed[k].size() > 0)
					{
						j.push_back(in_observed[k].begin());
						k++;
					}
					else
						in_observed.erase(in_observed.begin() + k);
				}

				while (j.size() > 0 && j.back() != in_observed.back().end())
				{
					vector<petri_index> s;
					canonical c = 0;
					//cout << "\t";
					for (size_t k = 0; k < j.size(); k++)
					{
						//cout << j[k]->first << " ";
						s.insert(s.end(), j[k]->first.begin(), j[k]->first.end());
						c |= j[k]->second;
					}
					//cout << endl;

					sort(s.begin(), s.end());
					s.resize(unique(s.begin(), s.end()) - s.begin());

					map<vector<petri_index>, canonical>::iterator z = at(counters[movable[i]].first).observed.find(s);
					if (z != at(counters[movable[i]].first).observed.end())
						z->second &= c;
					else
						at(counters[movable[i]].first).observed.insert(pair<vector<petri_index>, canonical>(s, c));

					j[0]++;
					for (size_t k = 0; j[k] == in_observed[k].end() && k < in_observed.size()-1; k++)
					{
						j[k] = in_observed[k].begin();
						j[k+1]++;
					}
				}
			}

			if (!counters[movable[i]].second && at(counters[movable[i]].first).observed == old)
				counters[movable[i]].second = true;

			vector<petri_index> n = next(counters[movable[i]].first);
			for (size_t j = n.size()-1; j >= 0 && j < n.size(); j--)
			{
				if (j > 0)
					counters.push_back(pair<petri_index, bool>(n[j], counters[movable[i]].second));
				else
					counters[movable[i]].first = n[j];
			}
		}

		done = true;
		for (size_t i = 0; done && i < counters.size(); i++)
			if (!counters[i].second)
				done = false;
	}

	//cout << "Observed" << endl;
	//for (size_t i = 0; i < S.size(); i++)
	//{
	//	cout << petri_index(i, true) << endl;
	//	for (map<vector<petri_index>, canonical>::iterator j = S[i].observed.begin(); j != S[i].observed.end(); j++)
	//		cout << "\t" << j->first << ": " << j->second.print(vars) << endl;
	//}
}

/**
 * Initializes a petri state given a single place. The resulting state
 * is one that has an index at the given place and then enough indices
 * at each parent parallel merge point to allow through travel of the
 * first index.
 */
vector<petri_index> petri_net::get_cut(vector<petri_index> base, bool backward, bool conditional)
{
	vector<petri_index> result;
	result.insert(result.end(), base.begin(), base.end());
	list<pair<vector<petri_index>, vector<bool> > > execs(1, pair<vector<petri_index>, vector<bool> >(base, vector<bool>(S.size() + T.size(), false)));

	/**
	 * Run through all possible executions from the starting index
	 * looking for deadlock.
	 */
	for (list<pair<vector<petri_index>, vector<bool> > >::iterator exec = execs.begin(); exec != execs.end(); exec = execs.erase(exec))
	{
		//cout << "\tStart Execution" << endl;
		bool done = false;
		while (!done)
		{
			vector<size_t> movable;
			for (size_t i = 0; i < exec->first.size(); i++)
			{
				vector<petri_index> n = backward ? prev(exec->first[i]) : next(exec->first[i]);
				if ((!conditional && exec->first[i].is_place()) || (conditional && exec->first[i].is_trans()))
				{
					size_t total = 0;
					for (size_t k = i; k < exec->first.size(); k++)
						if ((backward ? prev(exec->first[k]) : next(exec->first[k]))[0] == n[0])
							total++;

					vector<petri_index> temp = (backward ? next(n) : prev(n));
					sort(temp.begin(), temp.end());
					temp.resize(unique(temp.begin(), temp.end()) - temp.begin());
					if (total == temp.size())
						movable.push_back(i);
				}
				else
					movable.push_back(i);
			}

			if (movable.size() != 0)
			{
				// Check to see if we are done here...
				done = true;
				for (size_t i = 0; done && i < exec->first.size(); i++)
					if (!exec->second[exec->first[i].is_trans() ? exec->first[i].idx() + S.size() : exec->first[i].idx()])
						done = false;

				// Mark the nodes we just covered...
				for (size_t i = 0; i < exec->first.size(); i++)
					exec->second[exec->first[i].is_trans() ? exec->first[i].idx() + S.size() : exec->first[i].idx()] = true;
			}

			//cout << "\t" << movable.size() << exec->first << endl;

			/* If we are not done, handle the next set of movements
			 * duplicating executions or indices when necessary.
			 */
			for (size_t i = 0; !done && i < movable.size(); i++)
			{
				if ((!conditional && exec->first[movable[i]].is_place()) || (conditional && exec->first[movable[i]].is_trans()))
				{
					for (size_t k = movable[i]+1; k < exec->first.size(); )
					{
						if ((backward && prev(exec->first[k])[0] == prev(exec->first[movable[i]])[0]) ||
						   (!backward && next(exec->first[k])[0] == next(exec->first[movable[i]])[0]))
						{
							for (size_t j = i+1; j < movable.size(); j++)
								if (movable[j] > k)
									movable[j]--;

							exec->first.erase(exec->first.begin() + k);
						}
						else
							k++;
					}
				}

				vector<petri_index> n = backward ? prev(exec->first[movable[i]]) : next(exec->first[movable[i]]);
				for (size_t k = n.size()-1; k >= 0 && k < n.size(); k--)
				{
					if (k > 0)
					{
						if ((!conditional && exec->first[movable[i]].is_place()) || (conditional && exec->first[movable[i]].is_trans()))
						{
							execs.push_back(*exec);
							execs.back().first[movable[i]] = n[k];
						}
						else
							exec->first.push_back(n[k]);
					}
					else
						exec->first[movable[i]] = n[k];
				}
			}

			/**
			 * Every time deadlock is detected in the execution,
			 * insert enough indices at the merge point to allow
			 * execution to continue. Also, insert these indices
			 * into the state we are trying to initialize.
			 */
			if (!done && movable.size() == 0)
			{
				vector<petri_index> counts;
				// Count up how many indices we already have at each merge point.
				for (size_t i = 0; i < exec->first.size(); i++)
				{
					vector<petri_index> temp;
					if (backward)
						temp = next(prev(exec->first[i]));
					else
						temp = prev(next(exec->first[i]));

					counts.insert(counts.end(), temp.begin(), temp.end());
				}
				sort(counts.begin(), counts.end());
				counts.resize(unique(counts.begin(), counts.end()) - counts.begin());

				for (size_t i = 0; i < counts.size(); i++)
				{
					if (find(exec->first.begin(), exec->first.end(), counts[i]) == exec->first.end())
					{
						exec->first.push_back(counts[i]);
						result.push_back(counts[i]);
					}
				}
			}
		}
	}

	// Sort the state so that we can have some standard for comparison.
	sort(result.begin(), result.end());
	return result;
}

/**
 * Initializes a petri state given a single place. The resulting state
 * is one that has an index at the given place and then enough indices
 * at each parent parallel merge point to allow through travel of the
 * first index.
 */
vector<int> petri_net::get_arc_cut(vector<int> base, bool backward, bool conditional)
{
	//cout << "Getting Cut" << endl;
	vector<int> result;
	result.insert(result.end(), base.begin(), base.end());
	list<pair<vector<int>, vector<bool> > > execs(1, pair<vector<int>, vector<bool> >(base, vector<bool>(arcs.size(), false)));

	/**
	 * Run through all possible executions from the starting index
	 * looking for deadlock.
	 */
	for (list<pair<vector<int>, vector<bool> > >::iterator exec = execs.begin(); exec != execs.end(); exec = execs.erase(exec))
	{
		//cout << "\tStart Execution" << endl;
		bool done = false;
		while (!done)
		{
			sort(exec->first.begin(), exec->first.end());
			exec->first.resize(unique(exec->first.begin(), exec->first.end()) - exec->first.begin());

			vector<size_t> movable;
			for (size_t i = 0; i < exec->first.size(); i++)
			{
				petri_index n = backward ? arcs[exec->first[i]].first : arcs[exec->first[i]].second;
				if ((conditional && n.is_place()) || (!conditional && n.is_trans()))
				{
					size_t total = 0;
					for (size_t k = i; k < exec->first.size(); k++)
						if ((backward ? arcs[exec->first[k]].first : arcs[exec->first[k]].second) == n)
							total++;

					vector<petri_index> temp = (backward ? next(n) : prev(n));
					sort(temp.begin(), temp.end());
					temp.resize(unique(temp.begin(), temp.end()) - temp.begin());
					if (total == temp.size())
						movable.push_back(i);
				}
				else
					movable.push_back(i);
			}

			if (movable.size() != 0)
			{
				// Check to see if we are done here...
				done = true;
				for (size_t i = 0; done && i < exec->first.size(); i++)
					if (!exec->second[exec->first[i]])
						done = false;

				// Mark the nodes we just covered...
				for (size_t i = 0; i < exec->first.size(); i++)
					exec->second[exec->first[i]] = true;
			}

			//cout << "\t" << movable.size() << exec->first << endl;

			/* If we are not done, handle the next set of movements
			 * duplicating executions or indices when necessary.
			 */
			for (size_t i = 0; !done && i < movable.size(); i++)
			{
				petri_index c = backward ? arcs[exec->first[movable[i]]].first : arcs[exec->first[movable[i]]].second;
				if ((conditional && c.is_place()) || (!conditional && c.is_trans()))
				{
					for (size_t k = movable[i]+1; k < exec->first.size(); )
					{
						if ((backward && arcs[exec->first[k]].first == c) ||
						   (!backward && arcs[exec->first[k]].second == c))
						{
							for (size_t j = i+1; j < movable.size(); j++)
								if (movable[j] > k)
									movable[j]--;

							exec->first.erase(exec->first.begin() + k);
						}
						else
							k++;
					}
				}

				vector<int> n = backward ? prev_arc(exec->first[movable[i]]) : next_arc(exec->first[movable[i]]);
				for (size_t k = n.size()-1; k >= 0 && k < n.size(); k--)
				{
					if (k > 0)
					{
						if ((conditional && c.is_trans()) || (!conditional && c.is_place()))
						{
							execs.push_back(*exec);
							execs.back().first[movable[i]] = n[k];
						}
						else
							exec->first.push_back(n[k]);
					}
					else
						exec->first[movable[i]] = n[k];
				}
			}

			/**
			 * Every time deadlock is detected in the execution,
			 * insert enough indices at the merge point to allow
			 * execution to continue. Also, insert these indices
			 * into the state we are trying to initialize.
			 */
			if (!done && movable.size() == 0)
			{
				vector<int> counts;
				// Count up how many indices we already have at each merge point.
				for (size_t i = 0; i < exec->first.size(); i++)
				{
					vector<int> temp;
					if (backward)
						temp = next_arc(prev_arc(exec->first[i]));
					else
						temp = prev_arc(next_arc(exec->first[i]));

					counts.insert(counts.end(), temp.begin(), temp.end());
				}
				sort(counts.begin(), counts.end());
				counts.resize(unique(counts.begin(), counts.end()) - counts.begin());

				for (size_t i = 0; i < counts.size(); i++)
				{
					if (find(exec->first.begin(), exec->first.end(), counts[i]) == exec->first.end())
					{
						exec->first.push_back(counts[i]);
						result.push_back(counts[i]);
					}
				}
			}
		}
	}

	// Sort the state so that we can have some standard for comparison.
	sort(result.begin(), result.end());
	result.resize(unique(result.begin(), result.end()) - result.begin());

	//cout << base << " -> " << result << endl;

	return result;
}

struct place_encoding_execution
{
	place_encoding_execution() {}
	place_encoding_execution(const place_encoding_execution &c)
	{
		good = c.good;
		net = c.net;
		contributions = c.contributions;
		location = c.location;
		covered = c.covered;
		old = c.old;
		value = c.value;
	}
	place_encoding_execution(petri_index start, petri_net *n)
	{
		net = n;
		location = n->get_cut(vector<petri_index>(1, start), true, false);
		covered.resize(n->S.size() + n->T.size(), false);
		value = 0;
		old = 0;
		good = true;
	}
	~place_encoding_execution() {}

	petri_net *net;
	vector<petri_index> location;
	vector<bool> covered;
	canonical old;
	canonical value;
	vector<canonical> contributions;
	bool good;

	place_encoding_execution &operator=(place_encoding_execution c)
	{
		good = c.good;
		net = c.net;
		location = c.location;
		covered = c.covered;
		old = c.old;
		value = c.value;
		contributions = c.contributions;
		return *this;
	}

	bool check_covered(petri_index i)
	{
		if (i.is_trans())
			return covered[i.idx() + net->S.size()];
		else
			return covered[i.idx()];
	}

	void set_covered(petri_index i)
	{
		if (i.is_trans())
			covered[i.idx() + net->S.size()] = true;
		else
			covered[i.idx()] = true;
	}
};

canonical petri_net::get_effective_place_encoding(petri_index place, vector<petri_index> observer)
{
	//cout << "Effective Place Encoding from " << place << " to " << observer << endl;
	vector<vector<petri_index> > execs(1, get_cut(vector<petri_index>(1, place), false, false));
	vector<petri_index> s = get_cut(observer, false, false);
	vector<bool> covered(S.size() + T.size(), false);

	//cout << execs[0] << " -> " << s << endl;

	for (size_t i = 0; i < execs[0].size(); )
	{
		if (find(s.begin(), s.end(), execs[0][i]) != s.end())
			execs[0].erase(execs[0].begin() + i);
		else
			i++;
	}

	canonical encoding = at(place).predicate;

	for (size_t i = 0; i < execs.size(); i++)
	{
		bool done = false;
		while (!done)
		{
			sort(execs[i].begin(), execs[i].end());
			//cout << execs[i] << endl;

			vector<int> ready_places;
			vector<int> ready_transitions;
			for (size_t j = 0; j < execs[i].size(); j++)
				if (execs[i][j].is_trans() && !covered[S.size() + execs[i][j].idx()])
					ready_transitions.push_back(j);

			if (ready_transitions.size() == 0)
			{
				for (size_t j = 0; j < execs[i].size(); j++)
				{
					bool found = false;
					for (size_t k = 0; !found && k < observer.size(); k++)
						if (at(observer[k]).observed.find(execs[i]) != at(observer[k]).observed.end())
							found = true;

					if (execs[i][j].is_place() && !covered[execs[i][j].idx()] && execs[i][j] != place && !found)
					{
						size_t total = 0;
						for (size_t k = j; k < execs[i].size(); k++)
							if (prev(execs[i][k]) == prev(execs[i][j]))
								total++;

						if (total == next(prev(execs[i][j])).size())
						{
							for (size_t k = j+1; k < execs[i].size(); )
							{
								if (prev(execs[i][k]) == prev(execs[i][j]))
									execs[i].erase(execs[i].begin() + k);
								else
									k++;
							}

							ready_places.push_back(j);
						}
					}
				}
			}

			if (ready_transitions.size() > 0)
			{
				for (size_t j = 0; j < ready_transitions.size(); j++)
				{
					covered[S.size() + execs[i][ready_transitions[j]].idx()] = true;

					vector<petri_index> n = prev(execs[i][ready_transitions[j]]);
					for (size_t k = n.size()-1; k >= 0 && k < n.size(); k--)
					{
						if (k > 0)
							execs[i].push_back(n[k]);
						else
							execs[i][ready_transitions[j]] = n[k];
					}
				}
			}
			else if (ready_places.size() > 0)
			{
				for (size_t j = ready_places.size()-1; j >= 0 && j < ready_places.size(); j--)
				{
					covered[execs[i][ready_places[j]].idx()] = true;
					int i1 = i;
					if (j > 0)
					{
						execs.push_back(execs[i]);
						i1 = execs.size()-1;
					}

					vector<petri_index> n = prev(execs[i1][ready_places[j]]);
					for (size_t k = n.size()-1; k >= 0 && k < n.size(); k--)
					{
						if (k > 0)
						{
							execs.push_back(execs[i1]);
							execs.back()[ready_places[j]] = n[k];
						}
						else
							execs[i1][ready_places[j]] = n[k];
					}
				}
			}
			else
				done = true;
		}
	}

	sort(execs.begin(), execs.end());
	execs.resize(unique(execs.begin(), execs.end()) - execs.begin());

	for (size_t i = 0; i < execs.size(); i++)
	{
		canonical temp = 0;
		bool found = false;
		for (size_t j = 0; j < observer.size(); j++)
		{
			map<vector<petri_index>, canonical>::iterator k = at(observer[j]).observed.find(execs[i]);
			if (k != at(observer[j]).observed.end())
			{
				temp |= k->second;
				found = true;
			}
		}

		if (found)
			encoding &= temp;
	}

	return encoding;
}

canonical petri_net::get_effective_state_encoding(vector<petri_index> state, vector<petri_index> observer, vector<petri_index> path)
{
	//cout << "Effective State Encoding from " << state << " to " << observer << endl;
	vector<pair<petri_index, canonical> > idx;
	for (size_t i = 0; i < state.size(); i++)
		idx.push_back(pair<petri_index, canonical>(state[i], canonical(0)));

	canonical encoding = 1;
	for (size_t i = 0; i < state.size(); i++)
		encoding &= at(state[i]).predicate;

	//cout << "Start " << encoding.print(vars) << endl;

	canonical result = 0;

	bool bypass = false;
	while (idx.size() > 0)
	{
		vector<int> movable;
		for (size_t i = 0; i < idx.size(); i++)
		{
			size_t total = 0;
			for (size_t j = i; j < idx.size(); j++)
				if (idx[j].first == idx[i].first)
					total++;

			if (bypass || total == prev(idx[i].first).size())
			{
				for (size_t j = i+1; j < idx.size();)
				{
					if (idx[j].first == idx[i].first)
					{
						if (idx[i].first.is_place())
							idx[i].second &= idx[j].second;
						else if (idx[i].first.is_trans())
							idx[i].second |= idx[j].second;

						idx.erase(idx.begin() + j);
					}
					else
						j++;
				}

				movable.push_back(i);
			}
		}

		//for (size_t i = 0; i < movable.size(); i++)
		//	cout << "{" << idx[movable[i]].first << ", " << idx[movable[i]].second.print(vars) << "} ";
		//cout << endl;

		for (size_t i = 0; i < movable.size(); i++)
		{
			vector<petri_index> n = next(idx[movable[i]].first);
			for (size_t j = 0; j < n.size(); )
			{
				if (find(path.begin(), path.end(), n[j]) == path.end())
					n.erase(n.begin() + j);
				else
					j++;
			}

			if (n.size() == 0 || find(observer.begin(), observer.end(), idx[movable[i]].first) != observer.end() || idx[movable[i]].second == 1)
			{
				//cout << "Done " << idx[movable[i]].second.print(vars) << endl;
				if (idx[movable[i]].second != 1)
					result |= idx[movable[i]].second;

				for (size_t j = i+1; j < movable.size(); j++)
					if (movable[j] > movable[i])
						movable[j]--;
				idx.erase(idx.begin() + movable[i]);
			}
			else
			{
				if (idx[movable[i]].first.is_trans())
					idx[movable[i]].second = ::merge(idx[movable[i]].second, ~at(idx[movable[i]].first).predicate);

				for (size_t j = n.size()-1; j >= 0 && j < n.size(); j--)
				{
					if (j > 0)
					{
						idx.push_back(idx[movable[i]]);
						idx.back().first = n[j];
					}
					else
						idx[movable[i]].first = n[j];
				}
			}
		}

		bypass = false;
		if (movable.size() == 0)
			bypass = true;
	}

	if (result != 0)
		return (encoding & result);
	else
		return encoding;
}

dot_stmt petri_net::export_dot(int t_base, int s_base, bool node_ids, bool arc_ids)
{
	dot_stmt stmt;
	stmt.stmt_type = "subgraph";
	stmt.id = name;

	dot_a_list a_list;
	dot_stmt substmt;

	a_list.as.push_back(dot_a("label", label));
	string variables = "";
	for (size_t i = 0; i < vars.globals.size(); i++)
	{
		if (i != 0)
			variables += ",";
		variables += vars.globals[i].name;
	}
	a_list.as.push_back(dot_a("variables", variables));

	a_list.as.push_back(dot_a("reset", reset.print(vars)));
	a_list.as.push_back(dot_a("type", remote ? "remote" : "local"));
	a_list.as.push_back(dot_a("elaborate", elaborate ? "true" : "false"));
	a_list.as.push_back(dot_a("style", "dotted"));
	if (assumptions != 1)
		a_list.as.push_back(dot_a("assume", assumptions.print(vars)));
	for (size_t i = 0; i < assertions.size(); i++)
		a_list.as.push_back(dot_a("assert", assertions[i].print(vars)));

	substmt.stmt_type = "attr";
	substmt.attr_type = "graph";
	substmt.attr_list.attrs.push_back(a_list);
	stmt.stmt_list.stmts.push_back(substmt);

	for (size_t i = 0; i < S.size(); i++)
	{
		a_list.as.clear();
		substmt.attr_list.attrs.clear();
		substmt.node_ids.clear();
		substmt.stmt_list.stmts.clear();
		substmt.id = "";
		substmt.stmt_type = "";
		substmt.attr_type = "";

		if (elaborate)
		{
			string expr = "";
			string parse = S[i].predicate.print(vars);
			for (size_t j = 0; j < parse.size(); j++)
			{
				if (parse[j] == '|')
					expr += "|\n";
				else
					expr += parse[j];
			}

			a_list.as.push_back(dot_a("label", expr));

			if (find(M0.begin(), M0.end(), petri_index(i, true)) != M0.end())
				a_list.as.push_back(dot_a("peripheries", "2"));
		}
		else
		{
			a_list.as.push_back(dot_a("shape", "circle"));
			if (find(M0.begin(), M0.end(), petri_index(i, true)) != M0.end())
			{
				a_list.as.push_back(dot_a("width", "0.15"));
				a_list.as.push_back(dot_a("peripheries", "2"));
				a_list.as.push_back(dot_a("style", "filled"));
				a_list.as.push_back(dot_a("fillcolor", "#000000"));
			}
			else
				a_list.as.push_back(dot_a("width", "0.25"));

			a_list.as.push_back(dot_a("label", ""));
		}

		if (S[i].assumptions != 1)
			a_list.as.push_back(dot_a("assume", S[i].assumptions.print(vars)));

		for (size_t j = 0; j < S[i].assertions.size(); j++)
			a_list.as.push_back(dot_a("assert", S[i].assertions[j].print(vars)));

		if (node_ids)
			a_list.as.push_back(dot_a("xlabel", petri_index(i + s_base, true).name()));

		substmt.node_ids.push_back(dot_node_id(petri_index(i + s_base, true).name()));
		substmt.stmt_type = "node";
		substmt.attr_list.attrs.push_back(a_list);
		stmt.stmt_list.stmts.push_back(substmt);
	}

	for (size_t i = 0; i < T.size(); i++)
	{
		a_list.as.clear();
		substmt.attr_list.attrs.clear();
		substmt.node_ids.clear();
		substmt.stmt_list.stmts.clear();
		substmt.id = "";
		substmt.stmt_type = "";
		substmt.attr_type = "";

		a_list.as.push_back(dot_a("shape", "plaintext"));
		if (T[i].active)
			a_list.as.push_back(dot_a("label", T[i].predicate.print(vars)));
		else
			a_list.as.push_back(dot_a("label", "[ " + T[i].predicate.print(vars) + " ]"));

		if (node_ids)
			a_list.as.push_back(dot_a("xlabel", petri_index(i + t_base, false).name()));

		substmt.node_ids.push_back(dot_node_id(petri_index(i + t_base, false).name()));
		substmt.stmt_type = "node";
		substmt.attr_list.attrs.push_back(a_list);

		stmt.stmt_list.stmts.push_back(substmt);
	}

	for (size_t i = 0; i < arcs.size(); i++)
	{
		a_list.as.clear();
		substmt.attr_list.attrs.clear();
		substmt.node_ids.clear();
		substmt.stmt_list.stmts.clear();
		substmt.id = "";
		substmt.stmt_type = "";
		substmt.attr_type = "";

		pair<petri_index, petri_index> a = arcs[i];
		if (a.first.is_trans())
			a.first.data += t_base;
		else
			a.first.data += s_base;

		if (a.second.is_trans())
			a.second.data += t_base;
		else
			a.second.data += s_base;

		substmt.node_ids.push_back(dot_node_id(a.first.name()));
		substmt.node_ids.push_back(dot_node_id(a.second.name()));
		substmt.stmt_type = "edge";

		if (arc_ids)
		{
			a_list.as.push_back(dot_a("label", to_string(i)));
			substmt.attr_list.attrs.push_back(a_list);
		}

		stmt.stmt_list.stmts.push_back(substmt);
	}

	return stmt;
}

void petri_net::import_dot(tokenizer &tokens, const dot_stmt &g, int t_base, int s_base)
{
	canonical global_expr = 0;
	canonical assume_expr = 1;
	vector<canonical> assert_expr;
	bool global_is_reset_node = false;
	bool global_is_active = true;
	name = g.id;
	for (size_t i = 0; i < g.stmt_list.stmts.size(); i++)
	{
		if (g.stmt_list.stmts[i].stmt_type == "node")
		{
			petri_index id(g.stmt_list.stmts[i].node_ids[0].id.id);
			if (id.is_trans())
			{
				id.data -= t_base;
				T.resize(id.idx()+1, petri_node(global_expr, global_is_active));
			}
			else
			{
				id.data -= s_base;
				S.resize(id.idx()+1, petri_node(global_expr, global_is_active));
			}
			at(id).assumptions = assume_expr;
			at(id).assertions = assert_expr;

			bool is_reset_node = global_is_reset_node;

			for (size_t j = 0; j < g.stmt_list.stmts[i].attr_list.attrs.size(); j++)
				for (size_t k = 0; k < g.stmt_list.stmts[i].attr_list.attrs[j].as.size(); k++)
				{
					size_t old = tokens.index;
					tokens.index = g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.start_token;
					if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "label")
					{
						string expr = "";
						size_t depth = 0;
						for (size_t l = 0; l < g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.size(); l++)
						{
							if ((l == 0 && g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l] == '[') || (depth == 0 && l == g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.size()-1 && g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l] == ']'))
								at(id).active = false;
							else if (!sc(g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l]))
							{
								expr += g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l];
								if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l] == '[')
									depth++;
								else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l] == ']')
									depth--;
							}
						}
						if (expr != "")
							at(id).predicate = canonical(tokens, expr, vars);
					}
					else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "style")
					{
						if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id == "filled")
							is_reset_node = true;
						else
							is_reset_node = false;
					}
					else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "peripheries")
					{
						if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id != "1")
							is_reset_node = true;
						else
							is_reset_node = false;
					}
					else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "assume")
						at(id).assumptions &= canonical(tokens, g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id, vars);
					else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "assert")
						at(id).assertions.push_back(canonical(tokens, g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id, vars));
					tokens.index = old;
				}

			if (is_reset_node)
				M0.push_back(id);
		}
		else if (g.stmt_list.stmts[i].stmt_type == "edge")
		{
			pair<petri_index, petri_index> arc(petri_index(), petri_index(g.stmt_list.stmts[i].node_ids[0].id.id));
			if (arc.second.is_trans())
				arc.second.data -= t_base;
			else
				arc.second.data -= s_base;

			for (size_t j = 1; j < g.stmt_list.stmts[i].node_ids.size(); j++)
			{
				arc.first = arc.second;
				arc.second = petri_index(g.stmt_list.stmts[i].node_ids[j].id.id);
				if (arc.second.is_trans())
					arc.second.data -= t_base;
				else
					arc.second.data -= s_base;

				arcs.push_back(arc);
			}
		}
		else if (g.stmt_list.stmts[i].stmt_type == "attr")
		{
			if (g.stmt_list.stmts[i].attr_type == "graph")
			{
				for (size_t j = 0; j < g.stmt_list.stmts[i].attr_list.attrs.size(); j++)
					for (size_t k = 0; k < g.stmt_list.stmts[i].attr_list.attrs[j].as.size(); k++)
					{
						size_t old = tokens.index;
						tokens.index = g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.start_token;
						if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "reset")
						{
							string expr = "";
							for (size_t l = 0; l < g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.size(); l++)
								if (!sc(g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l]))
									expr += g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l];
							if (expr != "")
								reset = canonical(tokens, expr, vars);
						}
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "label")
							label = g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id;
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "type" && g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id == "remote")
							remote = true;
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "elaborate" && (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id == "false" || g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id == "0"))
							elaborate = false;
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "variables")
						{
							int a = 0;
							int b = g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.find_first_of(",");
							while (b != -1)
							{
								vars.globals.push_back(variable(g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.substr(a, b - a)));
								a = b+1;
								b = g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.find_first_of(",", a);
							}

							vars.globals.push_back(variable(g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.substr(a)));
						}
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "assume")
							assumptions &= canonical(tokens, g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id, vars);
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "assert")
							assertions.push_back(canonical(tokens, g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id, vars));
						tokens.index = old;
					}
			}
			else if (g.stmt_list.stmts[i].attr_type == "node")
			{
				assume_expr = 1;
				assert_expr.clear();
				for (size_t j = 0; j < g.stmt_list.stmts[i].attr_list.attrs.size(); j++)
					for (size_t k = 0; k < g.stmt_list.stmts[i].attr_list.attrs[j].as.size(); k++)
					{
						size_t old = tokens.index;
						tokens.index = g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.start_token;
						if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "peripheries")
						{
							if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id != "1")
								global_is_active = false;
							else
								global_is_active = true;
						}
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "label")
						{
							string expr = "";
							for (size_t l = 0; l < g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id.size(); l++)
								if (!sc(g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l]))
									expr += g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id[l];
							global_expr = canonical(tokens, expr, vars);
						}
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "penwidth")
						{
							if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id != "1")
								global_is_reset_node = true;
							else
								global_is_reset_node = false;
						}
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "assume")
							assume_expr &= canonical(tokens, g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id, vars);
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "assert")
							assert_expr.push_back(canonical(tokens, g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id, vars));
						tokens.index = old;
					}
			}
		}
	}
}
