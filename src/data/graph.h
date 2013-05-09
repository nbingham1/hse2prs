/*
 * graph.h
 *
 * Graph is a data structure that contains structural information about a given program's execution.
 * Each program has an associated graph. A graph contains the state space and trace space data structures.
 * It also contains edges describing how each state (and likewise trace) can transition to other states
 * based on the execution of the program.
 */

#include "../common.h"
#include "tspace.h"
#include "sspace.h"
#include "vspace.h"

#ifndef graph_h
#define graph_h

/*struct state
{
	value_array values;
	map<int, int> branch;
};

struct transition
{
	value_array delta;

	// limited definition
	// 01 is output
	// 10 is input
	// 00 is no obligation
	// 11 is no obligation
	value_array xdi;
};

template <class s, class t>
struct petri
{
	petri();
	~petri();

	vector<s> S;
	vector<t> T;
	matrix<int> Wp;
	matrix<int> Wn;
	vector<int> M0;
};*/

struct graph
{
	graph();
	~graph();

	state_space states;
	trace_space traces;

	trace_space delta;
	trace_space up;
	trace_space down;

	vector<vector<int> > up_firings;
	vector<vector<int> > up_firings_transpose;
	vector<vector<int> > down_firings;
	vector<vector<int> > down_firings_transpose;
	vector<vector<int> > up_conflicts;
	vector<vector<int> > down_conflicts;

	// From				  , To
	// Instruction indexed, Instruction indexed
	vector<vector<int> > front_edges;
	vector<vector<int> > back_edges;
	// Strings that caused given transition
	vector<vector<string> > transitions;

	int append_state(state s, vector<int> from, vector<string> chp = vector<string>());
	int append_state(state s, int from = -1, string chp = "");
	int insert_state(state s, int from);
	int duplicate_state(int from);
	void insert_edge(int from, int to, string chp);
	void get_trace(int from, int up, int down, trace *result);
	void get_trace(int from, vector<bool> *up, vector<bool> *down, trace *result);

	void set_trace(int uid, trace t);

	void gen_conflicts();
	void gen_traces();
	void gen_deltas();

	int size();
	int width();

	void merge();
	void trim();

	void print_states(vspace *v);
	void print_traces(vspace *v);
	void print_up(vspace *v);
	void print_down(vspace *v);
	void print_delta(vspace *v);
	void print_conflicts();
	void print_firings(vspace *v);
	void print_dot();
	void print_TS(vspace *v);
};

ostream &operator<<(ostream &os, graph g);
ostream &operator>>(ostream &os, graph g);

#endif
