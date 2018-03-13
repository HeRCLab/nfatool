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
#include <vector>
#include <iterator>
#include <iostream>
#include <cassert>

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

#define MAX_COLORS	100000
#define MAX_PART_SIZE	1024

unsigned char **next_state_table;

unsigned char *report_table,
              *start_state,
              *symbol_set;

char *visited;

unsigned int *subnfa;

int **edge_table,
    **orig_edge_table,
    **reverse_table,
    **FanoutTable;

int *movement_map,
    *visitedTwo,
    *visitedColorTrav,
    *visitedcycle,
    *ExactOutEdge;

xmlNode **node_table;

xmlNode *rootGlobal, 
        *start_stes[STATEMAP_SIZE];

FILE *Destination;

xmlDoc *document;

std::map <std::string,int> state_map;

std::vector<int> *state_colors;

std::vector<std::vector<int> > Tree_Cyle;

std::vector<int> Path;

int num_states=0,
	  subnfa_size = 0,
    subnfa_total_size = 0,
	  duplicate_states = 0,
	  subnfa_num = 0,
    STEinFile = 0,
    file_state = 0,
    reach = 0,
    max_fanout=0,
    globalstart = 0,
    max_stes=0,
    subnfaTotal = 0,
    STEduplications = 0,
    NextStart = 0,
    pass = 0,
    num_start = 0,
    states = 0,
    starts = 0,
    max_edges = 0,
    edges = 0, 
    edge_num = 0,
    files_generated = 0,
    num_reports = 0, 
    Change_of_start = 0,
    max_fan_in = 0,
    state_map2[STATEMAP_SIZE],
    current_color = 0,
    cycle_count =0,
    start_color,
    color_count[MAX_COLORS];


int extract_number (const char *str) 
{
  int i,
      j, 
      start,
      end;

  char str2[1024];

  end = strlen(str);

  for (i=0;i<strlen(str);i++) if (isdigit(str[i])) {start=i;break;}

  if (i==strlen(str))
  {
    fprintf (stderr,"error: id extraction failed for \"%s\"\n",str);
    exit(0);
  }

  for (j=i+1;j<strlen(str);j++) if (!isdigit(str[j])) {end=j;break;}

  strncpy(str2,str+start,end-start);
  str2[end-start]=0;
  return atoi(str2);
}

int Hash(const char *key)
{
  int hash = 0;

  return extract_number(key);

  for(int i = 0 ; i < strlen(key); i++)
  {
    hash = hash + (key[i]<<(8*(i%3)));
  }
  return hash % (1<<24);
}

int extract_string (char *str) 
{
  int i = 0,
      j = 0,
      start = 0,
      end = 0;

  char str2[64];

  for (i=0;i<strlen(str);i++) 
  {
    if (str[i]>='!' && str[i]<='~') break;
  }

  start = i;

  for (int i = 0;i<strlen(str);i++)
  {
    if (str[i]<'!' || str[i]>'~') break;
  }

  end=i;

  for (i=start;i<end;i++) str2[j++] = str[i];
  str2[j]=0;

  return atoi(str2);
}

int count_states (xmlNode *anode) 
{
  int tmp = 0;

  xmlAttr *attr;

  for (anode = anode->children; anode; anode = anode->next) 
  {
    count_states(anode);
    if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"state-transition-element"))
    {
      if (edges > max_edges)
      {
        max_edges = edges;
      }

        edges = 0;

      for (attr = anode->properties; attr; attr = attr->next){
        if (!strcmp((const char *)attr->name,"id")) 
        {
          tmp = Hash((const char *)attr->children->content);
          state_map[(const char *)attr->children->content]=states;
          state_map2[states] = tmp;
        }
        if(!strcmp((const char *)attr->name,"start"))
        {
          num_start++;
        }
      }
    states++;
    }
    if (anode->type==XML_ELEMENT_NODE && !strcmp((const char *)anode->name,"activate-on-match"))
    {
      edges++;
    }
  }
  return states;
}

void get_props_state (xmlNode *Node,int *id,int *start) 
{
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

	for (attr = Node->properties; attr; attr = attr->next)
  	{
		if(!strcmp((const char *)attr->name,"id"))
    	{
			*id =state_map[(const char *)attr->children->content];

		}else if (!strcmp((const char *)attr->name,"symbol-set"))
    	{
			re = pcre_compile((const char *)attr->children->content,0,&error,&erroffset,NULL);

			str[1]=0;

			for (int i=0;i<256;i++)
      		{
				str[0]=i;
				rc = pcre_exec(re,NULL,str,1,0,0,ovector,OVECCOUNT);

				if (rc == PCRE_ERROR_NOMATCH)
        		{
					next_state_table[*id][i]= 0;
				}else
        		{
					next_state_table[*id][i]= 1;
				}
			}
			pcre_free(re);
		}else if (!strcmp((const char *)attr->name,"start") && !strcmp((const char *)attr->children->content,"all-input")) 
    	{
      		*start = 1;
    	}
	}
}

void get_props_edge (xmlNode *aNode,int *id,int *areport) 
{
	xmlAttr *attr;

	for(attr = aNode->properties; attr; attr = attr->next)
  	{
		if(!strcmp((const char *)attr->name,"element"))
    	{
			*id = state_map[(const char *)attr->children->content];
		}else if(!strcmp((const char *)aNode->name,"report-on-match"))
    	{
			*areport = state_map[(const char *)attr->children->content];
		}
	}
}

void duplications(int a)
{
  for (int i = a; i < num_states; i++)
  {
    STEduplications++;
  }
}

void traverse_graph(int i)
{
  if(visited[i] == 1)
  {
    return;
  }

  if (i > NextStart)
  {
    duplications(i);
  }

  subnfa_size++; 

  visited[i] = 1;
}

void cycle_detection(int ste)
{
  if (visited[ste] == 1)
  {
    Path.push_back(ste);
    visited[ste]++;
  }
  else if (visited[ste] == 2)
  {
    Tree_Cyle.push_back(Path);
    Path.clear();
    return;
  }
  else
    visited[ste]++;

    for (int i = 0; i < max_edges; i++)
    {
      cycle_detection(edge_table[ste][i]);
    }
}

int fill_in_table (xmlNode *aNode,int current_state)
{

	int state_id = 0,
      edge_id = 0,
      shift = 0,
      start = 0,
      report = 0,
      plus = 0,
      fan = 0,
      **my_table,
      num_stes_since_last_start;

  char str[1024];

	for (aNode = aNode; aNode; aNode = aNode->next)
  	{

		if(aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"state-transition-element"))
    	{

			 get_props_state(aNode,&state_id,&start);
			 start_state[current_state]=start;
			 node_table[current_state]=aNode;

       		if(start == 1)
       		{
        		subnfa[plus] = current_state;
        		start_stes[starts] = aNode;
        		Change_of_start = current_state;
          		plus++; 
          		starts++;
      		}

      		if(shift == max_stes-1)
      		{
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
		}
		else if(aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"activate-on-match"))
    	{
			 get_props_edge(aNode,&edge_id,&report);
			 edge_table[current_state-1][edge_num]=edge_id;
       		 edge_num++;
		  }
		else if(aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"report-on-match"))
    	{
			 get_props_edge(aNode, &edge_id, &report);
			 num_reports++;
			 report_table[current_state-1] = 1;

		  }
		if (aNode->children)
    	{
      		fill_in_table(aNode->children, current_state);
    	}
	}
}

void traverse_partition(int ste)
{

	if (visitedColorTrav[ste] == 1)
	{
		return; 
	}
  else if(ste == -1)
  {
    return;
  }
  else

    printf("The STE is: %d\n", ste);

	 visitedColorTrav[ste] = 1;

	 state_colors[ste].push_back(current_color);

	 color_count[current_color]++;

   for (int i = 0; i < num_states; i++)
   {
     for (int j = 0; j < max_edges; j++)
     {
       if (edge_table[i][j] == -1)
       {
         break;
       }
       else if ((edge_table[i][j] == ste) && (i == start_color))
       {
         current_color++;
         break;
       }
     }
   }

	 for (int i = 0; i < max_edges; i++)
	 {

		  traverse_partition(edge_table[ste][i]);

	 }
}

// void replace_color(int str,int color,int new_color) 
// {

//   for(int i = 0; i < max_edges; i++){

// 		if (i==color) 
//     {
//         state_colors[str].erase(color);

// 			  state_colors[str].push_back(new_color);

// 	      color_count[new_color]++;
// 		}
// 		replace_color(edge_table[str][i], color, new_color);
// 	}
// }

// void split_color(int ste, int color)
// {

//   original_num_colors = num_colors;

//   for (std::vector<int>::iterator i = state_colors[ste].begin();i!=state_colors[ste].end();i++)
//   {
//     if (i==color) 
//       {
//         state_colors[ste].erase(i);
//       }
//   } 

//   for (int i = 0; i < max_edges; i++) 
//   {
//     if(edge_table[ste][i] == -1)
//     {
//       break;
//     }
//     else
//       replace_color(edge_table[ste][i],color,num_colors+1);
//       num_colors++;
//   }

//   if (color1 != -1) 
//   {
//     state_colors[ste].push_back(color1);
//     color_count[color1]++;
//   }

//   for (int i = 0; i < max_edges;i++){
//     if (edge_table[ste][i] == -1)
//     {
//       break;
//     }
//     else if ((color1 != -1) && (color2 != -1)) 
//     {
//       if (edge_table[ste][i]==-1) 
//       {
//         if (i==0) 
//         {
//           split_color (edge_table[ste][i],color,color1,-1)
//         }
//         else 
//             split_color (edge_table[ste][i],color,-1,color2);
//       } 
//       else 
//         split_color (edge_table[ste][i],color,color1,color2);
//     } 
//   }
// }

void split_colorv2(int ste, int color)
{ 

  int k;

  /*
   * Erases all the colors within the tree before adding new colors 
   */

  for (std::vector<int>::iterator i = state_colors[ste].begin();i!=state_colors[ste].end();i++)
  {
    if (i==color) 
      {
        state_colors[ste].erase(i);
      }
  }

  /*
   * add colors to the highest priortize ste for splitting based on the amount of outgoing edges it may have 
   */

  for (int i = 1; i <= ExactOutEdge[ste]; i++)
  {
    state_colors[ste].push_back(color++);
  }

  /*
   * when a new brach is found from the start state the color will change among that branch
   */

  for (std::vector<>::iterator i = state_colors[globalstart].begin(); i != state_colors[globalstart].end(); i++)
  {
    k++;
      
    if (edge_table[ste][k] == -1)
    {
      break;
    }

    replace_colorv2(edge_table[ste][k],color,i);  
  }

}
void replace_colorv2(int str, int old_color, int new_color)
{
  /*
   * This is a function that is called recursively onto the branch that it lies on.
   * iterator is used to check the vector if it contains the color
   * 
   * My solution for cycle: create multi-dem vector that stores every cycle found. if one ste that 
   *                        is found within the cycle, a nested for loop will trigger making a similar 
   *                        for every ste within the cycle.
   */

  if (str == -1)
  {
    return;
  }

  for (int i = 0; i < max_edges; i++){
    for (std::vector<>::iterator j = state_colors[str].begin(); j != state_colors[str].end(); j++)
    {
      if (j == old_color)
      {
        if (visitedcycle[str] == 1)
        {
          for (int a = 0; a < Tree_Cyle.size(); a++)
          {
            for (int b = 0; b < Tree_Cyle[a].size(); b++)
            {
              if (Tree_Cyle[a][b] == str)
              {
                for (int c = 0; c < Tree_Cyle[a].size(); c++)
                {
                  state_colors[str].erase(j);
                  state_colors[str].push_back(new_color);
                  color_count[new_color]++;
                }
              }
            }
          }
        }
        else
          state_colors[str].erase(i);
          state_colors[str].push_back(new_color);
          color_count[new_color]++;
      }    
    }
    replace_colorv2(edge_table[str][i], old_color, new_color);
  }
}

void becchi_partition () 
{

	cycle_detection(0);

	int color_size_exceeded;

	for (int i = 0; i<num_states; i++) 
	{                                                            
		if (start_state[i] == 1) 
		{
      start_color = i;

			for (int j = i; i < num_states; j++)
			{
				traverse_partition(j);	
			}
		}
	}
	
	color_size_exceeded=0;

  /*
   *  The nested for loop below will set a flag for each ste that is found 
   *  within the vector that stores stes that are contain inside a cycle
   *
   */

  for (int a = 0; a < Tree_Cyle.size(); a++)
  {
    for (int b = 0; b < Tree_Cyle[a].size(); b++)
    {
      for (int c = 0; c < num_states; c++)
      {
        for (int d = 0; d < max_edges; d++)
        {
          if (edge_table[c][d] == Tree_Cyle[a][b])
          {
            visitedcycle[Tree_Cyle[a][b]] = 1;
          }
        }
      }
    }
  }
  
	for (int i=0; i<MAX_COLORS; i++) {
    if (color_count[i] > MAX_PART_SIZE) {
      color_size_exceeded = 1;
    }
  }

	while (color_size_exceeded) {	
		color_size_exceeded=0;
		for (int i=0;i<MAX_COLORS;i++) if (color_count[i]>MAX_PART_SIZE) {
			color_size_exceeded=1;
			for (int j = 0;j<num_states;j++) {
				if (start_state[j]) 
        {
          globalstart = j;
					split_colorv2(j,current_color);
				}
			}
			color_count[i]=0;
		}


	} while (color_size_exceeded);
}

int fill_in_sub(int firstState)
{

  for(int i = firstState; i < num_states; i++)
  {                                                            
    if(start_state[i] == 1)
    {
      STEduplications = 0;

      for(int proc = i+1; proc < num_states; proc++)
      {  
        if (start_state[proc] == 1)
        {
          NextStart = proc;
          break;
        }
      }

      subnfaTotal = subnfaTotal + subnfa_size;

      subnfa_size = 0;  

      for(int j = 0; j < num_states; j++)
      {  
        visited[j] = 0;
      }
    }

    for(int j  = 0; j < max_edges; j++)
    { 
      if (edge_table[i][j] == -1) 
      {
        break;
      }
      else
        traverse_graph(edge_table[i][j]);
    }
  }
}

void dump_dot_file (char *filename, xmlNode *aNode)
{
  FILE *myFile;

  char str[1024];

  sprintf(str,"%s.dot",filename);
  myFile=fopen(str,"w+");

  if (!myFile) 
  {
    perror(filename);
    exit(0);
  }

  fprintf (myFile,"digraph {\n\trankdir=LR;\n");

  int i =0;
  for(aNode = aNode; aNode; aNode = aNode->next)
  {
    if (aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"state-transition-element"))
    {
      fprintf (myFile,"\t%d [label=\"%s:%s\" peripherals=%d, shape=circle, style=filled, fillcolor=red];\n",i,aNode->properties->children->content,
      aNode->properties->next->children->content,
      start_state[i]+report_table[i]+1);
      i++;
    }
  }
  for (int i=0;i<num_states;i++) 
  {
    for (int j=0;(j<max_edges) && (edge_table[i][j]!=-1);j++)
    {
      fprintf (myFile,"%d -> %d;\n",i,edge_table[i][j]);
    }
  }
  fprintf(myFile,"}\n");
  fclose(myFile);
}

int main(int argc, char **argv)
{

  	char filename[1024],
       		*filename2,
        	c,
        	str[1024];

  	xmlNode *root;
  	xmlNode *root2;
  	xmlDoc *document2;
  	xmlDoc *Doc2;

  	int basket;
  	int file_spec=0;
  	int temp = 0;
  	int val = 0;
  	int fanin = 0;
  	int variable = 0;

  	srand(10202);

  	if (argc < 2)
  	{
      	fprintf(stderr, "Usage: %s filename.xml [options]\n", argv[0]);
      	return 1;
  	}

  	while ((c=getopt(argc,argv,"i:m:f:"))!=-1)
    	switch (c) 
    	{
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

  	if (!file_spec) 
  	{
    	fprintf(stderr, "ERROR:  -i parameter is required\n");
    	return 0;
  	}

  	document = xmlReadFile(filename, NULL, 0);

  	root = xmlDocGetRootElement(document);

  	rootGlobal = root;

  	num_states = count_states(root);

  	node_table = (xmlNode **)malloc(sizeof (xmlNode *)*num_states);

  	symbol_set = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);

  	next_state_table = (unsigned char **)malloc((sizeof(char*))*num_states);
  	for(int i= 0;i< num_states;i++) next_state_table[i] = (unsigned char *)malloc(sizeof (unsigned char)*256);

  	report_table = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);
  	for (int i=0;i<num_states;i++) report_table[i]=0;

  	edge_table = (int **)malloc((sizeof(int *))*num_states);
  	for (int i=0;i<num_states;i++) 
  	{
    	edge_table[i]=(int *)malloc(sizeof (int)*max_edges);
    	for (int j=0;j<max_edges;j++) edge_table[i][j]=-1;
  	}

  	orig_edge_table = (int **)malloc((sizeof(int*))*num_states);
  	for (int i=0;i<num_states;i++)
  	{
    	orig_edge_table[i]=(int *)malloc(sizeof (int)*max_edges);
    	for (int j=0;j<max_edges;j++) orig_edge_table[i][j]=-1;
  	}
  
  	visitedTwo = (int *)malloc((sizeof(int*))*num_states);
  	for(int i = 0; i < num_states; i++) visitedTwo[i] = 0;
  
  	subnfa = (unsigned int *)malloc((sizeof(unsigned int))*num_states);
  	for (int i=0;i<num_start;i++) subnfa[i]=-1;
  
  	movement_map = (int *)malloc(sizeof(int)*num_states);
  	for (int i=0;i<num_states;i++) movement_map[i]=i;

  	visited = (char *)malloc((sizeof(char))*num_states);
  	for (int i=0;i<num_states;i++) visited[i]=0;

    visitedcycle = (int *)malloc((sizeof(int))*num_states);
    for(int i = 0; i < num_states; i++) visitedcycle[i] = 0;

  	visitedColorTrav = (int *)malloc((sizeof(int))*num_states);
  	for (int i = 0; i < num_states; i++) visitedColorTrav[i] = 0;

    ExactOutEdge = (int *)malloc((sizeof(int))*num_states);
    for(int i = 0; i < num_states; i++) ExactOutEdge[i] = 0;

  	start_state = (unsigned char *)malloc((sizeof(unsigned char*))*num_states);
  	for(int i=0; i < num_states; i++) start_state[0] = 0;

  	fill_in_table(root->children, 0);


    /*
     *stores the exact number of out going edges to prepare for becchi_partition
     *
     */

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

  	//fill_in_sub(0);

  	becchi_partition();

    //dump_dot_file(filename, node_table[0]);

    // printf("Hello \n" );
    // fill_in_sub(0);

    return 0;
}
