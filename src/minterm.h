/*
 * minterm.h
 *
 *  Created on: May 11, 2013
 *      Author: nbingham
 */

#include "common.h"
#include "tokenizer.h"

#ifndef minterm_h
#define minterm_h

struct canonical;
struct variable_space;

#define v_ 0x00000000
#define v0 0x55555555
#define v1 0xAAAAAAAA
#define vX 0xFFFFFFFF

uint32_t itom(int v);
int mtoi(uint32_t v);
uint32_t vidx(int v);
uint32_t vmsk(int v);

struct minterm
{
	minterm();
	minterm(const minterm &m);
	minterm(string str);
	~minterm();

	vector<uint32_t> values;
	uint32_t default_value;
	size_t size;


	// INTERNAL FUNCTIONS
	uint32_t get(size_t uid);
	uint32_t val(size_t uid);
	void set(size_t uid, uint32_t v);
	void resize(size_t s, uint32_t r = 0xFFFFFFFF);
	void clear();

	void sv_union(size_t uid, uint32_t v);
	void sv_intersect(size_t uid, uint32_t v);
	void sv_invert(size_t uid);
	void sv_or(size_t uid, uint32_t v);
	void sv_and(size_t uid, uint32_t v);
	void sv_not(size_t uid);

	bool subset(minterm s);
	bool conflict(minterm s);

	int diff_count(minterm s);
	pair<int, int> xdiff_count(minterm s);

	minterm xoutnulls();

	minterm mask();
	minterm inverse();

	void push_back(uint32_t v);

	vector<minterm> expand(vector<int> uids);

	// EXTERNAL FUNCTIONS
	minterm(uint32_t val);
	minterm(int var, uint32_t val);
	minterm(map<int, uint32_t> vals);
	minterm(tokenizer &tokens, string exp, variable_space &vars);

	vector<size_t> vars();
	void vars(vector<size_t> *var_list);

	minterm refactor(vector<size_t> ids);
	minterm refactor(vector<pair<size_t, size_t> > ids);

	minterm hide(size_t var);
	minterm hide(vector<size_t> vars);
	void extract(map<int, minterm> *result);
	map<int, minterm> extract();

	minterm pabs();
	minterm nabs();

	int satcount();
	map<int, uint32_t> anysat();
	vector<map<int, uint32_t> > allsat();

	minterm &operator=(minterm s);
	minterm &operator=(uint32_t s);

	minterm &operator&=(minterm s);
	minterm &operator|=(minterm s);

	minterm &operator&=(uint32_t s);
	minterm &operator|=(uint32_t s);

	minterm operator()(int i, uint32_t v);
	minterm operator[](int i);

	minterm operator&(minterm s);
	minterm operator|(minterm s);
	canonical operator~();

	minterm operator|(uint32_t s);
	minterm operator&(uint32_t s);

	bool constant();

	minterm operator>>(minterm t);

	string print(variable_space &vars, string prefix = "");
	string print_assign(variable_space &vars, string prefix = "");
	string print_with_quotes(variable_space &vars, string prefix = "");
};

bool operator==(minterm s1, minterm s2);
bool operator!=(minterm s1, minterm s2);

bool operator==(minterm s1, uint32_t s2);
bool operator!=(minterm s1, uint32_t s2);

bool operator<(minterm s1, minterm s2);
bool operator>(minterm s1, minterm s2);
bool operator<=(minterm s1, minterm s2);
bool operator>=(minterm s1, minterm s2);

#endif
