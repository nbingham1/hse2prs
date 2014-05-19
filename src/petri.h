/*
 * petri.h
 *
 *  Created on: May 9, 2013
 *      Author: nbingham
 */

#include "common.h"
#include "canonical.h"
#include "variable_space.h"
#include "dot.h"
#include "path_space.h"
#include "tokenizer.h"

#ifndef petri_h
#define petri_h

struct petri_net;

struct petri_index
{
	petri_index();
	petri_index(int idx, bool place);
	petri_index(string str);
	~petri_index();

	int data;

	bool is_place() const;
	bool is_trans() const;
	string name() const;
	int idx() const;

	//operator int() const;

	petri_index &operator=(petri_index i);
	petri_index &operator--();
	petri_index &operator++();
	petri_index &operator--(int);
	petri_index &operator++(int);
};

bool operator==(petri_index i, petri_index j);
bool operator!=(petri_index i, petri_index j);
bool operator<(petri_index i, int j);
ostream &operator<<(ostream &os, petri_index i);
bool operator>(petri_index i, petri_index j);
bool operator<(petri_index i, petri_index j);
bool operator>=(petri_index i, petri_index j);
bool operator<=(petri_index i, petri_index j);
petri_index operator+(petri_index i, int j);
petri_index operator-(petri_index i, int j);

typedef pair<petri_index, petri_index> petri_arc;

ostream &operator<<(ostream &os, petri_arc a);

struct petri_node
{
	petri_node();
	petri_node(canonical predicate, bool active);
	~petri_node();

	canonical predicate;
	map<vector<petri_index>, canonical> observed;
	bool active;

	canonical assumptions;
	vector<canonical> assertions;
	vector<petri_index> refs;

	pair<int, int> sense_count();
};

struct petri_net
{
	petri_net();
	~petri_net();

	string name;
	string label;
	bool remote;
	bool elaborate;
	vector<petri_node> S;
	vector<petri_node> T;
	vector<petri_index> M0;
	vector<petri_arc> arcs;

	list<pair<petri_index, petri_index> > parallel_nodes;

	map<petri_index, list<vector<petri_index> > > conflicts;
	map<petri_index, list<vector<petri_index> > > indistinguishable;
	int max_indistinguishables;

	vector<canonical> assertions;
	canonical assumptions;
	canonical reset;

	variable_space vars;

	// Functions for adding, removing, and connecting nodes
	petri_index put_place(canonical root);
	petri_index put_transition(canonical root, bool active);
	vector<petri_index> put_places(vector<canonical> root);
	vector<petri_index> put_transitions(vector<canonical> root, bool active);
	void cut(petri_index node);
	void cut(vector<petri_index> nodes);
	vector<petri_index> connect(vector<petri_index> from, vector<petri_index> to);
	petri_index connect(vector<petri_index> from, petri_index to);
	vector<petri_index> connect(petri_index from, vector<petri_index> to);
	petri_index connect(petri_index from, petri_index to);
	petri_index push_transition(petri_index from, canonical root, bool active);
	petri_index push_transition(vector<petri_index> from, canonical root, bool active);
	petri_index push_transition(petri_index from);
	petri_index push_transition(vector<petri_index> from);
	vector<petri_index> push_transitions(petri_index from, vector<canonical> root, bool active);
	vector<petri_index> push_transitions(vector<petri_index> from, vector<canonical> root, bool active);
	petri_index push_place(petri_index from);
	petri_index push_place(vector<petri_index> from);
	vector<petri_index> push_places(vector<petri_index> from);
	void pinch_forward(petri_index n);
	void pinch_backward(petri_index n);
	void insert(int a, canonical root, bool active);
	void insert_alongside(petri_index from, petri_index to, canonical root, bool active);
	void insert_before(petri_index i, canonical root, bool active);
	void insert_after(petri_index i, canonical root, bool active);
	petri_index duplicate(petri_index n);
	vector<petri_index> duplicate(vector<petri_index> n);
	petri_index merge(petri_index n0, petri_index n1);
	petri_index merge(vector<petri_index> n);

	// Functions for traversing the network
	vector<petri_index> next(petri_index n);
	vector<petri_index> next(vector<petri_index> n);
	vector<petri_index> prev(petri_index n);
	vector<petri_index> prev(vector<petri_index> n);
	vector<int> outgoing(petri_index n);
	vector<int> outgoing(vector<petri_index> n);
	vector<int> incoming(petri_index n);
	vector<int> incoming(vector<petri_index> n);
	vector<int> next_arc(int n);
	vector<int> next_arc(vector<int> n);
	vector<int> prev_arc(int n);
	vector<int> prev_arc(vector<int> n);

	// Connectivity and sibling checks
	bool is_floating(petri_index n);
	bool are_connected(petri_index n0, petri_index n1);
	bool have_same_source(petri_index n0, petri_index n1);
	bool have_same_dest(petri_index n0, petri_index n1);
	bool are_parallel_siblings(petri_index p0, petri_index p1);

	// Accessor functions
	petri_node &operator[](petri_index i);
	petri_node &at(petri_index i);

	// Functions for handling paths
	pair<int, int> closest_input(vector<int> from, vector<int> to);
	pair<int, int> closest_output(vector<int> from, vector<int> to);
	void get_paths(vector<int> from, vector<int> to, path_space *result);
	vector<int> start_path(vector<int> to, vector<int> from);
	void zero_paths(path_space *paths, petri_index from);
	void zero_paths(path_space *paths, vector<petri_index> from);
	void zero_ins(path_space *paths, petri_index from);
	void zero_ins(path_space *paths, vector<petri_index> from);
	void zero_outs(path_space *paths, petri_index from);
	void zero_outs(path_space *paths, vector<petri_index> from);
	vector<petri_index> end_path(petri_index to, vector<petri_index> ex);
	vector<petri_index> end_path(vector<petri_index> to, vector<petri_index> ex);

	void check_assertions();

	pair<int, int> get_input_sense_count(petri_index idx);
	pair<int, int> get_input_sense_count(vector<petri_index> idx);
	petri_index get_split_place(petri_index merge_place, vector<bool> *covered);
	void remove_invalid_split_points(pair<int, int> up_sense_count, vector<petri_index> up_start, path_space *up_paths, pair<int, int> down_sense_count, vector<petri_index> down_start, path_space *down_paths);
	vector<int> choose_split_points(path_space *paths);

	void add_conflict_pair(map<petri_index, list<vector<petri_index> > > *c, petri_index i, petri_index j);
	void generate_paths(pair<int, int> *up_sense_count, vector<petri_index> up_start, path_space *up_paths, pair<int, int> *down_sense_count, vector<petri_index> down_start,  path_space *down_paths);
	void generate_conflicts();
	bool solve_conflicts();

	canonical base(vector<int> idx);

	bool are_sibling_guards(petri_index i, petri_index j);
	canonical apply_debug(int pc);

	/**
	 * \brief	Removes vacuous pbranches, unreachable places, and dangling, vacuous, and impossible transitions.
	 * \sa		merge_conflicts() and zip()
	 */
	void compact();
	void generate_observed();

	canonical get_effective_place_encoding(petri_index place, vector<petri_index> observer);
	canonical get_effective_state_encoding(vector<petri_index> state, vector<petri_index> observer, vector<petri_index> path);

	vector<petri_index> get_cut(vector<petri_index> base, bool backward, bool conditional);
	vector<int> get_arc_cut(vector<int> base, bool backward, bool conditional);

	dot_stmt export_dot(int t_base, int s_base);
	void import_dot(tokenizer &tokens, const dot_stmt &g, int t_base, int s_base);
};

#endif
