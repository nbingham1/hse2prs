/*
 * process.cpp
 *
 *  Created on: Apr 30, 2014
 *      Author: nbingham
 */

#include "process.h"
#include "program_counter.h"
#include "message.h"

process::process()
{

}

process::process(tokenizer &tokens, dot_graph &graph)
{
	import_dot(tokens, graph);
}

process::~process()
{
	nets.clear();
}

void process::generate_translations()
{
	for (list<pair<petri_net, rule_space> >::iterator i = nets.begin(); i != nets.end(); i++)
		for (list<pair<petri_net, rule_space> >::iterator j = nets.begin(); j != nets.end(); j++)
			if (i != j)
			{
				vector<pair<size_t, size_t> > factors;
				for (size_t vi = 0; vi < i->first.vars.globals.size(); vi++)
				{
					string test = i->first.vars.globals[vi].name;
					if (test.substr(0, 5) == "this.")
						test = test.substr(5);

					size_t uid = j->first.vars.find(test);
					if (uid < j->first.vars.globals.size())
						factors.push_back(pair<size_t, size_t>(vi, uid));
				}

				translations.insert(pair<pair<petri_net*, petri_net*>, vector<pair<size_t, size_t> > >(pair<petri_net*, petri_net*>(&i->first, &j->first), factors));
			}
}

void process::elaborate()
{
	string start_statement = "elaborating {";

	// Clear all of the known state space information
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
	{
		if (net != nets.begin())
			start_statement += ", ";

		start_statement += net->first.name;

		net->first.parallel_nodes.clear();

		for (size_t i = 0; i < net->first.S.size(); i++)
		{
			net->first.S[i].predicate = canonical(0);
			net->first.S[i].effective = canonical(0);
		}
	}

	start_statement += "}";

	log("", start_statement, __FILE__, __LINE__);

	program_execution_space elaborater;
	elaborater.execs.push_back(program_execution());

	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
	{
		if (net->first.remote)
			elaborater.execs.back().init_rpcs(&net->first);
		else
			elaborater.execs.back().init_pcs(&net->first, net->first.elaborate);
	}

	elaborater.full_elaborate(this);

	if (get_verbose())
		export_dot().print();
}

void process::check()
{
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
		if (net->first.elaborate && !net->first.remote)
			net->first.generate_conflicts();
}

void process::solve()
{
	bool done = false;
	while (!done && is_clean())
	{
		done = true;
		for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
			if (net->first.elaborate && !net->first.remote)
				done = done && !net->first.solve_conflicts();

		if (!done && is_clean())
		{
			elaborate();
			check();
		}
	}
}

void process::generate_prs()
{
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
		if (net->first.elaborate && !net->first.remote)
		{
			net->second.generate_minterms(net->first);
			net->second.check(net->first);
		}
}

void process::generate_bubble()
{
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
		if (net->first.elaborate && !net->first.remote)
			net->second.generate_bubble(net->first.vars);
}

void process::bubble_reshuffle()
{
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
		if (net->first.elaborate && !net->first.remote)
			net->second.bubble_reshuffle(net->first.vars, net->first.name);
}

dot_graph_cluster process::export_bubble()
{
	dot_graph_cluster result;
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
		if (net->first.elaborate && !net->first.remote)
			result.graphs.push_back(net->second.export_bubble(net->first.vars, net->first.name));
	return result;
}

void process::export_prs(ostream &os)
{
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
	{
		if (net->first.elaborate && !net->first.remote)
		{
			os << "// Production Rules: " << net->first.name << endl;
			net->second.print(net->first.vars, os, "");
			os << endl;
		}
	}
}

minterm process::translate(minterm term, petri_net *from, petri_net *to)
{
	if (from != to)
		return term.refactor(translations[pair<petri_net*, petri_net*>(from, to)]);
	else
		return term;
}

void process::print_conflicts()
{
	cout << "Model" << endl << "{" << endl;
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
	{
		if (net->first.elaborate && !net->first.remote)
		{
			cout << "\tconflicts: " << net->first.name << endl;
			for (map<petri_index, list<vector<petri_index> > >::iterator i = net->first.conflicts.begin(); i != net->first.conflicts.end(); i++)
				cout << "\t" << i->first << ": " << i->second << endl;

			cout << "\tindistinguishables: " << net->first.name << endl;
			for (map<petri_index, list<vector<petri_index> > >::iterator i = net->first.indistinguishable.begin(); i != net->first.indistinguishable.end(); i++)
				cout << "\t" << i->first << ": " << i->second << endl;
		}
	}
	cout << "}" << endl;
}

void process::import_dot(tokenizer &tokens, dot_graph &graph)
{
	int t_base = 0, s_base = 0;
	for (size_t j = 0; j < graph.stmt_list.stmts.size(); j++)
	{
		if (graph.stmt_list.stmts[j].stmt_type == "subgraph")
		{
			nets.push_back(pair<petri_net, rule_space>());
			nets.back().first.import_dot(tokens, graph.stmt_list.stmts[j], t_base, s_base);
			t_base += nets.back().first.T.size();
			s_base += nets.back().first.S.size();
			nets.back().first.compact();
		}
	}
}

dot_graph process::export_dot()
{
	dot_graph result;
	result.id = "model";
	result.type = "digraph";
	size_t t_base = 0, s_base = 0;
	for (list<pair<petri_net, rule_space> >::iterator net = nets.begin(); net != nets.end(); net++)
	{
		result.stmt_list.stmts.push_back(net->first.export_dot(t_base, s_base));
		t_base += net->first.T.size();
		s_base += net->first.S.size();
	}
	return result;
}
