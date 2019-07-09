#ifndef LIST_H
#define LIST_H

struct node {
	int val;
	struct node *next;
};

struct jlist {
	struct node *head;
	int the_size;

	jlist ();
	void push_back (int val);
	void remove_from_list (int val);
	int operator[](int index);
	int size();
	void clear();
};

#endif
