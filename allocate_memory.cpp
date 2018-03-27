#include "nfatool.h"

void allocate_memory() {
  node_table = (xmlNode **)malloc(sizeof (xmlNode *)*num_states);

  symbol_set = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);

  next_state_table= (unsigned char **)malloc((sizeof(char*))*num_states);
  for(int i= 0;i< num_states;i++) next_state_table[i] = (unsigned char *)malloc(sizeof (unsigned char)*256);

  report_table = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);
    for (int i=0;i<num_states;i++) report_table[i]=0;

  edge_table = (int **)malloc((sizeof(int *))*num_states);
  for (int i=0;i<num_states;i++) {
    edge_table[i]=(int *)malloc(sizeof (int)*max_edges);
    for (int j=0;j<max_edges;j++) edge_table[i][j]=-1;
  }

  orig_edge_table = (int **)malloc((sizeof(int*))*num_states);
  for (int i=0;i<num_states;i++){
    orig_edge_table[i]=(int *)malloc(sizeof (int)*max_edges);
    for (int j=0;j<max_edges;j++) orig_edge_table[i][j]=-1;
  }
  
  movement_map = (int *)malloc(sizeof(int)*num_states);
  for (int i=0;i<num_states;i++) movement_map[i]=i;
  subnfa = (unsigned int *)malloc((sizeof(unsigned int))*num_states);
  for (int i=0;i<num_states;i++) subnfa[i]=-1;
  visited = (char *)malloc((sizeof(char))*num_states);
  
  start_state = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);

  ExactOutEdge = (int *)malloc((sizeof(int))*num_states);
  for(int i = 0; i < num_states; i++) ExactOutEdge[i] = 0;

  visitedcycle = (int *)malloc((sizeof(int))*num_states);
    for(int i = 0; i < num_states; i++) visitedcycle[i] = 0;

  visitedColorTrav = (int *)malloc((sizeof(int))*num_states);
    for (int i = 0; i < num_states; i++) visitedColorTrav[i] = 0;

  dfs_visited = (int *)malloc((sizeof(int))*num_states);

  for (int i = 0; i < num_states; i++)
    {
      for (int j = 0; j < max_edges; j++)
      {
        if (edge_table[i][j] != -1)
        {
          ExactOutEdge[i]+=1;
        }
        else
          break;
      }
    }

    state_colors = new vector<int>[num_states];

    for (int i=0;i<MAX_COLORS;i++) color_count[i]=0;

    sccs = new vector<int>[num_states];
    components=(int *)malloc(sizeof(int)*num_states);
    for (int i=0;i<num_states;i++) components[i]=-1;
}
