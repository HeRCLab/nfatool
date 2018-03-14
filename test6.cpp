#include<iostream>
#include<set>
#include<vector>
#include<algorithm>

using namespace std;
using std::vector;

#define max_edges 3
#define max_states 7

int edge_table[max_states][max_edges];
int count_cycle =1;

	std::vector<int> visited;

// Depth Search First Algorithm
/* 	v1: set of all nodes
 	v2: move visiting nodes from v1 to v2 (maybe cycle) and still are being explored
 	v3: move nodes from v2 to v3 , has no cycle to final set
*/
    
int reverse_edge_table () {
  int change,i,j,k,
    fanin = 0;

  int rev_edge_count[num_states];
  for (i=0;i<num_states;i++) rev_edge_count[i]=0;

  for (int i = 0; i < num_states; i++) {
    fanin=0;
      for(int j = 0; j < max_edges; j++){
        if(edge_table[i][j] == -1){
          break;
        }else {
          rev_edge_count[edge_table[i][j]]++;
          if (rev_edge_count[edge_table[i][j]] > max_fan_in) max_fan_in = rev_edge_count[edge_table[i][j]];
        }
    }
  }

void dfs(int current_node) {
        int flag,visited_node,in_loop=0;

    for (int i=0;i<visited.size();i++) if (current_node == visited[i]) return;
    visited.insert(visited.begin(),current_node);

    


     // ------------------------------------------------------------------			
     // Start going through all the edges of the current_node
        for(int j=0; j<max_edges; j++){
                //std::cout << "edge_table of current node" << current_node << " " << edge_table[current_node][j] << endl;
		
    		if (edge_table[current_node][j]!=0) {
            // Test if the edge in the final set, means no cycle found  
    		// test the looop ??? ?
    			visited_node=0;
                for(int k=0; k<visited.size(); k++)
                    if(edge_table[current_node][j] == visited[k]) {
                        visited_node=1;
                        break;
    				}
                    
    			if (!visited_node) dfs(edge_table[current_node][j]);
    		}
	}
	

	return;
}

//---------------------------------------------------------------------
int main(void) {

     	std::vector<int> start;
        std::vector<int> maycycle;
        std::vector<int> nocycle;

        // Store Graph 1->2->3->4->5  cycles: 1->1, 1->2->3->4->5->1, 1->2->3->4->5->3
     	edge_table[1][0] = 2;
        edge_table[1][1] = 5;
	edge_table[1][2] = 0;
        edge_table[2][0] = 3;
        edge_table[2][1] = 0;
        edge_table[3][0] = 4;
        edge_table[3][1] = 0;;
        edge_table[4][0] = 3;
        edge_table[4][1] = 0;
        edge_table[5][0] = 6;
        edge_table[5][1] = 0;
	edge_table[6][0] = 5;
	edge_table[6][1] = 0;


        // 1->2->1, 1->2->3->2, 1->2-3-4-3, 1-2-3-4-5-4
/*      edge_table[1][0] = 2;  This is found correctly two cycles 
        edge_table[1][1] = 0;
        edge_table[2][0] = 1;
        edge_table[2][1] = 3;
        edge_table[3][0] = 4;
        edge_table[3][1] = 0;
        edge_table[4][0] = 5;
        edge_table[4][1] = 0;
        edge_table[5][0] = 4;
        edge_table[5][1] = 0;
*/

/*
  	edge_table[1][0] = 2;
        edge_table[1][1] = 0;
        edge_table[2][0] = 1;
        edge_table[2][1] = 3;
        edge_table[3][0] = 1;
        edge_table[3][1] = 4;
        edge_table[4][0] = 5;
        edge_table[4][1] = 0;
        edge_table[5][0] = 0;   // Can't find a cycle when leaf 
        edge_table[5][1] = 0;
*/

/*  	edge_table[1][0] = 2;  
        edge_table[1][1] = 0;
        edge_table[2][0] = 3;
        edge_table[2][1] = 0;
        edge_table[3][0] = 4;
        edge_table[3][1] = 0;
        edge_table[4][0] = 5;
        edge_table[4][1] = 0;
        edge_table[5][0] = 1;
        edge_table[5][1] = 5;
*/
   // Initial start by path nodes
        
                dfs(1);



        for(int i=0; i<visited.size(); i++)
                std::cout << "L: " << visited[i] << endl;


return 0;
}

                  

