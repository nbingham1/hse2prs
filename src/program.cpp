#include "program.h"
#include "utility.h"

program::program()
{
	type_space.insert(pair<string, keyword*>("int", new keyword("int")));
	vars.types = &type_space;
}

program::program(string chp, int verbosity)
{
	vars.types = &type_space;
	parse(chp, verbosity);
	generate_states();
	insert_state_vars();
	generate_prs();
	cout << endl << endl<< "Done!" << endl;

}

program::~program()
{
	map<string, keyword*>::iterator i;
	for (i = type_space.begin(); i != type_space.end(); i++)
	{
		if (i->second != NULL)
			delete i->second;
		i->second = NULL;
	}

	type_space.clear();
}

program &program::operator=(program p)
{
	type_space = p.type_space;
	prs = p.prs;
	errors = p.errors;
	return *this;
}


void program::parse(string chp, int verbosity)
{
	//TODO: THIS BREAKS IF THERE ARE NO IMPLICANTS FOR A OUTPUT
	string::iterator i, j;
	string cleaned_chp = "";
	string word;
	string error;
	int error_start, error_len;

	process *p;
	operate *o;
	record *r;
	channel *c;

	// Define the basic types. In this case, 'int'
	type_space.insert(pair<string, keyword*>("int", new keyword("int")));

	//Remove line comments:
	size_t comment_begin = chp.find("//");
	size_t comment_end = chp.find("\n", comment_begin);
	while (comment_begin != chp.npos && comment_end != chp.npos){
		chp = chp.substr(0,comment_begin) + chp.substr(comment_end);
		comment_begin = chp.find("//");
		comment_end = chp.find("\n", comment_begin);
	}

	//Remove block comments:
	comment_begin = chp.find("/*");
	comment_end = chp.find("*/");
	while (comment_begin != chp.npos && comment_end != chp.npos){
		chp = chp.substr(0,comment_begin) + chp.substr(comment_end+2);
		comment_begin = chp.find("/*");
		comment_end = chp.find("*/");
	}

	// remove extraneous whitespace
	for (i = chp.begin(); i != chp.end(); i++)
	{
		if (!sc(*i))
			cleaned_chp += *i;
		else if (nc(*(i-1)) && (i == chp.end()-1 || nc(*(i+1))))
			cleaned_chp += ' ';
	}

	// split the program into records and processes
	int depth[3] = {0};
	for (i = cleaned_chp.begin(), j = cleaned_chp.begin(); i != cleaned_chp.end(); i++)
	{
		if (*i == '(')
			depth[0]++;
		else if (*i == '[')
			depth[1]++;
		else if (*i == '{')
			depth[2]++;
		else if (*i == ')')
			depth[0]--;
		else if (*i == ']')
			depth[1]--;
		else if (*i == '}')
			depth[2]--;

		// Are we at the end of a record or process definition?
		if (depth[0] == 0 && depth[1] == 0 && depth[2] == 0 && *i == '}')
		{
			// Make sure this isn't vacuous
			if (i-j+1 > 0)
			{
				// Is this a process?
				if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "process ") == 0)
				{
					p = new process(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
					type_space.insert(pair<string, process*>(p->name, p));
				}
				// Is this an operator?
				else if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "operator") == 0)
				{
					o = new operate(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
					type_space.insert(pair<string, operate*>(o->name, o));
				}
				// This isn't a process, is it a record?
				else if (cleaned_chp.compare(j-cleaned_chp.begin(), 7, "record ") == 0)
				{
					r = new record(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
					type_space.insert(pair<string, record*>(r->name, r));
				}
				// Is it a channel definition?
				else if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "channel ") == 0)
				{
					c = new channel(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
					type_space.insert(pair<string, channel*>(c->name, c));
					type_space.insert(pair<string, operate*>(c->name + "." + c->send->name, c->send));
					type_space.insert(pair<string, operate*>(c->name + "." + c->recv->name, c->recv));
					type_space.insert(pair<string, operate*>(c->name + "." + c->probe->name, c->probe));
				}
				// This isn't either a process or a record, this is an error.
				else
				{
					error = "Error: CHP block outside of process.\nIgnoring block:\t";
					error_start = j-cleaned_chp.begin();
					error_len = min(min(cleaned_chp.find("process ", error_start), cleaned_chp.find("record ", error_start)), cleaned_chp.find("channel ", error_start)) - error_start;
					error += cleaned_chp.substr(error_start, error_len);
					cout << error << endl;
					j += error_len;

					// Make sure we don't miss the next record or process though.
					if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "process ") == 0)
					{
						p = new process(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
						type_space.insert(pair<string, process*>(p->name, p));
					}
					else if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "operator") == 0)
					{
						o = new operate(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
						type_space.insert(pair<string, operate*>(o->name, o));
					}
					else if (cleaned_chp.compare(j-cleaned_chp.begin(), 7, "record ") == 0)
					{
						r = new record(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
						type_space.insert(pair<string, record*>(r->name, r));
					}
					else if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "channel ") == 0)
					{
						c = new channel(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), &type_space, verbosity);
						type_space.insert(pair<string, channel*>(c->name, c));
						type_space.insert(pair<string, operate*>(c->name + "." + c->send->name, c->send));
						type_space.insert(pair<string, operate*>(c->name + "." + c->recv->name, c->recv));
						type_space.insert(pair<string, operate*>(c->name + "." + c->probe->name, c->probe));
					}
				}
			}
			j = i+1;
		}
	}

	vars.insert(variable("Reset", "int", 1, false));
	vars.insert(variable("_Reset", "int", 1, false));

	prgm = (parallel*)expand_instantiation(NULL, "main _()", &vars, NULL, "", verbosity, true);

	cout << vars << endl;

	prgm->print_hse();
	cout << endl;
}

/* TODO Projection algorithm - when do we need to do projection? when shouldn't we do projection?
 * TODO Process decomposition - How big should we make processes?
 * TODO There is a problem with the interaction of scribe variables with bubbleless reshuffling because scribe variables insert bubbles
 */
void program::generate_states()
{
	cout << "Generating State Space" << endl;

	space.append_state(state(value("X"), vars.global.size()), -1, "Power On");
	prgm->generate_states(&space, 0, state());
	space.gen_traces();
	prgm->generate_scribes();
	space.gen_deltas();
	space.gen_conflicts();

	if(STATESP_CO)
	{
		cout << vars << endl;
		cout << space << endl << endl;
		cout >> space << endl << endl;
		space.print_up();
		space.print_down();
		space.print_delta();
	}
	if(STATESP_GR)
	{
		space.print_dot();
	}
}

void program::insert_state_vars()
{
	size_t i, j, k;
	list<path>::iterator lp;
	vector<int>::iterator ci;
	path_space up_paths(space.size());
	path_space down_paths(space.size());
	path_space temp(space.size());

	int up_conflict_path_count[space.size()][space.size()];
	int down_conflict_path_count[space.size()][space.size()];
	vector<path_space>	up_cov(space.size());
	vector<path_space>  down_cov(space.size());
	path up_total;
	path down_total;
	path_space cov;
	int m, u, d;

	int benefit[space.size()][space.size()];
	trace trans_trace[space.size()][space.size()];

	// So Here is the trick. This works for conditionals, blocks, assignments, guards. This does not work for parallel, loop.
	// This is a two dimensional analysis (one up transition and one down transition). To get a one guard loop to work, you need at least
	// a four dimensional analysis (two up transitions, and two down transitions). To get a two guard loop to work, I assume you need 8.
	// So, 2^(1 + max number of loop guards) dimensional analysis for any given program. Furthermore, it thinks that it has to separate
	// branches of a parallel block with a state variable transition... This is mildly problematic because it cannot be solved by an
	// examination of the state space alone. To solve this problem, you need to examine the parse tree as well.
	//
	// This algorithms execution time balloons very quickly and becomes very very very fucking slow, BUT! it does indeed calculate the optimal
	// state variable insertion points and resulting trace. So... projection + process decomposition to keep the space we are looking at very small?
	// or find a less optimal, faster way? Also, ALL of this has to be iterated... making it very very slow.

	// The comments explain step by step details of this algorithm:

	// Step 1:

	// Calculate the number of conflict paths that would be added were state i to be an implicant for an up production rule of a state variable
	for (i = 0; i < space.up_conflicts.size(); i++)
		for (j = 0; j < space.up_conflicts[i].size(); j++)
		{
			temp.clear();
			temp.merge(space.get_paths(i, space.up_conflicts[i][j], path(space.size())));
			temp.merge(space.get_paths(space.up_conflicts[i][j], i, path(space.size())));

			up_conflict_path_count[i][space.up_conflicts[i][j]] = temp.paths.size();
		}

	// Calculate the number of conflict paths that would be added were state i to be an implicant for a down production rule of a state variable
	for (i = 0; i < space.down_conflicts.size(); i++)
		for (j = 0; j < space.down_conflicts[i].size(); j++)
		{
			temp.clear();
			temp.merge(space.get_paths(i, space.down_conflicts[i][j], path(space.size())));
			temp.merge(space.get_paths(space.down_conflicts[i][j], i, path(space.size())));

			down_conflict_path_count[i][space.down_conflicts[i][j]] = temp.paths.size();
		}

	// Step 2:

	// Calculate all of the conflict paths caused by current up production rule firings
	for (i = 0; i < space.up_firings.size(); i++)
		for (j = 0; j < space.up_firings[i].size(); j++)
			for (k = 0; k < space.up_conflicts[space.up_firings[i][j]].size(); k++)
				if (space.traces[i][space.up_conflicts[space.up_firings[i][j]][k]].data != "1")
				{
					up_paths.merge(space.get_paths(space.up_firings[i][j], space.up_conflicts[space.up_firings[i][j]][k], path(space.size())));
					up_paths.merge(space.get_paths(space.up_conflicts[space.up_firings[i][j]][k], space.up_firings[i][j], path(space.size())));
				}

	// Calculate all of the conflict paths caused by current down production rule firings
	for (i = 0; i < space.down_firings.size(); i++)
		for (j = 0; j < space.down_firings[i].size(); j++)
			for (k = 0; k < space.down_conflicts[space.down_firings[i][j]].size(); k++)
				if (space.traces[i][space.down_conflicts[space.down_firings[i][j]][k]].data != "0")
				{
					down_paths.merge(space.get_paths(space.down_firings[i][j], space.down_conflicts[space.down_firings[i][j]][k], path(space.size())));
					down_paths.merge(space.get_paths(space.down_conflicts[space.down_firings[i][j]][k], space.down_firings[i][j], path(space.size())));
				}

	// Step 3:

	// Generate a 2^(1 + max number of loop guards) dimensional grid with a side length equal to the number of states in the state space
	// Each element in this grid is a calculated benefit value that represents the total number of conflict paths eliminated were there
	// to be up transitions at (x0, y0, z0, ...) and down transitions at (x1, y1, z1, ...). The current implimentation only allows a two
	// dimensional grid, so (x0, x1).
	cout << "BENEFIT" << endl;
	m = 0;
	u = -1;
	d = -1;
	for (i = 0; i < (size_t)space.size(); i++)
	{
		up_cov[i] = up_paths.coverage(i);
		for (j = 0; j < (size_t)space.size(); j++)
		{
			if (i == 0)
				down_cov[j] = down_paths.coverage(j);

			// Number of conflicts that will be eliminated

			// This benefit value is equal to		the number of up conflict paths that pass through x0	+
			//										the number of down conflict paths that pass through x1	-
			//										the number of conflict paths that pass through both x0 and x1
			benefit[i][j] = up_paths.total[i] + down_paths.total[j] - up_cov[i].total[j] - down_cov[j].total[i];

			// Number of conflicts that will be added

			// Now we need to subtract the number of conflict paths that will be added because the state variable
			// transition implicants have conflicting states. We need to make sure that we don't over count this
			// subtraction value because some of those conflicting states can be vacuous firings.

			// First, calculate the trace were there to be transitions at (x0, x1)
			trans_trace[i][j] = space.get_trace(0, i, j, trace(value("_"), space.size()), value("X"));

			// Then, use that trace to check for vacuous firings when adding up conflict paths
			for (k = 0; k < space.up_conflicts[i].size(); k++)
				if (trans_trace[i][j][space.up_conflicts[i][k]].data != "1")
					benefit[i][j] -= up_conflict_path_count[i][space.up_conflicts[i][k]];

			for (k = 0; k < space.down_conflicts[j].size(); k++)
				if (trans_trace[i][j][space.down_conflicts[j][k]].data != "0")
					benefit[i][j] -= down_conflict_path_count[j][space.down_conflicts[j][k]];

			// Now we look for the max of this benefit value and VUALA
			if (benefit[i][j] > m)
			{
				u = i;
				d = j;
				m = benefit[i][j];
			}

			cout << benefit[i][j] << "\t";
		}
		cout << endl;
	}

	cout << endl << endl << m << " Conflicting Paths Eliminated" << endl << "Up: " << u << endl << "Down: " << d << endl;
	cout << trans_trace[u][d] << endl << endl;

	// Now we need to execute the change, recalculate, and reiterate.

	// Fuck that hurt my head.





	/*vector<vector<int> >::iterator confli;
	int sv_from, sv_to;
	bool sv_up;
	//Up
	//Go through the list of conflicts
	for(confli = space.up_conflicts.begin(); confli!= space.up_conflicts.end(); confli++)
	{
		//If the list of states confli conflicts with is not zero,
		if(confli->second.size() > 0)
		{
			//Chose the first one arbitrarily.
			//Select an edge
			if(confli->second[0] > confli->first)
			{
				cout << "I am inserting a state variable after " << confli->first << " and before " << confli->second[0] << endl;
				//I commondeered an edge.
				sv_from = 4;
				sv_to = 5;
				sv_up = false; // ?
				break;
			}
			else
			{
				cout << "I am inserting a state variable before " << confli->first << " and after " << confli->second[0] << endl;
				sv_from = 5;
				sv_to = 4;
				sv_up = true; // ?
				break;
			}
		}
	}//Confli
	//If we didn't find anything, do the same search for down.



	//We have now selected an edge to commondeer.
	//Add a new variable to our globals list
	//pair<string, instruction*> add_unique_variable(string prefix, string postfix, string type, vspace *vars, string tab, int verbosity)
	add_unique_variable("sv", "", "int<1>", &vars, "", -1);*/
	//Create a new state for SV go high (probably based off of the from of the commondeered edge)

	//Remove the edge between them.

	//Add the edge from sv_from to our new state

	//Add the edge from our new state to sv_to

	//Add the values into state space

	//Add an additional variable to the trace

	//Add the values into trace space

	//Recalculate diff space

	//Recalculate conflicts

	//Iterate until complete
}
//Fuck this code
/*
//Indistinguishable states before PRS?

//The below, or a totally crazy idea:
//Make a datastructure containing 'indistinguishable from's for each state
//Somehow shrink these lists so they don't matter anymore or something. Magic.
//Other thoughts: Would it be dumb to iterate through once with all implicants instead?

//Set up data structures
vector<trace> up_conflicts;
vector<trace> down_conflicts;

up_conflicts.resize(vars.global.size());
down_conflicts.resize(vars.global.size());
for(size_t i = 0; i < vars.global.size(); i ++)
{
	up_conflicts[i].values.resize(space.size());
	down_conflicts[i].values.resize(space.size());
}

cout << " SV up start " << endl;
//===== SEARCH FOR NEEDED UP UP ======
// === iterate through every rule's implicants
for(size_t rulei = 0; rulei < vars.global.size(); rulei++)
{
	cout << " rulei " << endl;
	// === For each implicant...
	for(size_t impi = 0; impi < prs[rulei].up_implicants.size(); impi++)
	{
		cout << " impi " << endl;
		//...iterate once through the state space.
		for(int statei = 0; statei < space.size(); statei++)
		{
			cout << " statei " << statei<< endl;
			//Write down if the state is an okay firing, conflict firing, or mandatory firing (vector<trace>)
			int weaker = who_weaker(prs[rulei].up_implicants[impi], space.states[statei]);
			//cout << "prs_up[rulei].implicants[impi]" << prs_up[rulei].implicants[impi] << " space.states[statei] " << space.states[statei] << endl;
			//It is supposed to fire here!
			cout << " who weaker: " << weaker << " prs_up[rulei].implicants[impi].tag " << prs[rulei].up_implicants[impi].tag << endl;
			if(statei == (prs[rulei].up_implicants[impi].tag))
				up_conflicts[rulei][statei].data = "!";
			//It doesn't fire here.
			else if(weaker == 0 || weaker == 2)
				up_conflicts[rulei][statei].data = "_";
			//It fires here. Should it?
			else if(weaker == -1 || weaker == 1)
			{
				//(space[statei][prs_up[rulei].uid]=="0"))
				up_conflicts[rulei][statei].data = "C";
			}
			else
				up_conflicts[rulei][statei].data = "?";
			cout << "bottom of loop " << endl;
		} //statei for
	}//impi for
}//rulei for

cout << "Up conflict traces:" << endl;
for (size_t i = 0; i < vars.global.size(); i++)
	cout<< prs[i].up << " " << up_conflicts[i] << endl;

// === Chose indices in state space to insert state variables
// === Insert these into the CHP and reparse? Is better way?



//===== SEARCH FOR NEEDED DOWN SV ======
// === iterate through every rule's implicants

cout << "Done state var insert" << endl;*/

void program::generate_prs()
{
	for (int vi = 0; vi < space.width(); vi++)
		if (vars.get_name(vi).find_first_of("|&~") == string::npos && vars.get_name(vi).find("Reset") == string::npos)
			prs.push_back(rule(vi, &space, &vars));

	print_prs();
}

/* TODO: Factoring - production rules should be relatively short.
 * Look for common expressions between production rules and factor them
 * out into their own variable
 */
void program::factor_prs()
{

}

void program::print_prs()
{
	cout << "Production Rules: " << endl;

	for (size_t i = 0; i < prs.size(); i++)
		cout << prs[i];
}

/*
 * Do not delete without Nicholas' consent!! He has not proven that the construction method
 * used during bottom up will yield the same results! In fact, he thinks there is a defininte
 * region of the design space where this is not the case.
void program::weaken_guard(rule pr)
{
	//Go through every implicant of the rule
	for(int impi = 0; impi < pr.implicants.size(); impi++)
	{
		//Go through every variable of the given implicant
		for(int vari = 0; vari < pr.implicants[impi].size(); vari++)
		{
			//proposal will be the given implicant missing the vari-th variable
			state proposal = pr.implicants[impi];
			proposal[vari].data = "X";
			//Compare this proposal to the whole state space
			bool not_needed = true;
			for(int spacei = 0; spacei < space.states.size(); spacei++)
			{
				int weaker = who_weaker(proposal,space.states[spacei]);
				//If the current state is weaker than our proposal, or they are the same...
				if(weaker == 0 || weaker == 2)
				{
					//Check if it is not allowed to fire here
					if((space.states[spacei][pr.uid] == "1"&& pr.up == false) || (space.states[spacei][pr.uid] == "0"&& pr.up == true))
						not_needed = false;
				}//if

			}//spaci for

		}//vari for
	}//impi for
}
*/

