/*
 * program_counter.cpp
 *
 *  Created on: Sep 16, 2013
 *      Author: nbingham
 */

#include "program_counter.h"
#include "petri.h"
#include "variable_space.h"
#include "message.h"

program_counter::program_counter()
{
	net = NULL;
	elaborate = false;
}

program_counter::program_counter(string name, bool elaborate, petri_index index, petri_net *net)
{
	this->name = name;
	this->net = net;
	this->index = index;
	this->n = net->next(index);
	this->p = net->prev(index);
	if (net->reset.terms.size() > 0)
		this->state = net->reset.terms.front();
	this->elaborate = elaborate;
}

program_counter::~program_counter()
{

}

bool program_counter::is_active()
{
	return net->at(index).active;
}

bool program_counter::is_active(petri_index i)
{
	return net->at(i).active;
}

bool program_counter::is_satisfied()
{
	for (vector<minterm>::iterator term = net->at(index).predicate.terms.begin(); term != net->at(index).predicate.terms.end(); term++)
		if ((*term & state) != 0)
			return true;

	return false;
}

bool program_counter::is_satisfied(petri_index i)
{
	for (vector<minterm>::iterator term = net->at(i).predicate.terms.begin(); term != net->at(i).predicate.terms.end(); term++)
		if ((*term & state) != 0)
			return true;

	return false;
}

bool program_counter::next_has_active_or_satisfied()
{
	for (size_t i = 0; i < net->arcs.size(); i++)
		if (net->arcs[i].first == index && (net->at(net->arcs[i].second).active || is_satisfied(net->arcs[i].second)))
			return true;

	return false;
}

canonical &program_counter::predicate()
{
	return net->at(index).predicate;
}

void program_counter::apply(minterm term)
{
	state |= (state & term).xoutnulls();
}

void program_counter::set(minterm term)
{
	state = (state & term).xoutnulls();
}

bool operator==(program_counter p1, program_counter p2)
{
	return p1.name == p2.name && p1.net == p2.net && p1.index == p2.index;
}

bool operator<(program_counter p1, program_counter p2)
{
	return ((p1.name < p2.name) ||
			(p1.name == p2.name && p1.net < p2.net) ||
			(p1.name == p2.name && p1.net == p2.net && p1.index < p2.index) ||
			(p1.name == p2.name && p1.net == p2.net && p1.index == p2.index && p1.state < p2.state));
}

ostream &operator<<(ostream &os, program_counter p)
{
	os << p.net->name << ":" << p.name << ":" << p.index;
	return os;
}

remote_petri_index::remote_petri_index()
{

}

remote_petri_index::remote_petri_index(int idx, int iter, bool place) : petri_index(idx, place)
{
	iteration = iter;
}

remote_petri_index::remote_petri_index(petri_index i, int iter) : petri_index(i)
{
	iteration = iter;
}

remote_petri_index::~remote_petri_index()
{

}

bool operator==(remote_petri_index i1, remote_petri_index i2)
{
	return (i1.data == i2.data && i1.iteration == i2.iteration);
}

ostream &operator<<(ostream &os, remote_petri_index i)
{
	os << i.name() << "." << i.iteration;
	return os;
}

remote_program_counter::remote_program_counter()
{
	net = NULL;
}

remote_program_counter::remote_program_counter(string name, petri_net *net)
{
	this->place_iteration.resize(net->S.size(), 0);
	this->trans_iteration.resize(net->T.size(), pair<int, int>(0, 0));
	this->input_size.resize(net->T.size(), 0);
	for (petri_index i(0, false); i < net->T.size(); i++)
		this->input_size[i.idx()] = net->incoming(i).size();

	this->name = name;
	this->net = net;
	for (size_t i = 0; i < net->M0.size(); i++)
	{
		int iteration = nid(net->M0[i]);
		this->begin.push_back(remote_petri_index(net->M0[i], iteration));
		this->end.push_back(remote_petri_index(net->M0[i], iteration));
	}
}

remote_program_counter::~remote_program_counter()
{

}

vector<remote_petri_arc> remote_program_counter::input_arcs(remote_petri_index n)
{
	vector<remote_petri_arc> result;
	for (vector<remote_petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		if (arc->second == n)
			result.push_back(*arc);
	return result;
}

vector<remote_petri_arc> remote_program_counter::output_arcs(remote_petri_index n)
{
	vector<remote_petri_arc> result;
	for (vector<remote_petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		if (arc->first == n)
			result.push_back(*arc);
	return result;
}

vector<remote_petri_index> remote_program_counter::input_nodes(remote_petri_index n)
{
	vector<remote_petri_index> result;
	for (vector<remote_petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		if (arc->second == n)
			result.push_back(arc->first);
	return result;
}

vector<remote_petri_index> remote_program_counter::output_nodes(remote_petri_index n)
{
	vector<remote_petri_index> result;
	for (vector<remote_petri_arc>::iterator arc = arcs.begin(); arc != arcs.end(); arc++)
		if (arc->first == n)
			result.push_back(arc->second);
	return result;
}

bool remote_program_counter::is_active(petri_index i)
{
	return net->at(i).active;
}

bool remote_program_counter::is_satisfied(petri_index i, minterm state)
{
	for (vector<minterm>::iterator term = net->at(i).predicate.terms.begin(); term != net->at(i).predicate.terms.end(); term++)
		if ((*term & state) != 0)
			return true;

	return false;
}

bool remote_program_counter::is_vacuous(petri_index i, minterm state)
{
	canonical temp = ~net->at(i).predicate;
	return is_mutex(&temp, &state);
}

bool remote_program_counter::next_has_active_or_satisfied(remote_petri_index i, minterm state, vector<petri_index> &outgoing)
{
	canonical wait = waits(i);
	for (size_t j = 0; j < net->arcs.size(); j++)
		if (net->arcs[j].first == i && (net->at(net->arcs[j].second).active || (is_satisfied(net->arcs[j].second, state) && wait != 0)))
			outgoing.push_back(net->arcs[j].second);
	return outgoing.size() > 0;
}

bool remote_program_counter::is_one(petri_index i)
{
	return (net->at(i).predicate == 1);
}

int remote_program_counter::nid(petri_index idx)
{
	if (idx.is_place())
		return place_iteration[idx.idx()]++;
	else
	{
		int i = idx.idx();
		if (trans_iteration[i].second >= input_size[i])
		{
			trans_iteration[i].second = 0;
			trans_iteration[i].first++;
		}

		trans_iteration[i].second++;
		return trans_iteration[i].first;
	}
}

int remote_program_counter::count(size_t n)
{
	int total = 1;
	for (size_t j = n+1; j < end.size(); j++)
	{
		if (end[j] == end[n])
			total++;
	}
	return total;
}

void remote_program_counter::merge(size_t n)
{
	for (size_t j = n+1; j < end.size();)
	{
		if (end[j] == end[n])
			end.erase(end.begin() + j);
		else
			j++;
	}
}

minterm remote_program_counter::firings()
{
	minterm result;
	list<petri_index> firing_nodes;
	for (vector<remote_petri_arc>::iterator i = arcs.begin(); i != arcs.end(); i++)
	{
		if (i->second.is_trans() && net->at(i->second).active)
			firing_nodes.push_back(i->second);
		else if (i->first.is_trans() && net->at(i->first).active)
			firing_nodes.push_back(i->first);
	}

	firing_nodes.sort();
	firing_nodes.unique();

	for (list<petri_index>::iterator i = firing_nodes.begin(); i != firing_nodes.end(); i++)
		for (vector<minterm>::iterator j = net->at(*i).predicate.terms.begin(); j != net->at(*i).predicate.terms.end(); j++)
			result &= *j;

	return result;
}

canonical remote_program_counter::waits(remote_petri_index n)
{
	list<remote_petri_index> iter(1, n);
	list<petri_index> waiting_nodes;
	for (list<remote_petri_index>::iterator end = iter.begin(); end != iter.end(); end = iter.erase(end))
	{
		if (end->is_trans() && !net->at(*end).active)
			waiting_nodes.push_back(*end);

		vector<remote_petri_index> inputs = input_nodes(*end);
		iter.insert(iter.end(), inputs.begin(), inputs.end());
	}

	waiting_nodes.sort();
	waiting_nodes.unique();

	canonical result(1);
	for (list<petri_index>::iterator i = waiting_nodes.begin(); i != waiting_nodes.end(); i++)
		result &= net->at(*i).predicate;

	return result;
}

remote_program_counter &remote_program_counter::operator=(remote_program_counter pc)
{
	name = pc.name;
	net = pc.net;
	begin = pc.begin;
	end = pc.end;
	arcs = pc.arcs;
	place_iteration = pc.place_iteration;
	trans_iteration = pc.trans_iteration;
	input_size = pc.input_size;
	return *this;
}

ostream &operator<<(ostream &os, remote_program_counter p)
{
	os << p.net->name << ":" << p.name << ":(" << p.begin << "=>" << p.end << ")";
	return os;
}

program_execution::program_execution()
{
	deadlock = false;
	done = false;
}

program_execution::program_execution(const program_execution &exec)
{
	deadlock = exec.deadlock;
	done = exec.done;
	pcs.reserve(exec.pcs.size());
	rpcs.reserve(exec.rpcs.size());
	for (size_t i = 0; i < exec.pcs.size(); i++)
		pcs.push_back(exec.pcs[i]);
	for (size_t i = 0; i < exec.rpcs.size(); i++)
		rpcs.push_back(exec.rpcs[i]);
}

program_execution::~program_execution()
{

}

int program_execution::count(size_t pci)
{
	int total = 0;
	for (size_t pcj = 0; pcj < pcs.size(); pcj++)
		if (pcs[pci] == pcs[pcj])
			total++;

	return total;
}

int program_execution::merge(size_t pci)
{
	for (size_t pcj = 0; pcj < pcs.size(); )
	{
		if (pci != pcj && pcs[pci] == pcs[pcj])
		{
			pcs[pci].state &= pcs[pcj].state;
			pcs.erase(pcs.begin() + pcj);
			if (pcj < pci)
				pci--;
		}
		else
			pcj++;
	}

	return pci;
}

void program_execution::init_pcs(string name, petri_net *net, bool elaborate)
{
	for (size_t k = 0; k < net->M0.size(); k++)
		pcs.push_back(program_counter(name, elaborate, net->M0[k], net));
}

void program_execution::init_rpcs(string name, petri_net *net)
{
	rpcs.push_back(remote_program_counter(name, net));
}

program_execution &program_execution::operator=(program_execution e)
{
	pcs = e.pcs;
	rpcs = e.rpcs;
	deadlock = e.deadlock;
	done = e.done;
	return *this;
}

void program_execution_space::duplicate_execution(program_execution *exec_in, program_execution **exec_out)
{
	execs.push_back(*exec_in);
	*exec_out = &execs.back();
}

void program_execution_space::duplicate_counter(program_execution *exec_in, size_t pc_in, size_t &pc_out)
{
	exec_in->pcs.push_back(exec_in->pcs[pc_in]);
	pc_out = exec_in->pcs.size()-1;
}

bool program_execution_space::remote_end_ready(program_execution *exec, size_t rpc, size_t &idx, vector<petri_index> &outgoing, minterm state)
{
	outgoing.clear();
	if (idx >= exec->rpcs[rpc].end.size())
		idx = 0;

	size_t end = idx;
	bool first = true;
	while (first || idx != end)
	{
		first = false;
		if (exec->rpcs[rpc].end[idx].is_trans() && exec->rpcs[rpc].input_size[exec->rpcs[rpc].end[idx].idx()] == 1)
		{
			vector<petri_index> temp = exec->rpcs[rpc].net->next(exec->rpcs[rpc].end[idx]);
			outgoing.insert(outgoing.end(), temp.begin(), temp.end());
			return true;
		}
		else if (exec->rpcs[rpc].end[idx].is_trans() && exec->rpcs[rpc].count(idx) == exec->rpcs[rpc].input_size[exec->rpcs[rpc].end[idx].idx()])
		{
			exec->rpcs[rpc].merge(idx);
			vector<petri_index> temp = exec->rpcs[rpc].net->next(exec->rpcs[rpc].end[idx]);
			outgoing.insert(outgoing.end(), temp.begin(), temp.end());
			return true;
		}
		else if (exec->rpcs[rpc].end[idx].is_place() && exec->rpcs[rpc].next_has_active_or_satisfied(exec->rpcs[rpc].end[idx], state, outgoing))
			return true;

		idx++;
		if (idx >= exec->rpcs[rpc].end.size())
			idx = 0;
	}

	return false;
}

bool program_execution_space::remote_begin_ready(program_execution *exec, size_t rpc, size_t &idx, minterm state)
{
	if (idx >= exec->rpcs[rpc].begin.size())
		idx = 0;

	size_t end = idx;
	bool first = true;
	while (first || idx != end)
	{
		first = false;
		if (exec->rpcs[rpc].begin[idx].is_place() || (!exec->rpcs[rpc].net->at(exec->rpcs[rpc].begin[idx]).active && exec->rpcs[rpc].is_vacuous(exec->rpcs[rpc].begin[idx], state)))
		{
			bool wall = false;
			for (size_t j = 0; j < exec->rpcs[rpc].end.size() && !wall; j++)
				if (exec->rpcs[rpc].begin[idx] == exec->rpcs[rpc].end[j])
					wall = true;

			if (!wall)
				return true;
		}

		list<remote_petri_index> iter(1, exec->rpcs[rpc].begin[idx]);

		for (list<remote_petri_index>::iterator n = iter.begin(); n != iter.end(); n = iter.erase(n))
		{
			if (n->is_trans() && exec->rpcs[rpc].net->at(*n).active && !exec->rpcs[rpc].is_one(*n) && exec->rpcs[rpc].is_vacuous(*n, state))
			{
				bool wall = false;
				for (size_t j = 0; j < exec->rpcs[rpc].end.size() && !wall; j++)
					if (*n == exec->rpcs[rpc].end[j])
						wall = true;

				if (!wall)
					return true;
			}

			if (n->is_place() || exec->rpcs[rpc].net->at(*n).active || exec->rpcs[rpc].is_vacuous(*n, state))
				for (vector<remote_petri_arc>::iterator a = exec->rpcs[rpc].arcs.begin(); a != exec->rpcs[rpc].arcs.end(); a++)
					if (a->first == *n)
						iter.push_back(a->second);
		}

		idx++;
		if (idx >= exec->rpcs[rpc].begin.size())
			idx = 0;
	}

	return false;
}

void program_execution_space::full_elaborate()
{
	string start_statement = "elaborating {";
	translations.clear();

	bool first = true;
	for (list<program_execution>::iterator exec = execs.begin(); exec != execs.end(); exec++)
	{
		for (vector<program_counter>::iterator pc = exec->pcs.begin(); pc != exec->pcs.end(); pc++)
		{
			if (pc->elaborate && find(nets.begin(), nets.end(), pc->net) == nets.end())
			{
				pc->net->parallel_nodes.clear();
				nets.push_back(pc->net);
				if (!first)
					start_statement += ", ";
				start_statement += pc->net->name;
				first = false;
			}

			for (vector<program_counter>::iterator pcj = exec->pcs.begin(); pcj != exec->pcs.end(); pcj++)
				if (pcj != pc && (pcj->net != pc->net || pcj->name != pc->name))
				{
					gen_translation(pcj->name, pcj->net, pc->name, pc->net);
					pc->set(translate(pcj->name, pcj->net, pcj->state, pc->name, pc->net));
				}

			for (vector<remote_program_counter>::iterator pcj = exec->rpcs.begin(); pcj != exec->rpcs.end(); pcj++)
			{
				gen_translation(pcj->name, pcj->net, pc->name, pc->net);
				gen_translation(pc->name, pc->net, pcj->name, pcj->net);
				if (pcj->net->reset.terms.size() > 0)
					pc->set(translate(pcj->name, pcj->net, pcj->net->reset.terms.front(), pc->name, pc->net));
			}
		}
	}

	start_statement += "}";
	log("", start_statement, __FILE__, __LINE__);

	for (vector<petri_net*>::iterator net = nets.begin(); net != nets.end(); net++)
		for (size_t i = 0; i < (*net)->S.size(); i++)
			(*net)->S[i].predicate = canonical(0);

	int number_processed = 0;
	int max_width = 0;
	list<program_state> observed_states;
	while (execs.size() > 0)
	{
		if ((int)execs.size() > max_width)
			max_width = execs.size();

		program_execution current_execution = execs.back();
		program_execution *exec = &current_execution;
		execs.pop_back();

		if (get_verbose())
			log("", "Execution " + to_string(number_processed) + " " + to_string(execs.size()), __FILE__, __LINE__);
		else if (500*(int)(number_processed/500) == number_processed)
			progress("", "Execution " + to_string(number_processed) + " " + to_string(execs.size()), __FILE__, __LINE__);

		while (!exec->done && !exec->deadlock)
		{
			/**********************************
			 * Handle remote program counters *
			 **********************************/
			for (size_t pc = 0; pc < exec->rpcs.size(); pc++)
			{
				minterm state;
				for (size_t i = 0; i < exec->pcs.size(); i++)
					state &= translate(exec->pcs[i].name, exec->pcs[i].net, exec->pcs[i].state, exec->rpcs[pc].name, exec->rpcs[pc].net);

				size_t idx = 0;
				vector<petri_index> outgoing;
				while (remote_begin_ready(exec, pc, idx, state))
				{
					vector<remote_petri_index> ia = exec->rpcs[pc].input_nodes(exec->rpcs[pc].begin[idx]);
					vector<remote_petri_index> oa = exec->rpcs[pc].output_nodes(exec->rpcs[pc].begin[idx]);

					if (oa.size() == 0)
						exec->rpcs[pc].begin.erase(exec->rpcs[pc].begin.begin() + idx);
					else
					{
						for (vector<remote_petri_index>::iterator i = oa.begin(); i != oa.end(); i++)
						{
							log("", "-" + to_string(exec->rpcs[pc]) + "." + to_string(*i) + " [" + exec->rpcs[pc].firings().print(exec->rpcs[pc].net->vars) + "] " + to_string(exec->rpcs[pc].arcs), __FILE__, __LINE__);
							exec->rpcs[pc].begin.push_back(*i);
						}

						for (vector<remote_petri_arc>::iterator arc = exec->rpcs[pc].arcs.begin(); ia.size() == 0 && arc != exec->rpcs[pc].arcs.end(); )
						{
							if (arc->first == exec->rpcs[pc].begin[idx])
								arc = exec->rpcs[pc].arcs.erase(arc);
							else
								arc++;
						}

						exec->rpcs[pc].begin.erase(exec->rpcs[pc].begin.begin() + idx);
					}
				}

				idx = 0;
				while (remote_end_ready(exec, pc, idx, outgoing, state))
				{
					if (outgoing.size() == 0)
					{
						error("", "remote net has an acyclic component at " + to_string(exec->rpcs[pc].end[idx]), "", __FILE__, __LINE__);
						exec->rpcs[pc].end.erase(exec->rpcs[pc].end.begin() + idx);
					}
					else
					{
						if (exec->rpcs[pc].end[idx].is_place() && outgoing.size() > 1)
							log("", "adding " + to_string(outgoing.size()-1) + " executions for a conditional split", __FILE__, __LINE__);

						if (exec->rpcs[pc].end[idx].is_trans() && exec->rpcs[pc].net->at(exec->rpcs[pc].end[idx]).active && exec->rpcs[pc].net->at(exec->rpcs[pc].end[idx]).predicate.terms.size() > 0)
							state = state >> exec->rpcs[pc].net->at(exec->rpcs[pc].end[idx]).predicate.terms.back();

						for (size_t i = outgoing.size()-1; i >= 0 && i < outgoing.size(); i--)
						{
							log("", "+" + to_string(exec->rpcs[pc]) + "." + to_string(outgoing[i]) + " {" + exec->rpcs[pc].net->at(outgoing[i]).predicate.print(exec->rpcs[pc].net->vars) + "} [" + exec->rpcs[pc].firings().print(exec->rpcs[pc].net->vars) + "] " + to_string(exec->rpcs[pc].arcs), __FILE__, __LINE__);

							program_execution *cexec = exec;
							int cidx = idx;

							if (i > 0)
							{
								if (cexec->rpcs[pc].end[cidx].is_place())
									duplicate_execution(cexec, &cexec);
								else
								{
									cexec->rpcs[pc].end.push_back(cexec->rpcs[pc].end[cidx]);
									cidx = cexec->rpcs[pc].end.size()-1;
								}
							}

							remote_petri_index new_node(outgoing[i], cexec->rpcs[pc].nid(outgoing[i]));
							cexec->rpcs[pc].arcs.push_back(remote_petri_arc(cexec->rpcs[pc].end[cidx], new_node));
							cexec->rpcs[pc].end[cidx] = new_node;
						}
					}
				}
			}

			for (size_t j = 0; j < exec->rpcs.size(); j++)
				for (size_t i = 0; i < exec->pcs.size(); i++)
					exec->pcs[i].apply(translate(exec->rpcs[j].name, exec->rpcs[j].net, exec->rpcs[j].firings(), exec->pcs[i].name, exec->pcs[i].net));

			sort(exec->pcs.begin(), exec->pcs.end());

			/* Since the program counters have been sorted at this point,
			 * we can also assume that the indices are sorted relative to eachother.
			 * this means that all we have to do iterate and check whether the pair i,j
			 * is already in the parallel node list. And if it isn't, then we need to put it there.
			 */
			for (size_t i = 0; i < exec->pcs.size(); i++)
				for (size_t j = i+1; j < exec->pcs.size(); j++)
					if (exec->pcs[i].name == exec->pcs[j].name && exec->pcs[i].net == exec->pcs[j].net && exec->pcs[i].index != exec->pcs[j].index)
					{
						pair<petri_index, petri_index> para(exec->pcs[i].index, exec->pcs[j].index);
						list<pair<petri_index, petri_index> >::iterator lb = lower_bound(exec->pcs[i].net->parallel_nodes.begin(), exec->pcs[i].net->parallel_nodes.end(), para);
						if (lb == exec->pcs[i].net->parallel_nodes.end() || *lb != para)
							exec->pcs[i].net->parallel_nodes.insert(lb, para);
					}


			/*********************************
			 * Handle local program counters *
			 *********************************/
			vector<size_t> ready_transitions;
			vector<pair<petri_index, vector<size_t> > > ready_places;

			/* Transitions are pretty much skipped over since at this point
			 * they have "enabled". There is also no need to check for parallel
			 * merges here since that is included in the requirements for a
			 * transition to become "enabled". So basically, if there is a
			 * transition, we just add it to the ready list.
			 */
			for (size_t pc = 0; pc < exec->pcs.size(); pc++)
				if (exec->pcs[pc].index.is_trans())
					ready_transitions.push_back(pc);

			if (ready_transitions.size() == 0)
			{
				/* At this point, we want to loop through groups of program counters
				 * where each group represents a different process in the simulation.
				 */
				for (size_t ps = 0, pe = 0; ps < exec->pcs.size(); ps = pe)
				{
					for (; pe < exec->pcs.size() && exec->pcs[pe].name == exec->pcs[ps].name && exec->pcs[pe].net == exec->pcs[ps].net; pe++);

					/* Get the list of transitions that we suspect could become enabled
					 * at this state in this particular process.
					 */
					vector<petri_index> next;
					for (size_t p = ps; p < pe; p++)
						next.insert(next.end(), exec->pcs[p].n.begin(), exec->pcs[p].n.end());
					sort(next.begin(), next.end());
					next.resize(unique(next.begin(), next.end())-next.begin());

					for (size_t i = 0; i < next.size(); i++)
					{
						// Check if this transition is enabled.

						// Check to see how many program counter we expect to see coming into this transition
						vector<petri_index> prev = exec->pcs[ps].net->prev(next[i]);
						sort(prev.begin(), prev.end());

						/* Figure out how many program counters are actually coming into this transition
						 * and calculate the total state of all of those program counters.
						 */
						vector<size_t> count;
						minterm combined;
						for (size_t p = ps, j = 0; p < pe && j < prev.size(); )
						{
							if (exec->pcs[p].index < prev[j])
								p++;
							else if (prev[j] < exec->pcs[p].index)
								j++;
							else
							{
								count.push_back(p);
								combined &= exec->pcs[p].state;
								p++;
								j++;
							}
						}

						if (count.size() == prev.size())
						{
							/* Cool! we can do a parallel merge at this point, but is the transition
							 * itself ready to be enabled? (i.e. is it active, or does the total state satisfy the guard?)
							 */
							bool satisfied = exec->pcs[ps].net->at(next[i]).active;
							for (size_t j = 0; !satisfied && j < exec->pcs[ps].net->at(next[i]).predicate.terms.size(); j++)
								if ((combined & exec->pcs[ps].net->at(next[i]).predicate.terms[j]) != 0)
									satisfied = true;

							// If so, then all places leading into this transition are designated as ready.
							if (satisfied)
								ready_places.push_back(pair<petri_index, vector<size_t> >(next[i], count));
						}
					}
				}

				// Check for unstable guards
				for (size_t i = 0; i < exec->ready_places.size(); i++)
				{
					bool found = false;
					for (size_t j = 0; !found && j < ready_places.size(); j++)
						if (exec->ready_places[i].first == ready_places[j].first)
							found = true;

					string location = exec->pcs[exec->ready_places[i].second[0]].net->name + ":" + exec->ready_places[i].first.name();
					if (!found && find(instabilities.begin(), instabilities.end(), location) == instabilities.end())
					{
						error(location, "instability detected", "", __FILE__, __LINE__);
						instabilities.push_back(location);
					}
				}

				exec->ready_places = ready_places;
			}

			string logstring;
			for (size_t i = 0; i < ready_transitions.size(); i++)
			{
				if (i != 0)
					logstring += " ";

				logstring += to_string(exec->pcs[ready_transitions[i]]) + "(" + exec->pcs[ready_transitions[i]].state.print(exec->pcs[ready_transitions[i]].net->vars) + ")";
			}
			for (size_t i = 0; i < ready_places.size(); i++)
			{
				logstring += to_string(ready_places[i].first) + "{";
				for (size_t j = 0; j < ready_places[i].second.size(); j++)
				{
					if (j != 0)
						logstring += " ";
					logstring += to_string(exec->pcs[ready_places[i].second[j]]) + "(" + exec->pcs[ready_places[i].second[j]].state.print(exec->pcs[ready_places[i].second[j]].net->vars) + ")";
				}
				logstring += "}";
			}
			log("", logstring, __FILE__, __LINE__);

			if (ready_transitions.size() != 0)
			{
				vector<size_t> j(ready_transitions.size(), 0);
				vector<vector<minterm> > passing;
				int count = 1;
				for (size_t i = 0; i < ready_transitions.size(); i++)
				{
					if (exec->pcs[ready_transitions[i]].is_active())
						passing.push_back(exec->pcs[ready_transitions[i]].net->at(exec->pcs[ready_transitions[i]].index).predicate.terms);
					else
					{
						passing.push_back(vector<minterm>());
						for (size_t j = 0; j < exec->pcs[ready_transitions[i]].net->at(exec->pcs[ready_transitions[i]].index).predicate.terms.size(); j++)
							if ((exec->pcs[ready_transitions[i]].net->at(exec->pcs[ready_transitions[i]].index).predicate.terms[j] & exec->pcs[ready_transitions[i]].state) != 0)
								passing.back().push_back(exec->pcs[ready_transitions[i]].net->at(exec->pcs[ready_transitions[i]].index).predicate.terms[j]);
					}
					count *= passing.back().size();
				}

				if (count > 1)
					log("", "adding " + to_string(count-1) + " executions for passing through a multiterm transition", __FILE__, __LINE__);

				bool last = false;
				while (!last)
				{
					last = true;
					for (size_t i = 0; last && i < j.size(); i++)
						if (j[i] < passing[i].size()-1)
							last = false;

					program_execution *texec = exec;
					if (!last)
						duplicate_execution(texec, &texec);

					for (size_t i = 0; i < j.size(); i++)
					{
						if (texec->pcs[ready_transitions[i]].is_active())
						{
							texec->pcs[ready_transitions[i]].state = texec->pcs[ready_transitions[i]].state >> passing[i][j[i]];
							for (size_t apc = 0; apc < texec->pcs.size(); apc++)
								if (apc != ready_transitions[i])
									texec->pcs[apc].apply(translate(texec->pcs[ready_transitions[i]].name, texec->pcs[ready_transitions[i]].net, passing[i][j[i]], texec->pcs[apc].name, texec->pcs[apc].net));
						}
						else
							texec->pcs[ready_transitions[i]].state &= passing[i][j[i]];

						for (size_t k = texec->pcs[ready_transitions[i]].n.size()-1; k >= 0 && k < texec->pcs[ready_transitions[i]].n.size(); k--)
						{
							size_t cpc = ready_transitions[i];
							// Create a new program counter for parallel splits
							if (k > 0)
								duplicate_counter(texec, cpc, cpc);

							texec->pcs[cpc].index = texec->pcs[cpc].n[k];
							texec->pcs[cpc].n.clear();
							texec->pcs[cpc].p.clear();
							for (size_t i = 0; i < texec->pcs[cpc].net->arcs.size(); i++)
							{
								if (texec->pcs[cpc].net->arcs[i].first == texec->pcs[cpc].index)
									texec->pcs[cpc].n.push_back(texec->pcs[cpc].net->arcs[i].second);
								if (texec->pcs[cpc].net->arcs[i].second == texec->pcs[cpc].index)
									texec->pcs[cpc].p.push_back(texec->pcs[cpc].net->arcs[i].first);
							}
						}
					}

					j[0]++;
					for (size_t i = 0; i < j.size()-1 && j[i] >= passing[i].size(); i++)
					{
						j[i] = 0;
						j[i+1]++;
					}
				}
			}
			else if (ready_places.size() > 0)
			{
				program_state current_state(exec);
				list<program_state>::iterator lb = lower_bound(observed_states.begin(), observed_states.end(), current_state);
				if (lb == observed_states.end() || *lb != current_state)
				{
					observed_states.insert(lb, current_state);

					// We want to create a new program execution for every possible ordering of events.
					for (size_t i = ready_places.size()-1; i >= 0 && i < ready_places.size(); i--)
						for (size_t j = i-1; exec->pcs[ready_places[i].second[0]].elaborate && j >= 0 && j < ready_places.size(); j--)
							if (!exec->pcs[ready_places[j].second[0]].elaborate)
							{
								pair<petri_index, vector<size_t> > temp = ready_places[i];
								ready_places[i] = ready_places[j];
								ready_places[j] = temp;
							}

					vector<int> moving;
					map<int, pair<bool, bool> > done;
					for (size_t i = 0; i < ready_places.size(); i++)
						moving.insert(moving.end(), ready_places[i].second.begin(), ready_places[i].second.end());
					sort(moving.begin(), moving.end());
					moving.resize(unique(moving.begin(), moving.end()) - moving.begin());

					for (size_t i = 0; i < moving.size(); i++)
						if (exec->pcs[moving[i]].elaborate)
						{
							exec->pcs[moving[i]].net->at(exec->pcs[moving[i]].index).predicate.push_back(exec->pcs[moving[i]].state);
							exec->pcs[moving[i]].net->at(exec->pcs[moving[i]].index).predicate.mccluskey_or(exec->pcs[moving[i]].net->at(exec->pcs[moving[i]].index).predicate.terms.size()-1);
						}

					if (ready_places.size() > 1)
						log("", "adding " + to_string(ready_places.size()-1) + " executions for different orderings", __FILE__, __LINE__);

					for (size_t i = 0; i < ready_places.size(); i++)
					{
						program_execution *oexec = exec;
						int pc = ready_places[i].second[0];

						if (i < ready_places.size()-1)
							duplicate_execution(oexec, &oexec);

						for (int j = ready_places[i].second.size()-1; j > 0; j--)
						{
							oexec->pcs[ready_places[i].second[0]].state &= oexec->pcs[ready_places[i].second[j]].state;
							oexec->pcs.erase(oexec->pcs.begin() + ready_places[i].second[j]);
						}

						for (size_t k = 0; k < oexec->ready_places.size(); k++)
							if (oexec->ready_places[k].first == ready_places[i].first)
								oexec->ready_places.erase(oexec->ready_places.begin() + k);

						oexec->pcs[pc].index = ready_places[i].first;
						oexec->pcs[pc].n.clear();
						oexec->pcs[pc].p.clear();
						for (size_t k = 0; k < oexec->pcs[pc].net->arcs.size(); k++)
						{
							if (oexec->pcs[pc].net->arcs[k].first == oexec->pcs[pc].index)
								oexec->pcs[pc].n.push_back(oexec->pcs[pc].net->arcs[k].second);
							if (oexec->pcs[pc].net->arcs[k].second == oexec->pcs[pc].index)
								oexec->pcs[pc].p.push_back(oexec->pcs[pc].net->arcs[k].first);
						}

						sort(oexec->pcs.begin(), oexec->pcs.end());
					}
				}
				else
					exec->done = true;
			}
			else
			{
				string note = "";
				for (vector<program_counter>::iterator pc = exec->pcs.begin(); pc != exec->pcs.end(); pc++)
				{
					if (pc != exec->pcs.begin())
						note += "&";
					note += pc->state.print(pc->net->vars);
				}
				for (vector<remote_program_counter>::iterator pc = exec->rpcs.begin(); pc != exec->rpcs.end(); pc++)
				{
					if (pc != exec->rpcs.begin() || exec->pcs.size() != 0)
						note += "&";
					note += pc->firings().print(pc->net->vars);
				}
				error(to_string(exec->pcs) + to_string(exec->rpcs), "deadlock detected", note, __FILE__, __LINE__);
				exec->deadlock = true;
			}
		}

		log("", "done", __FILE__, __LINE__);

		number_processed++;
	}

	if (get_verbose())
		log("", "executions (maximum width/total count): " + to_string(max_width) + "/" + to_string(number_processed), __FILE__, __LINE__);
	else
		done_progress();
}

void program_execution_space::reset()
{
	execs.clear();
	nets.clear();
}

void program_execution_space::gen_translation(string name0, petri_net *net0, string name1, petri_net *net1)
{
	vector<pair<size_t, size_t> > factors;
	for (size_t vi = 0; vi < net0->vars.globals.size(); vi++)
	{
		string test = net0->vars.globals[vi].name;
		if (test.substr(0, 5) == "this.")
			test = test.substr(5);

		if (name0 != "")
			test = name0 + "." + test;

		size_t uid = net1->vars.globals.size();
		if (name1.length() == 0)
			uid = net1->vars.find(test);
		else if (test.substr(0, name1.length()) == name1)
			uid = net1->vars.find(test.substr(name1.length()+1));

		if (uid < net1->vars.globals.size())
			factors.push_back(pair<size_t, size_t>(vi, uid));
	}

	translations.insert(
		pair<pair<pair<string, petri_net*>, pair<string, petri_net*> >, vector<pair<size_t, size_t> > >(
			pair<pair<string, petri_net*>, pair<string, petri_net*> >(
				pair<string, petri_net*>(name0, net0),
				pair<string, petri_net*>(name1, net1)
			),
			factors
		)
	);
}

minterm program_execution_space::translate(string name0, petri_net *net0, minterm t, string name1, petri_net *net1)
{
	if (name0 != name1 || net0 != net1)
	{
		return t.refactor(
			translations[
				pair<pair<string, petri_net*>, pair<string, petri_net*> >(
					pair<string, petri_net*>(name0, net0),
					pair<string, petri_net*>(name1, net1)
				)
			]
		);
	}
	else
		return t;
}

/* PROGRAM INDEX AND PROGRAM STATE ARE ONLY USED FOR DEBUGGING */

program_index::program_index()
{
	net = NULL;
}

program_index::program_index(string name, petri_net *net, petri_index index, minterm encoding)
{
	this->name = name;
	this->net = net;
	this->index = index;
	this->encoding = encoding;
}

program_index::program_index(const program_index &i)
{
	name = i.name;
	net = i.net;
	index = i.index;
	encoding = i.encoding;
}

program_index::~program_index()
{
	net = NULL;
}

program_index &program_index::operator=(program_index i)
{
	name = i.name;
	net = i.net;
	index = i.index;
	encoding = i.encoding;
	return *this;
}

program_index &program_index::operator=(petri_index i)
{
	index = i;
	return *this;
}

bool operator==(program_index i1, program_index i2)
{
	return (i1.name == i2.name && i1.net == i2.net && i1.index == i2.index && i1.encoding.subset(i2.encoding));
}

bool operator!=(program_index i1, program_index i2)
{
	return (i1.name != i2.name || i1.net != i2.net || i1.index != i2.index || !i1.encoding.subset(i2.encoding));
}

bool operator<(program_index i1, program_index i2)
{
	return ((i1.name < i2.name) ||
			(i1.name == i2.name && i1.net < i2.net) ||
			(i1.name == i2.name && i1.net == i2.net && i1.index < i2.index) ||
			(i1.name == i2.name && i1.net == i2.net && i1.index == i2.index && i1.encoding < i2.encoding));
}

program_state::program_state()
{

}

program_state::program_state(program_execution *exec)
{
	for (size_t p = 0; p < exec->pcs.size(); p++)
		state.push_back(program_index(exec->pcs[p].name, exec->pcs[p].net, exec->pcs[p].index, exec->pcs[p].state));
	sort(state.begin(), state.end());
}

program_state::program_state(const program_state &s)
{
	state = s.state;
}

program_state::~program_state()
{

}

program_state &program_state::operator=(program_execution *exec)
{
	for (size_t p = 0; p < exec->pcs.size(); p++)
		state.push_back(program_index(exec->pcs[p].name, exec->pcs[p].net, exec->pcs[p].index, exec->pcs[p].state));
	sort(state.begin(), state.end());
	return *this;
}

program_state &program_state::operator=(program_state s)
{
	state = s.state;
	return *this;
}

ostream &operator<<(ostream &os, program_state s)
{
	os << "{";
	for (size_t i = 0; i < s.state.size(); i++)
	{
		if (i != 0)
			os << " ";
		os << s.state[i].index << "(" << s.state[i].encoding.print(s.state[i].net->vars) << ")";
	}
	os << "}";

	return os;
}

bool operator==(program_state s1, program_state s2)
{
	return s1.state == s2.state;
}

bool operator!=(program_state s1, program_state s2)
{
	return s1.state != s2.state;
}

bool operator<(program_state s1, program_state s2)
{
	return s1.state < s2.state;
}
