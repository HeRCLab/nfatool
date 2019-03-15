#ifndef PARSE_ANML_H
#define PARSE_ANML_H

int read_anml_file (char *filename,
					nfa *my_nfa);
			
int count_states (nfa *my_nfa);
				  
int reverse_edge_table (nfa *my_nfa);
int extract_number (const char *str);
int Hash(const char *key);
int extract_string (char *str);
int fill_in_table (nfa *my_nfa);

#endif
