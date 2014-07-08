/*
 * rule.h
 *
 * Rule is a data structure that contains firing information for a given variable. It contains
 * name of the variable, as well as the up and down firings for that variable. Program utilizes
 * a vec of rules to generate a full set of PRs.
 */

#include "common.h"
#include "petri.h"
#include "canonical.h"

#ifndef rule_h
#define rule_h

void merge_guards(canonical &guard0, canonical implicant0, canonical &guard1, canonical implicant1);

struct reduction_index : petri_index
{
	reduction_index();
	reduction_index(petri_index i, bool placeholder);
	~reduction_index();

	bool conflict;
	bool placeholder;

	reduction_index &operator=(petri_index i);
};

ostream &operator<<(ostream &os, reduction_index r);

struct reductionhdl
{
	reductionhdl();
	reductionhdl(const reductionhdl &r);
	reductionhdl(petri_net &net, petri_index start);
	~reductionhdl();

	canonical implicant;
	canonical guard;
	vector<bool> covered;
	int separator;
	vector<petri_index> begin;
	vector<reduction_index> end;

	void set_covered(petri_index i);
	bool is_covered(petri_index i);

	reductionhdl &operator=(reductionhdl r);
};

ostream &operator<<(ostream &os, reductionhdl r);

struct rule
{
	rule();
	rule(int uid);
	rule(variable_space &vars, string u, string d, string v);
	~rule();

	int uid;
	canonical guards[2];
	canonical explicit_guards[2];
	vector<petri_index> implicants[2];

	rule &operator=(rule r);

	canonical &up();
	canonical &down();

	bool separate(petri_net &net, reductionhdl &reduction, int t);
	void strengthen(petri_net &net, int t);

	bool is_combinational();
	void invert();

	void print(variable_space &vars, ostream &os = cout, string prefix = "");
};

#endif
