#include "nfatool.h" 
#include <string.h> 



void dofile_gates()  
{ 		

  // store edge_table in gates_2D 

	for(int i=0; i<num_states; i++) 
	  for(int j=0; j<max_edges; j++) {

             int dest = edge_table[i][j];
                                gates_2D[i][abs(dest-i)]=1;
             }
	
        
			

 // Add the reports 

	for(int i=0; i<num_states; i++) 
		if(report_table[i]) gates_2D[i][max_fan-1] =1 ; 


	for(int i=(max_stes-1); i>=0; i--) 
		for(int j=(max_fan-1); j>=0; j--) 
			gates_1D[i*max_fan+j] = gates_2D[i][j]; 


	int time =do_time; 

	FILE *gatesdo; 
	FILE *gatesfile; 

	gatesdo = fopen("gates_dofile.do", "w+");  
	gatesfile = fopen("gates_bit.txt", "w+"); 


 	for(int row=max_stes*max_fan+max_fan; row>=0 ; row--)
        {

	        fprintf(gatesdo, "force configure_interconnect 1 %d\n", time);
        	fprintf(gatesdo, "force gates_serial_in %d %d\n", gates_1D[row], time);
		fprintf(gatesfile, "%d", gates_1D[row]); 

	        time=time+100;
        }

        fprintf(gatesdo, "force configure_interconnect 0 %d\n", time);
	fprintf(gatesfile, "\n"); 



        // Set to start read from M20k, and enable reading from next_state_table

        fprintf(gatesdo, "force count_start 0 %d, 1 %d, 0 %d\n", time-100, time, time+1000);
        fprintf(gatesdo, "force update_next_state 0 %d\n", time);

        printf("Finish gatesin configuration time = %d \n", time);

	fclose(gatesdo); 
	fclose(gatesfile); 

}



	


