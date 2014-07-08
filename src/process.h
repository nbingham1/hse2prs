/*
 * process.h
 *
 *  Created on: Apr 30, 2014
 *      Author: nbingham
 */

#include "petri.h"
#include "variable_space.h"
#include "dot.h"
#include "tokenizer.h"
#include "rule_space.h"

#ifndef process_h
#define process_h

struct process
{
	process();
	process(tokenizer &tokens, dot_graph &graph);
	~process();

	list<pair<petri_net, rule_space> > nets;
	map<pair<petri_net*, petri_net*>, vector<pair<size_t, size_t> > > translations;

	void generate_translations();
	void elaborate();
	void check();
	void solve();

	void generate_prs();
	void generate_bubble();
	void bubble_reshuffle();

	dot_graph_cluster export_bubble();

	void export_prs(ostream &os = cout);

	minterm translate(minterm term, petri_net *from, petri_net *to);

	void print_conflicts();

	void import_dot(tokenizer &tokens, dot_graph &graph);
	dot_graph export_dot();
};

#endif
