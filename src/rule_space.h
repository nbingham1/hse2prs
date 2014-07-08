/*
 * rule_space.h
 *
 *  Created on: Aug 22, 2013
 *      Author: nbingham
 */

#include "rule.h"
#include "petri.h"
#include "canonical.h"
#include "common.h"

#ifndef rule_space_h
#define rule_space_h

struct petri_net;

struct rule_space
{
	rule_space();
	~rule_space();

	vector<rule> rules;
	vector<pair<vector<int>, int> > excl;
	map<pair<int, int>, pair<bool, bool> > bubble_net;

	void generate_minterms(petri_net &net);
	void check(petri_net &net);

	void generate_bubble(variable_space &vars);
	void bubble_reshuffle(variable_space &vars, string name);
	vector<pair<vector<int>, bool> > reshuffle_algorithm(map<pair<int, int>, pair<bool, bool> >::iterator idx, bool forward, map<pair<int, int>, pair<bool, bool> > *net, vector<int> cycle, vector<bool> *inverted);

	dot_graph export_bubble(variable_space &vars, string name);

	void print(variable_space &vars, ostream &fout = cout, string newl = "\n");
};

#endif
