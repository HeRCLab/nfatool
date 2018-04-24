#include "nfatool.h"

int jlist::operator[] (int index) {
	struct node *anode=this->head;
	int i=0;

	while (anode && i++<index) anode=anode->next;

	return anode ? anode->val : 1<<31;
}

void jlist::clear() {
	struct node *anode = this->head;
	struct node *prevnode = 0;

	while (anode) {
		prevnode=anode;
		anode=anode->next;
		free(prevnode);
	}

	this->head = 0;
	this->the_size=0;
}

int jlist::size() {
	return this->the_size;
}

jlist::jlist () {
	this->head = 0;
	this->the_size = 0;
}

void jlist::push_back (int val) {
	struct node *anode = this->head;

	if (!anode) {
		anode = this->head = (struct node *)malloc(sizeof(struct node));
		anode->next = 0;
		anode->val = val;
	} else {
		while (anode->next) anode=anode->next;
		anode = anode->next = (struct node *)malloc(sizeof(struct node));
		anode->next = 0;
		anode->val = val;
	}

	this->the_size++;
}

void jlist::remove_from_list (int val) {
	struct node *anode = this->head;
	struct node *prevnode=0; 

	if (!anode) {
		return;
	} else {
		while (anode && anode->val!=val) {
			prevnode = anode;
			anode=anode->next;
		}

		if (anode) {
			if (prevnode) {
				prevnode->next = anode->next;
			} else {
				this->head=anode->next;
			}

			free(anode);
		}
	}

	this->the_size--;
}
