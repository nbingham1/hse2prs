/*
 * dot.cpp
 *
 *  Created on: Feb 3, 2014
 *      Author: nbingham
 */

#include "dot.h"

dot_id::dot_id()
{
	start_token = 0;
	end_token = 0;
}

dot_id::dot_id(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	parse(tokens);
}

dot_id::dot_id(string id)
{
	this->start_token = 0;
	this->end_token = 0;
	this->id = id;
}

dot_id::~dot_id()
{

}

void dot_id::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	id = tokens.next();
	if (id != "" && (id[0] == '\"' || id[0] == '\''))
		id = id.substr(1, id.size()-2);

	end_token = tokens.index;
}

void dot_id::print(ostream &fout, string newl)
{
	fout << "\"" << id << "\"";
}

dot_a::dot_a()
{
	start_token = 0;
	end_token = 0;
}

dot_a::dot_a(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	parse(tokens);
}

dot_a::dot_a(string first, string second)
{
	this->start_token = 0;
	this->end_token = 0;
	this->first = first;
	this->second = second;
}

dot_a::~dot_a()
{

}

void dot_a::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	first.parse(tokens);

	tokens.increment();
	tokens.push_expected("=");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	second.parse(tokens);

	end_token = tokens.index;
}

void dot_a::print(ostream &fout, string newl)
{
	first.print(fout, newl);
	fout << "=";
	second.print(fout, newl);
}

dot_a_list::dot_a_list()
{
	start_token = 0;
	end_token = 0;
}

dot_a_list::dot_a_list(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	parse(tokens);
}

dot_a_list::~dot_a_list()
{
}

void dot_a_list::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	while (tokens.peek_next() != "]")
		as.push_back(dot_a(tokens));

	end_token = tokens.index;
}

void dot_a_list::print(ostream &fout, string newl)
{
	for (size_t i = 0; i < as.size(); i++)
	{
		if (i != 0)
			fout << " ";
		as[i].print(fout, newl);
	}
}

dot_attr_list::dot_attr_list()
{
	start_token = 0;
	end_token = 0;
}

dot_attr_list::dot_attr_list(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	parse(tokens);
}

dot_attr_list::~dot_attr_list()
{
}

bool dot_attr_list::is_next(tokenizer &tokens, int i)
{
	return (tokens.peek_next() == "[");
}

void dot_attr_list::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	while (tokens.peek_next() == "[")
	{
		tokens.increment();
		tokens.push_expected("[");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		tokens.increment();
		tokens.push_bound("]");
		attrs.push_back(dot_a_list(tokens));
		tokens.decrement();

		tokens.increment();
		tokens.push_expected("]");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}

	end_token = tokens.index;
}

void dot_attr_list::print(ostream &fout, string newl)
{
	for (size_t i = 0; i < attrs.size(); i++)
	{
		fout << "[";
		attrs[i].print(fout, newl);
		fout << "]";
	}
}

dot_node_id::dot_node_id()
{
	start_token = 0;
	end_token = 0;
}

dot_node_id::dot_node_id(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	parse(tokens);
}

dot_node_id::dot_node_id(string str)
{
	start_token = 0;
	end_token = 0;
	id = str;
}

dot_node_id::~dot_node_id()
{
}

bool dot_node_id::is_next(tokenizer &tokens, int i)
{
	string token = tokens.peek(i);
	if (token != "" && (token[0] == '\"' || token[0] == '\''))
		return true;

	for (size_t i = 0; i < token.size(); i++)
		if (!nc(token[i]))
			return false;

	return true;
}

void dot_node_id::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	id.parse(tokens);
	if (tokens.peek_next() == ":")
	{
		tokens.next();
		port.parse(tokens);
	}

	if (tokens.peek_next() == ":")
	{
		tokens.next();
		compass_pt.parse(tokens);
	}

	end_token = tokens.index;
}

void dot_node_id::print(ostream &fout, string newl)
{
	id.print(fout, newl);
	if (port.id != "")
	{
		fout << ":";
		port.print(fout, newl);
	}
	if (compass_pt.id != "")
	{
		fout << ":";
		compass_pt.print(fout, newl);
	}
}

dot_stmt::dot_stmt()
{
	start_token = 0;
	end_token = 0;
}

dot_stmt::dot_stmt(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	parse(tokens);
}

dot_stmt::~dot_stmt()
{

}

bool dot_stmt::is_next(tokenizer &tokens, int i)
{
	string token = tokens.peek(i);
	return (token == "graph" || token == "node" || token == "edge" || token == "subgraph" || dot_node_id::is_next(tokens, i));
}

void dot_stmt::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("graph");
	tokens.push_expected("edge");
	tokens.push_expected("node");
	tokens.push_expected("subgraph");
	tokens.push_expected("[dot_node_id]");
	string token = tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	if (token == "subgraph")
	{
		stmt_type = "subgraph";
		if (tokens.peek_next() != "{")
			id = tokens.next();

		tokens.increment();
		tokens.push_expected("{");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		tokens.increment();
		tokens.push_bound("}");
		stmt_list.parse(tokens);
		tokens.decrement();

		tokens.increment();
		tokens.push_expected("}");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}
	else if (token == "graph" || token == "edge" || token == "node")
	{
		stmt_type = "attr";
		attr_type = token;
	}
	else if (tokens.peek(2) == "->")
		stmt_type = "edge";
	else
		stmt_type = "node";

	if (stmt_type == "edge" || stmt_type == "node")
	{
		node_ids.push_back(dot_node_id(tokens));
		while (stmt_type == "edge" && tokens.peek_next() == "->")
		{
			tokens.increment();
			tokens.push_expected("->");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			tokens.increment();
			tokens.push_bound("->");
			tokens.push_bound("[attr_list]");
			tokens.push_expected("[dot_node_id]");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			if (dot_node_id::is_next(tokens))
				node_ids.push_back(dot_node_id(tokens));
		}
	}

	if ((stmt_type == "edge" || stmt_type == "node" || stmt_type == "attr") && dot_attr_list::is_next(tokens))
		attr_list.parse(tokens);

	end_token = tokens.index;
}

void dot_stmt::print(ostream &fout, string newl)
{
	if (stmt_type == "subgraph")
	{
		fout << "subgraph " << id << newl;
		fout << "{" << newl;
		if (stmt_list.stmts.size() > 0)
		{
			fout << "\t";
			stmt_list.print(fout, newl + "\t");
			fout << newl;
		}
		fout << "}" << newl;
	}
	else
	{
		for (size_t i = 0; i < node_ids.size(); i++)
		{
			if (i != 0)
				fout << "->";
			node_ids[i].print(fout, newl);
		}

		fout << attr_type;
		attr_list.print(fout, newl);
		fout << ";";
	}
}

dot_stmt_list::dot_stmt_list()
{
	start_token = 0;
	end_token = 0;
}

dot_stmt_list::~dot_stmt_list()
{

}

void dot_stmt_list::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	while (dot_stmt::is_next(tokens) && tokens.peek_next() != "")
	{
		tokens.increment();
		tokens.push_bound(";");
		stmts.push_back(dot_stmt(tokens));
		tokens.decrement();

		if (tokens.peek_next() == ";")
		{
			tokens.increment();
			tokens.push_expected(";");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();
		}
	}

	end_token = tokens.index;
}

void dot_stmt_list::print(ostream &fout, string newl)
{
	for (size_t i = 0; i < stmts.size(); i++)
	{
		if (i != 0)
			fout << newl;

		stmts[i].print(fout, newl);
	}
}

dot_graph::dot_graph()
{
	start_token = 0;
	end_token = 0;
	strict = false;
	type = "";
}

dot_graph::dot_graph(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	strict = false;
	type = "";
	parse(tokens);
}

dot_graph::~dot_graph()
{
}

bool dot_graph::is_next(tokenizer &tokens, int i)
{
	string token = tokens.peek(i);
	string token1 = tokens.peek(i+1);
	return ((token == "strict" && (token1 == "graph" || token1 == "digraph")) || token == "graph" || token == "digraph");
}

void dot_graph::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	if (tokens.peek_next() == "strict")
	{
		strict = true;
		tokens.next();
	}

	type = tokens.next();
	if (tokens.peek_next() != "{")
		id = tokens.next();

	tokens.increment();
	tokens.push_expected("{");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_bound("}");
	stmt_list.parse(tokens);
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("}");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	end_token = tokens.index;
}

void dot_graph::print(ostream &fout, string newl)
{
	if (strict)
		fout << "strict ";

	fout << type << " " << id << newl << "{" << newl;
	if (stmt_list.stmts.size() > 0)
	{
		fout << "\t";
		stmt_list.print(fout, newl + "\t");
		fout << newl;
	}
	fout << "}" << newl;
}

void dot_graph::clear()
{
	strict = false;
	type.clear();
	id.clear();
	stmt_list.stmts.clear();
}

dot_graph_cluster::dot_graph_cluster()
{
	start_token = 0;
	end_token = 0;
}

dot_graph_cluster::dot_graph_cluster(tokenizer &tokens)
{
	start_token = 0;
	end_token = 0;
	parse(tokens);
}

dot_graph_cluster::~dot_graph_cluster()
{

}

void dot_graph_cluster::parse(tokenizer &tokens)
{
	start_token = tokens.index+1;

	while (tokens.peek_next() != "")
	{
		while (!dot_graph::is_next(tokens) && tokens.peek_next() != "")
			tokens.next();

		if (tokens.peek_next() != "")
			graphs.push_back(dot_graph(tokens));
	}

	end_token = tokens.index;
}

void dot_graph_cluster::print(ostream &fout, string newl)
{
	for (size_t i = 0; i < graphs.size(); i++)
	{
		graphs[i].print(fout, newl);
		fout << newl;
	}
}

