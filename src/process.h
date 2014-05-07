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

#ifndef process_h
#define process_h

struct process
{
	process();
	process(tokenizer &tokens, dot_graph &graph);
	~process();

	list<petri_net> nets;

	void elaborate();
	void check();
	void solve();

	void print_conflicts();

	void import_dot(tokenizer &tokens, dot_graph &graph);
	dot_graph export_dot(bool label = false);
};

#endif
