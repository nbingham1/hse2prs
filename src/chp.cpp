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
	bool label = false;
	bool stream = false;
	bool check = false;
	bool unique = false;
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "--help", 6) == 0 || strncmp(argv[i], "-h", 2) == 0)			// Help
		{
			cout << "Usage: hse2states [options] file..." << endl;
			cout << "Options:" << endl;
			cout << " -h,--help\t\t\tDisplay this information" << endl;
			cout << "    --version\t\tDisplay compiler version information" << endl;
			cout << " -s,--stream\t\tPrint the results to standard out" << endl;
			cout << " -v,--verbose\t\tPrint all steps taken in simulation" << endl;
			cout << " -l,--label\t\t\tPlace the id of each node next to it as an external label" << endl;
			cout << " -u,--unique\t\t\tInsert state variable transitions to ensure that the resulting set of states have unique predicates" << endl;
			cout << " -c,--check\t\t\tDisplay states with conflicting predicates" << endl;
			return 0;
		}
		else if (strncmp(argv[i], "--version", 9) == 0)	// Version Information
		{
			cout << "chp2hse 1.0.0" << endl;
			cout << "Copyright (C) 2013 Sol Union." << endl;
			cout << "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;
			cout << endl;
			return 0;
		}
		else if (strncmp(argv[i], "--verbose", 9) == 0 || strncmp(argv[i], "-v", 2) == 0)
			set_verbose();
		else if (strncmp(argv[i], "--label", 6) == 0 || strncmp(argv[i], "-l", 2) == 0)
			label = true;
		else if (strncmp(argv[i], "--stream", 8) == 0 || strncmp(argv[i], "-s", 2) == 0)
			stream = true;
		else if (strncmp(argv[i], "--check", 7) == 0 || strncmp(argv[i], "-c", 2) == 0)
			check = true;
		else if (strncmp(argv[i], "--unique", 8) == 0 || strncmp(argv[i], "-u", 2) == 0)
			unique = true;
		else
			files.push_back(argv[i]);
	}

	if (files.size() > 0)
	{
		tokenizer tokens;

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

		dot_graph_cluster result;

		if (is_clean())
		{
			dot_graph_cluster cluster(tokens);

			for (size_t i = 0; i < cluster.graphs.size(); i++)
			{
				process p(tokens, cluster.graphs[i]);
				p.elaborate();
				if (unique && is_clean())
					p.solve();
				if (check && is_clean())
				{
					bool saved_verbosity = get_verbose();
					set_verbose();
					p.check();
					set_verbose(saved_verbosity);

				}
				if (is_clean())
					result.graphs.push_back(p.export_dot(label));
			}
			cluster.graphs.clear();
		}

		if (is_clean())
		{
			if (stream)
				result.print(cout);
			else
			{
				for (size_t i = 0; i < result.graphs.size(); i++)
				{
					if (result.graphs[i].stmt_list.stmts.size() > 0)
					{
						ofstream os((result.graphs[i].stmt_list.stmts[0].id + ".dot").c_str());
						result.graphs[i].print(os);
						os.close();
					}
				}
			}
		}

		complete();
	}
	else
	{
		cout << "Usage: hse2states [options] file..." << endl;
		cout << "Options:" << endl;
		cout << " -h,--help\t\t\tDisplay this information" << endl;
		cout << "    --version\t\tDisplay compiler version information" << endl;
		cout << " -s,--stream\t\tPrint the results to standard out" << endl;
		cout << " -v,--verbose\t\tPrint all steps taken in simulation" << endl;
		cout << " -l,--label\t\t\tPlace the id of each node next to it as an external label" << endl;
		cout << " -u,--unique\t\t\tInsert state variable transitions to ensure that the resulting set of states have unique predicates" << endl;
		cout << " -c,--check\t\t\tDisplay states with conflicting predicates" << endl;
	}
	return 0;
}
