#include "nfatool.h"

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

using namespace std; 

void add_connected_stes (int ste,vector<int> &members,int **graph,int max_edges) {
  if (visted[ste]) return;
  visited[ste]=1;

  members.push_back(ste);
  for (int i=0;i<max_edges;i++) {
  	if (graph[ste][i]==-1) break;
  	add_connected_stes(graph[ste][i],members,graph,max_edges);
  }
}

void dump_color_info (vector <int> *color_membership, vector <int> &virtual_root_colors) {
	int i,j;

/*
	printf ("state colors\n");
	printf ("%5d ",-1);
	for (j=0;j<virtual_root_colors.size();j++) {
		if (j>0) printf (",");
		printf ("%d",virtual_root_colors[j]);
	}
	printf ("\n");

	for (i=0;i<num_states;i++) {
		printf ("%5d ",i);
		for (j=0;j<state_colors[i].size();j++) {
			if (j>0) printf (",");
			printf ("%d",state_colors[i][j]);
		}
		printf ("\n");
	}

	printf ("\n");
*/
	int total_stes=0;

	//printf ("color states\n");
	for (i=0;i<current_color;i++) {
		//printf ("%5d (%d) ",i,(int)color_membership[i].size());
		for (j=0;j<color_membership[i].size();j++) {
			//if (j>0) printf (",");
			//printf ("%d",color_membership[i][j]);
			total_stes++;
		}
		//printf ("\n");
	}

	int partitions=0;
	double utilization=0.0;
	for (i=0;i<MAX_COLORS;i++) {
		if (color_membership[i].size() != 0) partitions++;
		utilization += (double)color_membership[i].size()/(double)max_stes;
	}
	utilization /= (double)partitions;

	printf ("\n");
	printf ("partitions = %d\naverage partition utilization = %0.2f%%\noriginal stes = %d\nstes after partitioning = %d\nreplications = %d\n",
																					partitions,
																					utilization*100.0,
																					num_states,
																					total_stes,
																					total_stes-num_states-partitions); // subtract virt. root


}

void clear_visited_flags () {
	// clear visitied to avoid inifinite loops
	for (int i=0;i<num_states;i++) visited[i]=0;
}

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

void partition (int max_partition_size) {
	int i,j,max_color_membership,color_to_split;
	//map <int,vector <int> > color_membership;
//	vector <int> v = new vector<int>(10); 
	vector <int> color_membership[MAX_COLORS]; // new vector<int>();
	vector <int> virtual_root_colors;
	vector <int> virtual_root_edges;//,virtual_root_colors;
	char str[1024];
	vector<int> myemptyvector;
	
	// initialize colors and set up virtual root
	virtual_root_colors.push_back(0);
	color_membership[0].push_back(-1); // -1 is stand-in for virtual root
	for (i=0;i<num_states;i++) {
		state_colors[i].push_back(0);
		color_membership[0].push_back(i);
		if (root_node[i]) virtual_root_edges.push_back(i); // create outgoing edges from virtual root
	}

	// find "natural partitions
	vector <int> natural_partitions[MAX_COLORS];
	int natural_partition_num=0;
	clear_visited_flags();
	for (i=0;i<virtual_root_edges.size();i++) {
		add_connected_stes(virtual_root_edges[i],natural_partitions[natural_partition_num],edge_table,max_edges);
		add_connected_stes(virtual_root_edges[i],natural_partitions[natural_partition_num],reverse_table,max_reverse_edges);
		if (natural_partitions[natural_partition_num].size()) natural_partition_num++;
	}

	int min_partition=num_states,max_partition=0;
	for (i=0;i<natural_partitions;i++) {
		if (natural_partitions[i].size() < min_partition) min_partition=natural_partitions[i].size();
		if (natural_partitions[i].size() > max_partition) max_partition=natural_partitions[i].size();
	}

	printf ("found %d natural partitions ranging in size from %d to %d\n",natural_partitions,min_partition,max_partition);
	
	// perform first check for partition violations
	max_color_membership=0;
	for (i=0;i<current_color;i++) if (color_membership[i].size() > max_color_membership) {
		max_color_membership=color_membership[i].size();
		color_to_split=i;
	}
	
	int coloring = 0;

	// dump the dot file
	//sprintf (str,"coloring%d",coloring++);
	//dump_dot_file (str,rootGlobal,myemptyvector,1);
	//dump_color_info(color_membership,virtual_root_colors);

	while (max_color_membership > max_partition_size) {
		
		int ste_to_split = find_lowest_pure_node(color_to_split,virtual_root_edges,virtual_root_colors);
		if (ste_to_split==-2) {
			fprintf (stderr,"ERROR:  cannot find pure color %d node; cannot partition!\n",color_to_split);
			exit(0);
		}
		split_colors(ste_to_split,color_membership,virtual_root_edges,virtual_root_colors);
		
		max_color_membership=0;
		for (i=0;i<current_color;i++) if (color_membership[i].size() > max_color_membership) {
			max_color_membership=color_membership[i].size();
			color_to_split=i;
		}

		// dump the dot file
		//sprintf (str,"coloring%d",coloring++);
		//dump_dot_file (str,rootGlobal,myemptyvector,1);
		//dump_color_info(color_membership, virtual_root_colors);
	}
	
	// consolidation step
	int merged;
	do {		
		merged=0;
		// consolidate
		for (i=0;i<current_color;i++) {
			if (color_membership[i].size()==0) continue;
			for (j=i+1;j<current_color;j++) {
				if (color_membership[j].size()==0) continue;
				if ((color_membership[i].size() + color_membership[j].size()) <= max_partition_size) {
					merge_colors(i,j,color_membership,virtual_root_colors);
					merged=1;
				}
			}
		}
		// dump the dot file
		//sprintf (str,"coloring%d",coloring++);
		//dump_dot_file (str,rootGlobal,myemptyvector,1);
		//dump_color_info(color_membership,virtual_root_colors);
	} while (merged);

	dump_color_info(color_membership,virtual_root_colors);
}

void merge_colors(int color1,int color2,vector <int> *color_membership, vector <int> &virtual_root_colors) {
	// TODO:  deal with virtual root node!
	int i;
	
	for (i=0;i<color_membership[color2].size();i++) {
		int ste = color_membership[color2][i];
		int found=0;
		for (int j=0;j<color_membership[color1].size();j++) if (color_membership[color1][j]==ste) found=1;
		if (!found) color_membership[color1].push_back(ste);
		if (ste==-1) {
			virtual_root_colors.push_back(color1);
			for (vector<int>::iterator j = virtual_root_colors.begin() ; j != virtual_root_colors.end() ; j++) {
				if (*j==color2) {
					virtual_root_colors.erase(j);
				}
			}
		} else {
			state_colors[ste].push_back(color1);
			for (vector<int>::iterator j = state_colors[ste].begin() ; j != state_colors[ste].end() ; j++) {
				if (*j==color2) {
					state_colors[ste].erase(j);
				}
			}
		}
	}
	color_membership[color2].clear();
}

int find_lowest_pure_node(int color,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors) {
	queue <int> myqueue;
	int i,found;
	
	if (virtual_root_edges.size() > 1)
		myqueue.push(-1); // stand-in for virtual root
	else
		myqueue.push(virtual_root_edges[0]);
	
	clear_visited_flags();

	while (myqueue.size() > 0) {
		int dequeued = myqueue.front();
		myqueue.pop();

		// virtual root cannot be part of a cycle, so there's no reason to check if it's already visited
		if (dequeued != -1) {
			if (visited[dequeued]) continue;
			visited[dequeued]=1;
		}

		int num_colors = dequeued == -1 ? virtual_root_colors.size() : state_colors[dequeued].size();
		int first_color = dequeued == -1 ? virtual_root_colors[0] : state_colors[dequeued][0];
		
		// find the number of "real" (non-loopback) outgoing edges
		int outgoing_edges=0;
		if (dequeued == -1) {
			outgoing_edges = (int)virtual_root_edges.size();
		} else {
			for (i=0;i<max_edges;i++) {
				if (edge_table[dequeued][i]==-1) break;
				if (edge_table[dequeued][i]!=dequeued) outgoing_edges++;
			}
		}

		if (num_colors==1 && first_color==color && outgoing_edges!=0 && outgoing_edges!=1) return dequeued;
		
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
			if (edge_table[scc[i]][j]==-1) break;
			
			found=0;
			for (k=0;k<scc.size();k++) if (edge_table[scc[i]][j]==scc[k]) found=1;
			if (!found) edges.push_back(edge_table[scc[i]][j]);
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
			if (reverse_table[scc[i]][j]==-1) break;
			
			found=0;
			for (k=0;k<scc.size();k++) if (reverse_table[scc[i]][j]==scc[k]) found=1;
			if (!found) edges.push_back(reverse_table[scc[i]][j]);
		}
	}
	
	return edges;
}

void split_colors (int ste, vector <int> *color_membership,vector <int> &virtual_root_edges, vector <int> &virtual_root_colors) {
	int i,k,
		original_color,
		old_current_color=current_color;
	

		
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
			for (k=0;k<scc_contents.size();k++) {
				state_colors[scc_contents[k]].push_back(current_color); // note: current_color is a new color!
				color_membership[current_color].push_back(scc_contents[k]);
			}
			int dest_node = outgoing_edges[i];
			clear_visited_flags();
			replace_color(dest_node,original_color,current_color,color_membership,virtual_root_edges,virtual_root_colors);
			current_color++;
		}
		
		// STEP 3:  recurse backward to root
		vector <int> incoming_edges = find_incoming_edges(scc_contents);
		for (k=0;k<incoming_edges.size();k++) {
			clear_visited_flags();
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
			clear_visited_flags();
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
		vector<int> &colors = color_membership[new_color];
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
