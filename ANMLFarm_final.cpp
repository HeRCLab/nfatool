#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <pcre.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <map>
#include <iostream>
#include <cassert>
#include <vector>

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

using namespace std;

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

//int  state_map[1<<24];

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
    max_fan_in = 0;

xmlNode *rootGlobal, /*< root node of the .ANML file */
        *start_stes[STATEMAP_SIZE];

FILE *Destination;

xmlDoc *document; /*< .ANML document that will be used on the program */

int extract_number (const char *str) {
  int i,j,start,end;
  char str2[1024];

  end = strlen(str);

  for (i=0;i<strlen(str);i++) if (isdigit(str[i])) {start=i;break;}
  if (i==strlen(str)) {
    fprintf (stderr,"error: id extraction failed for \"%s\"\n",str);
    exit(0);
  }
  for (j=i+1;j<strlen(str);j++) if (!isdigit(str[j])) {end=j;break;}

  strncpy(str2,str+start,end-start);
  str2[end-start]=0;
  return atoi(str2);
}

int Hash(const char *key){
  int hash = 0;

  return extract_number(key);

  for(int i = 0 ; i < strlen(key); i++){
    hash = hash + (key[i]<<(8*(i%3)));
  }
  return hash % (1<<24);
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

int count_states (xmlNode *anode) {
  int tmp;
  xmlAttr *attr;

  for (anode = anode->children; anode; anode = anode->next) {
    count_states(anode);
    if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"state-transition-element")){
      if (edges > max_edges) {
        max_edges = edges;
        }

        edges = 0;

      for (attr = anode->properties; attr; attr = attr->next){
        if (!strcmp((const char *)attr->name,"id")) {
          tmp = Hash((const char *)attr->children->content);
          /*
          if(state_map[tmp] != -1){
            printf("collision");
          }
	  if (state_map[std::string(attr->children->content)] != -1) {
		fprintf(stderr,"hash collision detection, hash %d (id=\"%s\") is already occupied by entry %d\n",tmp,attr->children->content,state_map[tmp]);
		exit(1);
	  }*/
          state_map[(const char *)attr->children->content]=states;
          state_map2[states] = tmp;
        }
      }
    states++;
    }
    if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"activate-on-match")){
      edges++;
    }
  }
  return states;
}

void get_props_state (xmlNode *Node,int *id,int *start) {
	static char str[1024],
              *str2=str+2;
	pcre *re;

	const char *error;

	int erroroff = 0,
	    erroffset = 0,
      rc = 0,
      ovector[OVECCOUNT];

	xmlAttr *attr;

	*start = 0;

	for (attr = Node->properties; attr; attr = attr->next){
		if(!strcmp((const char *)attr->name,"id")){

			*id =state_map[(const char *)attr->children->content];

		}else if (!strcmp((const char *)attr->name,"symbol-set")){

			re = pcre_compile((const char *)attr->children->content,0,&error,&erroffset,NULL);
    //  printf("State: %d symbol-set Hash: %s \n",SS++, attr->children->content);

			str[1]=0;

			for (int i=0;i<256;i++){
				str[0]=i;
				rc = pcre_exec(re,NULL,str,1,0,0,ovector,OVECCOUNT);

				if (rc == PCRE_ERROR_NOMATCH){
					next_state_table[*id][i]= 0;
				}else{
					next_state_table[*id][i]= 1;
				}
			}
			pcre_free(re);
		}else if (!strcmp((const char *)attr->name,"start") && !strcmp((const char *)attr->children->content,"all-input")) {
      *start = 1;
    }
	}
}

void get_props_edge (xmlNode *aNode,int *id,int *areport) {
	xmlAttr *attr;

	for(attr = aNode->properties; attr; attr = attr->next){
		if(!strcmp((const char *)attr->name,"element")){
			*id = state_map[(const char *)attr->children->content];

		}else if(!strcmp((const char *)aNode->name,"report-on-match")){
			 *areport = state_map[(const char *)attr->children->content];
		}
	}
}

int fill_in_table (xmlNode *aNode,int current_state){

	int state_id,
      edge_id,
      shift = 0,
      start,
      report,
      fan,
      **my_table,
      num_stes_since_last_start;

  char str[1024];

	for (aNode = aNode; aNode; aNode = aNode->next){


		if(aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"state-transition-element")){

			get_props_state(aNode,&state_id,&start);
			start_state[current_state]=start;
			node_table[current_state]=aNode;

      if(start == 1){
        start_stes[starts++] = aNode;
        Change_of_start = current_state;
      }

      if(shift == max_stes-1){
        node_table[Change_of_start]->prev->next = NULL;
        sprintf(str, "subautomata%d.anml", files_generated++);
        Destination = fopen(str, "w+");
        xmlDocDump(Destination, document);
        fclose(Destination);
        rootGlobal->children = node_table[Change_of_start];
        shift = 0;
      }

		  edge_num = 0;
		  current_state++;
      file_state++;
      shift++;

		}else if(aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"activate-on-match")){
			get_props_edge(aNode,&edge_id,&report);
			edge_table[current_state-1][edge_num]=edge_id;
			//test_edge[current_state-1] = edge_num;
      //fan = edge_id - current_state;
      //FanoutTable[current_state-1][edge_num] = fan;

      //fprintf(fanFile, "the fanout is equal to: %d\n", FanoutTable[current_state-1][edge_num]);


      edge_num++;
		}else if(aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"report-on-match")){
			get_props_edge(aNode, &edge_id, &report);
			num_reports++;
			report_table[current_state-1] = 1;

		}if (aNode->children){
      fill_in_table(aNode->children, current_state);
    }
	}
}

void dump_dot_file (char *filename, xmlNode *aNode) {
  FILE *myFile;

  char str[1024];

  sprintf(str,"%s.dot",filename);
  myFile=fopen(str,"w+");

  if (!myFile) {
    perror(filename);
    exit(0);
  }

  fprintf (myFile,"digraph {\n\trankdir=LR;\n");

  int i =0;
  for(aNode = aNode; aNode; aNode = aNode->next){
    if (aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"state-transition-element")){
        fprintf (myFile,"\t%d [label=\"%s:%s\" peripherals=%d];\n",i,aNode->properties->children->content,
        aNode->properties->next->children->content,
        start_state[i]+report_table[i]+1);
        i++;
      }
  }
  for (int i=0;i<num_states;i++) {
    for (int j=0;(j<max_edges) && (edge_table[i][j]!=-1);j++) {
        fprintf (myFile,"%d -> %d;\n",i,edge_table[i][j]);
    }
  }
  fprintf(myFile,"}\n");
  fclose(myFile);
}

void move_ste (int from, int to) {
  int temp[max_edges],temp2[max_fan_in],i,j,k,rev,forward,temp3;
  int rev_edge_count[num_states];

  if (from<to) {
  
	  // scan the whole array!!!!!
//	  #pragma omp parallel for
	  for (i=0;i<num_states;i++) {
			for (j=0;j<max_edges;j++) {
				if (edge_table[i][j]==-1) break;
				if (edge_table[i][j]==from) edge_table[i][j]=to; else
				if ((edge_table[i][j]>from) && (edge_table[i][j]<=to)) edge_table[i][j]--;
			}
	  }
 
  }
  
  
  // STEP 2b:  update edge references INTO (FROM,TO]
  else {
  
	  // scan the whole array!!!!!
//	  #pragma omp parallel for
	  for (i=0;i<num_states;i++) {
			for (j=0;j<max_edges;j++) {
				if (edge_table[i][j]==-1) break;
				if (edge_table[i][j]==from) edge_table[i][j]=to; else
				if ((edge_table[i][j]>=to) && (edge_table[i][j]<from)) edge_table[i][j]++;
			}
	  }
  
  }
  
  for (i=0;i<max_edges;i++) temp[i]=edge_table[from][i];
  
  // STEP 4a:  shift contents
  if (from<to) {
	temp3=movement_map[from];
	
	for (i=from;i<to;i++) {
/* 		for (j=0;j<max_fan_in;j++) {
			reverse_table[i][j]=reverse_table[i+1][j];
		} */
		for (j=0;j<max_edges;j++) {
			edge_table[i][j]=edge_table[i+1][j];
		}
		movement_map[i]=movement_map[i+1];
	}
	
	movement_map[to]=temp3;
  }
  
  // STEP 4b:  shift contents
  if (to<from) {
	temp3=movement_map[from];
	
	for (i=from;i>to;i--) {
/* 		for (j=0;j<max_fan_in;j++) {
			reverse_table[i][j]=reverse_table[i-1][j];
		} */
		for (j=0;j<max_edges;j++) {
			edge_table[i][j]=edge_table[i-1][j];
		}
		movement_map[i]=movement_map[i-1];
	}
	movement_map[to]=temp3;
  }
  
   // STEP 3:  in reverse table, copy incoming edges into node from to node to
  //for (i=0;i<max_fan_in;i++) reverse_table[to][i] = temp2[i];
  for (i=0;i<max_edges;i++) edge_table[to][i] = temp[i];
  
  // rebuild reverse tabls
  //#pragma omp parallel for
  for (int i=0;i<num_states;i++) for (int j=0;j<max_fan_in;j++) reverse_table[i][j]=-1;
  //#pragma omp parallel for
  for (i=0;i<num_states;i++) rev_edge_count[i]=0;
  //#pragma omp parallel for
  for(int i = 0; i < num_states; i++) for(int j = 0; j < max_edges; j++) if(edge_table[i][j] == -1) break;
  else {
  //#pragma omp atomic
  rev_edge_count[edge_table[i][j]]++;
	reverse_table[edge_table[i][j]][rev_edge_count[edge_table[i][j]]] = i;
	if (rev_edge_count[edge_table[i][j]] > max_fan_in) {
		printf("exceeded max fan_in!");
		assert(0);
	}
  }
}

int Score(int a, int b){

  int temp,sum = 0, diff;

  if(a > b) {
	temp=a;
	a=b;
	b=temp;
  }

  //#pragma omp parallel for reduction(+,sum)
    for(int i = a; i <= b; i++) {

      for(int j = 0; j < max_edges; j++){

        if(edge_table[i][j] == -1){
          break;
        }
        else
		  diff = edge_table[i][j] - i;
		  //if ((diff < (-(max_fanout-1)/2)) || (diff > (max_fanout/2))) diff = diff*3;
          sum += abs(diff);
      }
      for (int j = 0; j < max_fan_in; j++){
        if(reverse_table[i][j] == -1){
          break;
        }
        else
		  diff = reverse_table[i][j] - i;
		  //if ((diff < (-(max_fanout-1)/2)) || (diff > (max_fanout/2))) diff = diff*3;
          sum += abs(diff);
       } 
    } 

  return sum;
}

// new code
int reverse_movement_map (int n) {
	int i;
	
	for (i=0;i<num_states;i++) if (movement_map[i]==n) return i;
}

void check_graphs () {
	int i,j,k;
	
	//#pragma omp parallel for
	for (i=0;i<num_states;i++) {
		for (j=0;j<max_edges;j++) {
			if (edge_table[i][j]==-1) break;
			if (orig_edge_table[movement_map[i]][j] != movement_map[edge_table[i][j]]) {
				fprintf (stderr,"error: new edge %d->%d should map to original edge %d->%d\n",i,edge_table[i][j],movement_map[i],orig_edge_table[movement_map[i]][j]);
				assert(0);
			}
		}
	}
	
 	//#pragma omp parallel for
	for (i=0;i<num_states;i++) {
		for (j=0;j<max_edges;j++) {
			if (orig_edge_table[i][j]==-1) break;
			if (reverse_movement_map(orig_edge_table[i][j]) != edge_table[reverse_movement_map(i)][j]) {
				fprintf (stderr,"error: original edge %d->%d should map to new edge %d->%d\n",i,orig_edge_table[i][j],reverse_movement_map(i),edge_table[reverse_movement_map(i)][j]);
				assert(0);
			}
		}
	}
}

int dump_edges () {
	int k,l;
/*   for (int i=0;i<6;i++) {
    printf ("[%d]",i);
    for (int j=0;j<max_edges;j++) if (edge_table[i][j]!=-1) printf (" ->%d",edge_table[i][j]);
    printf ("\n");
  } */
  
   		// debug
		for (k=0;k<20;k++) {
			printf ("%d -> ",k);
			for (l=0;l<max_edges;l++) printf ("%d ",edge_table[k][l]);
			printf ("\n");
			fflush(stdout);
		}

}

int validate_interconnection() {
  int violations = 0,
    from = 0,
    to = 0,
    max_differential_score = 0,
    best_from,best_to = 0,
    differential_score = 0,i,j,k;

  for(i = 0; i < num_states; i++) {
  
  //if (i%100==0) printf ("%0.2f%% scan complete, v=%d\n",(float)i/(float)num_states*100.0f,violations);
  
    for(j = 0; j < max_edges; j++) {
	
		if (edge_table[i][j] == -1) break;
       // check for interconnect violations and fix when necessary

        if(((edge_table[i][j] - i) < (-(max_fanout-1)/2)) || ((edge_table[i][j] - i) > (max_fanout/2))) { 
		
        max_differential_score = -INT_MAX;
      
        // proposal 1:  move source
		from = i;
		assert(from >= 0);
     
        for (k=0;k<max_fanout-1;k++) {
          to = edge_table[i][j] - max_fanout/2 + k;
          if ((to >= 0) && (to < num_states -1)){
	  
            differential_score = Score(from,to);
            // make the move
            move_ste(from,to);
			//check_graphs();
            differential_score -= Score(from,to);
            // undo the move
            move_ste(to,from);
			//check_graphs();
            if (differential_score > max_differential_score) {
              max_differential_score=differential_score;
              best_from = from;
              best_to=to ;
            }
          }
        }
      
        // proposal 2:  move destination
        from = edge_table[i][j];
        assert(from >= 0);

        for (k=0;k<max_fanout-1;k++) {
          to = i - (max_fanout-1)/2 + k;
          if ((to >= 0) && (to < num_states -1)) {

            differential_score = Score(from,to);
            // make the move
            move_ste(from,to);
			//check_graphs();
            differential_score -= Score(from,to);
            // undo the move
            move_ste(to,from);
			//check_graphs();
            if (differential_score > max_differential_score) {
              max_differential_score=differential_score;
              best_from=from;
              best_to=to;
            }
          }
        }
      
	    //if (max_differential_score>0) {
			//printf ("moved STE in position %d to position %d due to bad edge %d->%d, best score = %d\n",best_from,best_to,i,edge_table[i][j],min_differential_score);
			move_ste(best_from,best_to);
			printf("----------moving %d to %d\n",best_from,best_to);
		//}
        violations++;
		
		
		

		
		
		}
	}
  }
  
  check_graphs();
  return violations;
}



int reverse_edge_table () {
  int change,i,j,k,
    fanin = 0;

  int rev_edge_count[num_states];
  for (i=0;i<num_states;i++) rev_edge_count[i]=0;

  for (int i = 0; i < num_states; i++) {
    fanin=0;
      for(int j = 0; j < max_edges; j++){
        if(edge_table[i][j] == -1){
          break;
        }else {
          rev_edge_count[edge_table[i][j]]++;
          if (rev_edge_count[edge_table[i][j]] > max_fan_in) max_fan_in = rev_edge_count[edge_table[i][j]];
        }
    }
  }

  reverse_table = (int **)malloc((sizeof(int*))*num_states);
    for (int i=0;i<num_states;i++){
      reverse_table[i]=(int *)malloc(sizeof (int)*max_fan_in);
      for (int j=0;j<max_fan_in;j++) reverse_table[i][j]=-1;
    }

  for (i=0;i<num_states;i++) rev_edge_count[i]=0;
  
    for(int i = 0; i < num_states; i++){
      for(int j = 0; j < max_edges; j++){
        if(edge_table[i][j] == -1){
          break;
        }else {
          reverse_table[edge_table[i][j]][rev_edge_count[edge_table[i][j]]++] = i;
        }
      }
    }
}

void dfs(int current_node) {
    int flag,visited_node,in_loop=0;

    for (int i=0;i<visited2.size();i++) if (current_node == visited2[i]) return;
    visited2.insert(visited2.begin(),current_node);

    


     // ------------------------------------------------------------------      
     // Start going through all the edges of the current_node
        for(int j=0; j<max_edges; j++){
                //std::cout << "edge_table of current node" << current_node << " " << edge_table[current_node][j] << endl;
    
        if (edge_table[current_node][j]!=-1) {
            // Test if the edge in the final set, means no cycle found  
        // test the looop ??? ?
          visited_node=0;
                for(int k=0; k<visited2.size(); k++)
                    if(edge_table[current_node][j] == visited2[k]) {
                        visited_node=1;
                        break;
            }
                    
          if (!visited_node) dfs(edge_table[current_node][j]);
        }
  }
  

  return;
}

void assign(int u,int root,int *components) {
  int i;
  if (components[u]==-1) {
    components[u] = root;

    for (i=0;i<max_fan_in;i++) {
      if (reverse_table[u][i]==-1) break;

      assign(reverse_table[u][i],root,components);
    }
  }
}

//---------------------------------------------------------------------
int main(void) {
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

void traverse_graph (int i) {
	int j=0;
	
	if (visited[i]) return;
	
	subnfa_size++;
	
	visited[i]=1;
	
	while (edge_table[i][j]!=-1) {
		
		if (subnfa[i]!=-1)
			duplicate_states++;
		else
			subnfa[i]=subnfa_num;
		
		traverse_graph(edge_table[i][j]);
		
		j++;
	}
}

int main2(int argc, char **argv){
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

  for (int i=0;i<num_states;i++) visited[i]=0;

  fill_in_table(root->children, 0);
  
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
