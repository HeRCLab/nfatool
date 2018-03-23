/*
 Name: 				Lothrop Omari Richards
 
 Date: 				March 23, 2018

 Information:		The Purppose of these three functions are to find the largest strong connection of stes. To determine 
 					to value of the largest cycle, you will call "pass_one()" after the "fill_in_table" function. Everything 
 					within this document is required  to be used in order to run. 
 */

std::vector<int> *strong_cycles;

int start_of_cycle=0;
int potential_node_in_cycle=0;
int possible_end_of_cycle=0;
int max_strong_node=0;

boolean cycle_confirm=false;
/*
 	pass_one:				pass one will check values stored within the edge table. if 
 							it is found that there is a back connection (as of now, i assume
 							this is how every cycle will start in the code), the function "check" will
 							be called in order to see if the connection that was refered will connect 
 							to the initial ste. If it is found that the cycle is detected, the initial
 							state will be passed into the pass_two function witch will take care of 
 							finding the total path of the cycle. After the path for the cycle is found,
 							"pass_one" function will compare exiting cycles found based on sizes and save 
 							the bigger one out of the two.

 	pass_two:				taking value and passing its connection to the "pass_two" function

 	pass_three:				the job of the "pass_three" will to do a DFS of a node to determine if it may 
 							have a path to the end of the cycle. If so the existing node will be stored within 
 							the vector. 
 */

void check(int val)
{
	for (int i = val; i < num_states; i++)
	{
		for (int j = 0; j < max_edges; j++)
		{
		  if (edge_table[i][j] == possible_end_of_cycle)
		  {
		  	cycle_confirm=true;
		  	return;
		  }
		}
	}
}

void pass_two(int val)
{
	for (int i = 0; i < max_edges; i++)
	{
		potential_node_in_cycle=edge_table[val][i];
		pass_three(edge_table[val][i]);
	}
}

void pass_three(int val)
{
	if (val == possible_end_of_cycle)
	{
		strong_of_cycle.push_back(potential_node_in_cycle)
		pass_two(potential_node_in_cycle);
		return;
	}
	for (int i =  0; i <max_edges; i++){
		pass_three(edge_table[val][j])
	}
}

void pass_one()
{
	for (int i = 0; i < num_states; i++)
	{
		for (int j = 0; j < max_edges; j++)
		{
			if (edge_table[i][j] < i)
			{
				start_of_cycle = edge_table[i][j];
				possible_end_of_cycle = i;

				if (cycle_confirm)
				{
					strong_cycles.push_back(edge_table[i][j]);
					pass_two(edge_table[i][j]);

					if(strong_cycles.size() > max_strong_node)
					{
						max_strong_node=strong_cycles.size();
						strong_cycles.clear();
						cycle_confirm=false;
					}
				}
			}
		}
	}
}
