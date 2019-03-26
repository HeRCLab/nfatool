#ifndef NFATOOL_H
#define NFATOOL_H

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <pcre.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <map>
#include <iostream>
#include <cassert>
#include <vector>
#include <list>
#include <deque>
#include <queue>

using namespace std;

#define OVECCOUNT       30
#define STATEMAP_SIZE   150000
#define MAX_COLORS      10000
#define MAX_PART_SIZE   1024

#define	SAT_SOLVER_COMMAND	"/usr/bin/docker run --rm -i msoos/cryptominisat"
#define ANML_NAME(ste_num)  node_table[ste_num]->properties->children->content

// NFA graph
typedef struct {
	int						**edge_table;
	int						**orig_edge_table;
	int						**reverse_table;
	char					**symbol_table;
	int						*report_table;
	unsigned char			*start_state;
	unsigned char			*symbol_set;
	map <string,int>		state_map;
	map <int,string>		state_map2;
	int						max_fanout;
	int						max_stes;
	int						max_fan_in;
	int						max_edges;
	int						*movement_map;
	xmlNode					*root;
	xmlDoc					*document;
	int						edge_num;
	int						num_states;
	int						num_reports;
	xmlNode					**node_table;
	unsigned char			**next_state_table;
	int						**gates_2D; 
	int						*gates_1D; 
	int						**next_2D;
	int						do_time;
	xmlNode					*start_stes[STATEMAP_SIZE];
	char					*visited;
	int						*dfs_visited;
	vector<int>				*state_colors;
	int						color_count[MAX_COLORS];
	int						*root_node;
	vector<int>				*sccs;
	map <int,vector<int> >	component_list;
	int						*components;
	
} nfa;

#include "partition.h"
#include "parse_anml.h"
#include "visualization.h"
#include "allocate_stes.h"
#include "scc.h"
#include "allocate_memory.h"
#include "list.h"
#include "v2.h"
#include "gen_cnf.h"
 
#include "do_next.h" 
#include "do_gates.h"

/*
 * global variabless
 */
 
/*
 * NFA data (used in multiple places)
 */
extern int						**edge_table;
extern int						**orig_edge_table;
extern int						**reverse_table;
extern char						**symbol_table;
extern unsigned char			*report_table;
extern unsigned char			*start_state;
extern unsigned char			*symbol_set;
extern map <std::string,int>	state_map;
extern int						state_map2[STATEMAP_SIZE];
extern int						max_fanout;
extern int						max_stes;
extern int						max_fan_in;
extern int						max_edges;

/*
 * NAPOLY configuration data
 */
extern unsigned char			**next_state_table;
extern int						**gates_2D; 
extern int						*gates_1D; 
extern int						**next_2D;
extern int						do_time;

/*
 * allocation/mapping algorithm
 */
extern int						*movement_map;

/*
 * Used in strong components analysis (why is it global?)
 */
extern bool						cycle_confirm;

/*
 * Used in early code to find distinct sub-graphs; no longer used and should
 * delete
 */
extern unsigned int				*subnfa;
extern int						subnfa_size;
extern int						subnfa_total_size;
extern int						subnfa_num;

/*
 * Used for partitioning and strongly connected component analysis
 */
extern char						*visited;
extern vector<int>				visited2;
extern vector<int>				strong_cycles;
extern vector<int>				*state_colors;
extern vector<int>				*sccs;
extern map <int,vector<int> >	component_list;
extern int						*components;
extern int						largest_component_size;
extern int						largest_component;
extern int						current_color;
extern int						cycle_count ;
extern int						start_color;
extern int						path_compare;
extern int						path_length;
extern int						max_path;
extern int						Path_compare;
extern int						start_of_cycle;
extern int						potential_node_in_cycle;
extern int						possible_end_of_cycle;
extern int						max_strong_node;
extern vector<int>				deepest_path;
extern int						deepest;
extern int						max_reverse_edges;
extern int						color_count[MAX_COLORS];
extern int						max_fan;
extern int						*dfs_visited;
extern int						max_loop;
extern int						max_loop_constituent;
extern int						*root_node;

/*
 * Used in parsing ANML
 */
extern xmlNode					**node_table;
extern int						edges;
extern int						states;
extern int						file_state;

/*< root node of the .ANML file */
extern xmlNode					*rootGlobal;
extern xmlNode					*start_stes[STATEMAP_SIZE];

/*< .ANML document that will be used on the program */
extern xmlDoc					*document;
extern int						edge_num;
extern int						num_states;
extern int						num_reports;

/*< counter for report-on-match */    
extern int						Change_of_start;

/*< counter for state-transition-element that holds a start state */
extern int						starts;

#endif
