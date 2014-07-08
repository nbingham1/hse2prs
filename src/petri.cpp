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
	active = false;
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
	max_indistinguishables = 0;
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

	for (size_t i = 0; i < n1.size(); i++)
		at(n).predicate &= at(n1[i]).predicate;

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

	for (size_t i = 0; i < n1.size(); i++)
		at(n).predicate &= at(n1[i]).predicate;

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
	petri_index t = put_transition(root, active);
	if (from.is_place())
		connect(from, t);
	else
	{
		petri_index p = put_place(0);
		connect(from, p);
		connect(p, t);
	}

	if (to.is_place())
		connect(t, to);
	else
	{
		petri_index p = put_place(0);
		connect(t, p);
		connect(p, to);
	}
}

void petri_net::insert_before(petri_index i, canonical root, bool active)
{
	vector<int> input = incoming(i);
	petri_index p = put_place(0);
	petri_index t = put_transition(root, active);

	if (i.is_place())
	{
		connect(t, i);
		connect(p, t);
		for (size_t j = 0; j < input.size(); j++)
			connect(arcs[input[j]].first, p);
	}
	else
	{
		connect(p, i);
		connect(t, p);
		for (size_t j = 0; j < input.size(); j++)
			connect(arcs[input[j]].first, t);
	}

	sort(input.rbegin(), input.rend());
	for (size_t j = 0; j < input.size(); j++)
		arcs.erase(arcs.begin() + input[j]);
}

void petri_net::insert_after(petri_index i, canonical root, bool active)
{
	vector<int> input = outgoing(i);
	petri_index p = put_place(0);
	petri_index t = put_transition(root, active);

	if (i.is_place())
	{
		connect(i, t);
		connect(t, p);
		for (size_t j = 0; j < input.size(); j++)
			connect(p, arcs[input[j]].second);
	}
	else
	{
		connect(i, p);
		connect(p, t);
		for (size_t j = 0; j < input.size(); j++)
			connect(t, arcs[input[j]].second);
	}

	sort(input.rbegin(), input.rend());
	for (size_t j = 0; j < input.size(); j++)
		arcs.erase(arcs.begin() + input[j]);
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

bool petri_net::are_parallel_siblings(size_t a0, size_t a1)
{
	return (are_parallel_siblings(arcs[a0].first, arcs[a1].first) ||
			are_parallel_siblings(arcs[a0].first, arcs[a1].second) ||
			are_parallel_siblings(arcs[a0].second, arcs[a1].first) ||
			are_parallel_siblings(arcs[a0].second, arcs[a1].second));
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

bool path_sort(vector<path> p1, vector<path> p2)
{
	if (p1.size() == 0)
		return true;
	else if (p2.size() == 0)
		return false;
	else
		return p1[0].to < p2[0].to;
}

/**
 * Generate the set of paths such that if you cut all paths in that set with
 * state variable transitions, then you cannot get from 'from' to 'to' without
 * crossing at least one of those transitions. This algorithm is linear with
 * the number of places with a worst case of 2N (and a best case of 0).
 *
 * @param from
 * @param to
 * @param result This must be a non-null path space. The resulting paths are appended to
 * 				 this path space.
 */
void petri_net::get_paths(vector<int> from, vector<int> to, path_space *result)
{
	//cout << "Getting Paths " << from << " -> " << to << endl;
	if (result == NULL)
		return;

	/* Start at the reset state because this state is the only state that we know
	 * is not on a branch of a conditional or parallel split. This means that we can
	 * iterate over the net, merging program counters for both conditional and parallel
	 * merges. This is the key feature that allows this algorithm to be linear time.
	 */
	vector<int> start = outgoing(M0);

	/* Each program counter is an array of paths where every path in the array has the
	 * same to node and every path has only one to node. They are allowed to have different
	 * and multiple from nodes.
	 */
	vector<vector<path> > pcs(start.size(), vector<path>());
	for (size_t i = 0; i < start.size(); i++)
	{
		pcs[i].push_back(path(arcs.size()));
		pcs[i][0].to.push_back(start[i]);
	}

	vector<pair<petri_index, vector<size_t> > > ready;
	do
	{
		//cout << pcs << endl;

		ready.clear();

		// Get the set of next possible nodes
		vector<petri_index> n;
		for (size_t i = 0; i < pcs.size(); i++)
			n.push_back(arcs[pcs[i][0].to.front()].second);

		// n is sorted in ascending order
		// the program counters are sorted in ascending order
		sort(n.begin(), n.end());
		sort(pcs.begin(), pcs.end(), path_sort);
		n.resize(unique(n.begin(), n.end()) - n.begin());

		for (size_t i = 0; i < n.size(); i++)
		{
			// p is sorted in ascending order
			vector<int> p = incoming(n[i]);

			// check to see if the program counters cover all required arcs in p
			vector<size_t> count;
			for (size_t j = 0, k = 0; j < pcs.size() && k < p.size(); )
			{
				if (pcs[j][0].to[0] < p[k])
					j++;
				else if ((p[k] < pcs[j][0].to[0]) || (find(to.begin(), to.end(), p[k]) != to.end() && (pcs[j].size() > 1 || (pcs[j].size() == 1 && pcs[j][0].from.size() > 0))))
					k++;
				else
				{
					count.push_back(j);
					j++;
					k++;
				}
			}

			// If it does, then we can merge these counters and increment to the next arc
			if (count.size() == p.size())
				ready.push_back(pair<petri_index, vector<size_t> >(n[i], count));
		}

		//cout << "Ready: " << ready.size() << " " << ready << endl;

		/* update the paths in the ready counters, marking that we visited a particular arc
		 * as long as this path has already crossed one of the from arcs.
		 */
		for (size_t i = 0; i < ready.size(); i++)
			for (size_t j = 0; j < ready[i].second.size(); j++)
				for (size_t k = 0; k < pcs[ready[i].second[j]].size(); k++)
					if (pcs[ready[i].second[j]][k].from.size() > 0)
						pcs[ready[i].second[j]][k].nodes[pcs[ready[i].second[j]][k].to[0]] = 1;

		vector<int> remove;
		for (size_t i = 0; i < ready.size(); i++)
		{
			/* Merge the counters in each group. It is important to notice
			 * that because we are dealing with arcs, an arc can only be in
			 * one merge group at a time because an arc only has one output node.
			 * This simplifies things because we don't have to deal with making
			 * choices when merging groups.
			 */
			if (ready[i].first.is_place())
			{
				/* If we are merging at a place, then we just merge the lists of paths
				 * as a way of saying "here are all of the decisions that can be made
				 * that will end up taking us to this node in the net".
				 */
				for (size_t j = 1; j < ready[i].second.size(); j++)
				{
					if (pcs[ready[i].second[0]].size() == 1 && pcs[ready[i].second[0]][0].from.size() == 0)
						pcs[ready[i].second[0]] = pcs[ready[i].second[j]];
					else if (pcs[ready[i].second[j]].size() > 1 || (pcs[ready[i].second[j]].size() == 1 && pcs[ready[i].second[j]][0].from.size() != 0))
						pcs[ready[i].second[0]].insert(pcs[ready[i].second[0]].end(), pcs[ready[i].second[j]].begin(), pcs[ready[i].second[j]].end());

					// We can't remove the merged paths until the end because of indexing
					remove.push_back(ready[i].second[j]);
				}
			}
			else
			{
				/* If we are merging at a transition, then we need to look at every combination
				 * of paths given the sets of paths for each program counter in the merge. It is
				 * a way of saying "We could have made this set of decisions and still gotten to
				 * this node, or we could have made this other set of decisions, ..." and so on.
				 * In this case we actually combine the paths by taking the max crossing count for
				 * each node on the paths. We do this because if you cut one parallel branch with a
				 * transition, that transition will fire at some point during execution of the other
				 * branches, and it will fire by the time we hit this merge.
				 */
				for (size_t j = 1; j < ready[i].second.size(); j++)
				{
					// We have to save the current number of paths in this set because
					// this number will change and we don't want an infinite loop.
					size_t limit = pcs[ready[i].second[0]].size();
					for (size_t k = 0; k < limit; k++)
						for (size_t l = 0; l < pcs[ready[i].second[j]].size(); l++)
						{
							// Insert a new path to handle this particular combination
							int pc = k;
							if (l < pcs[ready[i].second[j]].size() - 1)
							{
								pc = pcs[ready[i].second[0]].size();
								pcs[ready[i].second[0]].push_back(pcs[ready[i].second[0]][k]);
							}

							// Merge to paths in question by taking the max
							for (size_t m = 0; m < arcs.size(); m++)
								if (pcs[ready[i].second[0]][pc].nodes[m] < pcs[ready[i].second[j]][l].nodes[m])
									pcs[ready[i].second[0]][pc].nodes[m] = pcs[ready[i].second[j]][l].nodes[m];

							// Merge the list of from nodes and sort and unique
							pcs[ready[i].second[0]][pc].from.insert(pcs[ready[i].second[0]][pc].from.end(), pcs[ready[i].second[j]][l].from.begin(), pcs[ready[i].second[j]][l].from.end());
							sort(pcs[ready[i].second[0]][pc].from.begin(), pcs[ready[i].second[0]][pc].from.end());
							pcs[ready[i].second[0]][pc].from.resize(unique(pcs[ready[i].second[0]][pc].from.begin(), pcs[ready[i].second[0]][pc].from.end()) - pcs[ready[i].second[0]][pc].from.begin());
						}

					// We can't remove the merged paths until the end because of indexing
					remove.push_back(ready[i].second[j]);
				}

				sort(pcs[ready[i].second[0]].begin(), pcs[ready[i].second[0]].end());
				pcs[ready[i].second[0]].resize(unique(pcs[ready[i].second[0]].begin(), pcs[ready[i].second[0]].end()) - pcs[ready[i].second[0]].begin(), path(arcs.size()));

				for (size_t j = 0; j < pcs[ready[i].second[0]].size(); )
				{
					bool bad = false;
					vector<petri_index> duplicates;
					for (size_t k = 0; !bad && k < arcs.size(); k++)
						if (arcs[k].first.is_place() && pcs[ready[i].second[0]][j].nodes[k] > 0)
						{
							if (find(duplicates.begin(), duplicates.end(), arcs[k].first) != duplicates.end())
								bad = true;
							else
								duplicates.push_back(arcs[k].first);
						}

					if (bad)
						pcs[ready[i].second[0]].erase(pcs[ready[i].second[0]].begin() + j);
					else
						j++;
				}
			}

			// Increment all of the ready program counters
			vector<int> n = outgoing(ready[i].first);
			for (size_t j = 0; j < n.size(); j++)
			{
				// Duplicate program counters for both parallel and conditional splits
				int pc = ready[i].second[0];
				if (j < n.size()-1)
				{
					pc = pcs.size();
					pcs.push_back(pcs[ready[i].second[0]]);
				}

				/* If this path ran into a from arc, then clear whatever
				 * from list we have and save this particular arc.
				 */
				if (find(from.begin(), from.end(), n[j]) != from.end())
				{
					if (pcs[pc].size() > 1 || (pcs[pc].size() == 1 && pcs[pc][0].from.size() > 0))
						pcs[pc] = vector<path>(1, path(arcs.size(), n[j], n[j]));
					else
						for (size_t k = 0; k < pcs[pc].size(); k++)
							pcs[pc][k].from.push_back(n[j]);
				}
				/* Otherwise if the path's from list is currently clear and this arc is in parallel
				 * with one of the from arcs and not in parallel with any of the to arcs, then we
				 * save it to this path's from list.
				 *
				 * TODO This causes state variable insertion algorithm to create inconsistency.
				 *
				 * While this is technically true, there is a case where this causes the state variable
				 * insertion algorithm to create inconsistency by inserting opposing transitions in parallel.
				 */
				/*else if (pcs[pc].size() == 1)
				{
					bool invalid = false;
					for (size_t k = 0; !invalid && k < to.size(); k++)
						if (are_parallel_siblings(to[k], n[j]))
							invalid = true;

					if (!invalid)
						for (size_t k = 0; pcs[pc][0].from.size() == 0 && k < from.size(); k++)
							if (are_parallel_siblings(from[k], n[j]))
								pcs[pc][0].from.push_back(n[j]);
				}*/

				for (size_t k = 0; k < pcs[pc].size(); )
				{
					// Remove any paths from the program counters that have gone in circles
					if (pcs[pc][k].nodes[n[j]] != 0)
						pcs[pc].erase(pcs[pc].begin() + k);
					// Otherwise, just increment it to the next arc
					else
					{
						pcs[pc][k].to = vector<int>(1, n[j]);
						k++;
					}
				}

				/* If all of the paths in this program counter have gone in circles, just
				 * delete the program counter
				 */
				if (pcs[pc].size() == 0)
					remove.push_back(pc);
			}
		}

		// remove the program counters erased by the merge
		sort(remove.rbegin(), remove.rend());
		remove.resize(unique(remove.begin(), remove.end()) - remove.begin());
		for (size_t i = 0; i < remove.size(); i++)
			pcs.erase(pcs.begin() + remove[i]);

	} while (ready.size() > 0);

	/* Add all of the valid paths, i.e. all of the paths that actually go from 'from' to 'to'
	 * We have to check this because there will be paths left over waiting at parallel or
	 * conditional merges that were never able to merge but aren't at a to arc or never found
	 * a from arc.
	 */
	for (size_t i = 0; i < pcs.size(); i++)
		for (size_t j = 0; j < pcs[i].size(); j++)
		{
			for (size_t k = 0; k < pcs[i][j].from.size(); k++)
				pcs[i][j].nodes[pcs[i][j].from[k]] = 0;

			if (vector_intersection_size(&pcs[i][j].from, &from) != 0 && vector_intersection_size(&pcs[i][j].to, &to) != 0)
				result->paths.push_back(pcs[i][j]);
		}

	/* We need to fix up the total count for the resulting path space
	 * because we added a bunch of new paths.
	 */
	result->repair();

	//result->export_dot(this).print();
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

/**
 * Given a set of up paths and down paths that we want to cut using state variable transitions,
 * we need to filter out arcs on the path that we aren't actually allowed to cut.
 *
 * The following cases need to be filtered out:
 *  - arcs between the place of a conditional split and the guarding transitions of that conditional
 *  - arcs that are in or are in parallel with both up and down path sets
 *
 * @param up_start
 * @param up_paths
 * @param down_start
 * @param down_paths
 */
void petri_net::remove_invalid_split_points(vector<petri_index> up_start, path_space *up_paths, vector<petri_index> down_start, path_space *down_paths)
{
	path up_mask = up_paths->inverse().get_mask();
	path down_mask = down_paths->inverse().get_mask();
	up_mask.nodes.resize(arcs.size(), 0);
	down_mask.nodes.resize(arcs.size(), 0);

	for (size_t i = 0; i < arcs.size(); i++)
		for (size_t j = i+1; j < arcs.size(); j++)
			if (up_mask.nodes[i] == 0 && down_mask.nodes[j] == 0 &&
			   (up_mask.nodes[j] != 0 || down_mask.nodes[i] != 0) &&
			   are_parallel_siblings(i, j))
			{
				up_mask.nodes[j] = 0;
				down_mask.nodes[i] = 0;
			}

	// TODO add a heuristic to choose which (up or down) one gets masked out
	up_paths->apply_mask(down_mask);
	//down_paths->apply_mask(up_mask);

	// TODO conditional merge issue

	// arcs between the place of a conditional split and the guarding transitions of that conditional
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

void petri_net::generate_paths(vector<petri_index> up_start, path_space *up_paths, vector<petri_index> down_start,  path_space *down_paths)
{
	progress("", name + " -- generate_paths: ...", __FILE__, __LINE__);

	down_paths->clear();
	up_paths->clear();
	vector<int> iin, jin;
	for (size_t i = 0; i < arcs.size(); i++)
	{
		if (find(up_start.begin(), up_start.end(), arcs[i].first) != up_start.end() ||
			find(up_start.begin(), up_start.end(), arcs[i].second) != up_start.end())
			iin.push_back(i);

		if (find(down_start.begin(), down_start.end(), arcs[i].first) != down_start.end() ||
			find(down_start.begin(), down_start.end(), arcs[i].second) != down_start.end())
			jin.push_back(i);
	}

	get_paths(iin, jin, up_paths);
	get_paths(jin, iin, down_paths);

	done_progress();
}

/**
 * Figure out what groups of places conflict with what other groups. We are going
 * to need to separate these groups with state variable transitions. This algorithm
 * is O(N^2) with the number of places.
 */
void petri_net::generate_conflicts()
{
	progress("", name + " -- generate_conflicts: ...", __FILE__, __LINE__);

	conflicts.clear();
	indistinguishable.clear();

	for (petri_index i(0, true); i < S.size(); i++)
	{
		// The transitions for which the place i is an implicant
		vector<petri_index> oi = next(i);

		// Get intersection of those transitions
		minterm ti = 1;
		for (size_t j = 0; j < oi.size(); j++)
			if (at(oi[j]).active)
				ti &= at(oi[j]).predicate.terms[0];

		/* We need to hide all of the variables from a given term whose values
		 * in that term are changed by those transitions because that means that
		 * we cannot use those variables to separate this implicant place from
		 * any other place.
		 *
		 * si will contain all of the terms such that affected variables for a term
		 * have the value NULL.
		 * st will contain all of the terms such that unaffected variables for a term
		 * have the value NULL.
		 *
		 * Keep in mind that if a variable's value is unknown in the implicant state and
		 * affected by the transition, it should be counted as affected. However, neither
		 * si nor st will have a NULL value for that variable.
		 */
		canonical nti = ~ti;
		canonical si, st;
		for (size_t j = 0; j < at(i).predicate.terms.size(); j++)
		{
			si.terms.push_back(at(i).predicate.terms[j] & ti);
			st.terms.push_back(at(i).predicate.terms[j] & ti.inverse());
		}

		for (size_t k = 0, l = 0; k < si.terms.size() && l < at(i).predicate.terms.size(); l++)
		{
			/* We need to ignore state encodings for which all of these transitions are vacuous
			 * This will happen when si has no NULL values and st has NULL values for all of
			 * the transitions' variables.
			 *
			 * NOTE: It is sufficient for now to check that st has at least one NULL value because
			 * if a place has more than one output transition, all of those transitions will be guards
			 * and none of them will be active.
			 */
			if (si.terms[k] != 0 && st.terms[l] == 0)
				si.terms.erase(si.terms.begin() + k);
			/* Otherwise if at least one of these transitions are not vacuous, we need to hide
			 * the affected variables. By turning all NULL values to X.
			 */
			else
			{
				si.terms[k] = si.terms[k].xoutnulls() | at(i).predicate.terms[l];
				k++;
			}
		}

		// Now that we have the value for si, we need to check that against all of the other places
		for (petri_index j(0, true); j < S.size(); j++)
		{
			// that aren't i
			if (i != j)
			{
				// We need to keep all of the guards between i and j in mind
				canonical sj = at(j).effective;//get_effective(i, j);

				canonical sboth = si & sj;

				/* Places are indistinguishable if:
				 *  - they are not the same place
				 *  	> i != j
				 *  - the two places do not exist in parallel
				 *  	> are_parallel_siblings(i, j) < 0
				 *  - the two effective state encodings (see above) are not mutually exclusive
				 *  	> si & sj != 0
				 */
				if (!are_parallel_siblings(i, j) && sboth != 0)
				{
					// The transitions for which the place j is an implicant
					vector<petri_index> oj = next(j);

					// Get intersection of those transitions
					minterm tj = 1;
					for (size_t k = 0; k < oj.size(); k++)
						if (at(oj[k]).active)
							tj &= at(oj[k]).predicate.terms[0];

					canonical jtj = (sboth >> tj);

					/* States are conflicting if:
					 *  - they are indistinguishable
					 *  - the conflict is not caused my non-mutually exclusive guards in a conditional
					 *  	> !are_sibling_guards(i, j)
					 *  - the transition which causes the conflict is not a vacuous firing in the other state
					 *  - the transition which causes the conflict would not normally happen anyways as a result of the other state
					 */
					if (!is_mutex(&nti, &jtj))
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

	done_progress();

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

	//export_dot(0, 0).print();

	bool has_error = false;

	progress("", name + " -- solve_conflicts: finding split points", __FILE__, __LINE__);
	for (map<petri_index, list<vector<petri_index> > >::iterator i = conflicts.begin(); i != conflicts.end(); i++)
	{
		for (list<vector<petri_index> >::iterator lj = i->second.begin(); lj != i->second.end(); lj++)
		{
			path_space up_paths(arcs.size()), down_paths(arcs.size());
			vector<int> uptrans, downtrans;

			generate_paths(vector<petri_index>(1, i->first), &up_paths, *lj, &down_paths);
			remove_invalid_split_points(vector<petri_index>(1, i->first), &up_paths, *lj, &down_paths);


			up_paths.print_bounds("Up");
			uptrans = choose_split_points(&up_paths);

			down_paths.print_bounds("Down");
			downtrans = choose_split_points(&down_paths);

			vector_symmetric_compliment(&uptrans, &downtrans);

			if (uptrans.size() == 0 || downtrans.size() == 0)
			{
				error("", "no solution in process '" + name + "' for the conflict set: " + to_string(i->first) + " -> " + to_string(*lj), "", __FILE__, __LINE__);
				has_error = true;
			}
			else if (uptrans <= downtrans)
				ip.push_back(pair<vector<int>, vector<int> >(uptrans, downtrans));
			else if (downtrans <= uptrans)
				ip.push_back(pair<vector<int>, vector<int> >(downtrans, uptrans));
			log("", "Done", __FILE__, __LINE__);
		}
	}

	if (ip.size() == 0)
		return false;

	if (!has_error)
	{
		vector<int> reset_arcs = outgoing(M0);
		sort(ip.begin(), ip.end());
		ip.resize(unique(ip.begin(), ip.end()) - ip.begin());
		string str;
		for (size_t j = 0; j < ip.size(); j++)
		{
			if (j != 0)
				str += ", ";

			vector<petri_arc> up_out, down_out;

			for (int k = 0; k < (int)ip[j].first.size(); k++)
				up_out.push_back(arcs[ip[j].first[k]]);
			for (int k = 0; k < (int)ip[j].second.size(); k++)
				down_out.push_back(arcs[ip[j].second[k]]);

			str += "{up:" + to_string(up_out) + " down:" + to_string(down_out) + "}";
		}

		progress("", name + " -- solve_conflicts: inserting state variables " + str, __FILE__, __LINE__);

		vector<size_t> vids;
		for (size_t j = 0; j < ip.size(); j++)
		{
			string vname = vars.unique_name();
			vids.push_back(vars.globals.size());
			vars.globals.push_back(variable(vname));
			vars.globals.back().written = true;

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
				reset &= minterm(vids.back(), 1);
			else if (down_coverage > 0 && up_coverage == 0)
				reset &= minterm(vids.back(), 0);
		}

		for (size_t j = 0; j < ip.size(); j++)
		{
			for (int k = 0; k < (int)ip[j].first.size(); k++)
				insert(ip[j].first[k], canonical(vids[j], 1), true);
			for (int k = 0; k < (int)ip[j].second.size(); k++)
				insert(ip[j].second[k], canonical(vids[j], 0), true);
		}
	}

	done_progress();

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
			vector<petri_index> n = next(i), p = prev(i), np = prev(n), pn = next(p), nn = next(n), pp = prev(p), ppn = next(pp), nnp = prev(nn);

			/*bool forward_types_equal = true;
			bool backward_types_equal = true;

			for (size_t j = 0; forward_types_equal && j < nn.size(); j++)
				if (at(nn[j]).active != at(i).active)
					forward_types_equal = false;

			for (size_t j = 0; backward_types_equal && j < pp.size(); j++)
				if (at(pp[j]).active != at(i).active)
					backward_types_equal = false;*/

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
			else if (at(i).predicate == 1 && p.size() == 1 && pp.size() == 1 && ppn.size() == 1 && pn.size() == 1)
			{
				pinch_backward(i);
				change = true;
			}
			else if (at(i).predicate == 1 && n.size() == 1 && nn.size() == 1 && nnp.size() == 1 && np.size() == 1)
			{
				pinch_forward(i);
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

	// Fix the conflict detection bug where if a node has multiple ingoing and outgoing arcs
	// and affects a channel variable, it can create an invisible conflict in the state after the
	// transition but before the parallel split
	change = true;
	while (change)
	{
		change = false;
		for (size_t i = 0; i < T.size(); i++)
		{
			size_t os = outgoing(petri_index(i, false)).size();
			size_t is = incoming(petri_index(i, false)).size();
			if (at(petri_index(i, false)).predicate != 1 && os > 1)
			{
				insert_after(petri_index(i, false), 1, true);
				change = true;
			}
			else if (at(petri_index(i, false)).predicate != 1 && is > 1)
			{
				insert_before(petri_index(i, false), 1, true);
				change = true;
			}
			else if (os > 1 && is > 1)
			{
				insert_after(petri_index(i, false), 1, true);
				change = true;
			}
		}

		for (size_t i = 0; i < S.size(); i++)
		{
			size_t os = outgoing(petri_index(i, true)).size();
			size_t is = incoming(petri_index(i, true)).size();
			if (os > 1 && is > 1)
			{
				insert_after(petri_index(i, true), 1, true);
				change = true;
			}
		}
	}
}

/**
 * Given the base set of node markings, this returns that base set along with the
 * minimal set of node markings required to allow this base set to traverse through
 * all merges it will encounter going either forward or backward.
 *
 * In general, conditional splits create a new execution and parallel splits
 * create a new program counter. Then conditional merges are ignored and
 * program counters at parallel merges must wait for a program counter at every
 * incoming arc to that merge before making progress. However, this can be
 * switched using the conditional flag.
 *
 * TODO well formed input assumption
 *
 * This function makes one assumption about the format of the input. That is
 * that you cannot have markings stuck at more than one merge in series.
 * For example (markings are designated by [brackets])
 *
 *        P1--T1--[P2]          P6--T5--P7
 *       /          \          /          \
 * P0--T0            T3--P5--T4            T7--P10
 *       \          /          \          /
 *        P3--T2--P4            P8--T6--[P9]
 *
 * If the input breaks this assumption, like in this example, the result will be the
 * insertion of an unnecessary marking at each merge not first in the series. For example
 * in this example, there will be an unnecessary marking inserted at P7 and a necessary
 * marking inserted at the arc P4.
 *
 * @param base set of node markings
 * @param backward iteration instead of forward iteration
 * @param conditional merges instead of parallel merges
 * @return minimal set of markings to prevent deadlock
 */
vector<petri_index> petri_net::get_cut(vector<petri_index> base, bool backward, bool conditional)
{
	vector<petri_index> result;
	result.insert(result.end(), base.begin(), base.end());
	list<pair<vector<petri_index>, vector<bool> > > execs(1, pair<vector<petri_index>, vector<bool> >(base, vector<bool>(S.size() + T.size(), false)));

	// Run through all possible executions from the starting index looking for deadlock.
	for (list<pair<vector<petri_index>, vector<bool> > >::iterator exec = execs.begin(); exec != execs.end(); exec = execs.erase(exec))
	{
		//cout << "\tStart Execution" << endl;
		bool done = false;
		while (!done)
		{
			vector<pair<petri_index, vector<size_t> > > merge_movable;
			vector<petri_index> merge_stuck;
			vector<size_t> movable;

			/* Figure out where we can move next and check to see if there are any markings
			 * at locations that are unaffected by merges (normally places because conditional
			 * merges are ignored, however this can be switched using the conditional flag).
			 */
			vector<petri_index> n;
			for (size_t i = 0; i < exec->first.size(); i++)
			{
				if ((!conditional && exec->first[i].is_place()) || (conditional && exec->first[i].is_trans()))
				{
					vector<petri_index> temp = backward ? prev(exec->first[i]) : next(exec->first[i]);
					n.insert(n.end(), temp.begin(), temp.end());
				}
				else
					movable.push_back(i);
			}

			// n is sorted in ascending order
			// the program counters are sorted in ascending order
			sort(n.begin(), n.end());
			sort(exec->first.begin(), exec->first.end());
			n.resize(unique(n.begin(), n.end()) - n.begin());

			/* If there aren't any markings at locations unaffected by merges, then loop through
			 * the set of next possible moves and figure out which ones aren't stuck at a merge.
			 * For the ones that are stuck at a merge, record which incoming markings are needed
			 * to unstick it.
			 */
			if (movable.size() == 0)
			{
				for (size_t i = 0; i < n.size(); i++)
				{
					// p is sorted in descending order so we need to reverse it
					vector<petri_index> p = backward ? next(n[i]) : prev(n[i]);
					reverse(p.begin(), p.end());

					// check to see if the program counters cover all required nodes in p
					vector<size_t> count;
					bool found = false;
					size_t k = 0;
					for (size_t j = 0; j < exec->first.size() && k < p.size(); )
					{
						if (exec->first[j] < p[k])
							j++;
						else if (p[k] < exec->first[j])
						{
							if (!found)
								merge_stuck.push_back(p[k]);

							k++;
							found = false;
						}
						else
						{
							count.push_back(j);
							j++;
							found = true;
						}
					}

					if (k+1 < p.size())
						merge_stuck.insert(merge_stuck.end(), p.begin() + k+1, p.end());

					/* easiest way is to count them up, but there may be duplicates
					 * the duplicates are intentional since it allows me to remove them
					 * from the execution later.
					 */
					vector<size_t> temp = count;
					temp.resize(unique(temp.begin(), temp.end()) - temp.begin());

					if (temp.size() == p.size())
						merge_movable.push_back(pair<petri_index, vector<size_t> >(n[i], count));
				}
			}

			/* If we aren't deadlocked, then we can check
			 * if the program counters have looped around and already covered
			 * the locations they are currently at. If everything has been covered
			 * then we are done.
			 */
			if (movable.size() != 0 || merge_movable.size() != 0)
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
			 * duplicating executions or program counters when necessary.
			 */
			if (!done && movable.size() > 0)
			{
				// Handle the markings at locations unaffected by merges
				for (size_t i = 0; i < movable.size(); i++)
				{
					vector<petri_index> n = backward ? prev(exec->first[movable[i]]) : next(exec->first[movable[i]]);
					for (size_t k = 0; k < n.size(); k++)
					{
						if (k < n.size()-1)
							exec->first.push_back(n[k]);
						else
							exec->first[movable[i]] = n[k];
					}
				}
			}
			else if (!done && merge_movable.size() > 0)
			{
				// Handle the markings at locations that are affected by merges

				/* Figure out which paths must be taken mutually exclusively. Two
				 * paths cannot both be taken in one execution if they share input
				 * markings because taking a path moves that input marking.
				 */
				vector<vector<size_t> > groupings;
				for (size_t i = 0; i < merge_movable.size(); i++)
				{
					// Look for groups that share input markings with this next marking
					vector<size_t> found;
					for (size_t j = 0; j < groupings.size(); j++)
						for (size_t k = 0; k < groupings[j].size(); k++)
							if (vector_intersection_size(&merge_movable[groupings[j][k]].second, &merge_movable[i].second) > 0)
								found.push_back(j);

					sort(found.rbegin(), found.rend());
					found.resize(unique(found.begin(), found.end()) - found.begin());

					// If none were found, create a new one
					if (found.size() == 0)
						groupings.push_back(vector<size_t>(1, i));
					else
					{
						// If more than one was found, merge them
						for (size_t j = 0; j < found.size()-1; j++)
						{
							groupings[found[found.size()-1]].insert(groupings[found[found.size()-1]].end(), groupings[found[j]].begin(), groupings[found[j]].end());
							groupings.erase(groupings.begin() + found[j]);
						}

						// insert this next marking into the final group and clean up
						groupings[found[found.size()-1]].push_back(i);

						sort(groupings[found[found.size()-1]].begin(), groupings[found[found.size()-1]].end());
						groupings[found[found.size()-1]].resize(unique(groupings[found[found.size()-1]].begin(), groupings[found[found.size()-1]].end()) - groupings[found[found.size()-1]].begin());
					}
				}

				/* There are cases in handshaking expansions where even though
				 * structurally, there are possible paths, those paths are not actually
				 * valid because of mutual exclusion guaranteed by the data. This happens
				 * in Andrew Lines' implementation of a pchb split.
				 *
				 * *[
				 *   (
				 *    [  A.e & S.r.f & L.r.f -> A.r.f+
				 *    [] A.e & S.r.f & L.r.t -> A.r.t+
				 *    [] S.r.t -> skip                   <----- Here
				 *    ]||
				 *    [  B.e & S.r.t & L.r.f -> B.r.f+
				 *    [] B.e & S.r.t & L.r.t -> B.r.t+
				 *    [] S.r.f -> skip                   <----- Here
				 *    ]
				 *   ); S.e-, L.e-;
				 *   (
				 *    [~A.e | ~A.r.f & ~A.r.t -> A.r.f-, A.r.t-] ||
				 *    [~B.e | ~B.r.f & ~B.r.t -> B.r.f-, B.r.t-]
				 *   ); [~S.r.f & ~S.r.t & ~L.r.f & ~L.r.t -> S.e+, L.e+]
				 *  ]
				 *
				 * S.r.f and S.r.t cannot both be true, so we need to ignore these cases. So
				 * we first generate all the possible decisions we can make.
				 */
				vector<vector<size_t> > valid_paths;
				vector<size_t> iter(groupings.size(), 0);
				bool last = false;
				while (!last)
				{
					last = true;
					for (size_t i = 0; last && i < iter.size(); i++)
						if (iter[i] < groupings[i].size()-1)
							last = false;

					valid_paths.push_back(iter);

					iter[0]++;
					for (size_t i = 0; i < iter.size()-1 && iter[i] >= groupings[i].size(); i++)
					{
						iter[i] = 0;
						iter[i+1]++;
					}
				}

				/* And then we remove the invalid paths (i.e. paths thats will result in
				 * a pair of markings that are not in parallel).
				 */
				for (size_t i = 0; i < valid_paths.size(); )
				{
					// Generate the resulting marking set and store it in 'n'
					bool valid = true;
					vector<petri_index> n;
					vector<size_t> used;
					for (size_t j = 0; j < groupings.size(); j++)
					{
						n.push_back(merge_movable[groupings[j][valid_paths[i][j]]].first);
						used.insert(used.end(), merge_movable[groupings[j][valid_paths[i][j]]].second.begin(), merge_movable[groupings[j][valid_paths[i][j]]].second.end());
					}

					sort(used.begin(), used.end());
					used.resize(unique(used.begin(), used.end()) - used.begin());

					for (size_t j = 0; j < exec->first.size(); j++)
						if (find(used.begin(), used.end(), j) == used.end())
							n.push_back(exec->first[j]);

					sort(n.begin(), n.end());
					n.resize(unique(n.begin(), n.end()) - n.begin());

					// Check to make sure all-to-all parallelism holds
					for (size_t j = 0; valid && j < n.size(); j++)
						for (size_t k = j+1; valid && k < n.size(); k++)
							if (!are_parallel_siblings(n[j], n[k]))
								valid = false;

					// If it doesn't, then remove this case
					if (!valid)
						valid_paths.erase(valid_paths.begin() + i);
					else
						i++;
				}

				/* Duplicate the execution once for each final possible path,
				 * execute the necessary merges, and then take that path.
				 */
				for (size_t i = 0; i < valid_paths.size(); i++)
				{
					// Duplicate
					list<pair<vector<petri_index>, vector<bool> > >::iterator temp_exec = exec;
					if (i < valid_paths.size()-1)
					{
						execs.push_back(*exec);
						temp_exec = execs.end();
						temp_exec--;
					}

					// Take the path
					vector<size_t> remove;
					for (size_t j = 0; j < groupings.size(); j++)
					{
						temp_exec->first[merge_movable[groupings[j][valid_paths[i][j]]].second[0]] = merge_movable[groupings[j][valid_paths[i][j]]].first;

						for (size_t k = merge_movable[groupings[j][valid_paths[i][j]]].second.size()-1; k > 0; k--)
							remove.push_back(merge_movable[groupings[j][valid_paths[i][j]]].second[k]);
					}

					// Execute the merge
					sort(remove.rbegin(), remove.rend());
					remove.resize(unique(remove.begin(), remove.end()) - remove.begin());

					for (size_t k = 0; k < remove.size(); k++)
						temp_exec->first.erase(temp_exec->first.begin() + remove[k]);
				}
			}
			/**
			 * Every time deadlock is detected in the execution,
			 * insert enough indices at the merge point to allow
			 * execution to continue. Also, insert these indices
			 * into the output set.
			 */
			else if (!done)
			{
				for (size_t i = 0; i < merge_stuck.size(); i++)
				{
					exec->first.push_back(merge_stuck[i]);
					result.push_back(merge_stuck[i]);
				}
			}
		}
	}

	// Sort the state so that we can have some standard for comparison.
	sort(result.begin(), result.end());
	return result;
}

/**
 * Given the base set of arc markings, this returns that base set along with the
 * minimal set of arc markings required to allow this base set to traverse through
 * all merges it will encounter going either forward or backward.
 *
 * In general, conditional splits create a new execution and parallel splits
 * create a new program counter. Then conditional merges are ignored and
 * program counters at parallel merges must wait for a program counter at every
 * incoming arc to that merge before making progress. However, this can be
 * switched using the conditional flag.
 *
 * TODO well formed input assumption
 *
 * This function makes one assumption about the format of the input. That is
 * that you cannot have markings stuck at more than one merge in series.
 * For example (markings are designated by [brackets])
 *
 *        P1--T1--P2            P6--T5--P7
 *       /         [\]         /          \
 * P0--T0            T3--P5--T4            T7--P10
 *       \          /          \         [/]
 *        P3--T2--P4            P8--T6--P9
 *
 * If the input breaks this assumption, like in this example, the result will be the
 * insertion of an unnecessary marking at each merge not first in the series. For example
 * in this example, there will be an unnecessary marking inserted at the arc P7 -> T7
 * and a necessary marking inserted at the arc P4 -> T3
 *
 * @param base set of arc markings
 * @param backward iteration instead of forward iteration
 * @param conditional merges instead of parallel merges
 * @return minimal set of markings to prevent deadlock
 */
vector<int> petri_net::get_arc_cut(vector<int> base, bool backward, bool conditional)
{
	cout << "Getting Cut" << endl;
	vector<int> result;
	result.insert(result.end(), base.begin(), base.end());
	list<pair<vector<int>, vector<bool> > > execs(1, pair<vector<int>, vector<bool> >(base, vector<bool>(arcs.size(), false)));

	// Run through all possible executions from the starting index looking for deadlock.
	for (list<pair<vector<int>, vector<bool> > >::iterator exec = execs.begin(); exec != execs.end(); exec = execs.erase(exec))
	{
		//cout << "\tStart Execution" << endl;
		bool done = false;
		while (!done)
		{
			sort(exec->first.begin(), exec->first.end());
			exec->first.resize(unique(exec->first.begin(), exec->first.end()) - exec->first.begin());

			vector<pair<petri_index, vector<size_t> > > merge_movable;
			vector<int> merge_stuck;
			vector<size_t> movable;

			/* Figure out where we can move next and check to see if there are any markings
			 * at locations that are unaffected by merges (normally arcs after places because
			 * conditional merges are ignored, however this can be switched using the conditional flag).
			 */
			vector<petri_index> n;
			for (size_t i = 0; i < exec->first.size(); i++)
			{
				if ((!conditional && arcs[exec->first[i]].second.is_trans()) || (conditional && arcs[exec->first[i]].second.is_place()))
					n.push_back(backward ? arcs[exec->first[i]].first : arcs[exec->first[i]].second);
				else
					movable.push_back(i);
			}

			// n is sorted in ascending order
			// the program counters are sorted in ascending order
			sort(n.begin(), n.end());
			sort(exec->first.begin(), exec->first.end());
			n.resize(unique(n.begin(), n.end()) - n.begin());

			/* If there aren't any markings at locations unaffected by merges, then loop through
			 * the set of next possible moves and figure out which ones aren't stuck at a merge.
			 * For the ones that are stuck at a merge, record which incoming markings are needed
			 * to unstick it.
			 */
			if (movable.size() == 0)
			{
				for (size_t i = 0; i < n.size(); i++)
				{
					// p is sorted in ascending order
					vector<int> p = backward ? outgoing(n[i]) : incoming(n[i]);

					// check to see if the program counters cover all required arcs in p
					vector<size_t> count;
					bool found = false;
					size_t k = 0;
					for (size_t j = 0; j < exec->first.size() && k < p.size(); )
					{
						if (exec->first[j] < p[k])
							j++;
						else if (p[k] < exec->first[j])
						{
							if (!found)
								merge_stuck.push_back(p[k]);

							k++;
							found = false;
						}
						else
						{
							count.push_back(j);
							j++;
							found = true;
						}
					}

					if (k+1 < p.size())
						merge_stuck.insert(merge_stuck.end(), p.begin() + k+1, p.end());

					/* easiest way is to count them up, but there may be duplicates
					 * the duplicates are intentional since it allows me to remove them
					 * from the execution later.
					 */
					vector<size_t> temp = count;
					temp.resize(unique(temp.begin(), temp.end()) - temp.begin());

					if (temp.size() == p.size())
						merge_movable.push_back(pair<petri_index, vector<size_t> >(n[i], count));
				}
			}

			/* If we aren't deadlocked, then we can check
			 * if the program counters have looped around and already covered
			 * the locations they are currently at. If everything has been covered
			 * then we are done.
			 */
			if (movable.size() != 0 || merge_movable.size() != 0)
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

			/* If we are not done, handle the next set of movements
			 * duplicating executions or program counters when necessary.
			 */
			if (!done && movable.size() > 0)
			{
				vector<vector<int> > n(movable.size());
				vector<vector<size_t> > valid_paths;
				for (size_t i = 0; i < movable.size(); i++)
					n[i] = backward ? prev_arc(exec->first[movable[i]]) : next_arc(exec->first[movable[i]]);

				vector<size_t> iter(movable.size(), 0);
				bool last = false;
				while (!last)
				{
					last = true;
					for (size_t i = 0; last && i < iter.size(); i++)
						if (iter[i] < n[i].size()-1)
							last = false;

					valid_paths.push_back(iter);

					iter[0]++;
					for (size_t i = 0; i < iter.size()-1 && iter[i] >= n[i].size(); i++)
					{
						iter[i] = 0;
						iter[i+1]++;
					}
				}

				for (size_t i = 0; i < valid_paths.size(); i++)
				{
					// Generate the resulting marking set and store it in 'n'
					bool valid = true;
					vector<petri_index> test;
					vector<size_t> used;
					for (size_t j = 0; j < exec->first.size(); j++)
					{
						vector<size_t>::iterator k = find(movable.begin(), movable.end(), j);
						if (k != movable.end())
							test.push_back(arcs[n[k-movable.begin()][valid_paths[i][k - movable.begin()]]].second);
						else
							test.push_back(arcs[exec->first[j]].second);
					}

					sort(test.begin(), test.end());
					test.resize(unique(test.begin(), test.end()) - test.begin());

					// Check to make sure all-to-all parallelism holds
					for (size_t j = 0; valid && j < test.size(); j++)
						for (size_t k = j+1; valid && k < test.size(); k++)
							if (!are_parallel_siblings(test[j], test[k]))
								valid = false;

					// If it doesn't, then remove this case
					if (!valid)
						valid_paths.erase(valid_paths.begin() + i);
					else
						i++;
				}

				for (size_t i = 0; i < valid_paths.size(); i++)
				{
					list<pair<vector<int>, vector<bool> > >::iterator temp = exec;
					if (i < valid_paths.size()-1)
					{
						execs.push_back(*exec);
						temp = execs.end();
						temp--;
					}

					for (size_t j = 0; j < iter.size(); j++)
						temp->first[movable[j]] = n[j][valid_paths[i][j]];
				}
			}
			else if (!done && merge_movable.size() > 0)
			{
				// Handle the markings at locations that are affected by merges

				/* Since an arc cannot be in multiple merges, we don't have to worry about
				 * grouping like we do in get_cut.
				 */
				for (size_t i = 0; i < merge_movable.size(); i++)
				{
					vector<int> n = backward ? incoming(merge_movable[i].first) : outgoing(merge_movable[i].first);

					// Take the path
					size_t j = 0;
					for (j = 0; j < merge_movable[i].second.size() && j < n.size(); j++)
						exec->first[merge_movable[i].second[j]] = n[j];

					if (j < n.size())
						for (; j < n.size(); j++)
							exec->first.push_back(n[j]);
					else if (j < merge_movable[i].second.size())
						for (size_t k = merge_movable[i].second.size()-1; k >= j && k < merge_movable[i].second.size(); k--)
							exec->first.erase(exec->first.begin() + merge_movable[i].second[k]);
				}
			}
			/**
			 * Every time deadlock is detected in the execution,
			 * insert enough indices at the merge point to allow
			 * execution to continue. Also, insert these indices
			 * into the state we are trying to initialize.
			 */
			else if (!done)
			{
				for (size_t i = 0; i < merge_stuck.size(); i++)
				{
					exec->first.push_back(merge_stuck[i]);
					result.push_back(merge_stuck[i]);
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

canonical petri_net::get_effective(petri_index observer, petri_index location)
{
	int count = -1;
	list<map<vector<petri_index>, pair<canonical, int> >::iterator> L2;
	for (map<vector<petri_index>, pair<canonical, int> >::iterator o = at(observer).observed.begin(); o != at(observer).observed.end(); o++)
	{
		if (find(o->first.begin(), o->first.end(), location) != o->first.end())
		{
			if (o->second.second > count)
			{
				L2.clear();
				count = o->second.second;
			}

			if (o->second.second == count)
				L2.push_back(o);
		}
	}

	canonical result = at(location).predicate;
	for (list<map<vector<petri_index>, pair<canonical, int> >::iterator>::iterator o = L2.begin(); o != L2.end(); o++)
		result &= (*o)->second.first;

	return result;
}

canonical petri_net::get_effective(petri_index observer, vector<petri_index> location)
{
	sort(location.begin(), location.end());

	// get the list of observed states that share the maximum number
	// of nodes with location (there must be at least one node shared)
	int count = 1;
	list<map<vector<petri_index>, pair<canonical, int> >::iterator> L1;
	for (map<vector<petri_index>, pair<canonical, int> >::iterator o = at(observer).observed.begin(); o != at(observer).observed.end(); o++)
	{
		int shared = vector_intersection_size(&location, &o->first);
		if (shared > count)
		{
			L1.clear();
			count = shared;
		}

		if (shared == count)
			L1.push_back(o);
	}

	count = -1;
	list<map<vector<petri_index>, pair<canonical, int> >::iterator> L2;
	for (list<map<vector<petri_index>, pair<canonical, int> >::iterator>::iterator o = L1.begin(); o != L1.end(); o++)
	{
		if ((*o)->second.second > count)
		{
			L2.clear();
			count = (*o)->second.second;
		}

		if ((*o)->second.second == count)
			L2.push_back(*o);
	}

	canonical result = 1;
	for (size_t i = 0; i < location.size(); i++)
		result &= at(location[i]).predicate;

	for (list<map<vector<petri_index>, pair<canonical, int> >::iterator>::iterator o = L2.begin(); o != L2.end(); o++)
		result &= (*o)->second.first;

	return result;
}

canonical petri_net::get_effective(vector<petri_index> observer, petri_index location)
{
	sort(observer.begin(), observer.end());

	canonical result;
	for (size_t i = 0; i < observer.size(); i++)
	{
		int count = -1;
		list<map<vector<petri_index>, pair<canonical, int> >::iterator> L2;
		for (map<vector<petri_index>, pair<canonical, int> >::iterator o = at(observer[i]).observed.begin(); o != at(observer[i]).observed.end(); o++)
		{
			if (find(o->first.begin(), o->first.end(), location) != o->first.end())
			{
				if (o->second.second > count)
				{
					L2.clear();
					count = o->second.second;
				}

				if (o->second.second == count)
					L2.push_back(o);
			}
		}

		canonical temp = at(location).predicate;
		for (list<map<vector<petri_index>, pair<canonical, int> >::iterator>::iterator o = L2.begin(); o != L2.end(); o++)
			temp &= (*o)->second.first;

		result |= temp;
	}

	return result;
}

canonical petri_net::get_effective(vector<petri_index> observer, vector<petri_index> location)
{
	sort(observer.begin(), observer.end());
	sort(location.begin(), location.end());

	canonical result;
	for (size_t i = 0; i < observer.size(); i++)
	{
		// get the list of observed states that share the maximum number
		// of nodes with location (there must be at least one node shared)
		list<map<vector<petri_index>, pair<canonical, int> >::iterator> L1;
		int count = 1;
		for (map<vector<petri_index>, pair<canonical, int> >::iterator o = at(observer[i]).observed.begin(); o != at(observer[i]).observed.end(); o++)
		{
			int shared = vector_intersection_size(&location, &o->first);
			if (shared > count)
			{
				L1.clear();
				count = shared;
			}

			if (shared == count)
				L1.push_back(o);
		}

		count = -1;
		list<map<vector<petri_index>, pair<canonical, int> >::iterator> L2;
		for (list<map<vector<petri_index>, pair<canonical, int> >::iterator>::iterator o = L1.begin(); o != L1.end(); o++)
		{
			if ((*o)->second.second > count)
			{
				L2.clear();
				count = (*o)->second.second;
			}

			if ((*o)->second.second == count)
				L2.push_back(*o);
		}

		canonical temp = 1;
		for (list<map<vector<petri_index>, pair<canonical, int> >::iterator>::iterator o = L2.begin(); o != L2.end(); o++)
			temp &= (*o)->second.first;

		result |= temp;
	}

	for (size_t i = 0; i < location.size(); i++)
		result &= at(location[i]).predicate;

	return result;
}

dot_stmt petri_net::export_dot(int t_base, int s_base)
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

	string pnodes = "";
	for (list<pair<petri_index, petri_index> >::iterator n = parallel_nodes.begin(); n != parallel_nodes.end(); n++)
	{
		if (n != parallel_nodes.begin())
			pnodes += " ";

		pnodes += to_string(n->first) + to_string(n->second);
	}

	a_list.as.push_back(dot_a("parallel_nodes", pnodes));

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
			string parse = S[i].effective.print(vars);
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

		substmt.attr_list.attrs.push_back(dot_a_list());
		substmt.attr_list.attrs.back().as.push_back(dot_a("label", to_string(i)));

		stmt.stmt_list.stmts.push_back(substmt);
	}

	return stmt;
}

void petri_net::import_dot(tokenizer &tokens, const dot_stmt &g, int t_base, int s_base)
{
	reset = 1;

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
						{
							at(id).predicate = canonical(tokens, expr, vars);

							if (id.is_trans())
							{
								vector<size_t> vl = at(id).predicate.vars();
								for (size_t l = 0; l < vl.size(); l++)
								{
									if (at(id).active)
										vars.globals[vl[l]].written = true;
									else
										vars.globals[vl[l]].read = true;
								}
							}
						}
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
						else if (g.stmt_list.stmts[i].attr_list.attrs[j].as[k].first.id == "parallel_nodes")
						{
							string pnodes = g.stmt_list.stmts[i].attr_list.attrs[j].as[k].second.id;
							size_t j, k;
							for (j = 0, k = pnodes.find_first_of(" "); j != string::npos && k != string::npos; j = k+1, k = pnodes.find_first_of(" ", j))
							{
								string npair = pnodes.substr(j, k-j);
								size_t l = npair.find_first_of("ST", 1);
								parallel_nodes.push_back(pair<petri_index, petri_index>(petri_index(npair.substr(0, l)), petri_index(npair.substr(l))));
							}
							if (j < pnodes.size())
							{
								string npair = pnodes.substr(j);
								size_t l = npair.find_first_of("ST", 1);
								parallel_nodes.push_back(pair<petri_index, petri_index>(petri_index(npair.substr(0, l)), petri_index(npair.substr(l))));
							}
						}
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
