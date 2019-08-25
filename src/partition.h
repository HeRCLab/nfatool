#ifndef PARTITION_H
#define PARTITION_H

#define OVECCOUNT  30
#define STATEMAP_SIZE 150000

#include "list.h"

// new partitioning code
int partition_graph (nfa *my_nfa,int subgraph,int max_fanout);
void find_subgraphs (nfa *my_nfa);
int num_paths(nfa *my_nfa, int node);
int num_loops(nfa *my_nfa, int node);
void find_all_paths(nfa *my_nfa, int src,bool visited_path2[], int &num_path); 
void find_loops(nfa *my_nfa, int src,bool visited_path2[], int &loop_size, int &num_loops, int &num_self_loops); 
#endif
