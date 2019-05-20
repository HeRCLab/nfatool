#ifndef PARTITION_H
#define PARTITION_H

#define OVECCOUNT  30
#define STATEMAP_SIZE 150000

#include "list.h"

// new partitioning code
void find_subgraphs (nfa *my_nfa);
void traverse(nfa *my_nfa,int state,int subgraph);
void add_connected_stes (int ste,vector<int> &members,int **graph,int max_edges);
void partition (int max_partition_size);
void merge_colors(int color1,int color2,vector <int> *color_membership, vector <int> &virtual_root_colors);
int find_lowest_pure_node(int color,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors);
vector <int> find_outgoing_edges (vector <int> scc);
vector <int> find_incoming_edges (vector <int> scc);
void split_colors (int ste, vector <int> *color_membership,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors);
void replace_color (int ste, int original_color, int new_color, vector <int> *color_membership,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors);
void reverse_replace_color (int ste, int orginal_color, int new_color_start, int new_color_end, vector <int> *color_membership);

// miscellaneous functions for bounding the partition size
void find_critical_path ();
void dfs_critical (int ste,int &depth,int &deepest,vector <int> &deepest_path,vector <int> &path);

#endif


