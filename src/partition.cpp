#include "nfatool.h"
#include <string.h> 

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

//int num_states = my_nfa->num_states; 

// this function finds distinct subgraphs within the original datafile
void traverse(nfa *my_nfa,int state,int subgraph) {
	int i;

	my_nfa->visited[state]=1;
	my_nfa->subgraph[state]=subgraph;

	for (i=0;i<my_nfa->max_edges;i++) {
		if (my_nfa->edge_table[state][i]==-1) break;
		if (!my_nfa->visited[my_nfa->edge_table[state][i]]) {
			traverse(my_nfa,my_nfa->edge_table[state][i],subgraph);
		}
	}

	for (i=0;i<my_nfa->max_fan_in;i++) {
		if (my_nfa->reverse_table[state][i]==-1) break;
		if (!my_nfa->visited[my_nfa->reverse_table[state][i]]) {
			traverse(my_nfa,my_nfa->reverse_table[state][i],subgraph);
		}
	}
}

void find_subgraphs (nfa *my_nfa) {
	int i,j,k,subgraph=0,state;
	int *state_offsets;

	state_offsets = (int *)malloc(sizeof(int)*my_nfa->num_states);

	// allocate and initialize memory
	my_nfa->visited = (char *)malloc(my_nfa->num_states*sizeof(char));
	for (i=0;i<my_nfa->num_states;i++) my_nfa->visited[i]=0;
	my_nfa->subgraph = (int *)malloc(my_nfa->num_states*sizeof(int));

	// find and mark subgraphs
	for (i=0;i<my_nfa->num_states;i++) {
		if (!my_nfa->visited[i]) {
			traverse(my_nfa,i,subgraph);
			subgraph++;
		}
	}
	my_nfa->distinct_subgraphs = subgraph;
	printf ("INFO: %d distinct subgraphs found\n",subgraph);

	// allocate memory for partitioning
	my_nfa->num_partitions = (int *)malloc(sizeof(int)*subgraph);
	my_nfa->partition_size = (int **)malloc(sizeof(int *)*subgraph);

	// allocate array to record size of each subgraph
	my_nfa->subgraph_size=(int *)malloc(subgraph*sizeof(int));
	for (i=0;i<subgraph;i++) my_nfa->subgraph_size[i]=0;

	// count states in each subgraph
	for (i=0;i<my_nfa->num_states;i++) {
		my_nfa->subgraph_size[my_nfa->subgraph[i]]++;
	}

	// allocate and build subgraphs
	// dimension 1:  subgraph number
	my_nfa->edge_tables=(int ***)malloc(subgraph * sizeof(int **));
	my_nfa->orig_edge_tables=(int ***)malloc(subgraph * sizeof(int **));
	my_nfa->movement_maps=(int **)malloc(subgraph * sizeof(int *));
	
	// also need this for partitioned subgraphs
	my_nfa->partitioned_movement_maps=(int ***)malloc(sizeof(int **)*subgraph);
	my_nfa->partitioned_reverse_tables=(int ****)malloc(sizeof(int ***)*subgraph);
	my_nfa->partitioned_orig_edge_tables=(int ****)malloc(sizeof(int ***)*subgraph);

	// this array supports subgraph partitioning
	int distinct_subgraphs = my_nfa->distinct_subgraphs;
	my_nfa->partitioned_edge_tables=
		(int ****)malloc((distinct_subgraphs)*sizeof(int ***));

	// for each subgraph
	for (i=0;i<subgraph;i++) {
		// allocate the states
		my_nfa->edge_tables[i]=(int **)malloc(my_nfa->subgraph_size[i] * sizeof(int *));
		my_nfa->orig_edge_tables[i]=(int **)malloc(my_nfa->subgraph_size[i] * sizeof(int *));
		my_nfa->movement_maps[i]=(int *)malloc(my_nfa->subgraph_size[i] * sizeof(int));

		// allocate the edges
		for (j=0;j<my_nfa->subgraph_size[i];j++) {
			my_nfa->edge_tables[i][j]=(int *)malloc(my_nfa->max_edges * sizeof(int));
			my_nfa->orig_edge_tables[i][j]=(int *)malloc(my_nfa->max_edges * sizeof(int));
		}

		// compute state offsets for re-labling states for subgraph decomposition
		// state_offsets[n] == the number of states having index < n that do not belong to the
		// same subgraph as the state having original number n.  this way, state n can now be
		// relabled as n - state_offsets[n] when placed in its own subgraph
		int not_in_subgraph=0;
		for (j=0;j<my_nfa->num_states;j++) {
			if (my_nfa->subgraph[j] != i) {
				not_in_subgraph++;
			} else {
				// record offset
				state_offsets[j]=not_in_subgraph;
			}
		}

		// copy edges
		int n=0;
		for (j=0;j<my_nfa->num_states;j++) {
			if (my_nfa->subgraph[j] == i) {
				for (k=0;k<my_nfa->max_edges;k++) {
					if (my_nfa->edge_table[j][k] != -1) {
						my_nfa->edge_tables[i][n][k] = my_nfa->edge_table[j][k] - state_offsets[my_nfa->edge_table[j][k]];
						my_nfa->orig_edge_tables[i][n][k] = my_nfa->edge_table[j][k] - state_offsets[my_nfa->edge_table[j][k]];
					} else {
						my_nfa->edge_tables[i][n][k] = -1;
						my_nfa->orig_edge_tables[i][n][k] = -1;
					}
				}
				n++;
			}
		}
	}

	free(my_nfa->visited);
}

int vector_contains (vector<int> &myvec,int value) {
	for (vector<int>::iterator it=myvec.begin();
			it != myvec.end();
			it++) {
		if (*it==value) return 1;
	}
	return 0;
} 

void copy_colors (vector<int> &to,
		vector<int> &from,
		vector<int> &color_size,
		vector <vector<int> > &color_membership,
		vector <vector<int> > &colors) {
	// for each color in the state
	for (vector<int>::iterator it=from.begin();
			it != from.end();
			it++) {

		// don't copy a color that is already present in the destination state
		if (!vector_contains(to,*it)) {
			to.push_back(*it);
			// increment membership count
			color_size[*it]++;

			// for each of the states having this color
			for (int i=0;i<colors.size();i++) {
				// add the color to the state
				if (vector_contains(colors[i],*it) && !vector_contains(color_membership[*it],i))
					color_membership[*it].push_back(i);
			}
		}
	}
}

int parent_of (int parent,int child,int **reverse_table,int max_fanin) {
	for (int i=0;i<max_fanin;i++) {
		if (reverse_table[child][i]==-1) break;
		if (reverse_table[child][i]==parent) return 1;
	}
	return 0;
}

void copy_edges (vector<edge> dst,vector<edge> src) {
	for (vector<edge>::iterator it = src.begin();
	     it != src.end();
	     it++) {
		dst.push_back(*it);
	}
}

void traverse_up(int **reverse_table,
				 int max_fanin,
				 int state,
				 int color,
				 map<edge,vector<int> > &mycolors,
				 int *visited) {

	for (int j=0;j<max_fanin;j++) {
		int state_up=reverse_table[state][j];
		if (state_up==-1) break;
		if (state_up==state) break;
		if (visited[state_up]) break;
		visited[state_up]=1;
		edge temp(state_up,state);
		mycolors[temp].push_back(color);
		traverse_up(reverse_table,max_fanin,state_up,color,mycolors,visited);
	}
}

void find_colors(int **edge_table,
				 int **reverse_table,
				 int num_states,
				 int max_edges,
				 int max_fanin,
				 int max_fanout,
				 int &color,
				 edge in_edge,
				 map<edge,vector<int> > &mycolors) {

	for (int j=0;j<max_edges;j++) {
		int succ = edge_table[in_edge.succ][j];
		if (succ==-1) break;
		if (in_edge.succ == succ) break;
		edge out_edge(in_edge.succ,succ);
		
		if (j!=0 && j%max_fanout==0) {
			color++;
			printf ("INFO: splitting edge %d->%d\n",in_edge.succ,succ);
			// use visited array to prevent loops in upward traversal
			int *visited=(int *)malloc(sizeof(int)*num_states);
			for (int i=0;i<num_states;i++) visited[i]=0;

			traverse_up(reverse_table,max_fanin,succ,color,mycolors,visited);
		} else {
			printf ("INFO: not splitting edge %d->%d\n",in_edge.succ,succ);
			mycolors[out_edge].push_back(color);
		}
		
		find_colors(edge_table,
					reverse_table,
					num_states,
					max_edges,
					max_fanin,
					max_fanout,
					color,
					out_edge,
					mycolors);
	}
}

int color_edges(nfa *my_nfa,
				int max_fanout,
				int subgraph,
				map<edge,vector<int> > &mycolors) {

	int **edge_table = my_nfa->edge_tables[subgraph],
		**reverse_table = my_nfa->reverse_tables[subgraph],
		num_states = my_nfa->subgraph_size[subgraph],
		max_edges = my_nfa->max_edges,
		max_fanin = my_nfa->max_fan_in;

	// allocate component array, which stores the component for each node
	int *components;
	components = (int *)malloc(num_states * sizeof(int));

	// find SCCs
	find_sccs(my_nfa,subgraph,components);

	// step 1:  allocate memory
	// associate a vector of colors to each node
	int root,i;

	// find a root node
	for (i=0;i<num_states;i++) {
		if(reverse_table[0][0] == i && reverse_table[0][1] == -1) {
			root=i; 
			break; 

		} else if (reverse_table[i][0]==-1) {
			root = i;
			break;
		}

			if (i==num_states) {
				fprintf (stderr,"ERROR: partition_graph(): could not identify a root node\n");
				exit(0);
			}
	}

	int color=0;
	int temp=0; 


	for (int i=0;i<max_edges;i++) {

		if (edge_table[root][i]==-1) break;

// this condition is to check if the root has any self loop, if so we don't give this edge a color
// and we start first color in the next edge 
// by Rasha 
		
		else if(edge_table[root][i] == root) temp = 0; 

		else {
			if(edge_table[root][i] !=root ) temp = 0;

			if(edge_table[root][i] != root && (i!=0 && i!=1)) temp = i;  
 			 
			int state=edge_table[root][i];
			edge myedge(root,state);
				
			if (temp!=0 && i%max_fanout==0) color++;
		
			mycolors[myedge].push_back(color);

			find_colors(edge_table,	
					reverse_table,
					num_states,
					max_edges,
					max_fanin,
					max_fanout,
					color,
					myedge,
					mycolors);
		}		
	}

	return color+1;
}

int color_states (nfa *my_nfa,
		int subgraph,
		int max_fanout,
		vector<vector<int> > &colors,
		vector <int> &color_size,
		vector <vector<int> > &color_membership,
		vector <vector<edge> > &color_membership_edge,
		vector <vector<edge> > &colors_edges) {

	int **edge_table = my_nfa->edge_tables[subgraph],
	**reverse_table = my_nfa->reverse_tables[subgraph],
	num_states = my_nfa->subgraph_size[subgraph],
	max_edges = my_nfa->max_edges,
	max_fanin = my_nfa->max_fan_in;

	// allocate component array, which stores the component for each node
	int *components;
	components = (int *)malloc(num_states * sizeof(int));

	// find SCCs
	find_sccs(my_nfa,subgraph,components);

	// step 1:  allocate memory
	// associate a vector of colors to each node
	int root,i;

	// find a root node
	for (i=0;i<num_states;i++) {
		if(reverse_table[0][0] == i && reverse_table[1][0] == -1) { 
			root = 0; 
			break; 
		}
		else if (reverse_table[i][0]==-1) {
			root = i;
			break;
		}
	}
	if (i==num_states) {
		fprintf (stderr,"ERROR: partition_graph(): could not identify a root node\n");
		exit(0);
	}

	// do a DFS from root and color the graph
	// initiate the root node with color 0
	int color=0;
	colors[root].push_back(color);
	vector<int> dfs_down_stack,
		back_trace;

	// allocate an entry for color 0 and set its state count to 1
	color_size.push_back(1);

	// add a new entry to color_membership for color 0 and add the root state
	vector<int> temp;
	temp.push_back(root);
	color_membership.push_back(temp);

	dfs_down_stack.push_back(root);

	// initialize vector of vector of edges
	vector<edge> my_vec_edge;
	color_membership_edge.push_back(my_vec_edge);

	while (!dfs_down_stack.empty()) {
		// pop node
		int parent = dfs_down_stack.back();
		dfs_down_stack.pop_back();

		while (back_trace.size() &&
				(!parent_of(back_trace.back(),parent,reverse_table,max_fanin)))
			back_trace.pop_back();

		back_trace.push_back(parent);

		// this vector saves the set of new colors generated by the current node (parent)
		// so we can defer adding these colors to the parent until after we process all
		// the children
		vector<int> new_colors;

		// same for edges
		vector<edge> new_edges;

		// for each of its children
		//int edges=0;
		for (int i=0;i<max_edges;i++) {
			int child = edge_table[parent][i];
			if (child == -1) break;
			if (child == parent) continue;

			// copy all the parent's colors to the child
			// NOTE that this does not include any new colors we added for splits
			copy_colors(colors[child],colors[parent],color_size,color_membership,colors);

			// check if we need to add a new color
			if (i!=0 && i%max_fanout==0) {
				// before we split, we need to make sure we are not adding
				// a new color to a member of the same SCC
				if (components[parent]!=components[child]) {
					// note:  condition above should prevent self-loops

					// create new color and initialize membership count
					// and membership list
					color++;
					color_size.push_back(0);

					// add a new color in color_membership
					vector<int> temp;
					temp.push_back(child);
					color_membership.push_back(temp);

					// add a new color in edge_membership
					edge myedge(parent,child);
					vector<edge> my_vec_edge;
					my_vec_edge.push_back(myedge);
					color_membership_edge.push_back(my_vec_edge);

			// temp = parent
			// for each member of backtrace
			// find all edge colors that contain edge *it->temp
			// copy all the edges cooresponding to these to the new color
			// set temp=*it

					// delete the split color
					int last_color_added = colors[child].back();
					color_size[last_color_added]--;
					for (vector<int>::iterator it=color_membership[last_color_added].begin();
							it!=color_membership[last_color_added].end();
							it++)
						if (*it==child) {
							color_membership[last_color_added].erase(it);
							break;
						}
					colors[child].pop_back();

					// add color to child
					colors[child].push_back(color);
					color_size[color]++;

					// add state to color
					//color_membership[color].push_back(child);

					// add the new color to the list of new colors added for this parent
					new_colors.push_back(color);
					new_edges.push_back(myedge);

				} else { // SCC check
					edge myedge(parent,child);
					color_membership_edge[color].push_back(myedge);
				}
			} else { // add color
				edge myedge(parent,child);
				color_membership_edge[color].push_back(myedge);
			}
			// push child onto stack
			dfs_down_stack.push_back(child);
		} // for each child

		// copy the new colors to the ancestors
		for (vector<int>::iterator it=back_trace.begin();
				it!=back_trace.end();
				it++) {
			copy_colors(colors[*it],new_colors,color_size,color_membership,colors);
			
		}
	}
	return color+1;
}

vector<int> states_in_color (map<edge,vector<int> > mycolors,int color) {
	vector<int> states;

	for (map<edge,vector<int> >::iterator it = mycolors.begin();
	     it != mycolors.end();
	     it++) {
		edge myedge = it->first;
		vector<int> colors = it->second;
		if (vector_contains(colors,color)) {
			if (!vector_contains(states,myedge.pred)) states.push_back(myedge.pred);
			if (!vector_contains(states,myedge.succ)) states.push_back(myedge.succ);
		}
	}

	return states;
}

/**
 * \brief		Partition a graph into a form having a maximum logical fanin and fanout of max_fanout (exclusing SCCs)
 * \param[in]	XXX
 */
int partition_graph (nfa *my_nfa,int subgraph,int max_fanout) {
	vector<int> color_size;
	int max_edges = my_nfa->max_edges;
	int num_states = my_nfa->subgraph_size[subgraph];
	vector<vector<int> > colors(num_states);
	vector<vector<int> > color_membership;
	vector<vector<edge> > color_membership_edges;

	map<edge,vector<int> > mycolors;
	
	int num_colors = color_edges(my_nfa,
		 		 max_fanout,
				 subgraph,
				 mycolors);
	
	// partition the subgraph
	/*
	int num_colors = color_states(my_nfa,
			subgraph,
			max_fanout,
			colors,
			color_size,
			color_membership,
			color_membership_edges,
		    colors_edges);
	*/
	// record number of partitions for this subgraph
	my_nfa->num_partitions[subgraph]=num_colors;

	// find the size of each partition
	for (int i=0;i<num_colors;i++) {
		color_membership.push_back(states_in_color(mycolors,i));
		color_size.push_back(color_membership[i].size());
	}

	// create an array that keeps track of individual partition sizes
	my_nfa->partition_size[subgraph]=(int *)malloc(num_colors * sizeof(int));
	for (int i=0;i<num_colors;i++) my_nfa->partition_size[subgraph][i]=color_size[i];

	// create an array to keep track of state index offsets for each partition
	// this allows us to convert the subgraph state number into a partition state number
	// in other words, we use this to compress the larger number of states in the
	// subgraph to the smaller number of states in the partition
	int **state_offsets = (int **)malloc(num_colors * sizeof(int *));
	for (int i=0;i<num_colors;i++) {
		state_offsets[i]=(int *)malloc(num_states * sizeof(int));
	}

	// this loop determines the translation between the subgraph numbering and the partition numbering
	for (int i=0;i<num_colors;i++) {
		int offset=0;
		for (int j=0;j<num_states;j++) {
			// if state j is not in color i then increment the offset
			if (!vector_contains(color_membership[i],j)) offset++;
			state_offsets[i][j]=offset;
		}
	}

	// number of graphs is equal to the number of colors
	my_nfa->partitioned_edge_tables[subgraph]=
		(int ***)malloc((num_colors)*sizeof(int **));

	// also allocate a duplicate original graph for cross-checking after mapping
	my_nfa->partitioned_orig_edge_tables[subgraph]=
		(int ***)malloc((num_colors)*sizeof(int **));

	// allocate the reverse table for the partitions
	my_nfa->partitioned_reverse_tables[subgraph]=
		(int ***)malloc((num_colors)*sizeof(int **));

	// allocate the reverse table for the partitions
	my_nfa->partitioned_movement_maps[subgraph]=
		(int **)malloc((num_colors)*sizeof(int *));
	
	int total_states=0;
	// size of each partition (number of states) is the corresponding membership count
	for (int i=0;i<num_colors;i++) {
		// sum up number of states
		total_states += color_size[i];

		my_nfa->partitioned_edge_tables[subgraph][i]=
			(int **)malloc(color_size[i]*sizeof(int *));
		my_nfa->partitioned_orig_edge_tables[subgraph][i]=
			(int **)malloc(color_size[i]*sizeof(int *));

		// allocate the edges for each state
		for (int j=0;j<color_size[i];j++) {
			// allocate edges
			my_nfa->partitioned_edge_tables[subgraph][i][j]=
				(int *)malloc(max_edges*sizeof(int));
			my_nfa->partitioned_orig_edge_tables[subgraph][i][j]=
				(int *)malloc(max_edges*sizeof(int));
		}

		// for each of the states in the subgraph, copy to its corresponding partition
		int state=0;
		for (int pred=0;pred<num_states;pred++) {
			int edge_num=0;
			for (int k=0;k<max_edges;k++) {
				int succ = my_nfa->edge_tables[subgraph][pred][k];
				if (succ==-1) break;

				edge myedge(pred,succ);
					
				if (vector_contains(mycolors[myedge],i)) {
					// convert the successor state into partition numbering
					succ=succ-state_offsets[i][succ];

					my_nfa->partitioned_edge_tables[subgraph][i][state][edge_num]=
						my_nfa->partitioned_orig_edge_tables[subgraph][i][state][edge_num]= 
						succ;

					edge_num++;
				}
			}

			// terminate the outgoing edges
			if (edge_num < max_edges)
				my_nfa->partitioned_edge_tables[subgraph][i][state][edge_num]=
					my_nfa->partitioned_orig_edge_tables[subgraph][i][state][edge_num]=
						-1;
			
			// increment state number if the state is a member of the partition
			if (vector_contains(color_membership[i],pred)) state++;

		} // for each state in the subgraph

	} // for each color

	// output the number of replicated states
	printf ("INFO: Partitioning successful!  Subgraph %d size increased from %d to %d (%d replications)\n",
			subgraph,
			num_states,
			total_states,
			total_states-num_states);

	// allocate movement maps for each partitioned edge table
	for (int i=0;i<num_colors;i++) {
		my_nfa->partitioned_movement_maps[subgraph][i]=(int *)malloc(color_size[i] * sizeof(int));
	}

	return total_states - num_states;
}



// traverse graph to find total number of paths
// by Rasha

int num_path=0; 

int num_paths(nfa *my_nfa, int node){

	bool *visited_path2; 
	int num_path=0;  

        visited_path2 =(bool *)malloc((sizeof(bool*))*my_nfa->num_states);

        for(int i=0; i<my_nfa->num_states;i++) visited_path2[i]=0;

	find_all_paths(my_nfa, node, visited_path2, num_path);
	
	return num_path;
} 

 
void find_all_paths(nfa *my_nfa, int src, bool visited_path2[], int &num_path) {
	
	visited_path2[src]=true;

	       for(int k=0; k<my_nfa->max_edges;k++) {
	       
			if(my_nfa->edge_table[src][k] ==-1) break ;
					
	       		if(my_nfa->report_table[my_nfa->edge_table[src][k]]) {
			
				num_path++;
//				printf("Total num of path in each ANML partition = %d\n", num_path);          
				
               		}
 
               		else  {
				if(!visited_path2[my_nfa->edge_table[src][k]]){
	      				find_all_paths(my_nfa,my_nfa->edge_table[src][k], visited_path2, num_path); 
	         		}	
			}
	     }
	visited_path2[src]=false; 
}


// Find num of loops in ANML file 
// by Rasha 

int num_loops(nfa *my_nfa, int node){

        bool *visited_path2;
        int num_loops=0;
	int loop_size= 1; 
	int num_self_loops=0; 

        visited_path2 =(bool *)malloc((sizeof(bool*))*my_nfa->num_states);

        for(int i=0; i<my_nfa->num_states;i++) visited_path2[i]=0;

        find_loops(my_nfa, node, visited_path2,loop_size, num_loops, num_self_loops);

        return num_loops;
}

void find_loops(nfa *my_nfa, int src, bool visited_path2[], int &loop_size, int &num_loops, int &num_self_loops) {

        visited_path2[src]=true;
	
		

               for(int k=0; k<my_nfa->max_edges;k++) {

                        if(my_nfa->edge_table[src][k] ==-1) break ;

                        if(my_nfa->edge_table[src][k]==src) {

                                num_loops++;
				if (loop_size=1) num_self_loops++; 
//                              	printf("Total num ofpath in each ANML partition = %d\n", loop_path);

                        }

                        else  {
                                if(!visited_path2[my_nfa->edge_table[src][k]]){
                                        find_loops(my_nfa,my_nfa->edge_table[src][k], visited_path2, loop_size, num_loops, num_self_loops);
					loop_size++;
                                }
                        }
             }
        visited_path2[src]=false;
}

// Find critical path 
// by Rasha 

/*
int critical_path(nfa *my_nfa, int node){

        bool *visited_path2; 
        int max_path=1;
        int path_length=0; 
	int path_compare=0;  

        visited_path2 =(bool *)malloc((sizeof(bool*))*my_nfa->num_states);

        for(int i=0; i<my_nfa->num_states;i++) visited_path2[i]=0;

        find_critical_path(my_nfa, node, visited_path2, max_path, path_length, path_compare);

        return max_path;
}


void find_critical_path(nfa *my_nfa, int node, int visited_path2[], int &max_path, int &path_length, int &path_compare)
{
        if (visited_path2[node] == 1)
        {
                return;
        }

        max_path++;

        for (int i = 0; i < max_edges; i++)
        {
                if(report_table[node] == 1)
                {
                        visited[node] = 1;

                        if (path_length > path_compare)
                        {
                                path_compare = max_path;
                                max_path = 1;
                        }
                }
                critical_path(mynfa, edge_table[node][i], visited_path2, max_path, path_length, path_compare);
        }
}
*/
