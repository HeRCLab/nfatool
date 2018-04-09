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
using namespace std;

#include "partition.h"
#include "parse_anml.h"
#include "visualization.h"
#include "allocate_stes.h"
#include "scc.h"
#include "allocate_memory.h"
#include "strong_components_analysis.h"

#define OVECCOUNT       30
#define STATEMAP_SIZE   150000
#define MAX_COLORS      100000
#define MAX_PART_SIZE   1024

#define ANML_NAME(ste_num)  node_table[ste_num]->properties->children->content

extern unsigned char **next_state_table;

extern int total_visited2_size;

extern int **edge_table;
extern int **orig_edge_table;
extern int **reverse_table;
extern int **FanoutTable;

extern int *movement_map;

extern unsigned char *report_table;
extern unsigned char *start_state;
extern unsigned char *symbol_set;

extern unsigned int *subnfa;
extern char *visited;

extern int subnfa_size;
extern int subnfa_total_size;

extern map <std::string,int> state_map;

extern int duplicate_states;
extern int subnfa_num;

extern int state_map2[STATEMAP_SIZE];

extern xmlNode **node_table;

extern int max_fanout,max_stes;

extern vector<int> visited2;

extern bool cycle_confirm;

extern int *dfs_visited;
extern int max_loop;
extern int max_loop_constituent;

extern int num_states,
    STEinFile,
    file_state,
    states,/*< counter for state-transition-element used in function "count_states" */
    starts, /*< counter for start_stes table */
    max_edges, /*< highest amount of activate-on-match a state-transition-element could have */
    edges, /*< counter for activate-on-match */
    edge_num,
    files_generated, /*<counter for file number printed*/
    num_reports, /*< counter for report-on-match */
    Change_of_start, /*< counter for state-transition-element that holds a start state */
    max_fan_in,
    current_color,
    cycle_count,
    start_color,
    max_path,
    path_length,
    path_compare,
    possinle_end_of_cycle,
    start_of_cycle,
    potential_node_in_cycle,
    possible_end_of_cycle,
    max_strong_node,
    color_count[MAX_COLORS];


extern xmlNode *rootGlobal, /*< root node of the .ANML file */
        *start_stes[STATEMAP_SIZE];

extern FILE *Destination;

extern xmlDoc *document; /*< .ANML document that will be used on the program */

extern int *ExactOutEdge;
extern int *visitedcycle;
extern int *visitedColorTrav;

extern vector<int> *state_colors;
extern vector<int> *sccs;
extern map <int,vector<int> > component_list;
extern vector<int> strong_cycles;

extern int largest_component_size,largest_component;

extern int *components;

<<<<<<< HEAD
extern vector<int> deepest_path;
extern int deepest;
=======
extern int largest_component;
>>>>>>> b8d5532d728aaebf8684615e250370346f9f323d

#endif
