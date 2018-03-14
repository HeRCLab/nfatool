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

int max_fanout=0,max_stes=0;

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
    current_color = 0,
    cycle_count =0,
    start_color,
    color_count[MAX_COLORS];

xmlNode *rootGlobal, /*< root node of the .ANML file */
        *start_stes[STATEMAP_SIZE];

FILE *Destination;

xmlDoc *document; /*< .ANML document that will be used on the program */

int *ExactOutEdge;
int *visitedcycle;
int *visitedColorTrav;

vector<int> *state_colors;

//---------------------------------------------------------------------
int main_rasha(void) {
  int *components,
      *assigned,
      i;

  num_states=6;
  max_edges=3;

  components=(int *)malloc(sizeof(int)*num_states);
  for (i=0;i<num_states;i++) components[i]=-1;

  assigned=(int *)malloc(sizeof(int)*num_states);
  for (i=0;i<num_states;i++) assigned[i]=0;

  edge_table = (int **)malloc((sizeof(int *))*num_states);
  for (i=0;i<num_states;i++) {
    edge_table[i]=(int *)malloc(sizeof (int)*max_edges);
    for (int j=0;j<max_edges;j++) edge_table[i][j]=-1;
  }
  
  // Store Graph 1->2->3->4->5  cycles: 1->1, 1->2->3->4->5->1, 1->2->3->4->5->3
  edge_table[0][0] = 1;
  edge_table[0][1] = 4;
  edge_table[0][2] = -1;
  edge_table[1][0] = 2;
  edge_table[1][1] = -1;
  edge_table[2][0] = 3;
  edge_table[2][1] = -1;
  edge_table[3][0] = 2;
  edge_table[3][1] = -1;
  edge_table[4][0] = 5;
  edge_table[4][1] = -1;
  edge_table[5][0] = 4;
  edge_table[5][1] = 0;
  edge_table[5][2] = -1;

  reverse_edge_table();
  dfs(0);
  for(int i=visited2.size()-1 ; i>=0; i--) {
    assign(visited2[i],visited2[i],components);

  }

    for(int j=0; j<visited2.size(); j++) {
      printf("node %d assigned to component %d\n",j,components[j]);
    }

  return 0;
}

int main(int argc, char **argv){
    char filename[1024];
    char *filename2,c;

    xmlNode *root;
    xmlNode *root2;
    xmlDoc *document2;
    xmlDoc *Doc2;

    int basket;
    int file_spec=0;
    int temp = 0;
    int val = 0;
    int fanin = 0;

    char str[1024];

    srand(10202);

    if (argc < 2){
        fprintf(stderr, "Usage: %s filename.xml [options]\n", argv[0]);
        return 1;
    }

    while ((c=getopt(argc,argv,"i:m:f:"))!=-1)
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
      fprintf(stderr, "ERROR:  -i parameter is required\n");
      return 0;
    }

    document = xmlReadFile(filename, NULL, 0);
    root = xmlDocGetRootElement(document);
    rootGlobal = root;

//    for(int i= 0 ; i < (1<<24); i++) state_map[i] = -1;

    num_states=count_states(root);

  allocate_memory();

  for (int i=0;i<num_states;i++) visited[i]=0;

  fill_in_table(root->children, 0);

  for (int i = 0; i < num_states; i++)
  {
    state_colors[i] = 1;
  }
  
  subnfa_num=0;
  for (int i=0;i<num_states;i++) {
	if (start_state[i]) {
		for (int j=0;j<num_states;j++) visited[j]=0;
		subnfa_size=0;
		traverse_graph(i);
		printf ("sub nfa %d, size = %d\n",subnfa_num,subnfa_size);
		subnfa_num++;
		subnfa_total_size += subnfa_size;
	}
  }
  printf ("state duplications = %d, should be %d - %d\n",duplicate_states,subnfa_total_size,num_states);
  
  reverse_edge_table();
  
  for (int i=0;i<num_states;i++) for (int j=0;j<max_edges;j++) orig_edge_table[i][j]=edge_table[i][j];

  int violations;
  int i=0;
  if (max_fanout) while (violations=validate_interconnection()) {
    if (!(i%1)) printf ("*********************scan complete, violations = %d\n",violations);
    i++;
  };

  return 0;
}
