#include "nfatool.h"

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

int fill_in_table (xmlNode *aNode,int current_state) {

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
