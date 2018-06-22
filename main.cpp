#include "nfatool.h"

/*
 * global variabless
 */
unsigned char **next_state_table;

char  **symbol_table ;
int **gates_2D; 
int *gates_1D; 
int **next_2D; 


int **edge_table;
int **orig_edge_table;
int **reverse_table;
int **FanoutTable;

int *movement_map;

bool cycle_confirm = false;

unsigned char *report_table;
unsigned char *start_state;
unsigned char *symbol_set;

unsigned int *subnfa;
char *visited;

int subnfa_size;
int subnfa_total_size=0;

map <std::string,int> state_map;

int duplicate_states=0;
int subnfa_num;

int  state_map2[STATEMAP_SIZE];

xmlNode **node_table;

int max_fanout=0;
int max_stes=0;

vector<int> visited2;

vector<int> strong_cycles;

int num_states=0,
    STEinFile = 0,
    file_state = 0,
    states = 0,/*< counter for state-transition-element used in function "count_states" */
    starts = 0, /*< counter for start_stes table */
    max_edges = 0, /*< highest amount of activate-on-match a state-transition-element could have */
    edges = 0, /*< counter for activate-on-match */
    edge_num = 0,
    files_generated = 0,/*<counter for file number printed*/
    num_reports = 0, /*< counter for report-on-match */
    Change_of_start = 0, /*< counter for state-transition-element that holds a start state */
    max_fan_in = 0,
    current_color = 1,
    cycle_count =0,
    start_color,
    path_compare=0,
    path_length=0,
    max_path =1,
    Path_compare = 0,
    max_reverse_edges,
    color_count[MAX_COLORS], 
    max_fan=0;

xmlNode *rootGlobal, /*< root node of the .ANML file */
        *start_stes[STATEMAP_SIZE];

FILE *Destination;

xmlDoc *document; /*< .ANML document that will be used on the program */

int *ExactOutEdge;
int *visitedcycle;
int *visitedColorTrav;

vector<int> *state_colors;
vector<int> *sccs;
map <int,vector<int> > component_list;
int *components;
int largest_component_size=0,largest_component;

int *dfs_visited;
int max_loop=0;
int max_loop_constituent=-1;

int **edge_list; 
int **reverse_list; 
/*
    global variables to find the maximum strong components number
 */

int start_of_cycle=0;
int potential_node_in_cycle=0;
int possible_end_of_cycle=0;
int max_strong_node=0;

vector<int> deepest_path;
int deepest=0;

int total_visited2_size=0;

int *root_node;

int main(int argc, char **argv){
    char filename[1024];
    char *filename2,c;

    xmlNode *root;
    xmlNode *root2;
    xmlDoc *document2;
    xmlDoc *Doc2;

    char str[1024];

    int file_spec=0;

    while ((c=getopt(argc,argv,"i:m:f:p:x:"))!=-1)
      switch (c) {
        case 'i':
          strcpy(filename,optarg);
          file_spec=1;
          break;
        case 'm':
          max_stes=atoi(optarg);
          printf ("setting max STEs per file to %d\n",max_stes);
          break;
        case 'f':
          max_fanout=atoi(optarg);
          printf("setting max fanout to %d\n",max_fanout);
          break;
 	case 'x':
          max_fan=atoi(optarg);
          printf("setting max fanout to %d\n",max_fan);
          break;

        case '?':
          if ((optopt == 'm' || optopt == 'f'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr,"Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
              return 1;
        }

    if (!file_spec) {
      fprintf(stderr, "ERROR:  -i parameter is required (needed to specify input file\n");
      return 0;
    }

/*
 * parse ANML file
 */
    document = xmlReadFile(filename, NULL, 0);
    if (!document) {
      fprintf(stderr,"Error: could not open ANML file\n");
      return 0;
    }
    root = xmlDocGetRootElement(document);
    rootGlobal = root; 

/*
 * find number of STEs in the ANML file
 */
    num_states=count_states(root);
    printf ("number of states = %d\n",num_states);

/*
 * allocate memory for graph data structures
 */
  allocate_memory();

/*
 * traverse ANML and transfer graph into tables
 */
  fill_in_table(root->children, 0);

 

	// Next_state_table bit stream  

  	FILE *myfile3 ;
        myfile3 = fopen("ns_bitstream.txt", "w+") ;

        for(int i=0; i<num_states; i++) {
                fprintf(myfile3, "\n");

                for(int j=0; j<256; j++){
		//	next_2D[i][j] = next_state_table[i][j]; 
                        fprintf(myfile3 , "%d ", next_state_table[i][j]);
		}
        }

        fclose(myfile3);

	dofile_next(); //num_states, next_state_table); //num_states, next_state_table[256][num_states]); 


	// gates bit stream

	for(int i=0; i<num_states; i++) {
                for(int j=0; j<max_edges; j++) {
			int dest = edge_table[i][j]; 
				gates_2D[i][abs(dest-i)]=1;
		}
	if(report_table[i]) gates_2D[i][109] =1; 
        }

    
	myfile3 = fopen("gates_bitstream.txt", "w+"); 
	
	for(int i=0; i< 1024; i++) {	
		fprintf(myfile3, "\n"); 
		for(int j=0; j<110; j++) 
			fprintf(myfile3, "%d", gates_2D[i][j]); 

	}



// for(int i=0; i< num_states; i++) 
//	for(int j=0; j<256; j++) 
//		printf("next_state_table[%d][%d] = %d\n", i, j, next_state_table[i][j]); 

/*
 * finds the critical path of tree
 */
  for (int i = 0; i < num_states; i++)

  {
//    fprintf(stderr,"STE %d (\"%s\") )!\n",i,ANML_NAME(i)); 
    visited[i] = 0;
  }

//  printf("---------------------------------------%s\n", (char *) node_table[11]); 
  
/*
 *finds the largest critical path
 */
  //critical_path(0);
  //printf("critical path is: %d\n", path_compare);

/*
 *finds the biggest strongly connected components as an int
 */
//  pass_one();
//  dump_dot_file((char *)"Omari_scc", root, strong_cycles);

/*
 * calculate transpose graph
 */

  reverse_edge_table();

 /* 
  * Find the strongly connected components and optionally dump to file
 */ 
  find_sccs();

#ifdef DEBUG
  printf ("largest component is %d (size=%d)\n",largest_component,largest_component_size);
  /*
  for (k=0;k<largest_component_size;k++) printf("%d (%s) ",component_list[largest_component][k],
  node_table[component_list[largest_component][k]]->properties->children->content);
  printf ("\n");
  */
  dump_dot_file((char *)"largest_component",rootGlobal,component_list[largest_component],0);
#endif

/*
 * validate largest SCC
 */
  /*
  vector<int> path;
  int component_ok = 1;
<<<<<<< HEAD
  for (int i=0;i<component_list[largest_component].size();i++) {
    for (int j=i;j<component_list[largest_component].size();j++) {
      if (!find_loop_path(i,j,path,0)) {
        component_ok=0;
    //    fprintf(stderr,"error:  component %d has unreachable path %d (\"%s\") -> %d (\"%s\")\n",
    //                                                                          largest_component,i,ANML_NAME(i),j,ANML_NAME(j));
      }
    }
  }
  */

/*
 * find maximum loop
 */
  printf ("max loop size = %d, constituent = %d\n",max_loop,max_loop_constituent);

  if (max_loop != 0) {
    // reset the visited array since we will recycle it
    vector<int> max_loop_path;
    int ste = 0;
    for (int i=0;i<num_states;i++) visited[i]=0;
    find_loop_path(max_loop_constituent,max_loop_constituent,max_loop_path,1);
    dump_dot_file((char *)"max_loop",root,max_loop_path,0);
  }

  /* 
   * Find the critical path , longest path in the tree 
  */ 
  find_critical_path(); 
  
//  printf ("deepest path = %d\n",deepest);
  
//  for (int i=0;i<deepest_path.size();i++) printf ("%d (%s)->",
//                          deepest_path[i],
//                          node_table[deepest_path[i]]->properties->children->content);
//  printf ("\n");
  
  /*
   * Partition the graph
   */
  if (max_stes) partition(max_stes);


//for(int i=0; i< num_states; i++) 
//	printf("report[%d] = %d\n", i, report_table[i]);  




  /* 
   * Validate partitions 
   */ 


// for (int i=0; i<num_states; i++) printf("%d (%s) ->", 
//				node_table[i], 
//				node_table[i]->properties->children->content); 
				
//   printf("\n"); 



/*
 * save the original state of the graph, since we'll be changing it significantly
 */
//  for (int i=0;i<num_states;i++) for (int j=0;j<max_edges;j++) orig_edge_table[i][j]=edge_table[i][j];

/*
 * partition graph into sub-NFAs, if the user specified a maximum number of STEs per graph
 */
 // if (max_stes) becchi_partition();

/*
 * map logical STEs to physical STEs, if the user specified a maximum hardware fan-out
 */
  int violations;
  int i=0;
  if (max_fanout) while (violations=validate_interconnection()) {
    if (!(i%1)) printf ("*********************scan complete, violations = %d\n",violations);
   	 i++;
  };


//for(int l=0; l<num_states; l++) 
//	printf("%s\n", symbol_table[l]); 

  return 0;
}
