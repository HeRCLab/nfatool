#include "nfatool.h"

void find_sccs () {
  int i,k,component_num,
      largest_component_size=0,largest_component;

  for (i=0;i<num_states;i++) visited[i]=0;

  dfs(0,1);

  for(int i=0;i<visited2.size();i++) {
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
}

void dfs(int current_node,int start) {
    int flag,visited_node,in_loop=0,i;

    int start_states=0;

    if (start) {
        for (i=0;i<num_states;i++) 

          if (start_state[i]) {
//		printf("state[%d] = %d\n", i, node_table[i]); 
          	if ((reverse_table[i][0]==-1) || (reverse_table[i][0]==i) && (reverse_table[i][1] == -1)) {
            		//visited2.insert(visited2.begin(),i);
            		start_states++;
            		dfs(i,0);
          	} else {
			fprintf(stderr,"WARNING!  STE %d (\"%s\") is marked as start STE in ANML but is not a root node (has non-self incoming edges)!\n",i,node_table[i]->properties->children->content);
		}
	  }
          printf("root nodes = %d\n",start_states);
    } else if (!visited[current_node]) {

      visited[current_node]=1;


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
