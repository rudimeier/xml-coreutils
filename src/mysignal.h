/* 
 * Copyright (C) 2006 Laird Breyer
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

#ifndef MYSIGNAL_H
#define MYSIGNAL_H

#include "common.h"

/* NEVER CALL THIS FILE signal.h OR YOU'LL BE SORRY :( */
#include <sys/types.h>
#include <signal.h>

#ifdef sig_atomic_t
typedef int sig_atomic_t; /* too bad, but the show must go on... */
#endif

void init_signal_handling();
void process_pending_signal();
void exit_signal_handling();

#endif
