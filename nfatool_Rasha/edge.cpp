#include "nfatool.h"

edge::edge(int p,int s) : pred(p), succ(s) {
}

bool edge::operator< (const edge &in) const {
	int delta_pred = pred - in.pred,
	    delta_succ = succ - in.succ;

	if (delta_pred < 0) return true; else
	if (delta_pred == 0 && delta_succ < 0) return true; else
	return false;
}

