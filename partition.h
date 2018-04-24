#ifndef PARTITION_H
#define PARTITION_H

#define OVECCOUNT  30
#define STATEMAP_SIZE 150000

#include "list.h"

// new partitioning code
void partition (int max_partition_size);
void merge_colors(int color1,int color2,struct jlist *color_membership);
int find_lowest_pure_node(int color,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors);
vector <int> find_outgoing_edges (vector <int> scc);
vector <int> find_incoming_edges (vector <int> scc);
void split_colors (int ste, struct jlist *color_membership,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors);
void replace_color (int ste, int original_color, int new_color, struct jlist *color_membership,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors);
void reverse_replace_color (int ste, int orginal_color, int new_color_start, int new_color_end, struct jlist *color_membership);

// original versions of the partitioning functions
void traverse_partition(int ste);
void split_colorv2(int ste, int color);
void replace_colorv2(int str, int old_color, int new_color);
void becchi_partition ();
void max_color_size (int &max_color_size,int &max_color);

// miscellaneous functions for bounding the partition size
void find_critical_path ();
void dfs_critical (int ste,int &depth,int &deepest,vector <int> &deepest_path,vector <int> &path);

#endif


