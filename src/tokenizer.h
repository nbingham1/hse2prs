/*
 * tokenizer.h
 *
 *  Created on: Apr 7, 2014
 *      Author: nbingham
 */

#include "common.h"

#ifndef tokenizer_h
#define tokenizer_h

struct token
{
	token();
	token(size_t file, size_t line, size_t col, size_t size);
	~token();

	size_t file;
	size_t line;
	size_t col;
	size_t size;
};

struct tokenizer
{
	tokenizer();
	tokenizer(string filename, string contents);
	tokenizer(string contents);
	~tokenizer();

	vector<vector<string> > compositions;
	vector<vector<string> > operations;
	vector<pair<string, vector<string> > > files;
	vector<token> tokens;
	string curr;
	size_t index;

	vector<vector<string> > bound;
	vector<vector<string> > expected;

	void increment();
	void decrement();
	void push_bound(string s);
	void push_bound(vector<string> s);
	void push_expected(string s);
	void push_expected(vector<string> s);
	vector<vector<string> >::iterator in_bound(size_t i);
	vector<vector<string> >::iterator in_expected(size_t i);
	vector<vector<string> >::iterator in_bound(string s);
	vector<vector<string> >::iterator in_expected(string s);
	string bound_string();
	string expect_string();
	int count_expected();

	string syntax(string debug_file, int debug_line);

	void insert(string filename, string contents);

	string at(size_t i);
	string line(size_t i);
	string file(size_t i);
	string prev();
	string next();
	string peek(size_t i);
	string peek_next();
	string peek_prev();

	tokenizer &operator=(string contents);
};

#endif
