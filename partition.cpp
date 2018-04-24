#include "nfatool.h"

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

using namespace std;

void find_critical_path () {
        int i,depth=0;
        vector<int> path;
        
        for (i=0;i<num_states;i++) visited[i]=0;

        for (i=0;i<num_states;i++) {
                if (start_state[i]) {
                        dfs_critical(i,depth,deepest,deepest_path,path);
                }
        }

        //free(visited);
}

void dfs_critical (int ste,int &depth,int &deepest,vector <int> &deepest_path,vector <int> &path) {
        int i;

        if (visited[ste]) return;
        path.push_back(ste);
        visited[ste]=1;
        depth++;

        if (depth > deepest) {
                deepest = depth;
                deepest_path = path;
        }

        for (i=0;i<max_edges;i++) {
                if (edge_table[ste][i]==-1) break;
                dfs_critical(edge_table[ste][i],depth,deepest,deepest_path,path);
        }
        
	path.pop_back();
    depth--;
}


void traverse_partition(int ste)
{

  if (visitedColorTrav[ste] == 1)
  {
    return; 
  }
  else if(ste == -1)
  {
    return;
  }
  else

    printf("The STE is: %d\n", ste);

   visitedColorTrav[ste] = 1;

   state_colors[ste].push_back(current_color);

   color_count[current_color]++;

   for (int i = 0; i < num_states; i++)
   {
     for (int j = 0; j < max_edges; j++)
     {
       if (edge_table[i][j] == -1)
       {
         break;
       }
       else if ((edge_table[i][j] == ste) && (i == start_color))
       {
         current_color++;
         break;
       }
     }
   }

   for (int i = 0; i < max_edges; i++)
   {

      traverse_partition(edge_table[ste][i]);

   }

} 

void partition (int max_partition_size) {
	int i,j,max_color_membership,color_to_split;
	//map <int,vector <int> > color_membership;
//	vector <int> v = new vector<int>(10); 
	vector <int>  color_membership[10000]; // new vector<int>();
	vector <int> virtual_root_colors;
	vector <int> virtual_root_edges;//,virtual_root_colors;
	char str[1024];
	
	// initialize colors and set up virtual root
	virtual_root_colors.push_back(0);
	color_membership[0].push_back(-1); // -1 is stand-in for virtual root
	for (i=0;i<num_states;i++) {
		state_colors[i].push_back(0);
		color_membership[0].push_back(i);
		if (root_node[i]) virtual_root_edges.push_back(i); // create outgoing edges from virtual root
	}
	
	// perform first check for partition violates
	max_color_membership=0;
	for (i=0;i<current_color;i++) if (color_membership[i].size() > max_color_membership) {
		max_color_membership=color_membership[i].size();
		color_to_split=i;
	}
	
	int coloring = 0;
	vector<int> myemptyvector;
	while (max_color_membership > max_partition_size) {
		// dump the dot file
		sprintf (str,"coloring%d",coloring++);
		dump_dot_file (str,rootGlobal,myemptyvector,1);
		
		int ste_to_split = find_lowest_pure_node(color_to_split,virtual_root_edges,virtual_root_colors);
		if (ste_to_split==-2) {
			fprintf (stderr,"ERROR:  cannot find pure color %d node!\n",color_to_split);
			exit(0);
		}
		split_colors(ste_to_split,color_membership,virtual_root_edges,virtual_root_colors);
		
		max_color_membership=0;
		for (i=0;i<current_color;i++) if (color_membership[i].size() > max_color_membership) {
			max_color_membership=color_membership[i].size();
			color_to_split=i;
		}
	}
	
	// consolidation step
	int merged;
	do {		
		merged=0;
		// consolidate
		for (i=0;i<current_color;i++) {
			for (j=i+1;j<current_color;j++) {
				if ((color_membership[i].size() + color_membership[j].size()) <= max_partition_size) {
					merge_colors(i,j,color_membership);
					merged=1;
				}
			}
		}
	} while (merged);
}

void merge_colors(int color1,int color2,vector <int> *color_membership) {
	int i;
	
	for (i=0;i<color_membership[color2].size();i++) {
		int ste = color_membership[color2][i];
		color_membership[color1].push_back(ste);
		state_colors[ste].push_back(color1);
		for (vector<int>::iterator j = state_colors[ste].begin() ; j != state_colors[ste].end() ; j++) {
			if (*j==color2) state_colors[ste].erase(j);
			break;
		}
	}
	color_membership[color2].clear();
}

int find_lowest_pure_node(int color,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors) {
	queue <int> myqueue;
	int i,found;
	
	myqueue.push(-1); // stand-in for virtual root
	
	while (myqueue.size()) {
		int dequeued = myqueue.front();
		myqueue.pop();
		int num_colors = dequeued == -1 ? virtual_root_colors.size() : state_colors[dequeued].size();
		int first_color = dequeued == -1 ? virtual_root_colors[0] : state_colors[dequeued][0];
		
		if (num_colors==1 && first_color==color) return dequeued;
		
		if (dequeued==-1)
			for (i=0;i<virtual_root_edges.size();i++) myqueue.push(virtual_root_edges[i]);
		else
			for (i=0;i<max_edges;i++) if (edge_table[dequeued][i]==-1) break; else myqueue.push(edge_table[dequeued][i]);
	}
	
	return -2; // cannot find appropriate STE
}

// This function finds all outgoing edges from an SCC
vector <int> find_outgoing_edges (vector <int> scc) {
	vector <int> edges;
	int i,j,k,found;
	
	for (i=0; i<scc.size(); i++) {
		for (j=0;j<max_edges;j++) {
			if (edge_table[i][j]==-1) break;
			
			found=0;
			for (k=0;k<scc.size();k++) if (edge_table[i][j]==scc[k]) found=1;
			if (!found) edges.push_back(edge_table[i][j]);
		}
	}
	
	return edges;
}

// This function finds all outgoing edges from an SCC
vector <int> find_incoming_edges (vector <int> scc) {
	vector <int> edges;
	int i,j,k,found;
	
	for (i=0; i<scc.size(); i++) {
		for (j=0;j<max_fan_in;j++) {
			if (reverse_table[i][j]==-1) break;
			
			found=0;
			for (k=0;k<scc.size();k++) if (reverse_table[i][j]==scc[k]) found=1;
			if (!found) edges.push_back(reverse_table[i][j]);
		}
	}
	
	return edges;
}

void split_colors (int ste, vector <int> *color_membership,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors) {
	int i,k,
		original_color,
		old_current_color=current_color;
	
	// clear visitied to avoid inifinite loops
	for (i=0;i<num_states;i++) visited[i]=0;
		
	if (ste!=-1) {
		// not virtual root
		
		vector<int> ste_colors=state_colors[ste];
		if (ste_colors.size() != 1) {
			fprintf(stderr,"ERROR:  split_colors() called on STE %d with %d colors (should be 1).\n",ste,(int)ste_colors.size());
			exit(0);
		}
		
		original_color = ste_colors[0];
	
		// STEP 1:  delete the original color from ste and its fellow SCC members
		int scc_num = components[ste]; // find component number
		vector <int> &scc_contents = component_list[scc_num]; // find members of that component
		for (k=0;k<scc_contents.size();k++) {
			int fellow_scc_member = scc_contents[k];
			// remove the color from the list of colors for each node in the SCC
			for (std::vector<int>::iterator j = state_colors[fellow_scc_member].begin() ; j != state_colors[fellow_scc_member].end() ; j++) {
				if (*j == original_color) {
					state_colors[fellow_scc_member].erase(j);
					break;
				}
			}
		}
		// STEP 2:  replace original color with a new color on each outgoing edge
		vector <int> outgoing_edges = find_outgoing_edges(scc_contents);
		
		for (i=0;i<outgoing_edges.size();i++) {
			for (k=0;k<scc_contents.size();k++) state_colors[scc_contents[k]].push_back(current_color); // note: current_color is a new color!
			int dest_node = outgoing_edges[i];
			replace_color(dest_node,original_color,current_color,color_membership,virtual_root_edges,virtual_root_colors);
			current_color++;
		}
		
		// clear visited to avoid inifinite loops
		for (i=0;i<num_states;i++) visited[i]=0;
		
		// STEP 3:  recurse backward to root
		vector <int> incoming_edges = find_incoming_edges(scc_contents);
		for (k=0;k<incoming_edges.size();k++) {
			reverse_replace_color(incoming_edges[k],original_color,old_current_color,current_color,color_membership);
		}
		
	} else {
		// virtual root
		
		vector <int> &ste_colors=virtual_root_colors;
		if (ste_colors.size() != 1) {
			fprintf(stderr,"ERROR:  split_colors() called on STE %d with %d colors (should be 1).\n",ste,(int)ste_colors.size());
			exit(0);
		}
		
		original_color = ste_colors[0];
		
		// STEP 1:  delete the original color from ste and its fellow SCC members
		for (std::vector<int>::iterator j = virtual_root_colors.begin() ; j != virtual_root_colors.end() ; j++) {
			if (*j == original_color) {
				virtual_root_colors.erase(j);
				break;
			}
		}
		
		// STEP 2:  replace original color with a new color on each outgoing edge
		vector <int> &outgoing_edges = virtual_root_edges;
//		virtual_root_colors.push_back(current_color); 
		for (i=0;i<outgoing_edges.size();i++) {
	//		for(std::vector<int>::iterator j=virtual_root_colors.begin(); j!=virtual_root_colors.end(); j++)
	//		virtual_root_colors.erase(j); 
			virtual_root_colors.push_back(current_color); // note: current_color is a new color!
			int dest_node = outgoing_edges[i];
			color_membership[current_color].push_back(ste);
			replace_color(dest_node,original_color,current_color,color_membership,virtual_root_edges,virtual_root_colors);
			current_color++;
		}
		
	}
	
	// clear the original color
	color_membership[original_color].clear();

}

void replace_color (int ste, int original_color, int new_color, vector <int> *color_membership,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors) {
	int i,j;
	
	if (ste!=-1) {
		if (visited[ste]) return;
		visited[ste]=1;
	}
	
	if (ste==-1) {
		//  virtual root
		
		for (std::vector<int>::iterator j = virtual_root_colors.begin() ; j != virtual_root_colors.end() ; j++) { // for each color (iterator)
			if (*j == original_color) {
				virtual_root_colors.erase(j);
				break;
			}
		}
		virtual_root_colors.push_back(new_color);
		struct jlist &colors = color_membership[new_color];
//		colors.push_back(ste);
//		&color_membership.push_back(ste); 
		for (i=0;i<virtual_root_edges.size();i++) { // for each edge
			replace_color (virtual_root_edges[i],original_color,new_color,color_membership,virtual_root_edges,virtual_root_colors);
		}
		
	} else {
		// not virtual root
		
		for (std::vector<int>::iterator j = state_colors[ste].begin() ; j != state_colors[ste].end() ; j++) { // for each color (iterator)
			if (*j == original_color) {
				state_colors[ste].erase(j);
				break;
			}
		}
		//state_colors.emplace(ste,vector<int>{}); // ?
		state_colors[ste].push_back(new_color);
		//vector<int> &colors=
		color_membership[new_color].push_back(ste);
//		colors.push_back(ste);
	//	color_membership.push_back(ste); 	
		for (i=0;i<max_edges;i++) { // for each edge
			if (edge_table[ste][i]==-1) break;
			replace_color (edge_table[ste][i],original_color,new_color,color_membership,virtual_root_edges,virtual_root_colors);
		}
	}
}

void reverse_replace_color (int ste, int original_color, int new_color_start, int new_color_end, vector <int> *color_membership) {
	int i,k;
	
	if (ste!=-1) {
		if (visited[ste]) return;
		visited[ste]=1;
	}
	
	for (std::vector<int>::iterator j = state_colors[ste].begin() ; j != state_colors[ste].end() ; j++) {
		if (*j == original_color) {
			state_colors[ste].erase(j);
			break;
		}
	}
	for (k=new_color_start;k<new_color_end;k++)	{
		state_colors[ste].push_back(k);
		color_membership[k].push_back(ste);
	}
	
	for (i=0;i<max_fan_in;i++) {
		if (reverse_table[ste][i]==-1) break;
		reverse_replace_color (reverse_table[ste][i],original_color,new_color_start,new_color_end,color_membership);
	}
}

void split_colorv2(int ste, int color_from, int color_to)    /* RASHA ** adding ste as parameter in split_colorv2   **/

{ 

  int k;

  /*
   * Erases all the colors within the tree before adding new colors 
   */

  for (std::vector<int>::iterator i = state_colors[ste].begin();i!=state_colors[ste].end();i++)
  {
    if (*i==color_from)     				/*RASHA ** change color to color_from **/ 
      {
        state_colors[ste].erase(i);
      }
  }

  /*
   * add colors to the highest priortize ste for splitting based on the amount of outgoing edges it may have 
   */

  for (int i = 1; i <= ExactOutEdge[ste]; i++)
  {
    state_colors[ste].push_back(color_to++);           /* RASHA**  Color to color_to **/ 
  }

  /*
   * when a new brach is found from the start state the color will change among that branch
   */

  for (std::vector<int>::iterator i = state_colors[ste].begin(); i != state_colors[ste].end(); i++)
  {
    k++;
      
    if (edge_table[ste][k] == -1)
    {
      break;
    }

    replace_colorv2(edge_table[ste][k],color_to,*i);      /* RASHA ** color to color_to **/
  }

}

void replace_colorv2(int str, int old_color, int new_color)
{
  /*
   * This is a function that is called recursively onto the branch that it lies on.
   * iterator is used to check the vector if it contains the color
   * 
   * My solution for cycle: create multi-dem vector that stores every cycle found. if one ste that 
   *                        is found within the cycle, a nested for loop will trigger making a similar 
   *                        for every ste within the cycle.
   */

  if (str == -1)
  {
    return;
  }

  for (int i = 0; i < max_edges; i++){
    if (edge_table[str][i] == -1)
    {
      break;
    }
    for (std::vector<int>::iterator j = state_colors[str].begin(); j != state_colors[str].end(); j++)
    {
      if (*j == old_color)
          {
            state_colors[str].erase(j);
            state_colors[str].push_back(new_color);
            color_count[new_color]++;
          }    
    }
    replace_colorv2(edge_table[str][i], old_color, new_color);
  }
}



void max_color_size (int &max_color_size,int &max_color) {
  int i; 
  max_color_size=1;
  max_color=-1;

  int color_size[MAX_COLORS];

  for (i=0;i<MAX_COLORS;i++) color_size[i]=0;
//  for (i=0;i<num_states;i++) color_size[state_colors[i]]++ ; 		/** RASHA:   NOT WORK ** ??? */ 
  
  for (i=0;i<MAX_COLORS;i++) if (color_size[i]>max_color_size) {
    max_color_size=color_size[i];
    max_color=i;
  }
}

