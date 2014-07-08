/*
 * rule_space.cpp
 *
 *  Created on: Aug 23, 2013
 *      Author: nbingham
 */

#include "rule_space.h"
#include "message.h"

rule_space::rule_space()
{
}

rule_space::~rule_space()
{
}

void rule_space::generate_minterms(petri_net &net)
{
	vector<size_t> vl, vl2;
	petri_index tid;
	canonical t;
	vector<bool> covered;
	minterm reset;

	rules.resize(net.vars.globals.size());
	for (size_t i = 0; i < net.T.size(); i++)
	{
		vl = net.T[i].predicate.vars();
		for (size_t j = 0; net.T[i].active && j < vl.size(); j++)
		{
			if ((net.T[i].predicate & canonical(vl[j], 1)) == 0)
			{
				// Check to make sure this firing is not vacuous
				vector<petri_index> p = net.prev(petri_index(i, false));
				canonical predicate = 1;
				for (size_t k = 0; k < p.size(); k++)
					predicate &= net.at(p[k]).predicate;

				if ((predicate >> canonical(vl[j], 0)) != predicate)
					rules[vl[j]].implicants[0].push_back(petri_index(i, false));
			}
			else if ((net.T[i].predicate & canonical(vl[j], 0)) == 0)
			{
				// Check to make sure this firing is not vacuous
				vector<petri_index> p = net.prev(petri_index(i, false));
				canonical predicate = 1;
				for (size_t k = 0; k < p.size(); k++)
					predicate &= net.at(p[k]).predicate;

				if ((predicate >> canonical(vl[j], 1)) != predicate)
					rules[vl[j]].implicants[1].push_back(petri_index(i, false));
			}
		}
	}

	size_t r = net.vars.find("reset");
	if (r == net.vars.globals.size())
		net.vars.globals.push_back(variable("reset"));

	for (size_t j = 0; j < net.M0.size(); j++)
	{
		for (size_t k = 0; k < net.at(net.M0[j]).predicate.terms.size(); k++)
		{
			if (j == 0 && k == 0)
				reset = net.at(net.M0[j]).predicate.terms[k];
			else
				reset |= net.at(net.M0[j]).predicate.terms[k];
		}
	}

	log("", "calculating rules", __FILE__, __LINE__);
	for (size_t vi = 0; vi < net.vars.globals.size(); vi++)
		if (net.vars.globals[vi].written)
		{
			log("", net.vars.globals[vi].name, __FILE__, __LINE__);
			rules[vi].uid = vi;
			rules[vi].guards[1] = 0;
			rules[vi].guards[0] = 0;
			rules[vi].explicit_guards[1] = 0;
			rules[vi].explicit_guards[0] = 0;

			for (size_t i = 0; i < 2; i++)
			{
				for (size_t j = 0; j < rules[vi].implicants[i].size(); j++)
				{
					tid = rules[vi].implicants[i][j];
					vl = net.at(tid).predicate.vars();
					vector<petri_index> ia = net.prev(tid);
					t = 1;
					for (size_t k = 0; k < ia.size(); k++)
						t &= net.at(ia[k]).predicate;

					for (size_t k = 0; k < t.terms.size();)
					{
						if (t.terms[k].val(vi) == (uint32_t)i)
							t.terms.erase(t.terms.begin() + k);
						else
							k++;
					}

					t = t.hide(vl);
					rules[vi].explicit_guards[i] |= t;

					log("", "start " + net.at(tid).predicate.print(net.vars), __FILE__, __LINE__);
					rules[vi].strengthen(net, i);
				}
			}

			if (reset.val(vi) == 1)
			{
				rules[vi].guards[1] |= canonical(r, 1);
				rules[vi].guards[0] &= canonical(r, 0);
			}
			else
			{
				rules[vi].guards[0] |= canonical(r, 1);
				rules[vi].guards[1] &= canonical(r, 0);
			}
		}

	log("", "done", __FILE__, __LINE__);
}

void rule_space::check(petri_net &net)
{
	bool has_error = false;
	for (size_t i = 0; i < net.S.size(); i++)
	{
		vector<petri_index> oa = net.next(petri_index(i, true));
		for (size_t j = 0; j < rules.size(); j++)
			for (size_t k = 0; k < 2; k++)
				if (rules[j].implicants[k].size() > 0)
				{
					bool para = false;
					for (size_t l = 0; l < rules[j].implicants[k].size() && !para; l++)
						if (net.are_parallel_siblings(petri_index(i, true), rules[j].implicants[k][l]))
							para = true;

					bool imp = false;
					for (size_t l = 0; l < oa.size() && !imp; l++)
						if (find(rules[j].implicants[k].begin(), rules[j].implicants[k].end(), oa[l]) != rules[j].implicants[k].end())
							imp = true;

					// Does it fire when it shouldn't?
					if (!imp && !para)
					{
						canonical shouldnt = rules[j].guards[k] & net.S[i].effective & canonical(rules[j].uid, 1-k) & canonical(net.vars.find("reset"), 0);
						if (shouldnt != 0)
						{
							has_error = true;
							error(net.name, net.vars.globals[rules[j].uid].name + (k == 0 ? "-" : "+") + " fires when it shouldn't at " + to_string(petri_index(i, true)), "", __FILE__, __LINE__);
						}
					}
					// Does it fire when it should?
					else if (imp)
					{
						canonical should = (~rules[j].guards[k]) & net.S[i].predicate & canonical(net.vars.find("reset"), 0) & canonical(rules[j].uid, 1-k);

						if (should != 0)
						{
							vector<size_t> names;
							for (size_t l = 0; l < rules[j].guards[k].terms.size(); l++)
								for (size_t m = 0; m < net.S[i].predicate.terms.size(); m++)
								{
									minterm test = rules[j].guards[k].terms[l] & net.S[i].predicate.terms[m];

									for (size_t n = 0; n < net.vars.globals.size(); n++)
										if (test.val(n) == (uint32_t)-1)
											names.push_back(n);
								}

							sort(names.begin(), names.end());
							names.resize(unique(names.begin(), names.end()) - names.begin());

							string tempstr;
							for (size_t l = 0; l < names.size(); l++)
							{
								if (l != 0 && names.size() > 2)
									tempstr += ", ";
								else if (l != 0)
									tempstr += " ";

								if (l == names.size()-1 && names.size() > 1)
									tempstr += "and ";

								tempstr += net.vars.globals[names[l]].name;
							}

							has_error = true;
							error(net.name, net.vars.globals[rules[j].uid].name + (k == 0 ? "-" : "+") + " doesn't fire when it should at " + (para ? "parallel " : "") + (imp ? "implicant " : "") + "place " + to_string(petri_index(i, true)), "The values of " + tempstr + " in the guard conflict with the predicate for " + to_string(petri_index(i, true)), __FILE__, __LINE__);
						}
					}
					else if (para)
					{
						canonical should = rules[j].guards[k] & net.S[i].predicate & canonical(net.vars.find("reset"), 0);

						if (should == 0)
						{
							vector<size_t> names;
							for (size_t l = 0; l < rules[j].guards[k].terms.size(); l++)
								for (size_t m = 0; m < net.S[i].predicate.terms.size(); m++)
								{
									minterm test = rules[j].guards[k].terms[l] & net.S[i].predicate.terms[m];

									for (size_t n = 0; n < net.vars.globals.size(); n++)
										if (test.val(n) == (uint32_t)-1)
											names.push_back(n);
								}

							sort(names.begin(), names.end());
							names.resize(unique(names.begin(), names.end()) - names.begin());

							string tempstr;
							for (size_t l = 0; l < names.size(); l++)
							{
								if (l != 0 && names.size() > 2)
									tempstr += ", ";
								else if (l != 0)
									tempstr += " ";

								if (l == names.size()-1 && names.size() > 1)
									tempstr += "and ";

								tempstr += net.vars.globals[names[l]].name;
							}

							has_error = true;
							error(net.name, net.vars.globals[rules[j].uid].name + (k == 0 ? "-" : "+") + " doesn't fire when it should at " + (para ? "parallel " : "") + (imp ? "implicant " : "") + "place " + to_string(petri_index(i, true)), "The values of " + tempstr + " in the guard conflict with the predicate for " + to_string(petri_index(i, true)), __FILE__, __LINE__);
						}
					}
				}
	}

	if (has_error)
		net.export_dot(0, 0).print();
}

void rule_space::generate_bubble(variable_space &vars)
{
	vector<pair<int, int> > bnet;

	vector<size_t> tvars, ivars;

	for (size_t i = 0; i < rules.size(); i++)
		for (int j = 0; j < 2; j++)
			for (size_t k = 0; k < rules[i].guards[j].terms.size(); k++)
			{
				ivars = rules[i].guards[j].terms[k].vars();
				for (size_t l = 0; l < ivars.size(); l++)
					bnet.push_back(pair<int, int>(ivars[l]*2 + rules[i].guards[j].terms[k].val(ivars[l]), rules[i].uid*2 + j));
			}

	bnet.resize(unique(bnet.begin(), bnet.end()) - bnet.begin());

	vector<pair<int, int> >::iterator remarc;
	for (size_t i = 0; i < vars.globals.size(); i++)
		for (size_t j = 0; j < vars.globals.size(); j++)
			if (i != j)
			{
				if ((remarc = find(bnet.begin(), bnet.end(), pair<int, int>(i*2, j*2))) != bnet.end() && find(bnet.begin(), bnet.end(), pair<int, int>(i*2+1, j*2)) != bnet.end())
				{
					error("", "dividing signal found in production rules {" + vars.globals[i].name + " -> " + vars.globals[j].name + "-}", "", __FILE__, __LINE__);
					for (size_t k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (size_t k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}

				if (find(bnet.begin(), bnet.end(), pair<int, int>(i*2, j*2+1)) != bnet.end() && (remarc = find(bnet.begin(), bnet.end(), pair<int, int>(i*2+1, j*2+1))) != bnet.end())
				{
					error("", "dividing signal found in production rules {" + vars.globals[i].name + " -> " + vars.globals[j].name + "+}", "", __FILE__, __LINE__);
					for (size_t k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (size_t k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}

				if ((remarc = find(bnet.begin(), bnet.end(), pair<int, int>(i*2, j*2))) != bnet.end() && find(bnet.begin(), bnet.end(), pair<int, int>(i*2, j*2+1)) != bnet.end())
				{
					error("", "gating signal found in production rules {" + vars.globals[i].name + "- -> " + vars.globals[j].name + "}", "", __FILE__, __LINE__);
					for (size_t k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (size_t k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}

				if (find(bnet.begin(), bnet.end(), pair<int, int>(i*2+1, j*2)) != bnet.end() && (remarc = find(bnet.begin(), bnet.end(), pair<int, int>(i*2+1, j*2+1))) != bnet.end())
				{
					error("", "gating signal found in production rules {" + vars.globals[i].name + "+ -> " + vars.globals[j].name + "}", "", __FILE__, __LINE__);
					for (size_t k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (size_t k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}
			}

	bubble_net.clear();
	map<pair<int, int>, pair<bool, bool> >::iterator net2iter;
	for (size_t i = 0; i < bnet.size(); i++)
	{
		net2iter = bubble_net.find(pair<int, int>(bnet[i].first/2, bnet[i].second/2));
		if (net2iter == bubble_net.end())
			bubble_net.insert(pair<pair<int, int>, pair<bool, bool> >(pair<int, int>(bnet[i].first/2, bnet[i].second/2), pair<bool, bool>(true, bnet[i].first%2 == bnet[i].second%2)));
		else
			net2iter->second.first = false;
	}
}

void rule_space::bubble_reshuffle(variable_space &vars, string name)
{
	cout << "Bubble Reshuffling " << name << endl;
	// Generate bubble reshuffle graph
	vector<bool> inverted(vars.globals.size(), false);
	vector<pair<vector<int>, bool> > cycles;

	// Execute bubble reshuffling algorithm
	for (map<pair<int, int>, pair<bool, bool> >::iterator i = bubble_net.begin(); i != bubble_net.end(); i++)
	{
		vector<pair<vector<int>, bool> > temp = reshuffle_algorithm(i, true, &bubble_net, vector<int>(), &inverted);
		cycles.insert(cycles.end(), temp.begin(), temp.end());
	}

	sort(cycles.begin(), cycles.end());
	cycles.resize(unique(cycles.begin(), cycles.end()) - cycles.begin());

	for (size_t i = 1; i < cycles.size(); i++)
		if (cycles[i].first == cycles[i-1].first)
		{
			cycles.erase(cycles.begin() + i);
			cycles.erase(cycles.begin() + i-1);
			i--;
		}

	for (size_t i = 0; i < cycles.size(); i++)
		if (cycles[i].second)
		{
			cycles.erase(cycles.begin() + i);
			i--;
		}

	// Remove Negative Cycles (currently negative cycles just throw an error message)
	for (size_t i = 0; i < cycles.size(); i++)
	{
		string tempstr;
		for (size_t j = 0; j < cycles[i].first.size(); j++)
		{
			if (j != 0)
				tempstr += ", ";
			tempstr += vars.globals[cycles[i].first[j]].name;
		}
		error("", "negative cycle found in process " + name, tempstr, __FILE__, __LINE__);
	}

	vector<variable> toadd;
	canonical temp;

	for (size_t i = 0; i < vars.globals.size(); i++)
		if (inverted[i])
		{
			for (size_t j = 0; j < rules.size(); j++)
				for (size_t k = 0; k < 2; k++)
					for (size_t l = 0; l < rules[j].guards[k].terms.size(); l++)
						rules[j].guards[k].terms[l].sv_not(i);

			rules[i].invert();

			toadd.push_back(vars.globals[i]);
			toadd.back().name = toadd.back().inverse_name();
		}

	for (size_t i = 0; i < toadd.size(); i++)
	{
		vars.globals.push_back(toadd[i]);

		if (toadd[i].written)
			rules.push_back(rule(vars, "~" + toadd[i].name, toadd[i].name, toadd[i].inverse_name()));
		else
			rules.push_back(rule(vars, "~" + toadd[i].inverse_name(), toadd[i].inverse_name(), toadd[i].name));
	}

	// Apply local inversions to production rules
	for (map<pair<int, int>, pair<bool, bool> >::iterator i = bubble_net.begin(); i != bubble_net.end(); i++)
	{
		if (i->second.second)
		{
			size_t vj = vars.find(vars.globals[i->first.first].inverse_name());
			if (vj == vars.globals.size())
			{
				vars.globals.push_back(vars.globals[i->first.first]);
				vars.globals.back().name = vars.globals[i->first.first].inverse_name();
				vj = vars.globals.size()-1;
				rules.push_back(rule(vars, "~" + vars.globals[i->first.first].name, vars.globals[i->first.first].name, vars.globals[i->first.first].inverse_name()));
			}

			for (size_t j = 0; j < 2; j++)
				for (size_t k = 0; k < rules[i->first.second].guards[j].terms.size(); k++)
				{
					rules[i->first.second].guards[j].terms[k].set(vj, rules[i->first.second].guards[j].terms[k].get(i->first.first));
					rules[i->first.second].guards[j].terms[k].sv_not(vj);
					rules[i->first.second].guards[j].terms[k].set(i->first.first, vX);
				}
		}
	}
}

vector<pair<vector<int>, bool> > rule_space::reshuffle_algorithm(map<pair<int, int>, pair<bool, bool> >::iterator idx, bool forward, map<pair<int, int>, pair<bool, bool> > *net, vector<int> cycle, vector<bool> *inverted)
{
	vector<pair<vector<int>, bool> > cycles;
	vector<int> negative_cycle;
	vector<int>::iterator found;

	cycle.push_back(forward ? idx->first.first : idx->first.second);

	found = find(cycle.begin(), cycle.end(), forward ? idx->first.second : idx->first.first);
	if (found == cycle.end())
	{
		if (idx->second.first && idx->second.second && forward)
		{
			(*inverted)[idx->first.second] = !(*inverted)[idx->first.second];
			for (map<pair<int, int>, pair<bool, bool> >::iterator j = net->begin(); j != net->end(); j++)
				if (j->first.first == idx->first.second || j->first.second == idx->first.second)
					j->second.second = !j->second.second;
		}
		else if (idx->second.first && idx->second.second && !forward)
		{
			(*inverted)[idx->first.first] = !(*inverted)[idx->first.first];
			for (map<pair<int, int>, pair<bool, bool> >::iterator j = net->begin(); j != net->end(); j++)
				if (j->first.first == idx->first.first || j->first.second == idx->first.first)
					j->second.second = !j->second.second;
		}

		for (map<pair<int, int>, pair<bool, bool> >::iterator i = net->begin(); cycles.size() == 0 && i != net->end(); i++)
		{
			vector<pair<vector<int>, bool> > temp;
			if (forward && (i->first.first == idx->first.second || i->first.second == idx->first.second) && i != idx)
				temp = reshuffle_algorithm(i, (i->first.first == idx->first.second), net, cycle, inverted);
			else if (!forward && (i->first.first == idx->first.first || i->first.second == idx->first.first) && i != idx)
				temp = reshuffle_algorithm(i, (i->first.first == idx->first.first), net, cycle, inverted);
			cycles.insert(cycles.end(), temp.begin(), temp.end());
		}
	}
	else
	{
		vector<int> temp(found, cycle.end());
		sort(temp.begin(), temp.end());
		temp.resize(unique(temp.begin(), temp.end()) - temp.begin());
		cycles.push_back(pair<vector<int>, bool>(temp, !idx->second.first || !idx->second.second));
	}

	sort(cycles.begin(), cycles.end());
	cycles.resize(unique(cycles.begin(), cycles.end()) - cycles.begin());

	for (size_t i = 1; i < cycles.size(); i++)
		if (cycles[i].first == cycles[i-1].first)
		{
			cycles.erase(cycles.begin() + i);
			cycles.erase(cycles.begin() + i-1);
			i--;
		}

	return cycles;
}

dot_graph rule_space::export_bubble(variable_space &vars, string name)
{
	dot_graph graph;
	graph.type = "digraph";
	graph.id = name;

	for (size_t i = 0; i < vars.globals.size(); i++)
	{
		graph.stmt_list.stmts.push_back(dot_stmt());
		dot_stmt &stmt = graph.stmt_list.stmts.back();
		stmt.stmt_type = "node";
		stmt.node_ids.push_back(dot_node_id("V" + to_string(i)));
		stmt.attr_list.attrs.push_back(dot_a_list());
		dot_a_list &attr = stmt.attr_list.attrs.back();
		attr.as.push_back(dot_a());
		dot_a &a = attr.as.back();
		a.first.id = "label";
		a.second.id = vars.globals[i].name;
	}

	map<pair<int, int>, pair<bool, bool> >::iterator net2iter;
	for (net2iter = bubble_net.begin(); net2iter != bubble_net.end(); net2iter++)
	{
		graph.stmt_list.stmts.push_back(dot_stmt());
		dot_stmt &stmt = graph.stmt_list.stmts.back();
		stmt.stmt_type = "edge";
		stmt.node_ids.push_back(dot_node_id("V" + to_string(net2iter->first.first)));
		stmt.node_ids.push_back(dot_node_id("V" + to_string(net2iter->first.second)));
		if (net2iter->second.first || net2iter->second.second)
		{
			stmt.attr_list.attrs.push_back(dot_a_list());
			dot_a_list &attr = stmt.attr_list.attrs.back();

			if (net2iter->second.first)
			{
				attr.as.push_back(dot_a());
				dot_a &a = attr.as.back();
				a.first.id = "style";
				a.second.id = "dashed";
			}

			if (net2iter->second.second)
			{
				attr.as.push_back(dot_a());
				dot_a &a1 = attr.as.back();
				a1.first.id = "arrowhead";
				a1.second.id = "odotnormal";
			}
		}
	}

	return graph;
}

void rule_space::print(variable_space &vars, ostream &fout, string newl)
{
	for (size_t i = 0; i < rules.size(); i++)
		if (vars.globals[i].written)
			rules[i].print(vars, fout, "");

	for (size_t i = 0; i < excl.size(); i++)
		if (excl[i].first.size() > 1)
		{
			fout << "mk_excl" << (excl[i].second == 0 ? "lo" : "hi") << "(";
			for (size_t j = 0; j < excl[i].first.size(); j++)
			{
				if (j != 0)
					fout << ",";
				fout << vars.globals[excl[i].first[j]].name;
			}
			fout << ")" << newl;
		}
}
