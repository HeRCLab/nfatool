#include "nfatool.h" 
#include <string.h>

void dofile_next () { // int num , unsigned char **next){ //int states_num, int next_do[256][states_num]) { 


	FILE *dofile; 

	int time=0; 

	dofile = fopen("ns_dofile.do", "w+"); 

	printf("Initialize signals in do file next state \n"); 


	fprintf(dofile, "# create a work directory for modelsim\n");
        fprintf(dofile, "vlib ./work\n");
        fprintf(dofile, "# use ste_array entity for simulates (load the design)\n\n");
        fprintf(dofile, "view wave\n");
        fprintf(dofile, "view signals\n");
        fprintf(dofile, "view structure\n");

        fprintf(dofile, "# show some the signals we are interested in\n");
        fprintf(dofile, "#add wave -noupdate -divder -height 32 inputs\n\n");

        fprintf(dofile, "add wave clk\n");
        fprintf(dofile, "add wave rst\n");
        fprintf(dofile, "add wave shift_next_state\n");
        fprintf(dofile, "add wave gates_serial_in\n");
        fprintf(dofile, "add wave configure_interconnect\n");
        fprintf(dofile, "add wave gates_serial\n");

        fprintf(dofile, "add wave shift_next_state_in\n");
        fprintf(dofile, "add wave shifting_enable\n");
        fprintf(dofile, "add wave next_state_serial\n");
        fprintf(dofile, "add wave Config_in_symbol\n");

        fprintf(dofile, "add wave update_next_state\n");
        fprintf(dofile, "add wave count_start\n");
        fprintf(dofile, "add wave counter\n");
        fprintf(dofile, "add wave Charles_in_symbol\n");
        fprintf(dofile, "add wave next_state_out\n");
        fprintf(dofile, "add wave incre_config_in_symbol\n");

        fprintf(dofile, "add wave accept\n\n");

        fprintf(dofile, "force -freeze sim:/ste_array_improved/clk 1 0, 0 {50 ps} -r 100\n");
        fprintf(dofile, "force rst 1 %d, 0 %d\n\n", 0, 100);                                    // reset @0ns
        fprintf(dofile, "force -freeze sim:/ste_array_improved/shift_next_state 1 25, 0 {75 ps} -r 100\n");
        fprintf(dofile, "force update_next_state 1'bz 0\n");
        fprintf(dofile, "force incre_config_in_symbol 0 0\n");

	unsigned char tmp[256][num_states]; 

	// Hardware next_state_table is transposed of logical next_state_table 
	// Hardware next_state_table starts by shifting highest to lowest bits order 

	// Transpose next_state_table in tmp 
	for(int row=0; row<num_states; row++) 
		for(int col=0; col<256; col++) 
			tmp[col][row] = next_state_table[row][col]; 


	//shifting bits into do file highest to lowest 

  	for(int row=0; row<256;  row++){

                fprintf(dofile, "#Add = %d\n", row);
                fprintf(dofile, "force shifting_enable 1  %d\n", time);
                fprintf(dofile, "force shift_next_state_in ");

                //Start read cols per row
                for(long col=(num_states-1); col>=0; col--){

                        if(col>0){
                                fprintf(dofile, " %d  %d,", tmp[row][col],time);
//				printf("tmp[%d][%ld]=%d\n", row, col, tmp[row][col]); 

			}else

                        if(col==0){
                                fprintf(dofile, " %d  %d", tmp[row][col],time);
//				printf("tmp[%d][%ld]=%d\n", row, col, tmp[row][col]);

			}


                time = time +100;
                } // End reading cols


                // Start write to Next_state_table
                fprintf(dofile, "\nforce shifting_enable 0 %d", time);

		int WIDTH = 16; 

                for(int n=0; n<(num_states/WIDTH); n++) { /// n= NUM_STE/NEXT_STATE_WIDTH

                        fprintf(dofile, "\nforce update_next_state 1 %d", time);
                        time =time+100; //25;
                }

                fprintf(dofile, "\nforce shifting_enable 1 %d\n", time);
                fprintf(dofile, "force update_next_state 1'bz  %d\n\n", time); //-25);
                fprintf(dofile, "force incre_config_in_symbol 1 %d\n", time); //-25);
                fprintf(dofile, "force incre_config_in_symbol 0 %d\n", time+100); // -25);

        } // End writing to Next_State_table

                fprintf(dofile, "force shifting_enable 0 %d\n", time+100);

                printf("Finish time to configure next_state without start reading from it = %d \n", time+125);


		do_time = time; 

	fclose(dofile); 

} 


