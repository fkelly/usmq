#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ablinklist.h"
#include <assert.h>

struct linkedlist *initialize(void)
{
    struct linkedlist *ll=(struct linkedlist *)malloc(sizeof(struct linkedlist));
    ll->head=NULL;
    ll->tail=NULL;
    ll->count=0;
    return ll;
}

void walk(struct linkedlist *ll, char *fmt, void (*printfn)(char *fmt, void *))
/* walks the linkedlist and prints out the contents of the nodes */
{

    if (!ll) { return; }

    struct node *ptr=ll->head;

    if (!ptr) { return; }

    while (1)
    {
        //printf("%s\n", ptr->data);
        (*printfn)(fmt, ptr->data);
        ptr=ptr->next;
        // if head and tail are the same, quit
        if (!ptr || ptr==ll->head) { break; }
    }
}

struct node *insertmiddle(struct linkedlist *ll, struct node *prior, const void *data)
{
    // create a new node
    if (!prior) {return NULL;}
    struct node *newnode;
    newnode=(struct node *)malloc(sizeof(struct node));
    //newnode->data=(char *)malloc((n+1)*sizeof(char));
    newnode->data=(void *)data;
    //memcpy ( newnode->data, data, n+1);
    newnode->next=NULL;
    // the new node needs to point to prior's next
    newnode->next=prior->next;
    // prior needs to point to that node
    prior->next=newnode;
    ll->count += 1;
    return newnode;
}

struct node *inserthead(struct linkedlist *ll, const void *data)
{
    // create a new node
    if (!ll) { return NULL; }
    struct node *newnode;
    newnode=(struct node *)malloc(sizeof(struct node));
    //newnode->data=(char *)malloc((n+1)*sizeof(char));
    newnode->data=(void *)data;
    //memcpy ( newnode->data, data, n+1);
    if (ll->head)
    {
        newnode->next=ll->head;
        ll->head=newnode;
    }
    else
    {
        /* if not head, then there must not be a tail either */
        ll->head=newnode;
        ll->tail=newnode;
        newnode->next=NULL;
    }
    ll->count += 1;
    sanitycheck(ll);
    return newnode;
}

struct node *inserttail(struct linkedlist *ll, const void *data)
{
    // create a new node
    if (!ll) { return NULL; }
    struct node *newnode;
    newnode=(struct node *)malloc(sizeof(struct node));
    //newnode->data=(char *)malloc((n+1)*sizeof(char));
    newnode->data=(void *)data;
    //memcpy ( newnode->data, data, n+1);
    if (ll->tail)
    {
        /* the current tail node needs to point to the newnode */
        ll->tail->next=newnode;
        /* and the linked list's tail needs to now point to newnode */
        ll->tail=newnode;
        newnode->next=NULL;
    }
    else
    {
        /* if no tail, then no head either */
        ll->tail=newnode;
        ll->head=newnode;
        newnode->next=NULL;
    }
    ll->count += 1;
    sanitycheck(ll);
    return newnode;
}

void append(struct linkedlist *ll, const void *data)
{

    inserttail(ll, data);
    sanitycheck(ll);
}

struct node *search(struct linkedlist *ll, void *compare_to, int (*compfn)(void *, void *))
{
   if (!ll) {return NULL;}
   struct node *ptr=ll->head; 
   if (!ptr) { return NULL;}
   while (ptr)
   {
       //if (!strcmp(ptr->data,data)) { return ptr; }
       if (!(*compfn)(compare_to, (void *)(ptr->data))) 
       { return ptr; }
       ptr=ptr->next;
   }
   return NULL;
}

struct node *find_and_pop(struct linkedlist *ll, void *compare_to, int (*compfn)(void *, void *))
/* searches for a node using the comparison function given by compfn and the search
   criteria given by compare_to. If the node is found, it removes it and returns its
   data element. */
{
   if (!ll) {return NULL;}
   struct node *to_pop=ll->head; 
   struct node *prior=NULL;
   if (!to_pop) { return NULL;}
   while (to_pop)
   {
       if (!(*compfn)(compare_to, (void *)(to_pop->data))) 
       { break; }
       prior=to_pop;
       to_pop=to_pop->next;
   }
   if (!to_pop) { return NULL; }
   // want to delete to_pop and return its data element
   // if !prior, then to_pop is the head element, so just use pop
   if (!prior) { return pop(ll); }
   /*
     [ head ] -> [ ... ] -> [ prior ] -> [ to_pop ] -> [ next ]
   */
   struct node *next=to_pop->next;
   prior->next=next;
   void *data=to_pop->data;
   if (to_pop==ll->tail) { ll->tail=prior; }
   free(to_pop);
   ll->count -= 1;
   sanitycheck(ll);
   return data;
}

int count(struct linkedlist *ll, void *compare_to, int (*compfn)(void *, void *))
{
   if (!ll) {return 0;}
   struct node *ptr=ll->head; 
   if (!ptr) { return 0;}
   int ct=0;
   while (ptr)
   {
       //if (!strcmp(ptr->data,data)) { ct++; }
       if (!(*compfn)(compare_to, (void *)(ptr->data))) { ct++; }
       ptr=ptr->next;
   }
   return ct;
}

struct node *getNth(struct linkedlist *ll, int nth)
{
   if (!ll) {return NULL;}
   struct node *ptr=ll->head; 
   if (!ptr) { return NULL;}
   int ct=1;
   while (ptr && ct < nth)
   {
       ptr=ptr->next;
       ct++;
   }
   return ptr;

}

void sanitycheck(struct linkedlist *ll)
{
    /* first check that the linkedlist is actually valid */
    assert(ll);
    /* next, either both head and tail are defined or neither */
    assert((ll->head && ll->tail) || ((!ll->head) && (!ll->tail)));
    /* tail's next should be null */
    if (ll->tail) {
        assert(!ll->tail->next);
     }
    /* definitely remove this in prod -- very expensive -- make sure no broken links */
    if (!ll->head) { return ;}
    struct node *ptr=ll->head;
    while (ptr)
    {
       if (!ptr->next)
       {    assert(ptr==ll->tail);  }
       ptr=ptr->next;
    }
    /* make sure counts make sense */
    if (ll->count) { 
        assert(ll->head && ll->tail); 
        if (ll->count>1) {assert(ll->head != ll->tail);}
    }
    else { assert((!ll->head) && (!ll->tail)); }
}

// TODO: fix this to deal with arbitrary data
void *pop(struct linkedlist *ll)
/* remove the head element from the linkedlist and returns
   the data contained within that element; it's up to the 
   caller to free the void * returned by this method.
 */
{
    struct node *head=ll->head;
    struct node *next;
    void *data;
    if (!head) { return NULL; }
    next=head->next; 
    data=head->data;
    if (!next)
    {
       // if this is true there should be only 1 node
       assert(ll->count==1);
       ll->head=NULL;
       ll->tail=NULL;
    }
    else
    {
        ll->head=next;
    }
    free(head);
    ll->count -= 1;
    sanitycheck(ll);
    return data;
}

void delete(struct linkedlist *ll, struct node *prior)
/* deletes the node after prior */
{
    if (!ll) { return; }
    struct node *to_delete, *next;
    if (prior) { to_delete=prior->next; }
    else {
         to_delete=ll->head; 
    }
    if (!to_delete) { return; }
    next=to_delete->next;
    if (prior) { prior->next=next; }
    else { ll->head=next; }
    if (to_delete->data) {
        //printf("freeing ptr %p.\n", to_delete->data);
        //printf("freeing %s.\n", to_delete->data);
        free(to_delete->data);
    }
    free(to_delete);
    ll->count -= 1;
    sanitycheck(ll);
}

void deleteall(struct linkedlist *ll)
{
   if (!ll) {return;}
   struct node *ptr=ll->head, *next; 
   if (!ptr) { return ;}
   while (ptr) 
   {
       next=ptr->next;
       if (ptr->data) { 
           //printf("(2) freeing %s.\n", ptr->data);
           free(ptr->data);
       }
       free(ptr);
       ptr=next;
   }
   ll->head=NULL;
   ll->tail=NULL;
   ll->count=0;
   sanitycheck(ll);
}
