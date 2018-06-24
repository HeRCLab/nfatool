#include "nfatool.h"

void allocate_memory() {
  node_table = (xmlNode **)malloc(sizeof (xmlNode *)*num_states);

//  symbol_set = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);

  symbol_table= (char **)malloc((sizeof(char*))*num_states);
  for(int i= 0;i< num_states;i++) symbol_table[i] = (char *)malloc(sizeof (char)*1024);


  next_2D = (int **)malloc((sizeof(int *))*256); 
  for(int i=0; i<256; i++) {
	next_2D[i] = (int *)malloc(sizeof(int)*num_states); 
	for(int j=0; j<num_states; j++) next_2D[i][j]=0; 
  } 


 // Gates_2D size = Hardware max_stes x max_fan 
  gates_2D = (int **)malloc((sizeof(int *))*max_stes);
  for (int i=0;i<max_stes;i++) {
    gates_2D[i]=(int *)malloc(sizeof (int)*max_fan);
    for (int j=0;j<max_fan;j++) gates_2D[i][j]=0;
  }
 

 
  gates_1D = (int *)malloc((sizeof(int*))*(max_stes*max_fan+max_fan)); 
  for(int i=0; i<(max_stes*max_fan+max_fan);i++) gates_1D[i] = 0; 

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

	root_node = (int *)malloc((sizeof(int))*num_states);
	 for (int i=0;i<num_states;i++) root_node[i]=0;
	
    for (int i=0;i<MAX_COLORS;i++) color_count[i]=0;

    sccs = new vector<int>[num_states];
    components=(int *)malloc(sizeof(int)*num_states);
    for (int i=0;i<num_states;i++) components[i]=-1;




edge_list = (int **)malloc((sizeof(int *))*num_states);
  for (int i=0;i<num_states;i++) {
    edge_list[i]=(int *)malloc(sizeof (int)*max_edges);
    for (int j=0;j<max_edges;j++) edge_list[i][j]=-1;
  }




reverse_list = (int **)malloc((sizeof(int *))*num_states);
  for (int i=0;i<num_states;i++) {
    reverse_list[i]=(int *)malloc(sizeof (int)*max_edges);
    for (int j=0;j<max_edges;j++) reverse_list[i][j]=-1;
  }


}
