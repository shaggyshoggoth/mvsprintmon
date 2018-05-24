//////////////////////////////////////////////////////////////////////////////
// Organization: 
//       Author: Josh Thibodaux
//    Copyright: Josh Thibodaux
//
//      Project: 
//  Description: 
//
//      Created: Fri 27 Apr 2018 06:09:17 PM PDT
//    Revisions:
//      1.0 - File created
//
//////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "daemon.h"

int daemon(int nochdir, int noclose) {
  pid_t pid;

  pid = fork();

  if(pid < 0)
          return -1;

  if(pid > 0)
          exit(0);

  if(setsid() < 0)
          return -1;

  // Need to handle SIGCHLD and SIGHUP

  pid = fork();

  if(pid < 0)
          return -1;

  if(pid > 0)
          exit(0);

  umask(0);

  if(!nochdir)
          chdir("/");

  if(!noclose) {
    for(int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
      close(x);
    }
  }

  return 0;
}
