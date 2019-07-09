#include <stdio.h>
#include "nfatool" 

#define	TO_LITERAL(STATE,SE)	STATE*7+SE+1
#define F 7


void cnf-generator () {
	int i,j,k,l;
  
	int nfa_graph[7][7] = {{0, 1, 1, 1, 1, 0, 0},
						 {0, 0, 0, 0, 0, 1, 0},
						 {0, 0, 0, 0, 0, 1, 1},
						 {0, 0, 0, 0, 0, 0, 1},
						 {0, 0, 0, 0, 0, 0, 1},
						 {0, 0, 0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0, 0, 0}};
	
	// fan-out constraint
	for (i=0;i<7;i++) // for each predecessor
		for (j=0;j<7;j++) { // for each successor
			if (nfa_graph[i][j]) for (k=0;k<7;k++) { // for each possible placement of predecessor
				printf ("-%d ",TO_LITERAL(i,k));
				//for (l=0;l<7;l++) if (l!=i) printf ("%d ",TO_LITERAL(l,k));
				for (l=-(F-1)/2;l<=F/2;l++) if ((k+l)>=0 && (k+l)<7 && l)
					printf ("%d ",TO_LITERAL(j,k+l));
				printf ("0\n");
			}
		}
	
	// map every state constraint
	for (i=0;i<7;i++) {
		for (k=0;k<7;k++) printf ("%d ",TO_LITERAL(i,k));
		printf ("0\n");
	}
	
	// don't double map se constaint
	for (k=0;k<7;k++) // k is the state
		
	for (i=0;i<7;i++) { // i is the SE
		for (j=0;j<7;j++) if (k!=j) { // j is the state
			printf ("-%d -%d ",TO_LITERAL(k,i),TO_LITERAL(j,i));
			printf ("0\n");
		}
	}
			
}
