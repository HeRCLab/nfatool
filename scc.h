#ifndef SCC_H
#define SCC_H

void find_sccs(); 
void dfs(int current_node,int start);
void assign(int u,int root,int *components);
void reset_dfs_visited_flags();
void update_dfs_visited (int current_node);

#endif
