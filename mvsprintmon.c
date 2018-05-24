//////////////////////////////////////////////////////////////////////////////
// Organization: 
//       Author: Josh Thibodaux
//    Copyright: Josh Thibodaux
//
//      Project: 
//  Description: 
//
//      Created: Thu 26 Apr 2018 08:44:49 PM PDT
//    Revisions:
//      1.0 - File created
//
//////////////////////////////////////////////////////////////////////////////
#include <sys/inotify.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <libgen.h>
#include <limits.h>
#include <syslog.h>
#include "daemon.h"

#define PID_DIR "/tmp"
#define PMIDENT "mvsprintmon"

char daemonize = 1;
char verbose = 0;

#define DPRT(...) { if(verbose) { syslog(LOG_DEBUG, __VA_ARGS__); } }

void usage(char *argv[]) {
  fprintf(stderr, "Usage: %s [flags] <pdf viewer> [directories to watch]\n", basename(argv[0]));
  fprintf(stderr, "flags:\n");
  fprintf(stderr, "-d  -- run in foreground\n");
  fprintf(stderr, "-v  -- report messages on stdout\n");
}

int main(int argc, char *argv[]) {
  // Check command line
  int opt;
  while((opt = getopt(argc, argv, "dv")) != -1) {
    switch(opt) {
    case 'd':
      daemonize = 0;
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      usage(argv);
      exit(1);
      break;
    }
  }

  if(verbose) {
    openlog(PMIDENT, LOG_PERROR, LOG_USER);
  } else {
    openlog(PMIDENT, 0, LOG_USER);
  }


  if((argc - optind) < 2) {
    usage(argv);
    exit(1);
  }

  if(daemonize) {
    // Daemonize
    int dcheck = daemon(1, 0);
    if(dcheck < 0) {
      fprintf(stderr, "daemon failed: %s\n", strerror(errno));
      exit(1);
    }
  }

  // Check for running instance
  DPRT("check for running instance\n");
  int pidfd;
  char buffer[1024];
  snprintf(buffer, 1024, "%s/%s.pid", PID_DIR, basename(argv[0]));
  DPRT("pid file %s\n", buffer);
  if((pidfd = open(buffer, O_RDONLY)) < 0) {
    DPRT("pidfile does not exist\n");
    pid_t opid = getpid();
    pidfd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(pidfd < 0) {
      fprintf(stderr, "could not open pid file: %s\n", strerror(errno));
    }
    write(pidfd, &opid, sizeof(opid));
    close(pidfd);
  } else {
    DPRT("reading existing pid file\n");
    pid_t opid;
    read(pidfd, &opid, sizeof(opid));
    if(kill(opid, 0) < 0) {
      if(errno == ESRCH) {
        DPRT("process does not exist, overwriting pid file\n");
        close(pidfd);
        opid = getpid();
        pidfd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC);
        write(pidfd, &opid, sizeof(opid));
        close(pidfd);
      } else {
        fprintf(stderr, "kill failed: %s\n", strerror(errno));
        exit(1);
      }
    } else {
      printf("already running\n");
      exit(0);
    }
  }

  // Start inotify
  DPRT("starting inotify\n");
  int ifd = inotify_init();
  if(ifd < 0) {
    fprintf(stderr, "inotify_init failed: %s\n", strerror(errno));
    exit(1);
  }

  // Setup directories to watch
  DPRT("setting up watches\n");
  int *wds = (int *)malloc(sizeof(int)*(argc - 2));
  if(wds == NULL) {
    fprintf(stderr, "malloc failed %s\n", strerror(errno));
    exit(1);
  }
  for(int i = optind + 1; i < argc; i++) {
    DPRT("watching %s\n", argv[i]);
    wds[i - optind - 1] = inotify_add_watch(ifd, argv[i], IN_CREATE);
    if(wds[i - optind - 1] < 0) {
      fprintf(stderr, "inotify_add_watch for (%s) failed: %s\n", argv[i], strerror(errno));
      exit(1);
    }
  }

  // Start watching said directories
  char ibuffer[sizeof(struct inotify_event) + NAME_MAX + 1];
  while(1) {
    DPRT("waiting for event\n");
    struct inotify_event *event = (struct inotify_event *)ibuffer;
    int ret = read(ifd, ibuffer, sizeof(ibuffer));
    if(ret < 0) {
      fprintf(stderr, "inotify read failed: %s\n", strerror(errno));
    }
    DPRT("event %d = %s\n", event->wd, event->name);
    char *ext = strrchr(event->name, '.');
    DPRT("event occurred on %s\n", event->name);
    if(strcasecmp(ext, ".pdf") == 0) {
      DPRT("and is a pdf\n");
      // New pdf - open with xdg-open
      if(fork()) {
        continue;
      } else {
        int i;
        for(i = 0; i < (argc - optind + 1); i++) {
          if(wds[i] == event->wd)
                  break;
        }
        snprintf(buffer, 1024, "%s %s/%s", argv[i + optind], argv[i + optind + 1], event->name);
        DPRT("executing: %s", buffer);
        system(buffer);
        exit(0);
      }
    }
  }
}
