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

void process::elaborate()
{
	program_execution_space elaborater;
	elaborater.execs.push_back(program_execution());

	for (list<petri_net>::iterator net = nets.begin(); net != nets.end(); net++)
	{
		if (net->remote)
			elaborater.execs.back().init_rpcs(net->label, &*net);
		else
			elaborater.execs.back().init_pcs(net->label, &*net, net->elaborate);
	}

	elaborater.full_elaborate();

	if (get_verbose())
		export_dot().print();
}

void process::check()
{
	elaborate();

	for (list<petri_net>::iterator net = nets.begin(); net != nets.end(); net++)
	{
		if (net->elaborate && !net->remote)
		{
			net->generate_observed();
			net->generate_conflicts();
			/*cout << net->second.conflicts << endl;
			cout << net->second.indistinguishable << endl;
			net->generate_conflicts(&net->second);*/
		}
	}

	print_conflicts();
}

void process::solve()
{
	bool done = false;
	do
	{
		check();

		done = true;
		for (list<petri_net>::iterator net = nets.begin(); net != nets.end(); net++)
			if (net->elaborate && !net->remote)
				done = done && !net->solve_conflicts();
	} while (!done && is_clean());
}

void process::print_conflicts()
{
	log("", "Model", __FILE__, __LINE__);
	log("", "{", __FILE__, __LINE__);
	for (list<petri_net>::iterator net = nets.begin(); net != nets.end(); net++)
	{
		if (net->elaborate && !net->remote)
		{
			log("", "\tconflicts: " + net->name, __FILE__, __LINE__);
			for (map<petri_index, list<vector<petri_index> > >::iterator i = net->conflicts.begin(); i != net->conflicts.end(); i++)
				log("", "\t" + to_string(i->first) + ": " + to_string(i->second), __FILE__, __LINE__);

			log("", "\tindistinguishables: " + net->name, __FILE__, __LINE__);
			for (map<petri_index, list<vector<petri_index> > >::iterator i = net->indistinguishable.begin(); i != net->indistinguishable.end(); i++)
				log("", "\t" + to_string(i->first) + ": " + to_string(i->second), __FILE__, __LINE__);
		}
	}
	log("", "}", __FILE__, __LINE__);
}

void process::import_dot(tokenizer &tokens, dot_graph &graph)
{
	int t_base = 0, s_base = 0;
	for (size_t j = 0; j < graph.stmt_list.stmts.size(); j++)
	{
		if (graph.stmt_list.stmts[j].stmt_type == "subgraph")
		{
			nets.push_back(petri_net());
			nets.back().import_dot(tokens, graph.stmt_list.stmts[j], t_base, s_base);
			t_base += nets.back().T.size();
			s_base += nets.back().S.size();
			nets.back().compact();
		}
	}
}

dot_graph process::export_dot()
{
	dot_graph result;
	result.id = "model";
	result.type = "digraph";
	size_t t_base = 0, s_base = 0;
	for (list<petri_net>::iterator net = nets.begin(); net != nets.end(); net++)
	{
		result.stmt_list.stmts.push_back(net->export_dot(t_base, s_base));
		t_base += net->T.size();
		s_base += net->S.size();
	}
	return result;
}
