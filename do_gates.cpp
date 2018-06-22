#include "nfatool.h" 
#include <string.h> 



void dofile_gates()  
{ 		

  // store edge_table in gates_2D 

	for(int i=0; i<num_states; i++) 
	 for(int j=0; j<max_edges; j++) 

             int dest = edge_table[i][j];
                                gates_2D[i][abs(dest-i)]=1;
             }
	
        }
			

	for(int i=0; i<max_stes; i++) 
		if(report_table[i]) gates_2D[i] =0 ; 


	for(int i=(max_stes-1); i>=0; i--) 
		for(int j=(max_fan-1); j>=0; j--) 
			gates_1D[i*max_fan+j] = gates_2D[i][j]; 


}

	
