#ifndef EDGE_H
#define EDGE_H

class edge {
public:
	int pred;
	int succ;
	bool operator< (const edge &in) const;
	edge(int p,int s);
};

#endif
