 #include "nfatool.h"

int perform_graph_analysis (xmlNode *root) {
	int i,k;
	
	// initialize visited array
	for (int i = 0; i < num_states; i++) visited[i] = 0;
	
	/*
	 * find critical path
	 */
	critical_path(0);
    printf("critical path is: %d\n", path_compare);
	
	/* 
	 * Find the strongly connected components and optionally dump to file
	 */ 
	find_sccs();
  
    printf ("largest component is %d (size=%d)\n",largest_component,largest_component_size);
	for (k=0;k<largest_component_size;k++) printf("%d (%s) ",component_list[largest_component][k],
	node_table[component_list[largest_component][k]]->properties->children->content);
	printf ("\n");
	dump_dot_file((char *)"largest_component",rootGlobal,component_list[largest_component],0);
  
	/*
	 * validate largest SCC
	 */
	vector<int> path;
	int component_ok = 1;

	for (int i=0;i<component_list[largest_component].size();i++) {
		for (int j=i;j<component_list[largest_component].size();j++) {
			if (!find_loop_path(i,j,path,0)) {
				component_ok=0;
				fprintf(stderr,"error:  component %d has unreachable path %d (\"%s\") -> %d (\"%s\")\n",
																largest_component,i,ANML_NAME(i),j,ANML_NAME(j));
			}
		}
	}

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
	 * Find the critical path (again???), longest path in the tree 
	 */ 
	find_critical_path(); 
  
	printf ("deepest path = %d\n",deepest);
  
	for (int i=0;i<deepest_path.size();i++) printf ("%d (%s)->",
                          deepest_path[i],
                          node_table[deepest_path[i]]->properties->children->content);
	printf ("\n");
  
	/*
	 * Partition the graph
	 */
	if (max_stes) partition(max_stes);
  
	return 1;
}
 
void dump_dot_file (char *filename, xmlNode *aNode, vector<int> subset, int colors) {
  FILE *myFile;
  int found,found2;
  char str[1024];
  
  const char *error;

  int *id; 
  xmlAttr *attr; 

  pcre *re;
  int rc=0; 

  int erroroff = 0,
       erroffset = 0;

  int ovector[OVECCOUNT];

  sprintf(str,"%s.dot",filename);
  myFile=fopen(str,"w+");

  if (!myFile) {
    perror(filename);
    exit(0);
  }

  fprintf (myFile,"digraph {\n\trankdir=LR;\n");

  int i =0;

  // need to find the first occurance of the "state-transition-element" node
  deque<xmlNode *> myqueue;
  myqueue.push_back(aNode);

  xmlNode *tmpNode;
  while (myqueue.size() > 0) {
    
     tmpNode = myqueue.front();
     myqueue.pop_front(); 

    if (tmpNode->type==XML_ELEMENT_NODE && !strcmp((const char *)tmpNode->name,"state-transition-element")) break;
    for (tmpNode=tmpNode->children;tmpNode;tmpNode=tmpNode->next) myqueue.push_back(tmpNode);
  }

	  for(aNode=tmpNode;aNode; aNode = aNode->next){
    		if (aNode->type==XML_ELEMENT_NODE && !strcmp((const char *)aNode->name,"state-transition-element")){
        		found=0;
        		for (int j=0;j<subset.size();j++) if (subset[j]==i) found=1; // check to see if the STE is part of the subset
				if (subset.size()==0) found=1; // HACK: if subset is empty, assume we want to print all STEs
        		if (found) {
					if (colors) { // only print colors if specified
						strcpy(str,":(");
						for (int k=0; k<state_colors[i].size(); k++) {
							sprintf (str,"%s,%d",str,state_colors[i][k]);
						}
						sprintf (str,"%s)",str);
					} else 
						strcpy(str,"");
						
					fprintf (myFile,"\t%d [label=\"%d:%s:%s%s\" peripherals=%d];\n",i,i,aNode->properties->children->content,
													    aNode->properties->next->children->content,
													    str,
													    start_state[i]+report_table[i]+1);
						
				}
        		i++;
      		}
  	}	

/*  for(aNode = aNode->children; aNode; aNode = aNode->next)
  	for (attr = aNode->properties; attr; attr = attr->next){

		if (!strcmp((const char *)attr->name,"symbol-set")){
                        re = pcre_compile((const char *)attr->children->content,0,&error,&erroffset,NULL);
                        str[1]=0;

                        for (int i=0;i<256;i++){
                                str[0]=i;
                                rc = pcre_exec(re,NULL,str,1,0,0,ovector,OVECCOUNT);
			}
		
			fprintf (myFile, "%d\n", rc);
		}
	}



   for(aNode = aNode->children; aNode; aNode = aNode->next) {
        for (attr = aNode->properties; attr; attr = attr->next){
                if(!strcmp((const char *)attr->name,"id")) { 
			*id =state_map[(const char *)attr->children->content];
			fprintf (myFile, "%d\n", *id); 
		}
	}
   }

*/
  for (int i=0;i<num_states;i++) {
    for (int j=0;(j<max_edges) && (edge_table[i][j]!=-1);j++) {
        found=0;
        found2=0;
        for (int k=0;k<subset.size();k++) {
          if (subset[k]==i) found=1;
          if (subset[k]==edge_table[i][j]) found2=1;
        }
        if ((found && found2)||(subset.size()==0)) fprintf (myFile,"%d -> %d;\n",i,edge_table[i][j]);
    }
  }



 

  fprintf(myFile,"}\n");
  fclose(myFile);
}
