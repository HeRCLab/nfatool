#include "nfatool.h"

/*
 * global variabless
 */
unsigned char **next_state_table;

int **edge_table;
int **orig_edge_table;
int **reverse_table;
int **FanoutTable;

int *movement_map;

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
    max_path =1,
    path_compare = 0,
    color_count[MAX_COLORS];

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

int main(int argc, char **argv){
    char filename[1024];
    char *filename2,c;

    xmlNode *root;
    xmlNode *root2;
    xmlDoc *document2;
    xmlDoc *Doc2;

    char str[1024];

    int file_spec=0;

    while ((c=getopt(argc,argv,"i:m:f:p:"))!=-1)
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

/*
 * finds the critical path of tree
 */
  for (int i = 0; i < num_states; i++)
  {
    visited[i] = 0;
  }

  critical_path(0);


//  critical_path(0);


/*
 * calculate transpose graph
 */

  reverse_edge_table();

 /* 
  * Find the strongly cycles components 
 */ 

  find_sccs();
 
  /* 
   * Find the critical path , longest path in the tree 
  */ 
  find_critical_path(); 
  






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
 // int violations;
 // int i=0;
 // if (max_fanout) while (violations=validate_interconnection()) {
  //  if (!(i%1)) printf ("*********************scan complete, violations = %d\n",violations);
   // i++;
//  };


  return 0;
}
