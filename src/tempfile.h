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

#ifndef TEMP_H
#define TEMP_H

#include "common.h"

void init_tempfile_handling();
void exit_tempfile_handling();

char *make_template_tempfile(const char *basename);
int open_tempfile(char *template);
int remove_tempfile(const char *template);
int save_stdin_tempfile(char **tempfile, pid_t *child_pid, const char *progname);

#endif
