#ifndef PARSE_ANML_H
#define PARSE_ANML_H

int read_anml_file (char *filename,
					nfa *my_nfa);

int reverse_edges_core (int num_states,
						 int max_edges,
						 int **edge_table,
						 int ***reverse_table);
			
int count_states (nfa *my_nfa);
				  
int reverse_edge_table (nfa *my_nfa,int subgraph);
int extract_number (const char *str);
int Hash(const char *key);
int extract_string (char *str);
int fill_in_table (nfa *my_nfa);
void graph_features(nfa *my_nfa, int node); 
#endif
