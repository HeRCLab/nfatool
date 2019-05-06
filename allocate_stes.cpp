#include "nfatool.h"

char *anml_name (nfa *my_nfa,int se) {
	return (char *)my_nfa->node_table[se]->properties->children->content;
}

int map_states_with_sat_solver (char *filename,nfa *my_nfa) {
	char cnf_filename[1024],str[2048];
	int i,violations,ret;
	
	// auto-generate CNF filename
	sprintf(cnf_filename,"%s.cnf",filename);
	
	// allocate memory for CNF description and for the result
	int **clauses;
	int num_states = my_nfa->num_states;
	int num_clauses = 3*num_states*num_states*num_states;
	char *line,*tok;
	int nchars,literal,state,se;
	
	//clauses = (int **)malloc(num_clauses*sizeof(int *));
	clauses = 0;
	int columns = max_fanout > num_states ? max_fanout : num_states;
	//for (i=0;i<num_clauses;i++) clauses[i]=(int *)malloc(columns*sizeof(int));
	vector<int> sat_solution;
	
	perform_cnf_translation(clauses,
							my_nfa,
							cnf_filename);
	
	char cmd[2048];
	char sat_output_filename[1024];
	sprintf(sat_output_filename,"%s.out",cnf_filename);
	sprintf(cmd,"cat %s | %s > %s",cnf_filename,SAT_SOLVER_COMMAND,sat_output_filename);
	
	ret=system(cmd);
	if (ret==-1) {
		fprintf (stderr,"ERROR:  could not invoke the SAT solver using command \"%s\"\n",SAT_SOLVER_COMMAND);
		return 0;
	}

	wait();
	FILE *myFile;
	myFile=fopen(sat_output_filename,"r+");
	if (!myFile) {
		sprintf (str,"ERROR:  cannot open SAT solver output file \"%s\" for reading.\n",sat_output_filename);
		perror(str);
		return 0;
	}

	while (getline(&line,(size_t *)&nchars,myFile) > 3) {
		if (line[0]=='s' && strncmp(line+2,"SATISFIABLE",11)) {
			fprintf (stderr,"ERROR:  SAT solver failed to find valid mapping.\n");
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
	}
	
	printf ("INFO:  Mapping solution found with SAT solver.\n");
			
	for (i=0;i<sat_solution.size();i++) {
		literal = sat_solution[i];
		literal_to_mapping(literal,&state,&se,my_nfa->num_states);
		my_nfa->movement_map[se]=state;
	}
			
	// apply the new mapping to the NFA
	apply_movement_map(my_nfa);
	
	// reverse the edge table, since this is needed by validate_interconnection()
	reverse_edge_table(my_nfa);
  
	// validate that the new mapping is consistant with the original NFA graph
	check_graphs(my_nfa,0);
	
	// validate that no fanout violations exist
	violations=validate_interconnection(my_nfa);
	if (violations) {
		fprintf(stderr,"ERROR:  SAT solver produced mapping solutions with fanout violations!\n");
		return 0;
	}
			
	
	return 1;
}

void apply_movement_map (nfa *my_nfa) {
	int i,j;
	int new_state;
	
	int **edge_table=my_nfa->edge_table;
	int **orig_edge_table=my_nfa->orig_edge_table;
	int num_states=my_nfa->num_states;
	int max_edges=my_nfa->max_edges;
	int *movement_map=my_nfa->movement_map;
	
	// copy the current edge_table to original_edge_table
	for (i=0;i<num_states;i++) {
		for (j=0;j<max_edges;j++) {
			orig_edge_table[i][j]=edge_table[i][j];
		}
	}
	
	// re-write edge_table according to movement_map
	// movement_map[i] will hold the original state now in SE i
	for (i=0;i<num_states;i++) {
		new_state = reverse_movement_map(my_nfa,i);
		if (new_state==-1) {
			fprintf(stderr,"ERROR: reverse_movement_map() returned error code\n");
			exit(0);
		}
		for (j=0;j<max_edges;j++) {
			if (orig_edge_table[i][j]!=-1)
				edge_table[new_state][j]=reverse_movement_map(my_nfa,orig_edge_table[i][j]);
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
		printf ("%10d%10d\n",i,reverse_movement_map(my_nfa,i));
	}
}

int perform_cnf_translation (int **clauses,nfa *my_nfa,char *filename) {
	int i,j,k,l,n=0,m;
	int literal,literal2;
	FILE *mycnf;
	char str[1024];
	
	int num_states = my_nfa->num_states;
	int max_edges = my_nfa->max_edges;
	int **edge_table = my_nfa->edge_table;
	int max_fanout = my_nfa->max_fanout;
	
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
	
	while (violations=validate_interconnection(my_nfa)) {
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

  for (i=0;i<n;i++) move_ste(my_nfa,
							 rand()%my_nfa->num_states,
							 rand()%my_nfa->num_states);
}

void move_ste (nfa *my_nfa,int from, int to) {
	int temp[my_nfa->max_edges],i,j,k,rev,forward,temp3;
	int rev_edge_count[my_nfa->num_states];
	int num_states = my_nfa->num_states;
	int max_edges = my_nfa->max_edges;
	int **edge_table = my_nfa->edge_table;
	int *movement_map = my_nfa->movement_map;
	int **reverse_table = my_nfa->reverse_table;
	int max_fan_in = my_nfa->max_fan_in;

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
}

int score(nfa *my_nfa,int a, int b) {

  int temp,sum = 0, diff;
  int **edge_table = my_nfa->edge_table;
  int **reverse_table = my_nfa->reverse_table;
  int max_edges = my_nfa->max_edges;

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
int reverse_movement_map (nfa *my_nfa,int n) {
	int i;
	
	int *movement_map = my_nfa->movement_map;
	
	for (i=0;i<my_nfa->num_states;i++)
		if (movement_map[i]==n) return i;
	
	return -1;
}

void check_graphs (nfa *my_nfa, int rev) {
	int i,j,k,a,b;
	int num_states = my_nfa->num_states;
	int max_edges = my_nfa->max_edges;
	int **edge_table = my_nfa->edge_table;
	int *movement_map = my_nfa->movement_map;
	int **orig_edge_table = my_nfa->orig_edge_table;
	
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
				a=orig_edge_table[reverse_movement_map(my_nfa,i)][j];
				b=reverse_movement_map(my_nfa,edge_table[i][j]);
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
				a=reverse_movement_map(my_nfa,orig_edge_table[i][j]);
				b=edge_table[reverse_movement_map(my_nfa,i)][j];
			} else {
				a=movement_map[orig_edge_table[i][j]];
				b=edge_table[movement_map[i]][j];
			}
			if (a != b) {
				fprintf (stderr,"error: original edge %d->%d should map to new edge %d->%d\n",i,orig_edge_table[i][j],reverse_movement_map(my_nfa,i),edge_table[reverse_movement_map(my_nfa,i)][j]);
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

int validate_interconnection(nfa *my_nfa) {
  int violations = 0,
    from = 0,
    to = 0,
    max_differential_score = 0,
    best_from,best_to = 0,
    differential_score = 0,i,j,k;
	
	int **edge_table=my_nfa->edge_table;
	int num_states=my_nfa->num_states;
	int max_edges=my_nfa->max_edges;
	int *movement_map=my_nfa->movement_map;
	int **orig_edge_table=my_nfa->orig_edge_table;
	int max_fanout=my_nfa->max_fanout;
	
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
	  
            differential_score = score(my_nfa,from,to);
            // make the move
            move_ste(my_nfa,from,to);
			//check_graphs();
            differential_score -= score(my_nfa,from,to);
            // undo the move
            move_ste(my_nfa,to,from);
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

            differential_score = score(my_nfa,from,to);
            // make the move
            move_ste(my_nfa,from,to);
			//check_graphs();
            differential_score -= score(my_nfa,from,to);
            // undo the move
            move_ste(my_nfa,to,from);
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

			move_ste(my_nfa,best_from,best_to);
			
			//printf("----------moving %d to %d\n",best_from,best_to);
		//}
        violations++;
		
		}
	}
  }
  
  check_graphs(my_nfa,0);
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
