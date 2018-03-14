#include "nfatool.h"

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

void split_colorv2(int ste, int color)
{ 

  int k;

  /*
   * Erases all the colors within the tree before adding new colors 
   */

  for (std::vector<int>::iterator i = state_colors[ste].begin();i!=state_colors[ste].end();i++)
  {
    if (*i==color) 
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

  for (std::vector<int>::iterator i = state_colors[ste].begin(); i != state_colors[ste].end(); i++)
  {
    k++;
      
    if (edge_table[ste][k] == -1)
    {
      break;
    }

    replace_colorv2(edge_table[ste][k],color,*i);  
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

void becchi_partition () 
{

  //cycle_detection(0);

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
          split_colorv2(j,current_color);
        }
      }
      color_count[i]=0;
    }


  } while (color_size_exceeded);
}
