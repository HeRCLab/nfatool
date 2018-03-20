#include "nfatool.h"

void find_sccs () {
  int *assigned,
      i,k,component_num,
      largest_component_size=0,largest_component;

  assigned=(int *)malloc(sizeof(int)*num_states);
  for (i=0;i<num_states;i++) assigned[i]=0;

  for (i=0;i<num_states;i++) if (start_state[i]) dfs(i);
  //dfs(0);

  for(int i=visited2.size()-1 ; i>=0; i--) {
    assign(visited2[i],visited2[i],components);
  }

  printf(" Visited_size = %d\n", (int)visited2.size()); 

  map <int,vector<int> > component_list;
  for (i=0;i<num_states;i++) {
    component_list[components[i]].push_back(i);
  }

  for (map<int,vector<int> >::iterator j=component_list.begin(); j!=component_list.end(); j++) {
    if ((*j).second.size() > largest_component_size) {
      largest_component = (*j).first;
      largest_component_size=(*j).second.size();
    }
    //printf("component %d: ",(*j).first);
    //for (k=0;k<(*j).second.size();k++) printf("%d ",(*j).second[k]);
    //printf ("\n");
  }

  printf ("largest component is %d (size=%d), members: ",largest_component,largest_component_size);
  for (k=0;k<largest_component_size;k++) printf("%d (%s) ",component_list[largest_component][k],
    node_table[component_list[largest_component][k]]->properties->children->content);
  printf ("\n");

  dump_dot_file((char *)"largest_scc",rootGlobal,component_list[largest_component]);

  free(assigned);
}

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
