#ifndef PARTITION_H
#define PARTITION_H

#define OVECCOUNT  30
#define STATEMAP_SIZE 150000

#include "list.h"

// new partitioning code
int partition_graph (nfa *my_nfa,int subgraph,int max_fanout);
void find_subgraphs (nfa *my_nfa);

#endif
