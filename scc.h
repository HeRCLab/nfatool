#ifndef SCC_H
#define SCC_H

void find_sccs(); 
void dfs(int current_node);
void assign(int u,int root,int *components);

#endif
