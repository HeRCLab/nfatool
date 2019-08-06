#ifndef VISUALIZATION_H
#define VISUALIZATION_H

int perform_graph_analysis (xmlNode *root);
void dump_dot_file (char *filename, int **edge_table, int num_states, int max_edges);

#endif
