#ifndef VISUALIZATION_H
#define VISUALIZATION_H

int perform_graph_analysis (xmlNode *root);
void dump_dot_file (char *filename, xmlNode *aNode, vector<int> subset, int colors);

#endif
