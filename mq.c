/*
Ultra Simple Message Queue 
Copyright (C) 2011  Francis Kelly (francis.kelly@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Messages have the following form:

put messages:

PUT
\r\n
<head>
\r\n
<bytes>
\r\n
<body>

get messages:

GET
\r\n

match messages:

MATCH
\r\n
<head>
\r\n

count messages:

COUNT
\r\n

*/
#include <event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include "ablinklist.h"
#include  <signal.h>

void  ctrlc_handler(int sig)
{
     char  c;

     signal(sig, SIG_IGN);
     printf("Do you really want to quit? [y/n] ");
     c = getchar();
     if (c == 'y' || c == 'Y')
          exit(0);
     else
          signal(SIGINT, ctrlc_handler);
}

#define RESPONSE_EMPTY "\0"
#define RESPONSE_INVALID_REQUEST "\1" 
#define SERVER_PORT 8788
int port=SERVER_PORT;
int verbose=0;


/* The mq_state structure is used to keep track of the
   message queue itself as well as the event_base structure,
   both of which need to be passed to the "accept" callback,
   which fires on a new incoming connection.  
*/
struct mq_state {
  struct linkedlist *messagelist;
  struct event_base *base;
};

/* The client_state structure is passed to the buffer read/write
   callbacks: it contains references to the client file descriptor,
   the bufferevent structure, and the message list itself.
*/
struct client_state {
  int fd;
  struct bufferevent *buf_ev;
  struct linkedlist *messagelist;
};

#define UNKNOWN_REQUEST -1
#define PUT_REQUEST 1
#define GET_REQUEST 2
#define COUNT_REQUEST 3
#define MATCH_REQUEST 4
#define PUT "PUT"
#define GET "GET"
#define MATCH "MATCH"
#define COUNT "COUNT"

struct request {
  int type;
  int length;
  char *head;
  char *body;
};

struct message {
  char *head;
  char *body;
};

typedef struct request request;
typedef struct message message;

/* The printfn_message function is used by the
   ablinklist library's "walk" function to print out
   a linkedlist.
*/
void printfn_message(char *fmt, void *data)
{
    message *msg = (message *)data;
    printf(fmt, (char *)(msg->head), (char *)(msg->body));
}

/* Prints out a linkedlist -- used only for very verbose
   debugging. 
*/
void show_messages(struct linkedlist *ll)
{
    char *fmt="%s / %s\n";
    return walk(ll, fmt, &printfn_message);
}
 


int parse_request(struct bufferevent *incoming, struct evbuffer *evreturn, request *req);
int parse_match(struct bufferevent *incoming, request *req);
int parse_put_message(struct bufferevent *incoming, request *req);
void debug_printf(const char *fmt, ...);

/* this comparison function is used when for comparing nodes
   consisting of a head (char *) and a body (char *) -- the
   ablinklist library takes a pointer to a function in
   it search method to find a given node.
 */
int compfn_head(void *compare_to, void *data)
{
    message *msg = (message *)data;
    return strcmp((char *)compare_to, (char *)(msg->head));
}    

int setnonblock(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

void debug_printf(const char *fmt, ...)
{
    if (verbose != 1) {return;}
    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
}

/* buffer_read_cb is the callback function that will be called
   if there is data to read in the bufferevent buffer */
void buffer_read_cb(struct bufferevent *incoming, void *arg)
{
  struct evbuffer *evreturn;
  evreturn = evbuffer_new();
  struct client_state *client= (struct client_state *)arg;
  struct linkedlist *messagelist;

  messagelist=client->messagelist;
  debug_printf("------ start -------------\n");
  debug_printf("Queue size (1): %d.\n", messagelist->count);

  request *req=(request *)malloc(sizeof(request));
  size_t buffer_len = evbuffer_get_length(incoming->input);
  debug_printf("Incoming buffer is of length %d.\n", buffer_len);
  int request_type=parse_request(incoming,evreturn,req);
  if (request_type==PUT_REQUEST)
  {
      message *msg= (message *)malloc(sizeof(message));
      msg->head=req->head;
      msg->body=req->body;
      free(req);
      append(messagelist, (void *)msg);
  }
  else if (request_type==COUNT_REQUEST) 
  {
      evbuffer_add_printf(evreturn,"%d", messagelist->count);
  }
  else if (request_type==GET_REQUEST) 
  {
      /*
         in the event of a simple GET request, the mq will return a
         message of the form "<head>\r\n<body>" -- nb that the head cannot contain \r or \n
      */
      message *msg= (message *)pop(messagelist);
      if (msg)
      {
          debug_printf("GET: Adding message (head: %s, body: %s) to evbuffer (len=%d)\n\n", msg->head, 
                   msg->body, strlen(msg->body));
          evbuffer_add_printf(evreturn,"%s\r\n%s\r\n", msg->head, msg->body);
          free(msg->head);
          free(msg->body);
          free(msg);
      }
      else
      {
          evbuffer_add(evreturn,RESPONSE_EMPTY,1);
      }
  }
  else if (request_type==MATCH_REQUEST) 
  {
      /* in the event of a MATCH request, the mq will search the linkedlist 
         and grab the first message who head matches the string value passed in,
         the mq will then return a message of the form "<head>\r\n<body>" -- 
         nb that the head cannot contain \r or \n
       */
      message *msg=(message *)find_and_pop(messagelist, (void *)(req->head), &compfn_head);
      if (msg)
      {
          debug_printf("MATCH: Adding message (head: %s, body: %s) to evbuffer (len=%d)\n\n", msg->head, 
                   msg->body, strlen(msg->body));
          evbuffer_add_printf(evreturn,"%s\r\n%s\r\n", msg->head, msg->body);
          free(msg->head);
          free(msg->body);
          free(msg);
      }
      else
      {
          evbuffer_add(evreturn,"\0",1);
      }

  }
  else
  {
      /* If the message type is invalid, return an empty response.  
         It might be better to return an error code ... the advantage of
         returning nothing is that if we have a badly formed request that
         is too long, we ignore the extra bytes correctly, but maybe that's
         being too indulgent.
      */
      evbuffer_add(evreturn,RESPONSE_INVALID_REQUEST,1);

  }
  debug_printf("AFTER OPERATION:\n");
  if (verbose==2) { show_messages(messagelist); }

  bufferevent_write_buffer(incoming,evreturn);
  debug_printf("Queue size (2): %d.\n", messagelist->count);
  evbuffer_free(evreturn);
  debug_printf("------ end -------------\n");
}

/* The drain_extra function is used to remove excessive bytes that
   might be included within a badly formed message. 
   NB: the handling of badly formed messages is still problematic.
*/
int drain_extra(struct bufferevent *incoming, char *msgtype)
{
      int extra_bytes=evbuffer_get_length(incoming->input);
      if (extra_bytes>0) { 
          debug_printf("Draining %d extra bytes from %s message.\n",extra_bytes, msgtype);
          evbuffer_drain(incoming->input,extra_bytes);
      }
      return extra_bytes;
}

/* The parse_request function is the core of the message queue program. It analyzes incoming
   requests and figures out what kind of request they are (PUT, GET, MATCH, COUNT). In the 
   case of a PUT or MATCH request, the requests themselves are processed. GETs and COUNTs only
   require actions on the message queue themselves and are handled by the function calling 
   parse_request.
*/
int parse_request(struct bufferevent *incoming, struct evbuffer *evreturn, request *req)
{
  // TODO: allow the message to be a maximum number of lines
  //       and a maximum number of characters
  size_t n_read_out;
  char *msgtype;
  int bytes_in=0;
  size_t buffer_len = evbuffer_get_length(incoming->input);
 
  req->type=UNKNOWN_REQUEST; 
  req->length=0;
  req->head=NULL;
  req->body=NULL;
  // the first line of the request should be the type
  msgtype= evbuffer_readln(incoming->input, &n_read_out, EVBUFFER_EOL_CRLF);
  debug_printf("msgtype: %s\n", msgtype);
 
  if (msgtype && !strcmp(msgtype,PUT)) 
  { 
      req->type=PUT_REQUEST; 
      /* the length of the message is given by the buffer length
         less the length of the PUT string, less 2 for the line feed after
         PUT, less 2 for the linefeed after the message data */
      parse_put_message(incoming, req);
      evbuffer_add_printf(evreturn,"%d", req->length);
  }
  else if (msgtype && !strcmp(msgtype,GET)) 
  { 
      /* drain the buffer here in case there's junk --
         if there is junk (extra_bytes>0), then the GET
         request is NOT valid
      */
      int extra_bytes=drain_extra(incoming,msgtype);
      if (!extra_bytes) { req->type=GET_REQUEST; }
  }
  else if (msgtype && !strcmp(msgtype,MATCH)) 
  { 
      parse_match(incoming, req);
      /* drain the buffer here in case there's junk --
         if there is junk (extra_bytes>0), then the MATCH 
         request is NOT valid
      */
      int extra_bytes=drain_extra(incoming,msgtype);
      if (extra_bytes) { req->type=UNKNOWN_REQUEST; }
  }
  else if (msgtype && !strcmp(msgtype,COUNT)) 
  { 
      int extra_bytes=drain_extra(incoming,msgtype);
      /* drain the buffer here in case there's junk --
         if there is junk (extra_bytes>0), then the COUNT 
         request is NOT valid
      */
      if (!extra_bytes) { req->type=COUNT_REQUEST; }
  }
  if (req->type==UNKNOWN_REQUEST)
  {
      bytes_in=evbuffer_drain(incoming->input,buffer_len-n_read_out);
  }
  if (msgtype) 
  { 
      free(msgtype); 
  }
  return req->type;
}


int parse_put_message(struct bufferevent *incoming, request *req)
/*

PUT messages have the form:

        PUT
        \r\n
        <head>
        \r\n
        <bytes>
        \r\n
        <body>
        \r\n

this function take a bufferevent that has had the PUT\r\n removed, parses
the remaining data, and fills out the request structure

Returns 0 on success, -1 on failure
*/
{
  size_t n_read_out;
  // if there's no head, that's an error, so quit
  char *head = evbuffer_readln(incoming->input, &n_read_out, EVBUFFER_EOL_CRLF);
  req->head=head;
  char *p;
  char *msglen;
  int errno = 0;
  // TODO: this needs to be made a lot more robust
  msglen= evbuffer_readln(incoming->input, &n_read_out, EVBUFFER_EOL_CRLF);
  debug_printf("msglen: %s\n", msglen);
  req->length= strtol(msglen, &p, 10);
  if (errno != 0 || *p != 0 || p == msglen)
          debug_printf("error! can't convert %s to integer.\n", msglen);
  char *msg=(char *)malloc(sizeof(char)*((req->length)+1)); // +1 for '\0'
  int bytes_in=evbuffer_remove(incoming->input,msg,req->length);
  msg[req->length]='\0';
  req->body=msg;
  // remove the trailing \r\n
  // TODO: figure out a way to read how many bytes are left in the buffer
  bytes_in=evbuffer_drain(incoming->input,2);
  // TODO: need some error handling in here
  return 1;
}

int parse_match(struct bufferevent *incoming, request *req)
/*

MATCH messages have the form:

    MATCH
    \r\n
    <head>
    \r\n

this function take a bufferevent that has had the MATCH\r\n removed, parses
the remaining data, and fills out the request structure

Returns 0 on success, -1 on failure
*/
{
  size_t n_read_out;
  // if there's no head, that's an error, so quit
  char *head = evbuffer_readln(incoming->input, &n_read_out, EVBUFFER_EOL_CRLF);
  req->head=head;
  req->body=NULL;
  req->length=0;
  req->type=MATCH_REQUEST;
  // TODO: need some error handling in here
  return 1;
}

void buffer_write_cb(struct bufferevent *bev,
                        void *arg)
{
    debug_printf("Calling buffer_write_cb: %p, %p.\n", bev, arg);
}

void buffer_err_cb(struct bufferevent *bev,
                        short what,
                        void *arg)
{
  struct client_state *client = (struct client_state *)arg;
  bufferevent_free(client->buf_ev);
  debug_printf("Closing file descriptor: %d.\n", client->fd);
  close(client->fd);
  free(client);
}

void accept_callback(int fd,
                     short ev,
                     void *arg)
/* accept_callback gets called when the application accepts
   a socket connection on file descriptor, fd 
   ev is the event that's generated the callback
   arg is the one argument you get to pass into the callback -
   in this case we're using arg to store an mq_state
   structure, which captures the linkedlist used to store
   messages and the event_base structure; the linkedlist
   is obviously needed to store and retrieve messages and
   the event_base is needed to instantiate a new bufferevent
*/
{
  int client_fd;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  struct client_state *client;
  struct bufferevent *bev;
  struct mq_state *mqs;

  struct event_base *base;

  struct linkedlist *messagelist;

  mqs=(struct mq_state *)arg;
  base = mqs->base;
  messagelist=mqs->messagelist;

  client_fd = accept(fd,
                     (struct sockaddr *)&client_addr,
                     &client_len);
  if (client_fd < 0)
    {
      warn("Client: accept() failed");
      return;
    }

  setnonblock(client_fd);

  client = calloc(1, sizeof(*client));
  if (client == NULL)
    err(1, "malloc failed");
  client->fd = client_fd;
  debug_printf("Setting client->request to unknown.\n");
  debug_printf("Opening file descriptor: %d.\n", client->fd);
  client->messagelist=messagelist;

  /* create the bufferevent */
  bev = bufferevent_socket_new(base, client_fd, BEV_OPT_CLOSE_ON_FREE);

  /* we're going to be passing the client object into the callbacks and
     we want a way to have access to the bufferevent object, so we stick
     a reference to it in the client structure
   */
  client->buf_ev=bev;
  /* set-up the callbacks for the bufferevent */
  bufferevent_setcb(bev, buffer_read_cb, buffer_write_cb, buffer_err_cb, client);
  /* flip the switch and turn the bufferevent on */
  bufferevent_enable(bev, EV_READ|EV_WRITE);
}

int main(int argc,
         char **argv)
{
  int socketlisten;
  struct sockaddr_in addrlisten;
  struct event *accept_event;
  struct event_base *accept_base;
  struct mq_state *mqs;
  struct linkedlist *messagelist;
  int reuse = 1;

  int i;

  for (i = 1; i < argc; i++)  /* Skip argv[0] (program name). */
  {
        if (strcmp(argv[i], "-v") == 0)  
        {
            // set the *global* verbose argument to 1
            verbose = 1; 
        }
        if (strcmp(argv[i], "-vv") == 0)  
        {
            verbose = 2;  
        }
        if (strcmp(argv[i], "-p") == 0)  
        {
            i++;
            port = atoi(argv[i]);
        }
  }
  signal(SIGINT, ctrlc_handler);

  messagelist=(struct linkedlist *)malloc(sizeof(struct linkedlist));
  mqs=(struct mq_state *)malloc(sizeof(struct mq_state));
  accept_base=event_base_new();

  mqs->base=accept_base;
  mqs->messagelist=messagelist;

  socketlisten = socket(AF_INET, SOCK_STREAM, 0);

  if (socketlisten < 0)
    {
      fprintf(stderr,"Failed to create listen socket");
      return 1;
    }

  memset(&addrlisten, 0, sizeof(addrlisten));

  addrlisten.sin_family = AF_INET;
  addrlisten.sin_addr.s_addr = INADDR_ANY;
  addrlisten.sin_port = htons(port);

  if (bind(socketlisten,
           (struct sockaddr *)&addrlisten,
           sizeof(addrlisten)) < 0)
    {
      fprintf(stderr,"Failed to bind");
      return 1;
    }

  if (listen(socketlisten, 5) < 0)
    {
      fprintf(stderr,"Failed to listen on socket");
      return 1;
    }

  setsockopt(socketlisten,
             SOL_SOCKET,
             SO_REUSEADDR,
             &reuse,
             sizeof(reuse));

  setnonblock(socketlisten);


  accept_event=event_new(accept_base,
            socketlisten,
            EV_READ|EV_PERSIST,
            accept_callback,
            mqs);

  event_add(accept_event, NULL);

  event_base_dispatch(accept_base);

  close(socketlisten);

  return 0;
}

