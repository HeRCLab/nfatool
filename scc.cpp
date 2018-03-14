#include "nfatool.h"

void dfs(int current_node) {
    int flag,visited_node,in_loop=0;

    for (int i=0;i<visited2.size();i++) if (current_node == visited2[i]) return;
    visited2.insert(visited2.begin(),current_node);

    


     // ------------------------------------------------------------------      
     // Start going through all the edges of the current_node
        for(int j=0; j<max_edges; j++){
                //std::cout << "edge_table of current node" << current_node << " " << edge_table[current_node][j] << endl;
    
        if (edge_table[current_node][j]!=-1) {
            // Test if the edge in the final set, means no cycle found  
        // test the looop ??? ?
          visited_node=0;
                for(int k=0; k<visited2.size(); k++)
                    if(edge_table[current_node][j] == visited2[k]) {
                        visited_node=1;
                        break;
            }
                    
          if (!visited_node) dfs(edge_table[current_node][j]);
        }
  }
  

  return;
}

void assign(int u,int root,int *components) {
  int i;
  if (components[u]==-1) {
    components[u] = root;

    for (i=0;i<max_fan_in;i++) {
      if (reverse_table[u][i]==-1) break;

      assign(reverse_table[u][i],root,components);
    }
  }
}
