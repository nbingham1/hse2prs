/*
 * canonical.h
 *
 *  Created on: May 11, 2013
 *      Author: nbingham
 */

#include "common.h"
#include "minterm.h"
#include "tokenizer.h"

#ifndef canonical_h
#define canonical_h

struct variable_space;

/**
 * This structure stores the canonical form of a binary boolean expression (a sum of minterms).
 */
struct canonical
{
	canonical();
	canonical(const canonical &c);
	canonical(int s);
	canonical(minterm m);
	canonical(vector<minterm> m);
	canonical(tokenizer &tokens, string s, variable_space &vars);
	canonical(int var, uint32_t val);
	canonical(map<int, uint32_t> vals);
	~canonical();

	vector<minterm> terms;

	vector<minterm>::iterator begin();
	vector<minterm>::iterator end();

	// INTERNAL FUNCTIONS
	int size();
	void assign(size_t i, minterm t);
	void remove(size_t i);

	void push_back(minterm m);
	void push_up(minterm m);
	void clear();

	void mccluskey();
	void mccluskey_or(size_t  separator);
	void mccluskey_and();
	minterm mask();

	// EXTERNAL FUNCTIONS
	vector<size_t> vars();
	void vars(vector<size_t> *var_list);

	canonical refactor(vector<size_t> ids);
	canonical refactor(vector<pair<size_t, size_t> > ids);

	canonical hide(size_t var);
	canonical hide(vector<size_t> vars);
	canonical restrict(canonical r);
	void extract(map<int, canonical> *result);
	map<int, canonical> extract();
	uint32_t val(size_t uid);

	canonical pabs();
	canonical nabs();

	int satcount();
	map<int, uint32_t> anysat();
	vector<map<int, uint32_t> > allsat();

	canonical &operator=(canonical c);
	canonical &operator=(minterm t);
	canonical &operator=(uint32_t c);

	canonical &operator|=(canonical c);
	canonical &operator&=(canonical c);
	canonical &operator^=(canonical c);

	canonical &operator|=(uint32_t c);
	canonical &operator&=(uint32_t c);

	canonical operator()(int i, uint32_t v);
	canonical operator[](int i);

	canonical operator|(canonical c);
	canonical operator&(canonical c);
	canonical operator~();
	canonical operator^(canonical c);
	canonical operator&&(canonical c);

	canonical operator|(uint32_t c);
	canonical operator&(uint32_t c);

	bool operator==(const canonical c) const;
	bool operator!=(const canonical c) const;

	bool operator==(minterm c);
	bool operator!=(minterm c);

	bool operator==(uint32_t c);
	bool operator!=(uint32_t c);

	bool constant();


	canonical operator>>(canonical t);

	string print(variable_space &v, string prefix = "");
	string print_assign(variable_space &v, string prefix = "");
	string print_with_quotes(variable_space &v, string prefix = "");
};

bool operator<(canonical c0, canonical c1);

bool is_mutex(canonical *c0, canonical *c1);
bool is_mutex(canonical c0, canonical c1);
bool is_mutex(minterm *m0, canonical *c1);
bool is_mutex(canonical *c0, minterm *m1);
bool is_mutex(canonical *c0, canonical *c1, canonical *c2);
bool is_mutex(canonical *c0, canonical *c1, canonical *c2, canonical *c3);
bool mergible(canonical *c0, canonical *c1);
bool mergible(minterm c0, minterm c1);
canonical merge(canonical c0, canonical c1);

#endif
