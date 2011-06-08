#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ablinklist.h"
#include <assert.h>



/* this is a basic comparison function used when
   the linked list nodes simply consist of char* data */
int compfn_basic(void *compare_to, void *data)
{
    return strcmp((char *)compare_to, (char *)data);
}

void printfn_basic(char *fmt, void *data)
{
    printf(fmt, (char *)data);
}

int test_basic(void)
{
    struct linkedlist *ll=initialize();
    struct node *newnode=NULL,*snode=NULL;
    char *head_data;
    newnode=inserthead(ll,"1");
    newnode=inserthead(ll,"2");
    append(ll,"999");

    char *compare_to="2\0"; 
    snode=search(ll, (void *)compare_to, &compfn_basic);
    assert( !strcmp(snode->data,compare_to) );
    printf("search node data: %s\n",(char *)(snode->data));
    //delete(ll,NULL);
    printf("walk: ");
    walk(ll,"&s\n",&printfn_basic);
    int cont=1;
    int pops=0;
    do {
        head_data=pop(ll);
        pops++;
        printf("pop data: %s\n",(char *)head_data);
        if (!head_data) { cont=0;}
        //free(head_data);
    } while (cont);
    // we should call pop 4 times
    assert(pops==4);
    return 0;
}

struct nodedata
{
    char *head;
    char *body;
};

/* this comparison function is used when for comparing nodes
   consisting of a head (char *) and a body (char *)
 */
int compfn_head(void *compare_to, void *data)
{
    struct nodedata *nd = (struct nodedata *)data;
    return strcmp((char *)compare_to, (char *)(nd->head));
}


struct nodedata *init_nodedata(char *head, char *body)
{

    struct nodedata *nd;
    nd=(struct nodedata *)malloc(sizeof(struct nodedata));
    nd->head=head;
    nd->body=body;
    return nd;
}

void printfn_head(char *fmt, void *data)
{
    struct nodedata *nd = (struct nodedata *)data;
    printf(fmt, (char *)(nd->head));
}

int test_head(void)
{
    struct linkedlist *ll=initialize();
    struct node *newnode=NULL,*snode=NULL;
    newnode=inserthead(ll,init_nodedata("1","foo1"));
    newnode=inserthead(ll,init_nodedata("2","foo2"));
    append(ll,init_nodedata("999","bar999"));

    char *compare_to="2\0"; 
    snode=search(ll, (void *)compare_to, &compfn_head);
    char *headvalue=((struct nodedata *)(snode->data))->head;
    assert( !strcmp(headvalue,compare_to) );
    printf("search node data: %s\n",(char *)(headvalue));
    //delete(ll,NULL);
    printf("walk: ");
    walk(ll,"head: %s\n", &printfn_head);
    int cont=1;
    int pops=0;
    char *data;
    do {
        data=pop(ll);
        pops++;
        if (!data) { 
            headvalue=NULL;
            cont=0;}
        else {headvalue=((struct nodedata *)(data))->head;}
        printf("pop data: %s\n",headvalue);
        free(data);
    } while (cont);
    // we should call pop 4 times
    assert(pops==4);
    return 0;
}

int test_find_and_pop(void)
{
    struct linkedlist *ll=initialize();
    struct node *newnode=NULL,*snode=NULL;
    newnode=inserthead(ll,init_nodedata("1","foo1"));
    newnode=inserthead(ll,init_nodedata("2","foo2"));
    append(ll,init_nodedata("999","bar999"));
    assert( ll->count==3 );

    char *compare_to="2\0"; 
    struct nodedata *data=(struct nodedata *)find_and_pop(ll, (void *)compare_to, &compfn_head);
    assert (data);
    char *headvalue=data->head;
    assert( !strcmp(headvalue,compare_to) );
    assert( ll->count==2 );

    compare_to="1\0"; 
    data=(struct nodedata *)find_and_pop(ll, (void *)compare_to, &compfn_head);
    assert (data);
    headvalue=data->head;
    assert( !strcmp(headvalue,compare_to) );
    assert( ll->count==1 );

    compare_to="999\0"; 
    data=(struct nodedata *)find_and_pop(ll, (void *)compare_to, &compfn_head);
    assert (data);
    headvalue=data->head;
    assert( !strcmp(headvalue,compare_to) );
    assert( ll->count==0 );
}

int main(void)
{
   //test_basic();
   //test_head();
   test_find_and_pop();
   return 0;
}
