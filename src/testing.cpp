#include "nfatool.h"

void nextState_dofile (int num_states1, unsigned char  **ns_table) 
{

	
	FILE *myfile ; 
	myfile = fopen("ns_bitstream.txt", "w+") ; 

	for(int i=0; i<num_states1; i++) {
		fprintf(myfile, "\n"); 

		for(int j=0; j<256; j++) 
			fprintf(myfile , "%d ", *ns_table[i][j]); 
	}

	fclose(myfile); 
}
