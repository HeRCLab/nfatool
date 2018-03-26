#include "nfatool.h"
#include "scc.h" 

#define	OVECCOUNT  30
#define STATEMAP_SIZE 150000

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


void split_colorv2(int ste, int color_from, int color_to)    /* RASHA ** adding ste as parameter in split_colorv2   **/

{ 

  int k;

  /*
   * Erases all the colors within the tree before adding new colors 
   */

  for (std::vector<int>::iterator i = state_colors[ste].begin();i!=state_colors[ste].end();i++)
  {
    if (*i==color_from)     				/*RASHA ** change color to color_from **/ 
      {
        state_colors[ste].erase(i);
      }
  }

  /*
   * add colors to the highest priortize ste for splitting based on the amount of outgoing edges it may have 
   */

  for (int i = 1; i <= ExactOutEdge[ste]; i++)
  {
    state_colors[ste].push_back(color_to++);           /* RASHA**  Color to color_to **/ 
  }

  /*
   * when a new brach is found from the start state the color will change among that branch
   */

  for (std::vector<int>::iterator i = state_colors[ste].begin(); i != state_colors[ste].end(); i++)
  {
    k++;
      
    if (edge_table[ste][k] == -1)
    {
      break;
    }

    replace_colorv2(edge_table[ste][k],color_to,*i);      /* RASHA ** color to color_to **/
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
    if (edge_table[str][i] == -1)
    {
      break;
    }
    for (std::vector<int>::iterator j = state_colors[str].begin(); j != state_colors[str].end(); j++)
    {
      // if (j == old_color)
      // {
      //   if (visitedcycle[str] == 1)
      //   {
      //     for (int a = 0; a < Tree_Cyle.size(); a++)
      //     {
      //       for (int b = 0; b < Tree_Cyle[a].size(); b++)
      //       {
      //         if (Tree_Cyle[a][b] == str)
      //         {
      //           for (int c = 0; c < Tree_Cyle[a].size(); c++)
      //           {
      //             state_colors[str].erase(j);
      //             state_colors[str].push_back(new_color);
      //             color_count[new_color]++;
      //           }
      //         }
      //       }
      //     }
      //   }
      //   else
      //     state_colors[str].erase(i);
      //     state_colors[str].push_back(new_color);
      //     color_count[new_color]++;
      // }

      if (*j == old_color)
          {
            state_colors[str].erase(j);
            state_colors[str].push_back(new_color);
            color_count[new_color]++;
          }    
    }
    replace_colorv2(edge_table[str][i], old_color, new_color);
  }
}



void max_color_size (int &max_color_size,int &max_color) {
  int i; 
  max_color_size=1;
  max_color=-1;

  int color_size[MAX_COLORS];

  for (i=0;i<MAX_COLORS;i++) color_size[i]=0;
//  for (i=0;i<num_states;i++) color_size[state_colors[i]]++ ; 		/** RASHA:   NOT WORK ** ??? */ 
  
  for (i=0;i<MAX_COLORS;i++) if (color_size[i]>max_color_size) {
    max_color_size=color_size[i];
    max_color=i;
  }
}


void becchi_partition () 
{
  int max_color,max_color_size;

  // find strongly-connected components, results stored in globals component_list and components
  find_sccs();

  // initial:  all STEs have one color (no replication)
  for (int i = 0; i < num_states; i++)
  {
    state_colors[i].push_back(0); /// = 0;  				/* RASHA ** use push back to initialize with 0 **/ 
  }




  // create virtual root node
  
  

  // for (int i = 0; i<num_states; i++) 
  // {                                                            
  //   if (start_state[i] == 1) 
  //   {
  //     start_color = i;

  //     for (int j = i; i < num_states; j++)
  //     {
  //       traverse_partition(j);  
  //     }
  //   }
  // }
  
  // color_size_exceeded=0;

  /*
   *  The nested for loop below will set a flag for each ste that is found 
   *  within the vector that stores stes that are contain inside a cycle
   *
   */

  // for (int a = 0; a < Tree_Cyle.size(); a++)
  // {
  //   for (int b = 0; b < Tree_Cyle[a].size(); b++)
  //   {
  //     for (int c = 0; c < num_states; c++)
  //     {
  //       for (int d = 0; d < max_edges; d++)
  //       {
  //         if (edge_table[c][d] == Tree_Cyle[a][b])
  //         {
  //           visitedcycle[Tree_Cyle[a][b]] = 1;
  //         }
  //       }
  //     }
  //   }
  // }
  
  // for (int i=0; i<MAX_COLORS; i++) {
  //   if (color_count[i] > MAX_PART_SIZE) {
  //     color_size_exceeded = 1;
  //   }
  // }

//find_max_color_size(max_color_size,max_color);    	/* RASHA **  No definitin to find_max_color_size */

for (int i = 0; i < num_states; i++)

while (max_color_size > max_stes) { 
  split_colorv2(i,max_color,current_color);
  color_count[max_color]=0;
//  max_color_size(max_color_size,max_color);      	/* RASHA ** change find_max_color_size to max_color_size NOT WORKED**/ 
  }
}

/*void max_color_size (int &max_color_size,int &max_color) {
  int i,max_color_size=1,max_color=-1;
  int color_size[MAX_COLORS];

  for (i=0;i<MAX_COLORS;i++) color_size[i]=0;
  for (i=0;i<num_states;i++) color_size[state_colors[i]]++;
  for (i=0;i<MAX_COLORS;i++) if (color_size[i]>max_color_size) {
    max_color_size=color_size[i];
    max_color=i;
  }
}
*/
