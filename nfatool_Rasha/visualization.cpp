 #include "nfatool.h"

void dump_dot_file (char *filename, int **edge_table, int num_states, int max_edges) {
	FILE *myFile;

    myFile = fopen(filename,"w+");
    if (!myFile) {
        perror("WARNING: dump_dot_file(): cannot open dot file\n");
        return;
    }


    fprintf (myFile,"digraph {\nrankdir=LR;\n");

    for (int i=0;i<num_states;i++) {
        for (int j=0;j<max_edges;j++) {
            if (edge_table[i][j]==-1) break;
            fprintf (myFile,"%d -> %d;\n",i,edge_table[i][j]);
        }
    }

	fprintf(myFile,"}\n");
	fclose(myFile);
}
