void move_ste (int from, int to) 
{
  int temp[max_edges],
  	  temp2[max_fan_in],
  	  rev,
  	  forward,
  	  temp3,
  	 rev_edge_count[num_states];

  if (from<to)
  {
  
	  // scan the whole array!!!!!
 //	  #pragma omp parallel for
	  for (int i=0;i<num_states;i++) 
    {
			for (int j=0;j<max_edges;j++) 
      {
				if (edge_table[i][j]==-1) break;
				if (edge_table[i][j]==from) edge_table[i][j]=to; else
				if ((edge_table[i][j]>from) && (edge_table[i][j]<=to)) edge_table[i][j]--;
			}
	  }
  }
  
  
  // STEP 2b:  update edge references INTO (FROM,TO]
  else 
  {
  
	  // scan the whole array!!!!!
 //	  #pragma omp parallel for
	  for (int i=0;i<num_states;i++) 
    {
			for (int j=0;j<max_edges;j++)
      {
				if (edge_table[i][j]==-1) break;
				if (edge_table[i][j]==from) edge_table[i][j]=to; else
				if ((edge_table[i][j]>=to) && (edge_table[i][j]<from)) edge_table[i][j]++;
			}
	  }
  }
  
  for (int i=0;i<max_edges;i++) temp[i]=edge_table[from][i];
  
  // STEP 4a:  shift contents
  if (from<to)
  {
	 temp3=movement_map[from];
	
	 for (int i=from;i<to;i++) 
   {
    for (int j=0;j<max_edges;j++)
    {
			edge_table[i][j]=edge_table[i+1][j];
		}
		movement_map[i]=movement_map[i+1];
	 }
	
	 movement_map[to]=temp3;
  }
  
  // STEP 4b:  shift contents
  if (to<from) 
  {
	 temp3=movement_map[from];
	
	 for (int i=from;i>to;i--)
   {
		  for (int j=0;j<max_edges;j++) 
      {
			 edge_table[i][j]=edge_table[i-1][j];
		  }
		  movement_map[i]=movement_map[i-1];
	 }
	 movement_map[to]=temp3;
  }
  
   // STEP 3:  in reverse table, copy incoming edges into node from to node to
  //for (i=0;i<max_fan_in;i++) reverse_table[to][i] = temp2[i];
  for (int i=0;i<max_edges;i++) edge_table[to][i] = temp[i];
  
  // rebuild reverse tabls
  //#pragma omp parallel for
  for (int i=0;i<num_states;i++) for (int j=0;j<max_fan_in;j++) reverse_table[i][j]=-1;
  //#pragma omp parallel for
  for (int i=0;i<num_states;i++) rev_edge_count[i]=0;
  //#pragma omp parallel for
  for(int i = 0; i < num_states; i++) for(int j = 0; j < max_edges; j++) if(edge_table[i][j] == -1) break;
  else 
  {
  //#pragma omp atomic
  rev_edge_count[edge_table[i][j]]++;
	reverse_table[edge_table[i][j]][rev_edge_count[edge_table[i][j]]] = i;
	if (rev_edge_count[edge_table[i][j]] > max_fan_in) 
  {
		printf("exceeded max fan_in!");
		assert(0);
	}
  }
}

int Score(int a, int b)
{

  int temp,sum = 0, diff;

  if(a > b) 
  {
	temp=a;
	a=b;
	b=temp;
  }

  //#pragma omp parallel for reduction(+,sum)
    for(int i = a; i <= b; i++) 
    {

      for(int j = 0; j < max_edges; j++)
      {

        if(edge_table[i][j] == -1)
        {
          break;
        }
        else
		  diff = edge_table[i][j] - i;
		  //if ((diff < (-(max_fanout-1)/2)) || (diff > (max_fanout/2))) diff = diff*3;
          sum += abs(diff);
      }
      for (int j = 0; j < max_fan_in; j++)
      {
        if(reverse_table[i][j] == -1)
        {
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

int reverse_movement_map (int n) 
{
	int i;
	
	for (i=0;i<num_states;i++) if (movement_map[i]==n) return i;
}

void check_graphs ()
{
	int i,j,k;
	
	//#pragma omp parallel for
	for (i=0;i<num_states;i++) 
  {
		for (j=0;j<max_edges;j++) 
    {
			if (edge_table[i][j]==-1) break;
			if (orig_edge_table[movement_map[i]][j] != movement_map[edge_table[i][j]]) 
      {
				fprintf (stderr,"error: new edge %d->%d should map to original edge %d->%d\n",i,edge_table[i][j],movement_map[i],orig_edge_table[movement_map[i]][j]);
				assert(0);
			}
		}
	}
	
 	//#pragma omp parallel for
	for (i=0;i<num_states;i++) 
  {
		for (j=0;j<max_edges;j++) 
    {
			if (orig_edge_table[i][j]==-1) break;
			if (reverse_movement_map(orig_edge_table[i][j]) != edge_table[reverse_movement_map(i)][j]) 
      {
				fprintf (stderr,"error: original edge %d->%d should map to new edge %d->%d\n",i,orig_edge_table[i][j],reverse_movement_map(i),edge_table[reverse_movement_map(i)][j]);
				assert(0);
			}
		}
	}
}

int dump_edges ()
{
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

int validate_interconnection() 
{
  int violations = 0,
    from = 0,
    to = 0,
    max_differential_score = 0,
    best_from,best_to = 0,
    differential_score = 0,i,j,k;

  for(i = 0; i < num_states; i++) {
  
  //if (i%100==0) printf ("%0.2f%% scan complete, v=%d\n",(float)i/(float)num_states*100.0f,violations);
  
    for(j = 0; j < max_edges; j++) 
    {

		  if (edge_table[i][j] == -1) break;
       // check for interconnect violations and fix when necessary

      if(((edge_table[i][j] - i) < (-(max_fanout-1)/2)) || ((edge_table[i][j] - i) > (max_fanout/2))) 
      { 
		
        max_differential_score = -INT_MAX;
      
        // proposal 1:  move source
		    from = i;
		    assert(from >= 0);
     
        for (k=0;k<max_fanout-1;k++) 
        {
          to = edge_table[i][j] - max_fanout/2 + k;
          if ((to >= 0) && (to < num_states -1))
          {
	  
            differential_score = Score(from,to);
            // make the move
            move_ste(from,to);
			       //check_graphs();
            differential_score -= Score(from,to);
            // undo the move
            move_ste(to,from);
			       //check_graphs();
            if (differential_score > max_differential_score) 
            {
              max_differential_score=differential_score;
              best_from = from;
              best_to=to ;
            }
          }
        }
      
        // proposal 2:  move destination
        from = edge_table[i][j];
        assert(from >= 0);

        for (k=0;k<max_fanout-1;k++) 
        {
          to = i - (max_fanout-1)/2 + k;
          if ((to >= 0) && (to < num_states -1)) 
          {
            differential_score = Score(from,to);
            // make the move
            move_ste(from,to);
			//check_graphs();
            differential_score -= Score(from,to);
            // undo the move
            move_ste(to,from);
			//check_graphs();
            if (differential_score > max_differential_score) 
            {
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

int reverse_edge_table () 
{
  int change,i,
      fanin = 0;

  int rev_edge_count[num_states];
  for (i=0;i<num_states;i++) rev_edge_count[i]=0;

  for (int i = 0; i < num_states; i++) 
  {
    fanin=0;
      for(int j = 0; j < max_edges; j++)
      {
        if(edge_table[i][j] == -1)
        {
          break;
        }else 
        {
          rev_edge_count[edge_table[i][j]]++;
          if (rev_edge_count[edge_table[i][j]] > max_fan_in) max_fan_in = rev_edge_count[edge_table[i][j]];
        }
    }
  }

  
  reverse_table = (int **)malloc((sizeof(int*))*num_states);

    for (int i=0;i<num_states;i++)
    {
      reverse_table[i]=(int *)malloc(sizeof (int)*max_fan_in);
      for (int j=0;j<max_fan_in;j++) reverse_table[i][j]=-1;
    }

  for (i=0;i<num_states;i++) rev_edge_count[i]=0;
  
    for(int i = 0; i < num_states; i++)
    {
      for(int j = 0; j < max_edges; j++)
      {
        if(edge_table[i][j] == -1)
        {
          break;
        }else 
        {
          reverse_table[edge_table[i][j]][rev_edge_count[edge_table[i][j]]++] = i;
        }
      }
    }
}

