#ifndef ALLOCATE_STES_H
#define ALLOCATE_STES_H

int perform_cnf_translation (int **clauses,nfa *my_nfa,char *filename);
int perform_state_mapping (char *filename);
void move_ste (int from, int to);
int Score(int a, int b);
int reverse_movement_map (int n);
void check_graphs ();
int dump_edges ();
int validate_interconnection();
void critical_path(int node);
void mix_it_up(int n);

#endif
