#include "nfatool.h"
#include <string.h> 

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

using namespace std; 


void clear_visited_flags () {
	// clear visitied to avoid inifinite loops
	for (int i=0;i<num_states;i++) visited[i]=0;
}

void add_connected_stes (int ste,vector<int> &members,int **graph,int max_edges) {

	if (visited[ste]) return;
	visited[ste]=1;

	members.push_back(ste);

	for (int i=0;i<max_edges;i++) {
		if (graph[ste][i]==-1) break;
		add_connected_stes(graph[ste][i],members,graph,max_edges); 
	}
}

void dump_color_info (vector <int> *color_membership, vector <int> &virtual_root_colors) {

	int i,j;
	int total_stes=0;

	for (i=0;i<current_color;i++) {
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

void find_critical_path () {
	int i,depth=0;
	vector<int> path;

	for (i=0;i<num_states;i++) visited[i]=0;

	for (i=0;i<num_states;i++) {
		if (start_state[i]) {
			dfs_critical(i,depth,deepest,deepest_path,path);
		}
	}

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

// this function finds distinct subgraphs within the original datafile
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

// TODO:  put this in whatever calls partition_graph()
/* // for each subgraph, create an entry for a potential set of
// partitions
int distinct_subgraphs = my_nfa->distinct_subgraphs;
my_nfa->partitioned_edge_tables=
(int ****)malloc((distinct_subgraphs)*sizeof(int ***)); */


/**
 * \brief		Partition a graph into a form having a maximum logical fanin and fanout of max_fanout (exclusing SCCs)
 * \param[in]	
 */
void partition_graph (nfa *my_nfa,int subgraph,int max_fanout) {
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
	vector<int> *colors = new vector<int>[num_states];
	int root,i;

	// find a root node
	for (i=0;i<num_states;i++) {
		if (reverse_table[i][0]==-1) {
			root = i;
			break;
		}
	}
	if (i==num_states) {
		fprintf (stderr,"ERROR: partition_graph(): could not identify a root node\n");
		exit(0);
	}

	// do a DFS from root and color the graph
	int color=0;
	vector<int> dfs_down_stack,color_size;
	dfs_down_stack.push_back(root);
	while (!dfs_down_stack.empty()) {
		// pop node
		int parent = dfs_down_stack.back();
		dfs_down_stack.pop_back();

		// for each of its outgoing edges...
		int edges=0;
		for (int i=0;i<max_edges;i++) {
			int child = edge_table[parent][i];
			if (child == -1) break;

			// copy all the parent's colors to the child
			for (vector<int>::iterator it=colors[parent].begin();
					it != colors[parent].end();
					it++) {
				colors[child].push_back(*it);
				// increment membership count
				color_size[*it]++;
			}

			// check if we need to add a new color
			if (i!=0 && i%max_fanout==0) {
				// before we split, we need to make sure we are not adding
				// a new color to a member of the same SCC
				if (components[parent]!=components[child]) {
			
					// create new color and initialize membership count
					color++;
					color_size[color]=0;

					// add color to child
					colors[child].push_back(color);
					color_size[color]++;

					// add color to parent and ancestors (DFS)
					vector<int> dfs_up_stack;
					dfs_up_stack.push_back(parent);
					while (!dfs_up_stack.empty()) {
						// pop node
						int ancestor = dfs_up_stack.back();
						dfs_up_stack.pop_back();

						// add the color
						colors[ancestor].push_back(color);
						// increment membership count
						color_size[color]++;

						for (int i=0;i<max_fanin;i++) {
							int node=reverse_table[ancestor][i];
							if (node==-1) break;
							dfs_up_stack.push_back(node);
						}
					}
				}
			} // add color
			dfs_down_stack.push_back(child);
		} // for each child
	} // end of downward DFS

	// number of graphs is equal to the number of colors
	my_nfa->partitioned_edge_tables[subgraph]=
		(int ***)malloc((color+1)*sizeof(int **));

	// size of each partition (number of states) is the corresponding membership count
	for (int i=0;i<=color;i++) {
		my_nfa->partitioned_edge_tables[subgraph][i]=
			(int **)malloc(color_size[i]*sizeof(int *));

		// allocate and copy the edges for each state
		for (int j=0;i<color_size[i];j++) {
			// allocate edges
			my_nfa->partitioned_edge_tables[subgraph][i][j]=
				(int *)malloc(max_edges*sizeof(int));

			// copy edges
			int state=0;

			// for each of the states in the subgraph
			for (int k=0;k<num_states;k++) {

				// check if the state belongs to color i
				int found=0;
				for (vector<int>::iterator it=colors[k].begin();
						it != colors[k].end();
						it++) {
					if (*it==i) found=1;
				}

				// if so, copy all its edges to the partition
				if (found) {
					for (int l=0;l<max_edges;l++)
						my_nfa->partitioned_edge_tables[subgraph][i][state][l]=
							my_nfa->edge_tables[subgraph][k][l];
					state++;
				}
			} // for each state in the subgraph

		} // for each color

	}

	delete colors;
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

