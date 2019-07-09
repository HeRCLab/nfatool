#include "nfatool.h"

/**
 * \brief	Given an SE ID, return SE name as defined by the ANML file
 * \param[in]	my_nfa	NFA data structure
 * \param[in]	se		SE number (pre-mapping)
 * \return				pointer to the string containing the ANML name
 *
 * This function returns the string version of the name of the SE from the order of its appearance starting at 0.
 */
char *anml_name (nfa *my_nfa,int se) {
	return (char *)my_nfa->node_table[se]->properties->children->content;
}

/**
 * \brief	Re-map SEs to satisfy placement contraints using a SAT solver
 * \param[in]	num_states		number of SEs
 * \param[in]	max_edges		maximum number of edges stored in the sparse graph representation
 * \param[in]	edge_table		sparse graph representation
 * \param[in]	reverse_table	reverse edge table (incoming edges instead of outgoing edges
 * \param[out]	orig_edge_table	a backup copy of the original edge table
 * \param[in]	max_fanout		the placement constraint
 * \param[out]	movement_map	a map that relates an physcial SE to a logical one
 * \param[in]	filename		a string containing the ANML filename
 * \param[in]	subgraph_num	which subgraph to map, -1 for original, raw graph as contained in the ANML file
 * \param[in]	timeout			SAT solver timeout, in seconds
 * \returns						1 on success, 0 on failure
 **/
int map_states_with_sat_solver_core(int num_states,
									int max_edges,
									int max_fan_in,
									int **edge_table,
									int **reverse_table,
									int **orig_edge_table,
									int max_fanout,
									int *movement_map,
									char *filename,
									int subgraph_num,
									int timeout) {
										
	char cnf_filename[1024],str[2048];
	int i,violations,ret;
	pid_t pid;
	char *line,*tok;
	int nchars,literal,state,se;
	vector<int> sat_solution;
	char cmd[2048];
	char sat_output_filename[1024];
	FILE *myFile;
	struct timeval t1,t2;
	
	// auto-generate CNF filename
	if (subgraph_num==-1)
		sprintf(cnf_filename,"%s.cnf",filename);
	else
		sprintf(cnf_filename,"%s_%d.cnf",filename,subgraph_num);
	
	// convert the graph and placement constraints into SAT
	perform_cnf_translation (num_states,max_edges,edge_table,max_fanout,cnf_filename);
	
	// issue command to initiate SAT solver
	sprintf(sat_output_filename,"%s.out",cnf_filename);
	sprintf(cmd,"-c \"cat %s | %s > %s\"",cnf_filename,SAT_SOLVER_COMMAND,sat_output_filename);
	/*
	ret=system(cmd);
	if (ret==-1) {
		fprintf (stderr,"ERROR:  could not invoke the SAT solver using command \"%s\"\n",SAT_SOLVER_COMMAND);
		return 0;
	}
	*/
	// spawn SAT solver
	/*
	sprintf(cmd,"-c \"cat %s | %s > %s\"",cnf_filename,SAT_SOLVER_COMMAND,sat_output_filename);
	pid=fork();
	if (!pid) {
		ret=execl("/bin/bash","bash",cmd,(char *)NULL);
		if (ret==-1) {
			perror("Could not spawn SAT solver");
			exit(0);
		}
	}
	*/
	
	pid=fork();
	
	// CHILD BEGIN
	if (!pid) {
		struct timeval t1,t2;
		
		gettimeofday(&t1,0);
		
		FILE *my_cmd = popen("/bin/bash","w");
		if (!my_cmd) {
			perror("ERROR: could not open shell");
			exit(0);
		}
		fprintf(my_cmd,"cat %s | %s > %s",cnf_filename,SAT_SOLVER_COMMAND,sat_output_filename);
		pclose(my_cmd);
		
		gettimeofday(&t2,0);
		
		printf ("INFO: SAT solver time elapsed = %d s (%d states)\n",
					(int)((long)(long)t2.tv_sec -
					      (long)(long)t1.tv_sec),
						  num_states);
		
		exit(2);
	}
	// CHILD END
	
	// wait for it with timeout
	int secs=0,status;
	do {
		ret=waitpid(pid,&status,WNOHANG);
		if (ret == -1) {
			perror("ERROR: waitpid() returned error");
			exit(0);
		} else if (ret != 0) {
			if (WIFEXITED(status)) break;
		}
		
		// otherwise, sleep for one second
		sleep(1);
		secs++;
	} while (secs < timeout);
	
	// check final status	
	if ((ret==0) || !WIFEXITED(status)) {
		if (subgraph_num==-1)
			fprintf(stderr,"ERROR: SAT solver timeout\n");
		else
			fprintf(stderr,"ERROR: SAT solver timeout for subgraph %d\n",subgraph_num);
		sprintf(cmd,"killall %s",SAT_SOLVER_COMMAND);
		ret=system(cmd);
		if (ret==-1) fprintf (stderr,"ERROR: system() failed\n");
		return 0;
	}
	
	// open the output file of the SAT solver
	myFile=fopen(sat_output_filename,"r+");
	if (!myFile) {
		sprintf (str,"ERROR: Cannot open SAT solver output file \"%s\" for reading",sat_output_filename);
		perror(str);
		return 0;
	}

	// read the results
	line=0;
	nchars=0;
	while (getline(&line,(size_t *)&nchars,myFile) > 3) {
		if (line[0]=='s' && strncmp(line+2,"SATISFIABLE",11)) {
			fprintf (stderr,"ERROR: SAT solver failed to find valid mapping.\n");
			return 0;
		}
		
		if (line[0]=='v') {
			tok=strtok(line+2," ");
			while (tok) {
				literal=atoi(tok);
				if (literal>0) sat_solution.push_back(literal);
				tok=strtok(0," ");
			}
		}
		free(line);
		line=0;
		nchars=0;
	}
	free(line);
	fclose(myFile);
	
	if (subgraph_num==-1)
		printf ("INFO: Mapping solution found with SAT solver.\n");
	else
		printf ("INFO: Mapping solution for subgraph %d found with SAT solver.\n",subgraph_num);
	
	for (i=0;i<sat_solution.size();i++) {
		literal = sat_solution[i];
		literal_to_mapping(literal,&state,&se,num_states);
		movement_map[se]=state;
	}
			
	// apply the new mapping to the NFA
	apply_movement_map (edge_table,
						orig_edge_table,
						num_states,
						max_edges,
						movement_map);
	
	// validate that the new mapping is consistant with the original NFA graph
	check_graphs (num_states,max_edges,edge_table,movement_map,orig_edge_table,0);
	
	// validate that no fanout violations exist
	violations=validate_interconnection(edge_table,reverse_table,num_states,max_edges,max_fan_in,movement_map,orig_edge_table,max_fanout);
								 
	if (violations) {
		fprintf(stderr,"ERROR:  SAT solver produced mapping solutions with fanout violations!\n");
		return 0;
	}
	
	// delete the old intermediate files
	ret = unlink(cnf_filename);
	if (ret) {
		sprintf(str,"ERROR: cannot delete CNF file \"%s\"",cnf_filename);
		perror(str);
		exit(0);
	}
	ret = unlink(sat_output_filename);
	if (ret) {
		sprintf(str,"ERROR: cannot delete SAT solver output file \"%s\"",sat_output_filename);
		perror(str);
		exit(0);
	}
	
	return 1;
}

/**
 * \brief	Wrapper for SAT solver mapper
 * \param[in]	filename	ANML filename
 * \param[in]	my_nfa		NFA data structure
 * \param[in]	subgraph	flag that specifies if the users wishes to map the file ANML file or the individual distinct subgraphs
 * \param[in]	timeout		timeout for SAT solver solutions
 * \returns		1 on success, 0 on failure
 */
int map_states_with_sat_solver (char *filename,
								nfa *my_nfa,
								int subgraph,
								int timeout) {
	int i,ret;
	
	if (!subgraph) {
		return map_states_with_sat_solver_core(my_nfa->num_states,
											   my_nfa->max_edges,
											   my_nfa->max_fan_in,
											   my_nfa->edge_table,
											   my_nfa->reverse_table,
											   my_nfa->orig_edge_table,
											   my_nfa->max_fanout,
											   my_nfa->movement_map,
											   filename,
											   -1,
											   timeout);
	} else {		
		for (i=0;i<my_nfa->distinct_subgraphs;i++) {
			ret=map_states_with_sat_solver_core(my_nfa->subgraph_size[i],
												my_nfa->max_edges,
												my_nfa->max_fan_in,
												my_nfa->edge_tables[i],
											    my_nfa->reverse_tables[i],
												my_nfa->orig_edge_tables[i],
												my_nfa->max_fanout,
												my_nfa->movement_maps[i],
												filename,
												i,
												timeout);
			if (ret==0) return 0;
		}
	}
	
}

void apply_movement_map (int **edge_table,
						 int **orig_edge_table,
						 int num_states,
						 int max_edges,
						 int *movement_map) {				 
	int i,j;
	int new_state;
	
	// copy the current edge_table to original_edge_table
	for (i=0;i<num_states;i++) {
		for (j=0;j<max_edges;j++) {
			orig_edge_table[i][j]=edge_table[i][j];
		}
	}
	
	// re-write edge_table according to movement_map
	// movement_map[i] will hold the original state now in SE i
	for (i=0;i<num_states;i++) {
		new_state = reverse_movement_map(num_states,movement_map,i);
		if (new_state==-1) {
			fprintf(stderr,"ERROR: reverse_movement_map() returned error code\n");
			exit(0);
		}
		for (j=0;j<max_edges;j++) {
			if (orig_edge_table[i][j]!=-1)
				edge_table[new_state][j]=reverse_movement_map(num_states,movement_map,orig_edge_table[i][j]);
			else
				edge_table[new_state][j]=-1;
		}
	}
			
}

int state_to_se_literal (int state,int se,int num_states) {
	return state * num_states + se + 1;
}

void literal_to_mapping (int literal,int *state, int *se,int num_states) {
	literal--;
	*state = literal/num_states;
	*se = literal % num_states;
}

void print_mapping (nfa *my_nfa) {
	int i;
	
	printf ("%10s%10s\n","SE","state");
	for (i=0;i<my_nfa->num_states;i++) {
		printf ("%10d%10d\n",i,my_nfa->movement_map[i]);
	}
	
	printf ("\n%10s%10s\n","state","SE");
	for (i=0;i<my_nfa->num_states;i++) {
		printf ("%10d%10d\n",i,reverse_movement_map(my_nfa->num_states,my_nfa->movement_map,i));
	}
}

int perform_cnf_translation (int num_states,int max_edges,int **edge_table,int max_fanout,char *filename) {
	int i,j,k,l,n=0,m;
	int literal,literal2;
	FILE *mycnf;
	char str[1024];
		
	// NOTE:  this function generates 2*num_states^2 + num_states CNF clauses
	// maximum clause width = num_states
	
	if (filename) {
		mycnf = fopen(filename,"w+");
		if (!mycnf) {
			sprintf(str,"ERROR:  cannot open CNF file for writing (\"%s\")",filename);
			perror(str);
			return 0;
		}
		fprintf(mycnf, "%s","c RUN: %solver --onlyindep --indep 1 --presimp 1 --varelim 1 --verb=0 %s | %OutputCheck %s\n");
	}
	
	// fan-out constraint:  at most 'num_states^2' clauses
	for (i=0;i<num_states;i++) { // for each predecessor state
		if (edge_table[i][0]!=-1) { // don't bother with states that don't have outgoing edges
			for (j=0;j<num_states;j++) { // for each possible se placement (assume number of SEs == number of states)
				
				
				for (k=0;k<max_edges;k++) { // for each successor
				
					if (edge_table[i][k]==-1) break;
					
					// don't worry about self-loops
					if (edge_table[i][k]!=i) {
						literal=-state_to_se_literal(i,j,num_states);
						if (filename) fprintf(mycnf,"%d ",literal);
						//clauses[n][0]=literal;
						for (l=-(max_fanout-1)/2;l<=max_fanout/2;l++) { // for each possible placement of the successor
							if ((l!=0) && (j+l)>=0 && (j+l)<num_states) { // disregard self-loop and avoid breach of array boundaries
								literal=state_to_se_literal(edge_table[i][k],j+l,num_states);
								//clauses[n][l+(max_fanout-1)/2+1]=literal;
								if (filename) fprintf(mycnf,"%d ",literal);
							}
						}
						if (filename) fprintf(mycnf,"0\n");
						n++;
					}
				}
			}
		}
	}
	
	
	// force a mapping of every state:  'num_states^2' clauses
	for (i=0;i<num_states;i++) {
		for (j=0;j<num_states;j++) {
			literal=state_to_se_literal(i,j,num_states);
			//clauses[n++][j]=literal;
			if (filename) fprintf(mycnf,"%d ",literal);
		}
		if (filename) fprintf(mycnf,"0\n");
	}
	
	/*
 	// force a mapping of every se:  'num_states^2' clauses
	for (i=0;i<num_states;i++) {
		for (j=0;j<num_states;j++) {
			literal=state_to_se_literal(j,i,num_states);
			clauses[n++][j]=literal;
			if (filename) fprintf(mycnf,"%d ",literal);
		}
		if (filename) fprintf(mycnf,"0\n");
	}
	*/
	
	// don't double map se constaint:  'num_states^2' clauses
	for (i=0;i<num_states;i++) {// i is the se
		for (j=0;j<num_states;j++) { // j is state 1
			for (k=j+1;k<num_states;k++) { // k is state 2
				if (j!=k) {
					literal=-state_to_se_literal(j,i,num_states);
					//clauses[n][0]=literal;
					if (filename) fprintf(mycnf,"%d ",literal);
					literal2=-state_to_se_literal(k,i,num_states);
					//clauses[n++][1]=literal2;
					if (filename) fprintf(mycnf,"%d ",literal2);
					fprintf(mycnf,"0\n");
				}
			}
			
		}
	}
	
	// don't double map state constaint:  'num_states^2' clauses
	for (i=0;i<num_states;i++) {// i is the state
		for (j=0;j<num_states;j++) { // j is se 1
			for (k=j+1;k<num_states;k++) { // k is se 2
				if (j!=k) {
					literal=-state_to_se_literal(i,j,num_states);
					//clauses[n][0]=literal;
					if (filename) fprintf(mycnf,"%d ",literal);
					literal2=-state_to_se_literal(i,k,num_states);
					//clauses[n++][1]=literal2;
					if (filename) fprintf(mycnf,"%d ",literal2);
					fprintf(mycnf,"0\n");
				}
			}
			
		}
	}
	
	
	if (filename) {
		fprintf(mycnf, "%s", "c CHECK-NEXT: ^v .*1$\n");
		fclose(mycnf);
	}
}

int perform_state_mapping (char *filename,nfa *my_nfa) {
	clock_t start,end;
	char filename3[1024];
	int violations;
	int old_violations=0;
	int cycles_without_forward_progress=0;
	FILE *myFile;
	int i;
	
	start = clock();
	
	
	violations=validate_interconnection(my_nfa->edge_table,
							 my_nfa->reverse_table,
							 my_nfa->num_states,
							 my_nfa->max_edges,
							 my_nfa->max_fan_in,
							 my_nfa->movement_map,
							 my_nfa->orig_edge_table,
							 my_nfa->max_fanout);
	
	while (violations) {
		printf ("*********************scan complete, violations = %d\n",violations);
		end = clock();
		if((end-start)>300000000) { // microseconds
			printf("\n*******************higher fanout than %d required\n\n", max_fanout);
			sprintf(filename3, "%s.txt", filename);
			myFile = fopen(filename3,"w+"); //filename3, "w+");
			fprintf(myFile, "Filename = %s\nnum_states= %d\n violation in max_fanout=%d\n",
																	filename,
																	num_states,
																	max_fanout);
			fclose(myFile);
			break;
		}
		if (violations >= (old_violations-10))
			cycles_without_forward_progress++;
		else
			cycles_without_forward_progress=0;
		
		old_violations = violations;
		
		if (cycles_without_forward_progress > 20) mix_it_up(my_nfa,10000);
		
		violations=validate_interconnection(my_nfa->edge_table,
							 my_nfa->reverse_table,
							 my_nfa->num_states,
							 my_nfa->max_edges,
							 my_nfa->max_fan_in,
							 my_nfa->movement_map,
							 my_nfa->orig_edge_table,
							 my_nfa->max_fanout);
	}
	
	// print solution as a logic expression
	printf ("mapping solution: ");
	for (i=0;i<my_nfa->num_states;i++) {
		printf ("%d ",state_to_se_literal(my_nfa->movement_map[i],i,my_nfa->num_states));
	}
	printf ("\n");
}

void mix_it_up (nfa *my_nfa,int n) {
  int i;

  for (i=0;i<n;i++) move_ste (my_nfa->num_states,
							  my_nfa->max_edges,
							  my_nfa->max_fan_in,
							  my_nfa->edge_table,
							  my_nfa->reverse_table,
							  my_nfa->movement_map,
							  rand()%my_nfa->num_states,
							  rand()%my_nfa->num_states);
}

void move_ste (int num_states,
			   int max_edges,
			   int max_fan_in,
			   int **edge_table,
			   int **reverse_table,
			   int *movement_map,
			   int from,
			   int to) {
	
	int temp[max_edges],i,j,k,rev,forward,temp3;
	int *rev_edge_count;

	rev_edge_count = (int *)malloc(sizeof(int)*num_states);

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
		for (j=0;j<max_fan_in;j++) {
			reverse_table[i][j]=reverse_table[i+1][j];
		}
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
		for (j=0;j<max_fan_in;j++) {
			reverse_table[i][j]=reverse_table[i-1][j];
		}
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
  free (rev_edge_count);
}

int score(int max_edges,int **edge_table,int **reverse_table,int a, int b) {

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
int reverse_movement_map (int num_states,int *movement_map,int n) {
	int i;
	
	for (i=0;i<num_states;i++)
		if (movement_map[i]==n) return i;
	
	return -1;
}

void check_graphs (int num_states,
				   int max_edges,
				   int **edge_table,
				   int *movement_map,
				   int **orig_edge_table,
				   int rev) {
		
	int i,j,k,a,b;
	
 	// for (i=0;i<num_states;i++) 
                // for (j=0;j<max_edges;j++) 
                      // printf("edge_table[%d][%d]= %d\n", i, j, edge_table[i][j]);
	
	//#pragma omp parallel for
	for (i=0;i<num_states;i++) {
		for (j=0;j<max_edges;j++) {
//			printf("edge_table[%d][%d]= %d\n", i, j, edge_table[i][j]);

			if (edge_table[i][j]==-1) break;
			if (!rev) {
				a=orig_edge_table[movement_map[i]][j];
				b=movement_map[edge_table[i][j]];
			} else {
				a=orig_edge_table[reverse_movement_map(num_states,movement_map,i)][j];
				b=reverse_movement_map(num_states,movement_map,edge_table[i][j]);
			}
			if (a != b) {
				fprintf (stderr,"error: new edge %d->%d should map to original edge %d->%d\n",i,edge_table[i][j],movement_map[i],orig_edge_table[movement_map[i]][j]);
				assert(0);
			}
		}
	}
	
 	//#pragma omp parallel for
	for (i=0;i<num_states;i++) {
		for (j=0;j<max_edges;j++) {
			if (orig_edge_table[i][j]==-1) break;
			if (!rev) {
				a=reverse_movement_map(num_states,movement_map,orig_edge_table[i][j]);
				b=edge_table[reverse_movement_map(num_states,movement_map,i)][j];
			} else {
				a=movement_map[orig_edge_table[i][j]];
				b=edge_table[movement_map[i]][j];
			}
			if (a != b) {
				fprintf (stderr,"error: original edge %d->%d should map to new edge %d->%d\n",
												i,
												orig_edge_table[i][j],
												reverse_movement_map(num_states,movement_map,i),
												edge_table[reverse_movement_map(num_states,movement_map,i)][j]);
				assert(0);
			}
		}
	}
}

int dump_edges () {
	int k,l;
  for (int i=0;i<6;i++) {
    printf ("[%d]",i);
    for (int j=0;j<max_edges;j++) if (edge_table[i][j]!=-1) printf (" ->%d",edge_table[i][j]);
    printf ("\n");
  }
  
   		// debug
		for (k=0;k<20;k++) {
			printf ("%d -> ",k);
			for (l=0;l<max_edges;l++) printf ("%d ",edge_table[k][l]);
			printf ("\n");
			fflush(stdout);
		}

}

int validate_interconnection(int **edge_table,
							 int **reverse_table,
							 int num_states,
							 int max_edges,
							 int max_fan_in,
							 int *movement_map,
							 int **orig_edge_table,
							 int max_fanout) {
		
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
	  
            differential_score = score(max_edges,edge_table,reverse_table,from,to);
            // make the move
			move_ste (num_states,max_edges,max_fan_in,edge_table,reverse_table,movement_map,from,to);
			//check_graphs();
            differential_score -= score(max_edges,edge_table,reverse_table,from,to);
            // undo the move
            move_ste (num_states,max_edges,max_fan_in,edge_table,reverse_table,movement_map,from,to);
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

            differential_score = score(max_edges,edge_table,reverse_table,from,to);
            // make the move
            move_ste (num_states,max_edges,max_fan_in,edge_table,reverse_table,movement_map,from,to);
			//check_graphs();
            differential_score -= score(max_edges,edge_table,reverse_table,from,to);
            // undo the move
            move_ste (num_states,max_edges,max_fan_in,edge_table,reverse_table,movement_map,from,to);
			//check_graphs();
            if (differential_score > max_differential_score) {
              max_differential_score=differential_score;
              best_from=from;
              best_to=to;
            }
          }
        }
      

	    //if (max_differential_score>0) {
			
/*  			printf ("moving STE in position %d to position %d due to bad edge %d (\"%s\")-> %d (\"%s\"), best score = %d\n",
							best_from,
							best_to,
							i,
							anml_name(my_nfa,movement_map[i]),  //ANML_NAME(i),
							edge_table[i][j],
							anml_name(my_nfa,orig_edge_table[movement_map[i]][j]), //(edge_table[orig_edge_table[movement_map[i]][j]][j]), // ANML_NAME(edge_table[i][j]), //orig_edge_table[movement_map[edge_table[i][j]][j]]),
							max_differential_score); */

			move_ste (num_states,max_edges,max_fan_in,edge_table,reverse_table,movement_map,from,to);
			
			//printf("----------moving %d to %d\n",best_from,best_to);
		//}
        violations++;
		
		}
	}
  }
  
  check_graphs (num_states,max_edges,edge_table,movement_map,orig_edge_table,0);
  
  return violations;
}

void critical_path(int node)
{
  if (visited[node] == 1)
  {
    return;
  }

  max_path++;

  for (int i = 0; i < max_edges; i++)
  { 
    if(report_table[node] == 1)
    {
      visited[node] = 1;

      if (path_length > path_compare)
      {
        path_compare = max_path;
        max_path = 1;
      }
    }
    critical_path(edge_table[node][i]);
  }
}
