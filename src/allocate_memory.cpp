#include "nfatool.h"

void allocate_memory(nfa *my_nfa) {
	int num_states = my_nfa->num_states;
	int max_fanout = my_nfa->max_fanout;
	int max_stes = my_nfa->max_stes;
	char *visited = my_nfa->visited;
	int	*dfs_visited = my_nfa->dfs_visited;
	vector<int> *state_colors = my_nfa->state_colors;
	int *root_node = my_nfa->root_node;
	vector<int> *sccs = my_nfa->sccs;
	int *components = my_nfa->components;
	
	// node table:  maps state numbers back to its original XML node
	my_nfa->node_table = (xmlNode **)malloc(sizeof (xmlNode *)*num_states);

	// symbol table:  characters on which each state activates
	my_nfa->symbol_table= (char **)malloc((sizeof(char*))*num_states);
	for (int i = 0;i < num_states;i++)
		my_nfa->symbol_table[i] = (char *)malloc(sizeof (char)*1024);

	//////////////////////////////////////////////////////////////
	// NAPOLY configuration arrays
	//////////////////////////////////////////////////////////////
	my_nfa->next_2D = (int **)malloc((sizeof(int *))*256); 
	for(int i=0; i<256; i++) {
		my_nfa->next_2D[i] = (int *)malloc(sizeof(int)*num_states); 
		for(int j=0; j<num_states; j++) my_nfa->next_2D[i][j]=0; 
	} 

	// Gates_2D size = Hardware max_stes x max_fan 
	// my_nfa->gates_2D = (int **)malloc((sizeof(int *))*max_stes);
	// for (int i=0;i<max_stes;i++) {
		// my_nfa->gates_2D[i]=(int *)malloc(sizeof (int)*max_fanout);
		// for (int j=0;j<max_fanout;j++) my_nfa->gates_2D[i][j]=0;
	// }
 
	// my_nfa->gates_1D = (int *)malloc((sizeof(int*))*(max_stes*max_fanout+max_fanout)); 
	// for(int i=0; i<(max_stes*max_fanout+max_fanout);i++) my_nfa->gates_1D[i] = 0;
	//////////////////////////////////////////////////////////////
	
	// next state table:  how is this different from symbol table?
	my_nfa->next_state_table= (unsigned char **)malloc((sizeof(char*))*num_states);
	for (int i= 0;i< num_states;i++)
		my_nfa->next_state_table[i] = (unsigned char *)malloc(sizeof (unsigned char)*256);

	// report table
	my_nfa->report_table = (int *)malloc((sizeof(int))*num_states);
    for (int i=0;i<num_states;i++)
		my_nfa->report_table[i]=0;

	// edge table
	my_nfa->edge_table = (int **)malloc((sizeof(int *))*num_states);
	for (int i=0;i<num_states;i++) {
		my_nfa->edge_table[i]=(int *)malloc(sizeof (int)*my_nfa->max_edges);
		for (int j=0;j<my_nfa->max_edges;j++) my_nfa->edge_table[i][j]=-1;
	}

	// original edge table
	my_nfa->orig_edge_table = (int **)malloc((sizeof(int*))*num_states);
	for (int i=0;i<num_states;i++) {
		my_nfa->orig_edge_table[i]=(int *)malloc(sizeof (int)*my_nfa->max_edges);
		for (int j=0;j<my_nfa->max_edges;j++) my_nfa->orig_edge_table[i][j]=-1;
	}
  
	// movement map
	my_nfa->movement_map = (int *)malloc(sizeof(int)*num_states);
	for (int i=0;i<num_states;i++) my_nfa->movement_map[i]=i;
	
	// start states
	my_nfa->start_state = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);
	
	//////////////////////////////////////////////////////////////
	// these are for graph analysis, leaving global for now
	//////////////////////////////////////////////////////////////
	visited = (char *)malloc((sizeof(char))*num_states);
	dfs_visited = (int *)malloc((sizeof(int))*num_states);
    state_colors = new vector<int>[num_states];
	root_node = (int *)malloc((sizeof(int))*num_states);
	for (int i=0;i<num_states;i++) root_node[i]=0;
	
	int *color_count = my_nfa->color_count = (int *)malloc(sizeof(int)*MAX_COLORS);
    for (int i=0;i<MAX_COLORS;i++) color_count[i]=0;
    sccs = new vector<int>[num_states];
    components=(int *)malloc(sizeof(int)*num_states);
    for (int i=0;i<num_states;i++) components[i]=-1;
	//////////////////////////////////////////////////////////////
}
