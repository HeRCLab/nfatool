#ifndef ALLOCATE_STES_H
#define ALLOCATE_STES_H

/*
 * state-to-SE mapping routines
 */
 
// perform SAT-based state-mapping
int map_states_with_sat_solver (char *filename,nfa *my_nfa,int subgraph,int timeout,int decompose_fanout);

// mapping heuristic described in Karakchi et. al, ReConFig 2017
int perform_state_mapping (char *filename,nfa *my_nfa);

/*
 * internal/auxillery routines
 */
 
// applys a mapping from the SAT solver
void apply_movement_map (int **edge_table,
						 int **orig_edge_table,
						 int num_states,
						 int max_edges,
						 int *movement_map);

// mapping SAT literals to state mappings and reverse
int state_to_se_literal (int state,int se,int num_states);
void literal_to_mapping (int literal,int *state, int *se,int num_states);

// generate CNF logic file to describe mapping problem
int perform_cnf_translation (int num_states,int max_edges,int **edge_table,int max_fanout,char *filename);

// checks for mapping violations and does one pass of corrections if needed
// returns number of violations after first pass
int validate_interconnection(int **edge_table,
							 int **reverse_table,
							 int num_states,
							 int max_edges,
							 int max_fan_in,
							 int *movement_map,
							 int **orig_edge_table,
							 int max_fanout);

// move a state in the array
void move_ste (int num_states,
			   int max_edges,
			   int max_fan_in,
			   int **edge_table,
			   int **reverse_table,
			   int *movement_map,
			   int from,
			   int to);

// mapping score function for the heuristic
int score(int max_edges,int **edge_table,int **reverse_table,int a, int b);

// miscellaneous
int reverse_movement_map (int num_states,int *movement_map,int n);
int dump_edges ();
void critical_path(int node);
void mix_it_up(nfa *my_nfa,int n);
char *anml_name (nfa *my_nfa,int se);

// validation
void check_graphs (int num_states,
				   int max_edges,
				   int **edge_table,
				   int *movement_map,
				   int **orig_edge_table,
				   int rev);
void print_mapping (nfa *my_nfa);

#endif
