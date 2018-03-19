#ifndef PARTITION_H
#define PARTITION_H

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

void find_critical_path(); 
void dfs_critical (int ste,int &depth,int &deepest,vector <int> &deepest_path,vector <int> &path) ;
void traverse_partition(int ste);
void split_colorv2(int ste, int color);
void replace_colorv2(int str, int old_color, int new_color);
void becchi_partition ();
void max_color_size (int &max_color_size,int &max_color);

#endif
