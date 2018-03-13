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

bool dfs(int current_node, std::vector<int> &v1, std::vector<int> &v2, std::vector<int> &v3) {
        int flag,visited_node,in_loop=0;

    for (int i=0;i<visited.size();i++) if (current_node == visited[i]) return false;
    visited.push_back(current_node);

    v2.push_back(current_node);
    //Test the nodes in v1 and v2 after movement 

        for(int k=0; k<v2.size(); k++)
                std::cout << "visited nodes " << v2[k] << endl;

     // ------------------------------------------------------------------			
     // Start going through all the edges of the current_node
        for(int j=0; j<max_edges; j++){
                //std::cout << "edge_table of current node" << current_node << " " << edge_table[current_node][j] << endl;
		
		if (edge_table[current_node][j]!=0) {
        // Test if the edge in the final set, means no cycle found  
		// test the looop ??? ?
			visited_node=0;
                	for(int k=0; k<v2.size(); k++)
                        	if(edge_table[current_node][j] == v2[k]) {
                        		std::cout << "confirmed cycle node " << edge_table[current_node][j] << endl;
					v3.insert(v3.begin(),edge_table[current_node][j]);
					visited_node=1;
					in_loop=1;
                        		break;
				}
                
			if (!visited_node) dfs(edge_table[current_node][j], v1, v2, v3);
		}
	}
	
	if (in_loop) v3.push_back(current_node);

	return false;
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
        for (int i = 1; i <= max_states; i++) {
                dfs(i,start,maycycle,nocycle);



        for(int i=0; i<nocycle.size(); i++)
                std::cout << "L: " << maycycle[i] << endl;

		maycycle.clear();
		nocycle.clear();
	}

return 0;
}

                  

