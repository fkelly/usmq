struct node 
{
   struct node *next;
   void *data;
};

struct linkedlist
{
    struct node *head;
    struct node *tail;
    int count;
};

void append(struct linkedlist *ll, const void *data);

// TODO: add a destructor function for the node
void delete(struct linkedlist *ll, struct node *prior);
// TODO: add a destructor function for the node
void deleteall(struct linkedlist *ll);
struct node *insertmiddle(struct linkedlist *ll,struct node *prior, const void *data);
struct node *inserthead(struct linkedlist *ll,const void *data);
struct node *inserttail(struct linkedlist *ll,const void *data);
void walk(struct linkedlist *ll, char *fmt, void (*printfn)(char *fmt, void *));
struct node *search(struct linkedlist *ll, void *compare_to, int (*compfn)(void *, void *));
int count(struct linkedlist *ll, void *compare_to, int (*compfn)(void *, void *));
struct node *getNth(struct linkedlist *ll, int nth);
void *pop(struct linkedlist *ll); 
struct node *find_and_pop(struct linkedlist *ll, void *compare_to, int (*compfn)(void *, void *));
struct linkedlist *initialize(void);
void sanitycheck(struct linkedlist *ll);
