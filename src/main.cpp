#include "nfatool.h"

#ifdef DEBUG
	const char *mode = "DEBUG";
#else
	const char *mode = "RELEASE";
#endif

/*
 * global variabless
 */
 
/*
 * NFA data (used in multiple places)
 */
int						**edge_table;
int						**orig_edge_table;
int						**reverse_table;
char					**symbol_table;
unsigned char			*report_table;
unsigned char			*start_state;
unsigned char			*symbol_set;
map <std::string,int>	state_map;
int						state_map2[STATEMAP_SIZE];
int						max_fanout=0;
int						max_stes=0;
int						max_fan_in = 0;
int						max_edges = 0;

/*
 * NAPOLY configuration data
 */
unsigned char			**next_state_table;
int						**gates_2D; 
int						*gates_1D; 
int						**next_2D;
int						do_time;

/*
 * allocation/mapping algorithm
 */
int						*movement_map;

/*
 * Used in strong components analysis (why is it global?)
 */
bool					cycle_confirm = false;

/*
 * Used in early code to find distinct sub-graphs; no longer used and should
 * delete
 */
unsigned int			*subnfa;
int						subnfa_size;
int						subnfa_total_size=0;
int						subnfa_num;

/*
 * Used for partitioning and strongly connected component analysis
 */
char					*visited;
vector<int>				visited2;
vector<int>				strong_cycles;
vector<int>				*state_colors;
vector<int>				*sccs;
map <int,vector<int> >	component_list;
int						*components;
int						largest_component_size=0;
int						largest_component;
int						current_color = 1;
int						cycle_count =0;
int						start_color;
int						path_compare=0;
int						path_length=0;
int						max_path =1;
int						Path_compare = 0;
int						start_of_cycle=0;
int						potential_node_in_cycle=0;
int						possible_end_of_cycle=0;
int						max_strong_node=0;
vector<int>				deepest_path;
int						deepest=0;
int						max_reverse_edges;
int						color_count[MAX_COLORS];
int						max_fan=0;
int						*dfs_visited;
int						max_loop=0;
int						max_loop_constituent=-1;
int						*root_node;

/*
 * Used in parsing ANML
 */
xmlNode					**node_table;
int						edges=0;
int						states=0;
int						file_state=0;

/*< root node of the .ANML file */
xmlNode					*rootGlobal;
xmlNode					*start_stes[STATEMAP_SIZE];

/*< .ANML document that will be used on the program */
xmlDoc					*document;
int						edge_num = 0;
int						num_states=0;
int						num_reports = 0;

/*< counter for report-on-match */    
int						Change_of_start = 0;

/*< counter for state-transition-element that holds a start state */
int						starts = 0;

void print_help (char **argv) {
	
	printf("         __      _              _\n");
	printf("        / _|    | |            | |\n");
	printf("  _ __ | |_ __ _| |_ ___   ___ | |\n");
	printf(" | '_ \\|  _/ _` | __/ _ \\ / _ \\| |\n");
	printf(" | | | | || (_| | || (_) | (_) | |\n");
	printf(" |_| |_|_| \\__,_|\\__\\___/ \\___/|_|\n\n");
                                              
	printf("nfatool:  map NFAs onto an FPGA-based automata overlay\n");
	printf("Designed for NAPOLY (Nondeterministic Automata Processor OverLay)\n");
	printf("Build:  %s %s (%s)\n\n",__DATE__,__TIME__,mode);

	printf("Usage:  %s -i <ANML input file> [options]\n\n",argv[0]);
	printf("Options:\n");
	printf("%14s\tPrint this help and exit\n","-h");
	printf("%14s\tUse partitioning to limit maximum SEs to <num>\n","-m <num>");
	printf("%14s\tSet maximum fan-out to <num>\n","-f <num>");
	printf("%14s\tPerform mapping with MINICRYPTOSAT solver\n","-c <timeout in seconds>");
	printf("%14s\tPerform NFA graph analysis\n","-g");
	printf("%14s\tGenerate NAPOLY configuration files\n","-n");
	printf("%14s\tPrint state-to-SE and SE-to-state mapping\n","-p");
	printf("%14s\tFind distinct subgraphs\n","-s");
}   printf("%14s\tFor subgraphs that don't map, decompose into subgraphs with maximum logical fanout of <n> (requires -s and -c)","-d <n>");

int main(int argc, char **argv){
    char c;
    FILE *myFile;
	int i,j;
	int violations;
	
	nfa *my_nfa;
	
	// FLAGS
	int graph_analysis=0;
	int use_sat_solver=0;
    int file_spec=0;
	int gen_config=0;
	int do_print_mapping=0;
	int subgraphs=0;
	
	// OPTIONS
    char filename[1024];
	int max_fanout=0;
	int timeout;
	int decompose_fanout=-1;

    while ((c=getopt(argc,argv,"hi:m:f:c:gsd:"))!=-1)
      switch (c) {
		case 'h':
			print_help(argv);
			return 0;
			break;
		case 'i':
			strcpy(filename,optarg);
			printf ("INFO: Setting input file to \"%s\"\n",filename);
			file_spec=1;
			break;
        case 'm':
			max_stes=atoi(optarg);
			printf ("INFO: Setting max STEs per file to %d\n",max_stes);
			break;
		case 'c':
			timeout=atoi(optarg);
			printf("INFO: Using SAT solver with timeout of %d s\n",timeout);
			use_sat_solver=1;
			break;
        case 'f':
			max_fanout=atoi(optarg);
			printf("INFO: Setting max fanout to %d\n",max_fanout);
			break;
		case 'd':
			decompose_fanout=atoi(optarg);
			printf("INFO: Will decompose to logical fanout %d on mapping failure\n",decompose_fanout);
		case 'g':
			graph_analysis=1;
			printf("INFO: Performing graph analysis\n");
			break;
		case 'n':
			gen_config=1;
			printf("INFO: Generating NAPOLY configuration file\n");
			break;
		case 'p':
			do_print_mapping=1;
			printf("INFO: Printing mapping result\n");
			break;
		case 's':
			subgraphs=1;
			printf("INFO: Finding distinct subgraphs\n");
			break;
        case '?':
			if ((optopt == 'i' || optopt == 'm' || optopt == 'f' || optopt == 'c' || optopt == 'd'))
				fprintf (stderr, "ERROR: Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr,"ERROR: Unknown option `-%c'.\n", optopt);
            else {
                fprintf (stderr,"ERROR: Unknown option character `\\x%x'.\n",optopt);
			}
			return 0;
        }

	// make sure input file is specified
    if (!file_spec) {
      fprintf(stderr, "ERROR: -i parameter is required (needed to specify input file\n");
	  print_help(argv);
      return 0;
    }
	
	my_nfa = new nfa;
	
	// TODO:  add all necessary arguments to fill in NFA data structure
	if (!read_anml_file (filename,my_nfa)) {
		fprintf(stderr,"ERROR: Error parsing input file.\n");
		return 0;
	}
	
	// reverse the edge table
	reverse_edge_table(my_nfa,0);
	
	// find distinct subgraphs
	if (subgraphs) {
		find_subgraphs(my_nfa);
		// reverse subgraph tables
		reverse_edge_table(my_nfa,1);
	}
	
	// graph analysis (not working)
	if (graph_analysis) {
		perform_graph_analysis(my_nfa->root);
	}

	if (max_fanout) {
		my_nfa->max_fanout = max_fanout;
	}
	
	if (max_fanout && !use_sat_solver) {
		// perform mapping heuristic
		perform_state_mapping(filename,my_nfa);
	}
	
	if (gen_config) {
		// configure next_State_table as do file 
		dofile_next(); 
		// configure gates table as do file 
		dofile_gates(); 
    }
	
	if (use_sat_solver) {
		// check for fanout constraint
		// if none then there is no reason to proceed
		if (!max_fanout) {
			fprintf(stderr,"ERROR:  cannot generate CNF files without a specified maximum fanout.\n");
			return 0;
		}
		if (map_states_with_sat_solver(filename,
									   my_nfa,
									   subgraphs,
									   timeout,
									   decompose_fanout)==0) return 0;
	}

	if (do_print_mapping) print_mapping(my_nfa);

  return 0;
}
