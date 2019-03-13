#include "nfatool.h"

#define TO_LITERAL(STATE,SE)    STATE*F+SE+1

int map_states_with_sat_solver (char *filename,nfa *my_nfa) {
	char cnf_filename[1024],str[2048];
	int i,violations;
	
	// check for fanout constraint
	// if none then there is no reason to proceed
	if (!max_fanout) {
		fprintf(stderr,"ERROR:  cannot generate CNF files without a specified maximum fanout.\n");
		return 0;
	}
	
	// auto-generate CNF filename
	sprintf(cnf_filename,"%s.cnf",filename);
	
	// allocate memory for CNF description and for the result
	int **clauses;
	int num_states = my_nfa->num_states;
	int num_clauses = 3*num_states*num_states*num_states;
	clauses = (int **)malloc(num_clauses*sizeof(int *));
	int columns = max_fanout > num_states ? max_fanout : num_states;
	for (i=0;i<num_clauses;i++) clauses[i]=(int *)malloc(columns*sizeof(int));
	vector<int> sat_solution;
	
	perform_cnf_translation(clauses,
							my_nfa,
							cnf_filename);
	
	char cmd[2048];
	char sat_output_filename[1024];
	sprintf(sat_output_filename,"%s.cnf.out",cnf_filename);
	sprintf(cmd,"cat %s | %s > %s",cnf_filename,SAT_SOLVER_COMMAND,sat_output_filename);
	
	system(cmd);

	wait();
	FILE *myFile;
	myFile=fopen(sat_output_filename,"r+");
	if (!myFile) {
		sprintf (str,"ERROR:  cannot open SAT solver output file \"%s\" for reading.\n",sat_output_filename);
		perror(str);
		return 0;
	}

	char *line,*tok;
	int nchars,literal,state,se;
	while (feof(myFile)) {
		getline(&line,(size_t *)&nchars,myFile);
		if (line[0]=='s' && strncmp(line+1,"SATISFIABLE",11)) {
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
			printf ("INFO:  Valid mapping found with SAT solver.\n");
			
			for (i=0;i<sat_solution.size();i++) {
				literal = sat_solution[i];
				literal_to_mapping(literal,&state,&se,my_nfa->num_states);
				my_nfa->movement_map[se]=state;
			}
			
			// apply the new mapping to the NFA
			apply_movement_map(my_nfa);
			
			// validate that the new mapping is consistant with the original NFA graph
			check_graphs(my_nfa);
			
			// validate that no fanout violations exist
			violations=validate_interconnection(my_nfa);
			if (violations) {
				fprintf(stderr,"ERROR:  SAT solver produced mapping solutions with fanout violations!\n");
				return 0;
			}
			
			break;
		}
	}
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
		new_state = movement_map[i];
		for (j=0;j<max_edges;j++) {
			edge_table[i][j]=orig_edge_table[new_state][j];
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

int perform_cnf_translation (int **clauses,nfa *my_nfa,char *filename) {
	int i,j,k,l,n=0,m;
	int literal;
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
				literal=-state_to_se_literal(i,j,num_states);
				clauses[n][0]=literal;
				if (filename) fprintf(mycnf,"%d ",literal);
				for (k=0;k<max_edges;k++) { // for each successor
					if (edge_table[i][k]==-1) break;
					if (edge_table[i][k]!=i) { // don't worry about self-loops
						for (l=-(max_fanout-1)/2;l<=max_fanout/2;l++) { // for each possible placement of the successor
							if ((l!=0) && (j+l)>=0 && (j+l)<num_states && l) { // disregard self-loop and avoid breach of array boundaries
								literal=state_to_se_literal(edge_table[i][k],j+l,num_states);
								clauses[n][l+(max_fanout-1)/2+1]=literal;
								if (filename) fprintf(mycnf,"%d ",literal);
							}
						}
					}
				}
				if (filename) fprintf(mycnf,"0\n");
				n++;
			}
		}
	}
	
	// force a mapping of every state:  'num_states^2' clauses
	for (i=0;i<num_states;i++) {
		for (j=0;j<num_states;j++) {
			literal=state_to_se_literal(i,j,num_states);
			clauses[n++][j]=literal;
			if (filename) fprintf(mycnf,"%d ",literal);
		}
		if (filename) fprintf(mycnf,"0\n");
	}
	
	// don't double map se constaint:  'num_states^2' clauses
	for (i=0;i<num_states;i++) {// i is the state
		for (j=0;j<num_states;j++) { // j is the SE
			literal=-state_to_se_literal(i,j,num_states);
			clauses[n][0]=literal;
			if (filename) fprintf(mycnf,"%d ",literal);
			m=1;
			for (k=0;k<num_states;k++) {
				if (i!=k) { // k is the state
					literal=-state_to_se_literal(k,j,num_states);
					clauses[n][m++]=literal;
					if (filename) fprintf(mycnf,"%d ",literal);
				}
			}
			if (filename) fprintf(mycnf,"0\n");
			n++;
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
	
	start = clock();
	
	while (violations=validate_interconnection(my_nfa)) {
		printf ("*********************scan complete, violations = %d\n",violations);
		end = clock();
		if((end-start)>100000) { //milisecond
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
		if (violations >= old_violations)
			cycles_without_forward_progress++;
		else
			cycles_without_forward_progress=0;
		
		old_violations = violations;
		
		if (cycles_without_forward_progress > 10) mix_it_up(1000);
	}
}

void mix_it_up (int n) {
  int i;

  for (i=0;i<n;i++) move_ste(rand()%num_states,rand()%num_states);
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
int reverse_movement_map (nfa *my_nfa,int n) {
	int i;
	
	int *movement_map = my_nfa->movement_map;
	
	for (i=0;i<num_states;i++) if (movement_map[i]==n) return i;
}

void check_graphs (nfa *my_nfa) {
	int i,j,k;
	int num_states = my_nfa->num_states;
	int max_edges = my_nfa->max_edges;
	int **edge_table = my_nfa->edge_table;
	
 	// for (i=0;i<num_states;i++) 
                // for (j=0;j<max_edges;j++) 
                      // printf("edge_table[%d][%d]= %d\n", i, j, edge_table[i][j]);
	
	//#pragma omp parallel for
	for (i=0;i<num_states;i++) {
		for (j=0;j<max_edges;j++) {
//			printf("edge_table[%d][%d]= %d\n", i, j, edge_table[i][j]); 
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
			if (reverse_movement_map(my_nfa,orig_edge_table[i][j]) != edge_table[reverse_movement_map(my_nfa,i)][j]) {
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
			printf ("moving STE in position %d to position %d due to bad edge %d (\"%s\")-> %d (\"%s\"), best score = %d\n",
							best_from,
							best_to,
							i,
							ANML_NAME(movement_map[i]),  //ANML_NAME(i),
							edge_table[i][j],
							ANML_NAME(orig_edge_table[movement_map[i]][j]), //(edge_table[orig_edge_table[movement_map[i]][j]][j]), // ANML_NAME(edge_table[i][j]), //orig_edge_table[movement_map[edge_table[i][j]][j]]),
							max_differential_score);

			move_ste(best_from,best_to);
			//printf("----------moving %d to %d\n",best_from,best_to);
		//}
        violations++;
		
		}
	}
  }
  
  check_graphs(my_nfa);
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
