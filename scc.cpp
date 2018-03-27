#include "nfatool.h"

void find_sccs () {
  int i,k,component_num,
      largest_component_size=0,largest_component;

  for (i=0;i<num_states;i++) visited[i]=0;

  dfs(0,1);
/*
  for(int i=0;i<visited2.size();i++) {
    assign(visited2[i],visited2[i],components);
  }
*/

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
#ifdef DEBUG
  for (k=0;k<largest_component_size;k++) printf("%d (%s) ",component_list[largest_component][k],
   node_table[component_list[largest_component][k]]->properties->children->content);
#endif
  printf ("\n");


  dump_dot_file((char *)"largest_scc",rootGlobal,component_list[largest_component]);
}

void reset_dfs_visited_flags() {
  for (int i=0;i<num_states;i++) dfs_visited[i]=0;
}

void update_dfs_visited (int current_node) {
  for (int i=0;i<num_states;i++) if (dfs_visited[i]) dfs_visited[i]++;
  dfs_visited[current_node]=1;
}

void decrement_dfs_visited () {
  for (int i=0;i<num_states;i++) if (dfs_visited[i]) dfs_visited[i]--;
  //dfs_visited[current_node]=1;
}

int visited_size () {
  int size = 0;
  for (int i=0;i<num_states;i++) if (visited[i]) size++;
  return size;
}

void dfs(int current_node,int start) {
    int flag,visited_node,in_loop=0,i;

    int root_nodes=0;

    if (start) {
      /*
       * ROOT NODES SECTION
       */

      /*
       * LOOK FOR REAL ROOT NODES
       */
        for (i=0;i<num_states;i++) {

          if (start_state[i]) {
           	if ((reverse_table[i][0]==-1) || ((reverse_table[i][0]==i) && (reverse_table[i][1] == -1))) {
            		root_nodes++;
                reset_dfs_visited_flags();
            		dfs(i,0);
                for(int i=0;i<visited2.size();i++) {
                  assign(visited2[i],visited2[i],components);
                }
                visited2.clear();

          	} else {
#ifdef DEBUG
			         fprintf(stderr,"WARNING!  STE %d (\"%s\") is marked as start STE in ANML but is not a root node (has non-self incoming edges)!\n",i,ANML_NAME(i));
#endif
		        }
          }
        }

        printf("ANML root nodes = %d\n",root_nodes);

        /*
         * FALLBACK MODE:  SELECT IMAGINARY ROOT NODES TO ACHIEVE 100% COVERAGE
         */
        if (root_nodes==0) {
          printf ("ANML file is rootless, going with plan B...\n");
          // we didn't find any root nodes.  initiate continguency plan where we randomly select a root node
          while (visited_size() < num_states) {
            // pick a "root" node and proceed with DFS
            for (i=0;i<num_states;i++) if (!visited[i]) break;
            printf ("selecting arbitrary root STE %d\n",i);
            reset_dfs_visited_flags();
            dfs(i,0);
            printf ("yield = %d/%d STEs\n",visited_size(),num_states);
            for(int i=0;i<visited2.size();i++) {
              assign(visited2[i],visited2[i],components);
            }
            visited2.clear();
          }
         }
        
        return;

    } else if (!visited[current_node]) {
      /*
       * RECURSION SECTION
       */

      visited[current_node]=1;
      update_dfs_visited(current_node);

    //for (int i=0;i<visited2.size();i++) if (current_node == visited2[i]) return;

     // ------------------------------------------------------------------      
     // Start going through all the edges of the current_node
        for(int j=0; j<max_edges; j++){
                //std::cout << "edge_table of current node" << current_node << " " << edge_table[current_node][j] << endl;
    
        if (edge_table[current_node][j]==-1) break;
            // Test if the edge in the final set, means no cycle found  
        // test the looop ??? ?
          /*
		visited_node=0;
                for(int k=0; k<visited2.size(); k++)
                    if(edge_table[current_node][j] == visited2[k]) {
                        visited_node=1;
                        break;
            	    }
            */        
          //if (!visited_node) dfs(edge_table[current_node][j],0);
		dfs(edge_table[current_node][j],0);
        }
  
    visited2.insert(visited2.begin(),current_node);

    decrement_dfs_visited();
    return;
  } else { // visited == 1
    /*
     * LOOP ANALYSIS SECTION
     * /

    // loop potentially detected!  bail out, but first gather some info on the loop
    if (dfs_visited[current_node]) {
      // loop confirmed, find loop size!
      /*printf ("loop found (\"%s\" to \"%s\", length = %d\n",ANML_NAME(current_node),
                                                            ANML_NAME(current_node),
                                                            dfs_visited[current_node]+1);*/
      if ((dfs_visited[current_node]) > max_loop) max_loop=dfs_visited[current_node];
      decrement_dfs_visited();
      return;
  }
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
