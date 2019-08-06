#include "nfatool.h"
#include <string.h> 

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

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

int color_states (nfa *my_nfa,
		int subgraph,
		int max_fanout,
		vector<vector<int> > &colors,
		vector <int> &color_size,
		vector <vector<int> > &color_membership) {

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

					vector<int> temp;
					temp.push_back(child);
					color_membership.push_back(temp);

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

				} // SCC check
			} // add color

			// push child onto stack
			dfs_down_stack.push_back(child);
		} // for each child

		for (vector<int>::iterator it=back_trace.begin();
				it!=back_trace.end();
				it++) {
			copy_colors(colors[*it],new_colors,color_size,color_membership,colors);
		}

	}
	return color+1;
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

	int num_colors = color_states(my_nfa,
			subgraph,
			max_fanout,
			colors,
			color_size,
			color_membership);

	// record number of partitions for this subgraph
	my_nfa->num_partitions[subgraph]=num_colors;

	// create an array that keeps track of individual partition sizes
	my_nfa->partition_size[subgraph]=(int *)malloc(num_colors * sizeof(int));
	for (int i=0;i<num_colors;i++) my_nfa->partition_size[subgraph][i]=color_size[i];

	// create an aray to keep track of state index offsets for each partition
	// this allows us to convert the subgraph state number into a partition state number
	// in other words, we use this to compress the larger number of states in the
	// subgraph to the smaller number of states in the partition
	int **state_offsets = (int **)malloc(num_colors * sizeof(int *));
	for (int i=0;i<num_colors;i++) state_offsets[i]=(int *)malloc(num_states * sizeof(int));
	for (int i=0;i<num_colors;i++)
		for (int j=0;j<num_states;j++)
			// if state j is not in color i then increment the offset
			if (!vector_contains(colors[j],i)) state_offsets[i][j]++;

	// number of graphs is equal to the number of colors
	my_nfa->partitioned_edge_tables[subgraph]=
		(int ***)malloc((num_colors)*sizeof(int **));

	// also allocate a duplicate for cross-checking after mapping
	my_nfa->partitioned_orig_edge_tables[subgraph]=
		(int ***)malloc((num_colors)*sizeof(int **));

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
		for (int j=0;j<num_states;j++) {
			if (vector_contains(color_membership[i],j)) {
				for (int k=0;k<max_edges;k++)
					my_nfa->partitioned_edge_tables[subgraph][i][state][k]=
						my_nfa->partitioned_orig_edge_tables[subgraph][i][state][k]= 
						my_nfa->edge_tables[subgraph][j][k]-state_offsets[i][j];
				state++;
			}
		} // for each state in the subgraph

	} // for each color

	// output the number of replicated states
	printf ("INFO: Partitioning successful!  Subgraph %d size increased from %d to %d (%d replications)\n",
			subgraph,
			num_states,
			total_states,
			total_states-num_states);

	// allocate movement maps for each partitioned edge table
	for (int i=0;i<my_nfa->distinct_subgraphs;i++) {
		my_nfa->partitioned_movement_maps[i]=(int **)malloc(color_size[i] * sizeof(int *));
		for (int j=0;j<color_size[i];j++)
			my_nfa->partitioned_movement_maps[i][j]=(int *)malloc(max_edges * sizeof(int)); 
	}

	return total_states - num_states;
}

