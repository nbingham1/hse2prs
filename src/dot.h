/*
 * dot.h
 *
 *  Created on: Feb 3, 2014
 *      Author: nbingham
 */

#include "common.h"
#include "tokenizer.h"

#ifndef dot_h
#define dot_h

struct dot_id
{
	dot_id();
	dot_id(tokenizer &tokens);
	dot_id(string id);
	~dot_id();

	size_t start_token, end_token;
	string id;

	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

struct dot_a
{
	dot_a();
	dot_a(tokenizer &tokens);
	dot_a(string first, string second);
	~dot_a();

	size_t start_token, end_token;
	dot_id first;
	dot_id second;

	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

struct dot_a_list
{
	dot_a_list();
	dot_a_list(tokenizer &tokens);
	~dot_a_list();

	size_t start_token, end_token;
	vector<dot_a> as;

	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

struct dot_attr_list
{
	dot_attr_list();
	dot_attr_list(tokenizer &tokens);
	~dot_attr_list();

	size_t start_token, end_token;
	vector<dot_a_list> attrs;

	static bool is_next(tokenizer &tokens, int i = 1);
	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

struct dot_node_id
{
	dot_node_id();
	dot_node_id(tokenizer &tokens);
	dot_node_id(string str);
	~dot_node_id();

	size_t start_token, end_token;
	dot_id id;
	dot_id port;
	dot_id compass_pt;

	static bool is_next(tokenizer &tokens, int i = 1);
	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

struct dot_stmt;

struct dot_stmt_list
{
	dot_stmt_list();
	dot_stmt_list(tokenizer &tokens);
	~dot_stmt_list();

	size_t start_token, end_token;
	vector<dot_stmt> stmts;

	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

struct dot_stmt
{
	dot_stmt();
	dot_stmt(tokenizer &tokens);
	~dot_stmt();

	size_t start_token, end_token;
	string stmt_type;
	string attr_type;
	vector<dot_node_id> node_ids;
	dot_attr_list attr_list;
	string id;
	dot_stmt_list stmt_list;

	static bool is_next(tokenizer &tokens, int i = 1);
	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

struct dot_graph
{
	dot_graph();
	dot_graph(tokenizer &tokens);
	~dot_graph();

	size_t start_token, end_token;
	bool strict;
	string type;
	string id;
	dot_stmt_list stmt_list;

	static bool is_next(tokenizer &tokens, int i = 1);
	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");

	void clear();
};

struct dot_graph_cluster
{
	dot_graph_cluster();
	dot_graph_cluster(tokenizer &tokens);
	~dot_graph_cluster();

	size_t start_token, end_token;
	vector<dot_graph> graphs;

	void parse(tokenizer &tokens);
	void print(ostream &fout = cout, string newl = "\n");
};

#endif
