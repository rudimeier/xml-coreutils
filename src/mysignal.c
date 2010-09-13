/* 
 * Copyright (C) 2010 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 * 
 * Author:   Laird Breyer <laird@lbreyer.com>
 */

#include "mysignal.h"
#include "myerror.h"

#include <string.h>
#include <sys/wait.h>

typedef struct {
  const char *tempfile;
} signal_cleanup_t;

flag_t cmd = 0;

int sa_signal = 0;
signal_cleanup_t cleanup = { NULL };

#if defined HAVE_SIGACTION
/* sigaction structure used by several functions */
struct sigaction act;
#endif

void my_sa_handler(int signum) {
#if defined HAVE_SIGACTION
  sa_signal = signum;
#endif
}

void sigsegv(int signum) {
   errormsg(E_FATAL, "segmentation fault.\n");
}

/* intercepts typical termination signals and tries to do the right thing */
void init_signal_handling(flag_t flags) {
#if defined HAVE_SIGACTION

  memset(&act, 0, sizeof(struct sigaction));

  /* set up global sigaction structure */

  act.sa_handler = my_sa_handler;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask,SIGHUP);
  sigaddset(&act.sa_mask,SIGINT);
  sigaddset(&act.sa_mask,SIGQUIT);
  sigaddset(&act.sa_mask,SIGTERM);
  sigaddset(&act.sa_mask,SIGPIPE);
  sigaddset(&act.sa_mask,SIGALRM);
  if( !checkflag(flags,SIGNALS_NOCHLD) ) {
    sigaddset(&act.sa_mask,SIGCHLD);
  }
  act.sa_flags = 0;

  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGPIPE, &act, NULL);
  sigaction(SIGALRM, &act, NULL);
  if( !checkflag(flags,SIGNALS_NOCHLD) ) {
    sigaction(SIGCHLD, &act, NULL);
  }

  act.sa_handler = sigsegv;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask,SIGSEGV);
  act.sa_flags = 0;
  sigaction(SIGSEGV, &act, NULL);

#endif
}

void exit_signal_handling() {
  /* nothing - this is just to mess with your head ;-) */
}

/* this is always called in the normal thread */
void process_pending_signal() {

#if defined HAVE_SIGACTION
  
  if( sa_signal ) {

    sigprocmask(SIG_BLOCK, &act.sa_mask, NULL);
    switch(sa_signal) {
    case SIGINT:
      errormsg(E_SIGNAL, "caught interrupt request, exiting.\n");
      exit(EXIT_FAILURE);
      break;
    case SIGPIPE:
      /* should we terminate, or should we ignore? */
      errormsg(E_SIGNAL, "broken pipe on output, exiting.\n");
      exit(EXIT_FAILURE);
      break;
    case SIGHUP:
    case SIGQUIT:
    case SIGTERM:
      setflag(&cmd,CMD_QUIT);
      errormsg(E_SIGNAL, 
	       "caught termination request, ignoring further input.\n");
      break;
    case SIGCHLD:
      waitpid(-1, NULL, WNOHANG);
      setflag(&cmd,CMD_CHLD);
      break;
    case SIGALRM:
      setflag(&cmd,CMD_ALRM);
      break;
    default:
      /* nothing */
      break;
    }
    sa_signal = 0;

    sigprocmask(SIG_UNBLOCK, &act.sa_mask, NULL);

  }

#endif
}
