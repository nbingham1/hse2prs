/*
 * path_space.h
 *
 *  Created on: Mar 14, 2013
 *      Author: nbingham
 */

#include "path.h"

#ifndef path_space_h
#define path_space_h

struct petri_net;

struct path_space
{
	path_space(int s);
	path_space(path p);
	path_space(int s, int f, int t);
	path_space(int s, int f, vector<int> t);
	path_space(int s, vector<int> f, int t);
	path_space(int s, vector<int> f, vector<int> t);
	~path_space();

	list<path> paths;
	path total;

	int size();
	void merge(path_space s);
	void push_back(path p);
	list<path>::iterator erase(list<path>::iterator i);
	list<path>::iterator begin();
	list<path>::iterator end();
	void clear();

	void zero(int i);
	void zero(vector<int> i);
	void inc(int i, int v = 1);
	void dec(int i, int v = 1);
	void inc(list<path>::iterator i, int j, int v = 1);
	void dec(list<path>::iterator i, int j, int v = 1);

	void repair();

	int coverage_count(int n);
	int coverage_count(vector<int> n);

	int length();
	vector<int> coverage_maxes();
	int coverage_max();
	path get_mask();
	void apply_mask(path m);
	path_space inverse();
	path_space coverage(int n);
	path_space avoidance(int n);

	path_space &operator=(path_space s);

	void print_bounds(string name);

	int &operator[](int i);
	path &operator()(int i);

	dot_graph export_dot(petri_net *net);
};

ostream &operator<<(ostream &os, path_space p);

#endif
