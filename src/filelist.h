/* 
 * Copyright (C) 2009 Laird Breyer
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

#ifndef FILELIST_H
#define FILELIST_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* this is a small object which processes the command line
   filenames. Use this after removing all the command line
   options and any other special parameters. Each string in argv 
   is assumed by filelist to have the theoretical form "s1:s2", where s1
   represents a physical file and s2 represents an XPATH. There
   can be more than one XPATH s2 for each file s1.

   The real (nontheoretical) form of the command line is assumed
   to be 

   a1 b1 ... c1 :d2 :e2 ... :f2 g1 h1 ... i1 :j2 :k2 ... (etc)

   This is a sequence of one or more file names, followed by 
   one or more XPATHs (identified by the initial ':' character)
   then some more file names followed by some more XPATHs etc. 

   The filelist object attaches to each file name all of the 
   XPATHs immediately following it. Thus the above is transformed
   into the theoretical forms

   a1:d2,e2,...,f2
   b1:d2,e2,...,f2
   c1:d2,e2,...,f2
   g1:j2,k2,...
   h1:j2,k2,...

   If the initial list of filenames a1 b1 ... c1 is omitted, then
   the single file "stdin" is assumed. 

   The above method of processing filenames and XPATHs may seem complicated,
   but it has several advantages:

   1) A single XPATH can apply to many files. This saves typing, and is
   particularly useful when the files are specified by shell wildcards.
   
   2) A single file can have many XPATHs. This is more readable than
   asking a user to build a complex XPATH with several branches.
   Besides, the | character (which is used to combine XPATHs in the
   XPath spec) is already used by the shell for piping. 

   3) The shell automatic completion works for filenames, whereas it
   would fail if the XPATH was concatenated with the filename as in
   "s1:s2". 

   4) Since most filenames do not start with ':', the shell won't try 
   to autocomplete XPATHs, and if the XPATH contains wildcards, the
   shell is unlikely to expand them.

*/
typedef struct {
  cstringlst_t files;
  cstringlst_t *xpaths;
  cstringlst_t garb;
  size_t num;
  flag_t flags;
} filelist_t;

#define FILELIST_MIN1            0x01 
#define FILELIST_NEEDSTDIN       0x02
#define FILELIST_XPATHS          0x04
#define FILELIST_MULTIPATHS      0x08
#define FILELIST_MIN2            0x10
#define FILELIST_EQ1             0x20


bool_t create_filelist(filelist_t *fl, int argc, char *argv[], flag_t flags);
bool_t free_filelist(filelist_t *fl);

cstringlst_t getfiles_filelist(filelist_t *fl);
cstringlst_t *getxpaths_filelist(filelist_t *fl);
size_t getsize_filelist(filelist_t *fl);
bool_t hasxpaths_filelist(filelist_t *fl);
bool_t hasmultipaths_filelist(filelist_t *fl);

#endif
