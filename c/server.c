/*
 * Sample implementation of SSE-Server according to criterias specified by 
 * https://github.com/rexxars/sse-broadcast-benchmark
 *
 * Written by Ole Fredrik Skudsvik <oles@vg.no>, Dec 2014
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/epoll.h>

/* Defaults. */
#define LISTEN_PORT 1942
#define NUM_THREADS 8
#define LOGLEVEL LOG_INFO

#define MAXEVENTS 600

#define HTTP_200 "HTTP/1.1 200 OK\r\n"
#define HTTP_204 "HTTP/1.1 204 No Content\r\n"
#define HTTP_404 "HTTP/1.1 404 Not Found\r\n"

#define RESPONSE_HEADER_STREAM      "Content-Type: text/event-stream\r\n" \
                                    "Cache-Control: no-cache\r\n" \
                                    "Access-Control-Allow-Origin: *\r\n" \
                                    "Connection: keep-alive\r\n\r\n"

#define RESPONSE_HEADER_PLAIN       "Content-Type: text/plain\r\n" \
                                    "Cache-Control: no-cache\r\n"  \
                                    "Access-Control-Allow-Origin: *\r\n" \
                                    "Connection: close\r\n\r\n"

#define RESPONSE_OPTIONS_CORS       "Access-Control-Allow-Origin: *\r\n" \
                                    "Connection: close\r\n\r\n"

typedef struct {
  pthread_t thread;
  int id;
  int epollfd;
  int numclients;
} ssethread_t;


typedef enum  {
  LOG_INFO,
  LOG_ERROR,
  LOG_DEBUG
} loglevel_t;


int efd;
struct epoll_event *events;
ssethread_t **threadpool;
pthread_t t_main;

void writelog(loglevel_t level, const char* fmt, ...) {
  va_list arglist;
  time_t ltime;
  struct tm result;
  char stime[32];

  if (level == LOG_DEBUG && LOGLEVEL != LOG_DEBUG) return;
  if (level == LOG_INFO && LOGLEVEL == LOG_ERROR)  return;

  ltime = time(NULL);
  localtime_r(&ltime, &result);
  asctime_r(&result, stime);
  stime[strlen(stime)-1] = '\0';

  va_start(arglist, fmt);
  printf("%s ", stime);
  vprintf(fmt, arglist);
  printf("\n");
  va_end(arglist);
}

int writesock(int fd, const char* fmt, ...) {
  char buf[8096];
  va_list arglist;

  va_start(arglist, fmt);
  vsnprintf(buf, 8096, fmt, arglist);
  va_end(arglist);

  return send(fd, buf, strlen(buf), 0);
}

/*
 * Simple parser to extract uri from GET requests.
 * */
int get_uri(char* str, char *dst, int bufsiz) {
  int i, len; 
  
  len = strlen(str);
  memset(dst, '\0', bufsiz);

  if (strncmp("GET ", str, 4) == 0) {
    for(i=4; i < len && str[i] != ' ' && i < bufsiz; i++) {
      *dst++ = str[i];
    }
    *(dst+1) = '\0';
    dst -= i;
    return 1;
  }

  return 0;
}

int getnumclients() {
  int i;
  int count = 0;

  for (i = 0; i < NUM_THREADS; i++) {
    count += threadpool[i]->numclients;
  }

  return count;
}

/*
 * SSE channel handler.
 */
void *sse_thread(void* _this) {
 int t_maxevents, t_id, n, i, ret;
 struct epoll_event *t_events, t_event;
 struct timeval tv;
 ssethread_t* this = (ssethread_t*)_this; 

 t_maxevents = sysconf(_SC_OPEN_MAX) / NUM_THREADS;
 t_events = calloc(t_maxevents, sizeof(t_event));
 this->numclients = 0;

 while(1) {
   n = epoll_wait(this->epollfd, t_events, t_maxevents, -1);
   gettimeofday(&tv, NULL);

   for (i = 0; i < n; i++) {
    if ((t_events[i].events & EPOLLHUP)) {
      writelog(LOG_DEBUG, "sse_thread #%i: Client disconnected.", this->id);
      close(t_events[i].data.fd);
      this->numclients--;
      continue;
    }

     /* Close socket if an error occours. */
    if ((t_events[i].events & EPOLLERR)) {
      writelog(LOG_ERROR, "sse_thread(): Error occoured on socket: %s (errno: %i).", strerror(errno), errno);
      close (t_events[i].data.fd);
      this->numclients--;
      continue;
    }

    ret = writesock(t_events[i].data.fd, "data: %ld\n\n", tv.tv_sec*1000LL + tv.tv_usec/1000);
   }

   sleep(1); 
 }

}

/* 
 * Main handler.
 * */
void *main_thread() {
  int i, n, len, ret, cur_thread;
  char buf[512], uri[64];
  struct epoll_event event;

  cur_thread = 0;
  
  while(1) {
    n = epoll_wait(efd, events, MAXEVENTS, -1);

    for (i = 0; i < n; i++) {
      /* Close socket if an error occours. */
      if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
        writelog(LOG_DEBUG, "main_thread(): Error occoured while reading data from socket."); 
        close (events[i].data.fd);
        continue;
      }

      memset(buf, '\0', 512);

      /* Read from client. */
      len = read(events[i].data.fd, &buf, 512);
    
      /* FIXME: Return only on /connections and /sse */ 
      if (strncmp(buf, "OPTIONS", 7) == 0) {
          writesock(events[i].data.fd, "%s%s", HTTP_204, RESPONSE_OPTIONS_CORS);
          continue;
      } 
  
      if (get_uri(buf, uri, 64)) {
        writelog(LOG_DEBUG, "GET %s.", uri);
      
        if (strncmp(uri, "/connections\0", 13) == 0) { // Handle /connections.
          writesock(events[i].data.fd, "%s%s%i\n", HTTP_200, RESPONSE_HEADER_PLAIN, getnumclients());
        } else if (strncmp(uri, "/sse\0", 4) == 0) { // Handle SSE channel.
          writesock(events[i].data.fd, "%s%s:ok\n\n", HTTP_200, RESPONSE_HEADER_STREAM, uri);

           /* Add client to the epoll eventlist. */
          event.data.fd = events[i].data.fd;
          event.events = EPOLLOUT;
   
          ret = epoll_ctl(threadpool[cur_thread]->epollfd, EPOLL_CTL_ADD, events[i].data.fd, &event);
          if (ret == -1) {
            writelog(LOG_DEBUG, "main_thread: Could not add client to epoll eventlist (curthread: %i): %s.", cur_thread, strerror(errno));
            close(events[i].data.fd);
            continue;
          }

          /* Remove fd from main efd set. */
          epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
          
          /* Increment numclients and cur_thread. */
          threadpool[cur_thread]->numclients++;
          cur_thread = (cur_thread == NUM_THREADS-1) ? 0 : (cur_thread+1);

          continue; /* We should not close the connection. */
        } else { // Return 404 on everything else.
          writesock(events[i].data.fd, "%s%sNo such channel.\r\n", HTTP_404, RESPONSE_HEADER_PLAIN);
        }
      } 

      close (events[i].data.fd);
    }
  }
}

void cleanup() {
  int i;
  for (i = 0; i<NUM_THREADS; free(threadpool[i]), close(threadpool[i]->epollfd));
  free(threadpool);
  free(events);
}

int main(int argc, char **argv[]) {
  int serversock, ret, on, thread_id, i;
  struct sockaddr_in sin;
  struct epoll_event event;

  on = 1;

  /* Ignore SIGPIPE. */
  signal(SIGPIPE, SIG_IGN); 
   

  /* Set up listening socket. */
  serversock = socket(AF_INET, SOCK_STREAM, 0); 
  if (serversock == -1) {
    perror("Creating socket");
    exit(1);
  }

  setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
  
  memset((char*)&sin, '\0', sizeof(sin));
  sin.sin_family  = AF_INET;
  sin.sin_port  = htons(LISTEN_PORT);


  ret = bind(serversock, (struct sockaddr*)&sin, sizeof(sin));
  if (ret == -1) {
    writelog(LOG_ERROR, "bind(): %s", strerror(errno));
    exit(1);
  }

  ret = listen(serversock, 0);
   if (ret == -1) {
    writelog(LOG_ERROR, "listen(): %s", strerror(errno));
    exit(1);
  }

  writelog(LOG_INFO, "Listening on port %i.", LISTEN_PORT);

  /* Initial client handler thread. */
  t_main = pthread_create(&t_main, NULL, main_thread, NULL);

  /* Set up SSE worker threads. */
  threadpool = (ssethread_t**)malloc(NUM_THREADS  * sizeof(ssethread_t*));
  for (i=0; i <= NUM_THREADS; threadpool[i++] = (ssethread_t*)malloc(sizeof(ssethread_t)));

  writelog(LOG_INFO, "Starting %i worker threads.", NUM_THREADS);
  for (i = 0; i < NUM_THREADS; i++) {
    threadpool[i]->id = i;
    threadpool[i]->epollfd = epoll_create1(0);

    if (threadpool[i]->epollfd == -1) {
      writelog(LOG_ERROR, "Failed to create epoll descriptor for thread #%i: %s.", i,  strerror(errno));
      cleanup();
      exit(1);
    }

    pthread_create(&threadpool[i]->thread, NULL, sse_thread, threadpool[i]);
   }
  
  /* Set up epoll stuff. */
  efd = epoll_create1(0);
  if (efd == -1) {
    writelog(LOG_ERROR, "Failed to create epoll descriptor: %s.", strerror(errno));
    exit(1);
  }

  events = calloc(MAXEVENTS, sizeof(event)); 

  /* Mainloop, clients will be accepted here. */
  while(1) {
    struct sockaddr_in csin;
    int clen, tmpfd, ret;
    char ip[32];

    memset((char*)&csin, '\0', sizeof(csin));
    clen = sizeof(csin);

    // Accept the connection.
    tmpfd = accept(serversock, (struct sockaddr*)&csin, &clen);
   
    /* Got an error ? Handle it. */
    if (tmpfd == -1) {
      switch (errno) {
        case EMFILE:
          writelog(LOG_ERROR, "All connections available used. Cannot accept more connections.");
          usleep(100000);
        break;

        default:
          writelog(LOG_ERROR, "Error in accept(): %s.", strerror(errno));
      }

      continue; /* Try again. */
    } 

    /* If we got this far we've accepted the connection */
    fcntl(tmpfd, F_SETFL, O_NONBLOCK); // Set non-blocking on the clientsocket.

    /* Add client to the epoll eventlist. */
    event.data.fd = tmpfd;
    event.events = EPOLLIN | EPOLLET;
   
    ret = epoll_ctl(efd, EPOLL_CTL_ADD, tmpfd, &event);
    if (ret == -1) {
      writelog(LOG_DEBUG, "Could not add client to epoll eventlist: %s.", strerror(errno));
      close(tmpfd);
      continue;
    }

    inet_ntop(AF_INET, &csin.sin_addr, &ip, 32);
    writelog(LOG_DEBUG, "Accepted new connection from %s.", ip);
 }

  cleanup();
  return(0);
}
