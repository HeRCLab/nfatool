#ifndef PARSE_ANML_H
#define PARSE_ANML_H

void traverse_graph (int i);
int reverse_edge_table ();
int extract_number (const char *str);
int Hash(const char *key);
int extract_string (char *str);
int count_states (xmlNode *anode);
void get_props_state (xmlNode *Node,int *id,int *start);
void get_props_edge (xmlNode *aNode,int *id,int *areport);
int fill_in_table (xmlNode *aNode,int current_state);

#endif
