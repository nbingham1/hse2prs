/*
 * rule_space.cpp
 *
 *  Created on: Aug 23, 2013
 *      Author: nbingham
 */

#include "rule_space.h"

rule_space::rule_space()
{
	net = NULL;
}

rule_space::~rule_space()
{
	net = NULL;
}

void rule_space::insert(rule r)
{
	if ((int)rules.size() <= r.uid)
		rules.resize(r.uid+1);
	rules[r.uid] = r;
}

int rule_space::size()
{
	return (int)rules.size();
}

rule &rule_space::operator[](int i)
{
	return rules[i];
}

void rule_space::apply_one_of(canonical *s, svector<int> a, int v)
{
	canonical temp;
	minterm ex;
	int j;
	bool first = true;

	for (j = 0; j < (int)a.size(); j++)
		if (rules[a[j]].net != NULL)
		{
			if (first)
			{
				ex = minterm(rules[a[j]].uid, 1-v);
				first = false;
			}
			else
				ex &= minterm(rules[a[j]].uid, 1-v);
		}

	if (((*s) & ex) != 0)
		for (j = 0; j < (int)a.size(); j++)
			if (rules[a[j]].net != NULL && (rules[a[j]].guards[v] & (*s)) != 0)
				temp |= ((rules[a[j]].guards[v] & ex & (*s)) >> canonical(rules[a[j]].uid, v)) | ((~rules[a[j]].guards[v] | ~ex) & (*s));

	if (temp.terms.size() != 0)
		*s = temp;
}

canonical rule_space::apply(canonical s, canonical covered, sstring t)
{
	//cout << t << "Covered " << covered.print(vars) << endl;
	canonical result, test;
	int i, j, k, l;
	svector<pair<svector<int>, int> > applicable;
	canonical trans;
	canonical temp;
	canonical c;
	canonical ex;
	minterm extemp;

	for (k = 0; k < (int)s.terms.size(); k++)
	{
		//cout << t << "Minterm " << s.terms[k].print(vars) << endl;
		trans.clear();
		trans.terms.push_back(minterm(1));
		for (i = 0; i < (int)excl.size(); i++)
		{
			ex.clear();
			for (j = 0; j < (int)excl[i].first.size(); j++)
				if (rules[excl[i].first[j]].net != NULL)
				{
					extemp = 1;
					for (l = 0; l < (int)excl[i].first.size(); l++)
						if (l != j)
							extemp &= minterm(rules[excl[i].first[l]].uid, 1-excl[i].second);
					ex.push_back(extemp);
				}

			temp = trans;
			trans.clear();
			for (j = 0; j < (int)excl[i].first.size(); j++)
				if (rules[excl[i].first[j]].net != NULL && (rules[excl[i].first[j]].guards[excl[i].second] & (s.terms[k] & ex.terms[j])) != 0)
					for (l = 0; l < (int)temp.terms.size(); l++)
						trans.terms.push_back(temp.terms[l] & minterm(excl[i].first[j], excl[i].second));

			if (trans.terms.size() == 0)
				trans = temp;
		}

		//cout << t << "Transitions " << trans.print(vars) << endl;

		temp.clear();
		test = s.terms[k];
		for (i = 0; i < (int)trans.terms.size(); i++)
			temp.terms.push_back((s.terms[k] >> trans.terms[i]).xoutnulls());
		temp.mccluskey();

		//cout << t << "Applied " << temp.print(vars) << endl;

		if (find(covered.begin(), covered.end(), s.terms[k]) != covered.end())
		{
			temp = covered;
		//	cout << t << "Found Cycle " << s.terms[k].print(vars) << endl;
		}
		else if (temp != s.terms[k])
		{
			c = covered;
			c.push_back(s.terms[k]);
			temp = apply(temp, c, t + "\t");
		}

		result |= temp;
	}

	result.mccluskey();
	//cout << t << "Result " << result.print(vars) << endl;

	return result;
}

void rule_space::generate_minterms(flag_space *flags)
{
	svector<int> vl, vl2;
	int i, j, k;
	petri_index tid;
	canonical t;
	svector<bool> covered;
	minterm reset;

	rules.resize(net->vars->global.size());
	for (i = 0; i < net->T.size(); i++)
	{
		vl = net->T[i].index.vars().unique();
		for (j = 0; net->T[i].active && j < vl.size(); j++)
		{
			if ((net->T[i].index & canonical(vl[j], 1)) == 0)
				rules[vl[j]].implicants[0].push_back(petri_index(i, false));
			else if ((net->T[i].index & canonical(vl[j], 0)) == 0)
				rules[vl[j]].implicants[1].push_back(petri_index(i, false));
		}
	}

	int r = net->vars->get_uid("reset");

	for (j = 0; j < net->M0.size(); j++)
	{
		for (k = 0; k < net->at(net->M0[j]).index.terms.size(); k++)
		{
			if (j == 0 && k == 0)
				reset = net->at(net->M0[j]).index.terms[k];
			else
				reset |= net->at(net->M0[j]).index.terms[k];
		}
	}

	cout << "Calculating Rules" << endl;
	for (smap<sstring, variable>::iterator vi = net->vars->global.begin(); vi != net->vars->global.end(); vi++)
		if (vi->second.driven)
		{
			cout << vi->second.name << endl;
			rules[vi->second.uid].uid = vi->second.uid;
			rules[vi->second.uid].net = net;
			rules[vi->second.uid].flags = flags;
			rules[vi->second.uid].guards[1] = 0;
			rules[vi->second.uid].guards[0] = 0;
			rules[vi->second.uid].explicit_guards[1] = 0;
			rules[vi->second.uid].explicit_guards[0] = 0;

			for (i = 0; i < 2; i++)
			{
				cout << rules[vi->second.uid].implicants[i].size() << endl;
				for (j = 0; j < rules[vi->second.uid].implicants[i].size(); j++)
				{
					tid = rules[vi->second.uid].implicants[i][j];
					vl = net->at(tid).index.vars().unique();
					svector<petri_index> ia = net->prev(tid);
					for (k = 0, t = 1; k < ia.size(); k++)
						t &= net->at(ia[k]).index;

					for (k = 0; k < t.terms.size();)
					{
						if (t.terms[k].val(vi->second.uid) == (uint32_t)i)
							t.terms.erase(t.terms.begin() + k);
						else
							k++;
					}

					t = t.hide(vl);
					rules[vi->second.uid].explicit_guards[i] |= t;

					//rules[vi->second.uid].guards[i] |= t;

					cout << "Start " << net->at(tid).index.print(net->vars) <<  " ";
					rules[vi->second.uid].strengthen(i);
					cout << endl;
				}
			}

			/*if (reset.val(vi->second.uid) == 1)
			{
				rules[vi->second.uid].guards[1] |= canonical(r, 1);
				rules[vi->second.uid].guards[0] &= canonical(r, 0);
			}
			else
			{
				rules[vi->second.uid].guards[0] |= canonical(r, 1);
				rules[vi->second.uid].guards[1] &= canonical(r, 0);
			}*/
		}

	cout << "Done" << endl;
}

void rule_space::check()
{
	bool ok;
	canonical applied_guard;
	canonical oguard;
	canonical temp;

	bool error = false;
	for (int i = 0; i < net->S.size(); i++)
	{
		svector<petri_index> oa = net->next(petri_index(i, true));
		for (int j = 0; j < rules.size(); j++)
			for (int k = 0; k < 2 && rules[j].net != NULL && net->vars->find(rules[j].uid)->driven; k++)
			{
				bool para = false;
				for (int l = 0; l < rules[j].implicants[k].size() && !para; l++)
					if (net->are_parallel_siblings(petri_index(i, true), rules[j].implicants[k][l]) != -1)
						para = true;

				bool imp = false;
				for (int l = 0; l < oa.size() && !imp; l++)
					if (find(rules[j].implicants[k].begin(), rules[j].implicants[k].end(), oa[l]) != rules[j].implicants[k].end())
						imp = true;

				bool sib_guards = false;
				for (int l = 0; !sib_guards && l < rules[j].implicants[k].size(); l++)
				{
					svector<petri_index> ia = net->prev(rules[j].implicants[k][l]);
					for (int m = 0; !sib_guards && m < ia.size(); m++)
						if (net->are_sibling_guards(ia[m], petri_index(i, true)))
							sib_guards = true;
				}

				applied_guard = rules[j].guards[k] & net->S[i].index & canonical(net->vars->get_uid("reset"), 0);
				// Does it fire when it shouldn't?
				if (!sib_guards && !imp && !para && applied_guard != 0)
				{
					ok = false;
					// check if firing is vacuous
					temp = canonical(rules[j].uid, 1-k);
					if (is_mutex(&applied_guard, &temp))
						ok = true;

					// check if firing is inside the tail and check to make sure that if it is in the tail,
					// it is correctly separated by the guards
					for (int l = 0; l < rules[j].implicants[k].size() && !ok; l++)
					{
						//oguard = 0;
						oguard = net->get_effective_place_encoding(petri_index(i, true), svector<petri_index>(1, rules[j].implicants[k][l]));
						oguard &= ~net->at(rules[j].implicants[k][l]).index;
						/*for (int m = 0; m < oa.size(); m++)
							oguard |= net->T[net->index(oa[m])].index;
						oguard = ~oguard;*/
						cout << "LOOK " << oguard.print(net->vars) << "\n" << applied_guard.print(net->vars) << "\n" << (oguard & applied_guard).print(net->vars) << endl;

						ok = (ok || is_mutex(&applied_guard, &oguard));
					}

					if (!ok)
					{
						cerr << "Error: " << net->vars->get_name(rules[j].uid) << (k == 0 ? "-" : "+") << "\tfires when it shouldn't at state " << i << "\t" << applied_guard.print(net->vars) << endl;
						error = true;
					}
				}
				// Does it fire when it should?
				else if ((imp || para) && applied_guard == 0 && !is_mutex(&rules[j].explicit_guards[k], &net->S[i].index))
				{
					cerr << "Error: " << net->vars->get_name(rules[j].uid) << (k == 0 ? "-" : "+") << "\tdoesn't fire when it should in " << (para ? "parallel " : "") << (imp ? "implicant " : "") << "state " << i << "\t";
					for (int l = 0; l < rules[j].guards[k].terms.size(); l++)
						for (int m = 0; m < net->S[i].index.terms.size(); m++)
						{
							if (l != 0 || m != 0)
								cout << " | ";
							cout << (rules[j].guards[k].terms[l] & net->S[i].index.terms[m]).print(net->vars);
						}
					cout << endl;
					error = true;
				}
			}
	}

	if (error)
		print_enable_graph(&cout, "g");
}

smap<pair<int, int>, pair<bool, bool> > rule_space::gen_bubble_reshuffle_graph()
{
	svector<pair<int, int> > bnet;

	svector<int> tvars, ivars;

	for (int i = 0; i < rules.size(); i++)
		if (rules[i].net != NULL)
			for (int j = 0; j < 2; j++)
				for (int k = 0; k < rules[i].guards[j].terms.size(); k++)
				{
					ivars = rules[i].guards[j].terms[k].vars();
					for (int l = 0; l < ivars.size(); l++)
						bnet.push_back(pair<int, int>(ivars[l]*2 + rules[i].guards[j].terms[k].val(ivars[l]), rules[i].uid*2 + j));
				}

	bnet.unique();

	sstring label;
	/*cout << "digraph g0" << endl;
	cout << "{" << endl;

	for (smap<sstring, variable>::iterator i = net->vars->global.begin(); i != net->vars->global.end(); i++)
	{
		cout << "\tV" << i->second.uid*2 << " [label=\"" << i->second.name << "-\"];" << endl;
		cout << "\tV" << i->second.uid*2+1 << " [label=\"" << i->second.name << "+\"];" << endl;
	}

	for (int i = 0; i < net.size(); i++)
		cout << "\tV" << net[i].first << " -> V" << net[i].second << endl;

	cout << "}" << endl;*/

	svector<pair<int, int> >::iterator remarc;
	for (smap<sstring, variable>::iterator i = net->vars->global.begin(); i != net->vars->global.end(); i++)
		for (smap<sstring, variable>::iterator j = net->vars->global.begin(); j != net->vars->global.end(); j++)
			if (i != j)
			{
				if ((remarc = bnet.find(pair<int, int>(i->second.uid*2, j->second.uid*2))) != bnet.end() && bnet.find(pair<int, int>(i->second.uid*2+1, j->second.uid*2)) != bnet.end())
				{
					cerr << "Error: Dividing signal found in production rules {" << i->second.name << " -> " << j->second.name << "-}" << endl;
					for (int k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (int k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}

				if (bnet.find(pair<int, int>(i->second.uid*2, j->second.uid*2+1)) != bnet.end() && (remarc = bnet.find(pair<int, int>(i->second.uid*2+1, j->second.uid*2+1))) != bnet.end())
				{
					cerr << "Error: Dividing signal found in production rules {" << i->second.name << " -> " << j->second.name << "+}" << endl;
					for (int k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (int k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}

				if ((remarc = bnet.find(pair<int, int>(i->second.uid*2, j->second.uid*2))) != bnet.end() && bnet.find(pair<int, int>(i->second.uid*2, j->second.uid*2+1)) != bnet.end())
				{
					cerr << "Error: Gating signal found in production rules {" << i->second.name << "- -> " << j->second.name << "}" << endl;
					for (int k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (int k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}

				if (bnet.find(pair<int, int>(i->second.uid*2+1, j->second.uid*2)) != bnet.end() && (remarc = bnet.find(pair<int, int>(i->second.uid*2+1, j->second.uid*2+1))) != bnet.end())
				{
					cerr << "Error: Gating signal found in production rules {" << i->second.name << "+ -> " << j->second.name << "}" << endl;
					for (int k = 0; k < rules[remarc->second/2].guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);

					for (int k = 0; k < rules[remarc->second/2].explicit_guards[remarc->second%2].terms.size(); k++)
						rules[remarc->second/2].explicit_guards[remarc->second%2].terms[k].sv_union(remarc->first/2, remarc->first%2 == 0 ? v1 : v0);
					bnet.erase(remarc);
				}
			}

	/*cout << "digraph g1" << endl;
	cout << "{" << endl;

	for (smap<sstring, variable>::iterator i = net->vars->global.begin(); i != net->vars->global.end(); i++)
	{
		cout << "\tV" << i->second.uid*2 << " [label=\"" << i->second.name << "-\"];" << endl;
		cout << "\tV" << i->second.uid*2+1 << " [label=\"" << i->second.name << "+\"];" << endl;
	}

	for (int i = 0; i < net.size(); i++)
		cout << "\tV" << net[i].first << " -> V" << net[i].second << endl;

	cout << "}" << endl;*/

	smap<pair<int, int>, pair<bool, bool> > bnet2;
	smap<pair<int, int>, pair<bool, bool> >::iterator net2iter;
	for (int i = 0; i < bnet.size(); i++)
	{
		net2iter = bnet2.find(pair<int, int>(bnet[i].first/2, bnet[i].second/2));
		if (net2iter == bnet2.end())
			bnet2.insert(pair<pair<int, int>, pair<bool, bool> >(pair<int, int>(bnet[i].first/2, bnet[i].second/2), pair<bool, bool>(true, bnet[i].first%2 == bnet[i].second%2)));
		else
			net2iter->second.first = false;
	}

	cout << endl;

	cout << "digraph g0" << endl;
	cout << "{" << endl;

	for (smap<sstring, variable>::iterator i = net->vars->global.begin(); i != net->vars->global.end(); i++)
		cout << "\tV" << i->second.uid << " [label=\"" << i->second.name << "\"];" << endl;

	for (net2iter = bnet2.begin(); net2iter != bnet2.end(); net2iter++)
		cout << "\tV" << net2iter->first.first << " -> V" << net2iter->first.second << (net2iter->second.first ? " [style=dashed]" : "") << (net2iter->second.second ? " [arrowhead=odotnormal]" : "") << endl;

	cout << "}" << endl;

	cout << endl;

	return bnet2;
}

void rule_space::print(ostream *fout)
{
	int i, j;
	for (i = 0; i < (int)rules.size(); i++)
		if (rules[i].net != NULL)
			(*fout) << rules[i];

	for (i = 0; i < (int)excl.size(); i++)
		if (excl[i].first.size() > 1)
		{
			(*fout) << "mk_excl" << (excl[i].second == 0 ? "lo" : "hi") << "(";
			for (j = 0; j < (int)excl[i].first.size(); j++)
			{
				if (j != 0)
					(*fout) << ",";
				(*fout) << net->vars->get_name(excl[i].first[j]);
			}
			(*fout) << ")" << endl;
		}
}

void rule_space::print_enable_graph(ostream *fout, sstring name)
{
	sstring label;
	(*fout) << "digraph " << name << endl;
	(*fout) << "{" << endl;

	for (int i = 0; i < (int)net->S.size(); i++)
		if (!net->is_floating(petri_index(i, true)))
		{
			(*fout) << "\tS" << i << " [label=\"" << sstring(i) << " ";
			/*label = net->S[i].index.print(vars);

			for (int j = 0, k = 0; j < (int)label.size(); j++)
				if (label[j] == '|')
				{
					(*fout) << label.substr(k, j+1 - k) << "\\n";
					k = j+1;
				}

			(*fout) << label.substr(k) << "\\n";*/

			for (int j = 0; j < (int)rules.size(); j++)
			{
				if (rules[j].net != NULL && !is_mutex(&rules[j].guards[0], &net->S[i].index))
					cout << net->vars->get_name(rules[j].uid) << "-, ";
				if (rules[j].net != NULL && !is_mutex(&rules[j].guards[1], &net->S[i].index))
					cout << net->vars->get_name(rules[j].uid) << "+, ";
			}

			(*fout) << "\"];" << endl;
		}

	for (int i = 0; i < (int)net->T.size(); i++)
	{
		label = net->T[i].index.print(net->vars);
		label = sstring(i) + " " + label;
		if (label != "")
			(*fout) << "\tT" << i << " [shape=box] [label=\"" << label << "\"];" << endl;
		else
			(*fout) << "\tT" << i << " [shape=box];" << endl;
	}

	for (int i = 0; i < (int)net->arcs.size(); i++)
		(*fout) << "\t" << net->arcs[i].first << " -> " << net->arcs[i].second << "[label=\" " << i << " \"];" <<  endl;

	(*fout) << "}" << endl;
}
