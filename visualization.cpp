#include "nfatool.h"

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
