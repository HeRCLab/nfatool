#ifndef SCC_H
#define SCC_H

void find_sccs(); 
void dfs(int current_node,int start,int allow_weird_start_states);
void assign(int u,int root,int *components);
void reset_dfs_visited_flags();
void update_dfs_visited (int current_node);
int find_loop_path (int target,int ste,vector <int> &path,int start);

#endif
