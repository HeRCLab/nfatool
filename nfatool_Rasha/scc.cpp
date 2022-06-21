#include "nfatool.h"

void visit(int **edge_table,int num_states,int max_edges,int current_node,int *visited,list<int> *L) {
	int i,out_edge;

	if (!visited[current_node]) {
		visited[current_node]=1;
		for (i=0;i<max_edges;i++) {
			out_edge = edge_table[current_node][i];
			if (out_edge==-1) break;
			visit(edge_table,num_states,max_edges,out_edge,visited,L);
		}
		L->push_front(current_node);
	}
}

void assign(int u,int root,int **reverse_table,int max_fanin,int *components) {
	int i,in_edge;

	if (components[u]==-1) {
		components[u] = root;
		for (i=0;i<max_fanin;i++) {
			in_edge=reverse_table[u][i];
			if (in_edge==-1) break;
			assign(in_edge,root,reverse_table,max_fanin,components);
		}
	}
}

/**
 * \brief		Find strongly-connected components in a graph or sub-graph
 * \param[in]	my_nfa		nfa data structure
 * \param[in]	subgraph	target subgraph number
 */
void find_sccs (nfa *my_nfa,int subgraph,int *components) {
	int i,root;

	// get a handle to the current subgraph
	int **edge_table = my_nfa->edge_tables[subgraph],
	**reverse_table = my_nfa->reverse_tables[subgraph],
	num_states = my_nfa->subgraph_size[subgraph],
	max_edges = my_nfa->max_edges,
	max_fanin = my_nfa->max_fan_in;

	// initialize components
	for (i=0;i<num_states;i++) components[i]=-1;
	
	// allocate and initialize a "visited array"
	int *visited;
	visited = (int *)malloc(num_states * sizeof(int));
	for (i=0;i<num_states;i++) visited[i]=0;

	// allocate a list L, needed by the algorithm
	list<int> L;

	// step 2 of algorithm at https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm
	for (i=0;i<num_states;i++)
		visit(edge_table,num_states,max_edges,i,visited,&L);

	// step 3 of algorithm at https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm
	for (list<int>::iterator it=L.begin(); it != L.end(); it++) {
		assign(*it,*it,reverse_table,max_fanin,components);
	}
}
