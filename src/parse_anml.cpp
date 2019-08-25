#include "nfatool.h"
#include <string.h> 

int read_anml_file (char *filename,
					nfa *my_nfa) {

/*
 * parse ANML file
 */
    my_nfa->document = xmlReadFile(filename, NULL, 0);
    if (!my_nfa->document) {
      fprintf(stderr,"ERROR: could not open ANML file\n");
      return 0;
    }
	
    my_nfa->root = xmlDocGetRootElement(my_nfa->document);

/*
 * find number of STEs in the ANML file
 */
    count_states(my_nfa);
    printf ("INFO: Number of states = %d\n",my_nfa->num_states);

/*
 * allocate memory for graph data structures
 */
  allocate_memory(my_nfa);

/*
 * traverse ANML and transfer graph into tables
 */
  fill_in_table(my_nfa);
  
/*
 * save the original state of the graph
 */
  for (int i=0;i<my_nfa->num_states;i++)
	  for (int j=0;j<my_nfa->max_edges;j++)
		  my_nfa->orig_edge_table[i][j]=my_nfa->edge_table[i][j];
  
  return 1;
}

int reverse_edges_core (int num_states,
						 int max_edges,
						 int **edge_table,
						 int ***reverse_table) {
							 
	int change,i,j,k,fanin = 0,max_fan_in=0;
	int *rev_edge_count;
	int **my_reverse_table;
	
	rev_edge_count = (int *)malloc(sizeof(int)*num_states);
		
	// reset the fan-in for each state
	for (i=0;i<num_states;i++) rev_edge_count[i]=0;
	
	// count number of incoming edges to each state
	// (compute max_fan_in)
	for (i = 0; i < num_states; i++) {
		fanin=0;
		for(j = 0; j<max_edges; j++) {
			if(edge_table[i][j] == -1) {
				break;
			} else {
			  rev_edge_count[edge_table[i][j]]++;
			  if (rev_edge_count[edge_table[i][j]] > max_fan_in) max_fan_in = rev_edge_count[edge_table[i][j]];
			}
		}
	}

	// allocates the reverse table
	my_reverse_table = *reverse_table = (int **)malloc((sizeof(int*))*num_states);
	for (int i=0;i<num_states;i++){
	  my_reverse_table[i]=(int *)malloc(sizeof (int)*max_fan_in);
	  for (int j=0;j<max_fan_in;j++) my_reverse_table[i][j]=-1;
	}

	for (i=0;i<num_states;i++) rev_edge_count[i]=0;

	// build the reverse_table
	for (int i = 0; i < num_states; i++){
	  for (int j = 0; j < max_edges; j++){
		if(edge_table[i][j] == -1){
		  break;
		}else {
		  my_reverse_table[edge_table[i][j]][rev_edge_count[edge_table[i][j]]++] = i;
		}
	  }
	}
	
	free (rev_edge_count);
	return max_fan_in;
}

int reverse_edge_table (nfa *my_nfa,int subgraph) {
	int i,j;
	int num_states;
	int max_edges = my_nfa->max_edges;
	int **edge_table;
	int **reverse_table;
  
	if (!subgraph) {
		// if there are no subgraphs, treat this as a simple
		// wrapper
		num_states = my_nfa->num_states;
		edge_table = my_nfa->edge_table;
  
		my_nfa->max_fan_in=reverse_edges_core(num_states,max_edges,edge_table,&my_nfa->reverse_table);
	} else {
		// allocate reverse tables
		my_nfa->reverse_tables = (int ***)malloc(my_nfa->distinct_subgraphs*sizeof(int **));
		
		// for each subgraph, reverse the edge table
		for (i=0;i<my_nfa->distinct_subgraphs;i++) {
			num_states = my_nfa->subgraph_size[i];
			
			edge_table = my_nfa->edge_tables[i];
			my_nfa->reverse_tables[i] = (int **)malloc(num_states*sizeof(int *));
			for (j=0;j<num_states;j++)
				my_nfa->reverse_tables[i][j] = (int *)malloc(my_nfa->max_edges*sizeof(int));
			
			my_nfa->max_fan_in=reverse_edges_core(num_states,max_edges,edge_table,&my_nfa->reverse_tables[i]);
		}
	}
}

int extract_number (const char *str) {
  int i,j,start,end;
  char str2[1024];

  end = strlen(str);

  for (i=0;i<strlen(str);i++) if (isdigit(str[i])) {start=i;break;}
  if (i==strlen(str)) {
    fprintf (stderr,"ERROR: id extraction failed for \"%s\"\n",str);
    exit(0);
  }
  for (j=i+1;j<strlen(str);j++) if (!isdigit(str[j])) {end=j;break;}

  strncpy(str2,str+start,end-start);
  str2[end-start]=0;
  return atoi(str2);
}

int extract_string (char *str) {
  int i,
      j=0,
      start,
      end;
  char str2[64];

  for (i=0;i<strlen(str);i++) {
    if (str[i]>='!' && str[i]<='~') break;
  }

  start = i;

  for (;i<strlen(str);i++) {
    if (str[i]<'!' || str[i]>'~') break;
  }

  end=i;

  for (i=start;i<end;i++) str2[j++] = str[i];
  str2[j]=0;

  return atoi(str2);
}

int count_states (nfa *my_nfa) {
	int tmp;
	xmlAttr *attr;
	int states=0;
	int max_edges=0;
	int edges;
	xmlNode *bnode;
	xmlNode *anode=my_nfa->root;

	// check if the root XML node is the tag "automata-network"
	// if not, then it's probably "anml", in which case we need
	// iterate through its childen to find "automata-network"
	if (my_nfa->root->type==XML_ELEMENT_NODE && strcmp((const char *)my_nfa->root->name,"automata-network"))
		for (anode = my_nfa->root->children; anode; anode = anode->next)
			if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"automata-network")) break;
	
	for (anode = anode->children; anode; anode = anode->next) {
		if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"state-transition-element")) {
			
			edges=0;
			for (bnode = anode->children; bnode; bnode = bnode->next)
				if (bnode->type==XML_ELEMENT_NODE && !strcmp((const char *)bnode->name,"activate-on-match")) edges++;
			if (edges>max_edges) max_edges=edges;
		
			for (attr = anode->properties; attr; attr = attr->next) {
				if (!strcmp((const char *)attr->name,"id")) {
					my_nfa->state_map[(const char *)attr->children->content]=states;
					my_nfa->state_map2[states]=(const char *)attr->children->content;
				}
			}
			states++;
		}
	}
	
    my_nfa->num_states = states;
	my_nfa->max_edges = max_edges;
		
	return 1;
}

int fill_in_table (nfa *my_nfa) {
	xmlNode *bnode;
	xmlNode *anode = my_nfa->root;
	xmlAttr *attr;
	pcre *re;
	int state_id,
      edge_id,
      shift = 0,
      start,
      report,
      fan,
      **my_table,
      num_stes_since_last_start,current_state=0, start_node=0;
	const char *error;
	char str[1024];
	int erroffset=0,rc=0,ovector[OVECCOUNT];
		
	// drill down (up to one level) to get to the "automata-network" tag,
	// which is sometimes inside of an "anml" tag, but other times not (why?)
  	if (my_nfa->root->type==XML_ELEMENT_NODE && strcmp((const char *)my_nfa->root->name,"automata-network"))
	//	my_nfa->start_table[start_node++]=anode; 
		for (anode = my_nfa->root->children; anode; anode = anode->next)
			if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"automata-network")) break;
	
	// states are found among the children of the "automata" network tag
	for (anode = anode->children; anode; anode = anode->next) {
		if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"state-transition-element")) {
			my_nfa->node_table[current_state]=anode;
			
			// get state properties, namely the symbol set (through PCRE) and if it is a start state
			for (attr = anode->properties; attr; attr = attr->next) {
				if (!strcmp((const char *)attr->name,"symbol-set")) {
					re = pcre_compile((const char *)attr->children->content,0,&error,&erroffset,NULL);
					strcpy(my_nfa->symbol_table[current_state], (const char *)attr->children->content);
					str[1]=0;
					for (int i=0;i<256;i++) {
						str[0]=i;
						rc = pcre_exec(re,NULL,str,1,0,0,ovector,OVECCOUNT);
						if (rc == PCRE_ERROR_NOMATCH){
							my_nfa->next_state_table[current_state][i]= 0;
						} else {
							my_nfa->next_state_table[current_state][i]= 1;
						}
					}
					pcre_free(re);
				} else if (!strcmp((const char *)attr->name,"start")) {
					my_nfa->start_state[current_state]=start;
					my_nfa->start_table[current_state]=1;
				}
			}
			
			// get the state's predecessors and reports among the children of the state tage
			edge_num=0;
			for (bnode = anode->children; bnode; bnode = bnode->next) {
				if (bnode->type==XML_ELEMENT_NODE && !strcmp((const char *)bnode->name,"activate-on-match")) {
					for(attr = bnode->properties; attr; attr = attr->next) {
						if ( !strcmp((const char *)attr->name,"element")) {
							//	WARNING:  this line requires the state map to be working!
							my_nfa->edge_table[current_state][edge_num++]=my_nfa->state_map[(const char *)attr->children->content];
			
						}
					}
				} else if(bnode->type==XML_ELEMENT_NODE && !strncmp((const char *)bnode->name,"report-on-",10)) {
					my_nfa->num_reports++;
					my_nfa->report_table[current_state] = 1;
				}
			}
			
			if (edge_num < max_edges) my_nfa->edge_table[current_state][edge_num]=-1;
			current_state++;
		}
	}
}
