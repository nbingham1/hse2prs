/*
 * chp.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "common.h"
#include "tokenizer.h"
#include "process.h"
#include "message.h"

int main(int argc, char **argv)
{
	vector<string> files;
	bool stream = false;
	int  num_steps = 5;
	bool all = false;
	for (int i = 1; i < argc; i++)
	{
		string arg = argv[i];
		if (arg == "--help" || arg == "-h")			// Help
		{
			cout << "Usage: hse2prs [options] file..." << endl;
			cout << "Options:" << endl;
			cout << " -h,--help\t\t\tDisplay this information" << endl;
			cout << "    --version\t\tDisplay compiler version information" << endl;
			cout << " -s,--stream\t\tPrint the results to standard out" << endl;
			cout << " -v,--verbose\t\tPrint all steps taken in simulation" << endl;
			cout << endl;
			cout << " -e,--elaborate\t\tStop after elaborating the state space" << endl;
			cout << " -c,--check\t\t\tStop after checking for conflicting states" << endl;
			cout << " -u,--unique\t\t\tStop after inserting state variables" << endl;
			cout << " -p,--prs\t\tStop after generating production rules" << endl;
			cout << " -b,--bubble\t\tStop after generating the bubble graph" << endl;
			cout << " -a,--all\t\tExport all of the intermediary steps as well" << endl;
			return 0;
		}
		else if (arg == "--version")	// Version Information
		{
			cout << "hse2prs 1.0.0" << endl;
			cout << "Copyright (C) 2013 Sol Union." << endl;
			cout << "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;
			cout << endl;
			return 0;
		}
		else if (arg == "--verbose" || arg == "-v")
			set_verbose();
		else if (arg == "--stream" || arg == "-s")
			stream = true;
		else if (arg == "--elaborate" || arg == "-e")
			num_steps = min(num_steps, 0);
		else if (arg == "--check" || arg == "-c")
			num_steps = min(num_steps, 1);
		else if (arg == "--unique" || arg == "-u")
			num_steps = min(num_steps, 2);
		else if (arg == "--prs" || arg == "-p")
			num_steps = min(num_steps, 3);
		else if (arg == "--bubble" || arg == "-b")
			num_steps = min(num_steps, 4);
		else if (arg == "--all" || arg == "-a")
			all = true;
		else
			files.push_back(arg);
	}

	tokenizer tokens;

	if (files.size() > 0)
	{
		for (size_t i = 0; i < files.size(); i++)
		{
			ifstream fin;
			fin.open(files[i].c_str(), ios::binary | ios::in);
			if (!fin.is_open())
			{
				error("", "file not found '" + files[i] + "'", "", __FILE__, __LINE__);
				return false;
			}
			else
			{
				fin.seekg(0, fin.end);
				size_t size = fin.tellg();
				string buffer(size, ' ');
				fin.seekg(0, fin.beg);
				fin.read(&buffer[0], size);
				fin.close();
				tokens.insert(files[i], buffer);
			}
		}
	}
	else
	{
		string pipe = "";
		string line = "";
		while (getline(cin, line))
			pipe += line + "\n";

		tokens.insert("stdin", pipe);
	}

	if (is_clean())
	{
		dot_graph_cluster cluster(tokens);

		for (size_t i = 0; i < cluster.graphs.size(); i++)
		{
			process p(tokens, cluster.graphs[i]);
			string name = p.nets.front().first.name;
			p.generate_translations();

			if (is_clean())
			{
				p.elaborate();

				if (num_steps == 0 || all)
				{
					if (stream)
						p.export_dot().print(cout);
					else
					{
						ofstream os((name + "_dot").c_str());
						p.export_dot().print(os);
						os.close();
					}
				}
			}

			if (num_steps > 0 && is_clean())
			{
				p.check();

				if (num_steps == 1 || all)
					p.print_conflicts();
			}

			if (num_steps > 1 && is_clean())
			{
				p.solve();

				if (num_steps == 2 || all)
				{
					p.print_conflicts();

					if (stream)
						p.export_dot().print(cout);
					else
					{
						ofstream os((name + "_unique.dot").c_str());
						p.export_dot().print(os);
						os.close();
					}
				}
			}

			if (num_steps > 2 && is_clean())
			{
				p.generate_prs();
				if (num_steps == 3 || all)
				{
					if (stream)
						p.export_prs(cout);
					else
					{
						ofstream os((name + "_invalid.prs").c_str());
						p.export_prs(os);
						os.close();
					}
				}
			}

			if (num_steps > 3 && is_clean())
			{
				p.generate_bubble();
				if (num_steps == 4 || all)
				{
					if (stream)
						p.export_bubble().print(cout);
					else
					{
						ofstream os((name + "_bubble_graph.dot").c_str());
						p.export_bubble().print(os);
						os.close();
					}
				}
			}

			if (num_steps > 4 && is_clean())
			{
				p.bubble_reshuffle();
				if (num_steps == 5 || all)
				{
					if (stream)
						p.export_prs(cout);
					else
					{
						ofstream os((name + ".prs").c_str());
						p.export_prs(os);
						os.close();
					}
				}
			}
		}
		cluster.graphs.clear();
	}

	complete();

	return 0;
}
