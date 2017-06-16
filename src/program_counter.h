/*
 * program_counter.h
 *
 *  Created on: Sep 16, 2013
 *      Author: nbingham
 */

#ifndef program_counter_h
#define program_counter_h

#include "common.h"
#include "canonical.h"
#include "petri.h"

struct remote_program_counter;

/**
 * This elaborator assumes nothing. It clears all of the previously known
 * state encodings before starting and then systematically explores the
 * entire state space in order to elaborate it and suffers horribly
 * from state space explosion. Some actions have been taken to try
 * and account for this. For example, the algorithm does a depth first
 * search keeping the total storage needed to a minimum. It does a form
 * of dynamic programing, ignoring cases that it has seen before. It
 * schedules case in which the environment can move to be considered first
 * because cases where the main program moves first generally
 * only produce a strict subset of the state encodings produced by the cases
 * where the environment moves first.
 */
struct program_counter
{
	program_counter();
	program_counter(string name, bool elaborate, petri_index index, petri_net *net);
	~program_counter();

	string name;
	petri_net *net;
	petri_index index;
	minterm state;
	bool elaborate;
	vector<petri_index> n, p;

	bool is_active();
	bool is_active(petri_index i);
	bool is_satisfied();
	bool is_satisfied(petri_index i);
	bool next_has_active_or_satisfied();
	canonical &predicate();

	void apply(minterm term);
	void set(minterm term);
};

bool operator==(program_counter p1, program_counter p2);
bool operator<(program_counter p1, program_counter p2);
ostream &operator<<(ostream &os, program_counter p);

struct remote_petri_index : petri_index
{
	remote_petri_index();
	remote_petri_index(int idx, int iter, bool place);
	remote_petri_index(petri_index i, int iter);
	~remote_petri_index();

	int iteration;
};

bool operator==(remote_petri_index i1, remote_petri_index i2);
ostream &operator<<(ostream &os, remote_petri_index i);

typedef pair<remote_petri_index, remote_petri_index> remote_petri_arc;

struct remote_program_counter
{
	remote_program_counter();
	remote_program_counter(string name, petri_net *net);
	~remote_program_counter();

	string name;
	petri_net* net;
	vector<remote_petri_index> begin;
	vector<remote_petri_index> end;
	vector<remote_petri_arc> arcs;
	vector<int> place_iteration;
	vector<pair<int, int> > trans_iteration;
	vector<int> input_size;

	vector<remote_petri_arc> input_arcs(remote_petri_index n);
	vector<remote_petri_arc> output_arcs(remote_petri_index n);
	vector<remote_petri_index> input_nodes(remote_petri_index n);
	vector<remote_petri_index> output_nodes(remote_petri_index n);

	bool is_active(petri_index i);
	bool is_satisfied(petri_index i, minterm state);
	bool is_vacuous(petri_index i, minterm state);
	bool next_has_active_or_satisfied(remote_petri_index i, minterm state, vector<petri_index> &outgoing);
	bool is_one(petri_index i);

	int nid(petri_index idx);

	void roll_to(remote_petri_index idx);

	int count(size_t n);
	void merge(size_t n);

	minterm firings();
	canonical waits(remote_petri_index n);

	remote_program_counter &operator=(remote_program_counter pc);
};

ostream &operator<<(ostream &os, remote_program_counter p);

struct program_execution
{
	program_execution();
	program_execution(const program_execution &exec);
	~program_execution();

	vector<program_counter> pcs;
	vector<remote_program_counter> rpcs;
	vector<pair<petri_index, vector<size_t> > > ready_places;
	bool deadlock;
	bool done;

	int count(size_t pci);
	int merge(size_t pci);

	void init_pcs(string name, petri_net *net, bool elaborate);
	void init_rpcs(string name, petri_net *net);

	program_execution &operator=(program_execution e);
};

struct program_execution_space
{
	list<program_execution> execs;
	vector<petri_net*> nets;
	map<pair<pair<string, petri_net*>, pair<string, petri_net*> >, vector<pair<size_t, size_t> > > translations;
	vector<string> instabilities;

	void duplicate_execution(program_execution *exec_in, program_execution **exec_out);
	void duplicate_counter(program_execution *exec_in, size_t pc_in, size_t &pc_out);
	bool remote_end_ready(program_execution *exec, size_t rpc, size_t &idx, vector<petri_index> &outgoing, minterm state);
	bool remote_begin_ready(program_execution *exec, size_t rpc, size_t &idx, minterm state);
	void full_elaborate();
	void reset();
	void gen_translation(string name0, petri_net *net0, string name1, petri_net *net1);
	minterm translate(string name0, petri_net *net0, minterm t, string name1, petri_net *net1);
};

struct program_index
{
	program_index();
	program_index(string name, petri_net *net, petri_index index, minterm encoding);
	program_index(const program_index &i);
	~program_index();

	string name;
	petri_net *net;
	petri_index index;
	minterm encoding;

	program_index &operator=(program_index i);
	program_index &operator=(petri_index i);
};

bool operator==(program_index i1, program_index i2);
bool operator!=(program_index i1, program_index i2);
bool operator<(program_index i1, program_index i2);

struct program_state
{
	program_state();
	program_state(program_execution *exec);
	program_state(const program_state &s);
	~program_state();

	vector<program_index> state;

	program_state &operator=(program_execution *exec);
	program_state &operator=(program_state s);
};

ostream &operator<<(ostream &os, program_state s);
bool operator==(program_state s1, program_state s2);
bool operator!=(program_state s1, program_state s2);
bool operator<(program_state s1, program_state s2);

#endif
