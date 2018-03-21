#include "nfatool.h"

void dump_dot_file (char *filename, xmlNode *aNode, vector<int> subset) {
  FILE *myFile;
  int found,found2;

  char str[1024];

  sprintf(str,"%s.dot",filename);
  myFile=fopen(str,"w+");

  if (!myFile) {
    perror(filename);
    exit(0);
  }

  fprintf (myFile,"digraph {\n\trankdir=LR;\n");

  int i =0;
  for(aNode = aNode->children; aNode; aNode = aNode->next){
    if (aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"state-transition-element")){
        found = 0;
        for (int j=0;j<subset.size();j++) if (subset[j]==i) found=1;
        if (found) fprintf (myFile,"\t%d [label=\"%s:%s\" peripherals=%d];\n",i,aNode->properties->children->content,
        aNode->properties->next->children->content,
        start_state[i]+report_table[i]+1);
        i++;
      }
  }


int *id; 
	
//static char str[1024], *str2=str+2; 
xmlAttr *attr; 
int k=0; 
   for(aNode = aNode->children; aNode; aNode = aNode->next) {
		printf("k=%d\n", k++) ; 
         for (attr = aNode->properties; attr; attr = attr->next){
                if(!strcmp((const char *)attr->name,"id")) { 
			*id =state_map[(const char *)attr->children->content];
			fprintf (myFile, "%d\n", *id); 
		}
	}
   }


  for (int i=0;i<num_states;i++) {
    for (int j=0;(j<max_edges) && (edge_table[i][j]!=-1);j++) {
        found=0;
        found2=0;
        for (int k=0;k<subset.size();k++) {
          if (subset[k]==i) found=1;
          if (subset[k]==edge_table[i][j]) found2=1;
        }
        if (found && found2) fprintf (myFile,"%d -> %d;\n",i,edge_table[i][j]);
    }
  }

 

  fprintf(myFile,"}\n");
  fclose(myFile);
}
