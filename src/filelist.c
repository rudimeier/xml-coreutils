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

#include "common.h"
#include "filelist.h"
#include "mem.h"
#include "myerror.h"

#include <string.h>

extern const char_t xpath_magic[];
extern const char_t xpath_delims[];

char *normalized_xpath(char *q) {
  return (!q ? NULL : (q + 1));
}

bool_t compute_argc(char *argv[], int *argc, int *xc, int *fc) {
  int i, j, k, m;
  bool_t s = FALSE;
  if( argv && argc && xc ) {
    m = (*argc < 0) ? MAXFILES : *argc;
    for(i = 0, j = 0, k = 0; argv[i] && (i < m); i++) {
      if( *argv[i] == *xpath_magic ) {
	j++;
	s = TRUE;
      } else {
	k++;
	if( s ) { j++; }
	s = FALSE;
      }
    }
    if( s ) { j++; }
    *argc = i;
    *xc = j;
    *fc = k;
    return TRUE;
  }
  return FALSE;
}

bool_t create_filelist(filelist_t *fl, int argc, char *argv[], flag_t flags) {
  int i, j, k, l, m, xc, fc;
  const char *q;
  size_t nf, np, npp;
  bool_t ok;
  compute_argc(argv, &argc, &xc, &fc);
  if( fl && ((argc == 0) || (argv != NULL)) ) {
    
    ok = TRUE;
    ok &= create_mem(&fl->files, &nf, sizeof(char_t *), fc + 1);
    ok &= create_mem(&fl->xpaths, &np, sizeof(cstringlst_t), fc + 1);
    ok &= create_mem(&fl->garb, &npp, sizeof(char_t *), xc + 2);
    if( ok ) {

      fl->files[0] = "stdin"; 
      fl->xpaths[0] = NULL;
      fl->garb[0] = xpath_delims;
      fl->garb[1] = NULL;
      fl->num = 1;
      fl->flags = flags;

      /* below only runs if argc > 0 */
      for(i = 0, j = 2, k = 0; i < argc; i++) {
	q = argv[i];

	if( q && (*q != *xpath_magic) ) { /* it's a FILE */

	  if( (fl->num > 1) && checkflag(fl->flags,FILELIST_EQ1) ) {
	    errormsg(E_WARNING,
	  	     "this command accepts a single file, ignoring remaining.\n");
	    break;
	  }

	  fl->files[fl->num] = q;
	  fl->xpaths[fl->num] = NULL;
	  fl->num++;

	} else if( *q == *xpath_magic ) { /* it's an XPATH */

	  if( i == 0 ) {
	    setflag(&fl->flags,FILELIST_NEEDSTDIN);
	  }

	  /* make a list of all paths in sequence */
	  for(l = j ; argv[i] && (*argv[i] == *xpath_magic); i++,j++) {
	    fl->garb[j] = &(argv[i][1]);
	  }
	  i--; /* one too many */

	  fl->garb[j] = NULL;
	  j++;
	  if( j > l + 2 ) {
	    setflag(&fl->flags,FILELIST_MULTIPATHS);
	  }

	  /* now attach this list to all available files */
	  for(m = k; m < fl->num; m++) {
	    if( !fl->xpaths[m] ) {
	      fl->xpaths[m] = &(fl->garb[l]);
	    }
	  }
	  k = fl->num; 

	  setflag(&fl->flags,FILELIST_XPATHS);
	}
      }

      if( j != (xc + 2) ) {
	errormsg(E_FATAL, "memory corruption.\n");
      }

      /* don't leave NULLs around */
      for(m = k; m < fl->num; m++) {
	if( !fl->xpaths[m] ) { 
	  fl->xpaths[m] = &(fl->garb[0]); 
	}
      }

      /* print_filelist(fl); */
      return TRUE;
    } else {
      errormsg(E_FATAL, "too many files");
    }
  }
  return FALSE;
}

bool_t free_filelist(filelist_t *fl) {
  if( fl ) {
    /* no need to delete actual strings */
    free_mem(&fl->files, &fl->num);
    free_mem(&fl->xpaths, &fl->num);
    free_mem(&fl->garb, &fl->num);
    fl->num = 0;
  }
  return FALSE;
}

/* void print_filelist(filelist_t *fl) { */
/*   int i,j; */
/*   for(i = 0; i < fl->num; i++) { */
/*     printf("%s ", fl->files[i]); */
/*     for(j = 0; fl->xpaths[i][j]; j++) { */
/*       printf(":%s ", fl->xpaths[i][j]); */
/*     } */
/*     printf("\n"); */
/*   } */
/* } */

cstringlst_t getfiles_filelist(filelist_t *fl) {
  bool_t keep0 = 
    ( checkflag(fl->flags,FILELIST_NEEDSTDIN) ||
      (checkflag(fl->flags,FILELIST_EQ1) && (fl->num == 1)) ||
      (checkflag(fl->flags,FILELIST_MIN1) && (fl->num == 1)) ||
      (checkflag(fl->flags,FILELIST_MIN2) && (fl->num == 2)) );
  return fl->files + (keep0 ? 0 : 1);
}

cstringlst_t *getxpaths_filelist(filelist_t *fl) {
  bool_t keep0 = 
    ( checkflag(fl->flags,FILELIST_NEEDSTDIN) ||
      (checkflag(fl->flags,FILELIST_EQ1) && (fl->num == 1)) ||
      (checkflag(fl->flags,FILELIST_MIN1) && (fl->num == 1)) ||
      (checkflag(fl->flags,FILELIST_MIN2) && (fl->num == 2)) );
  return fl->xpaths + (keep0 ? 0 : 1);
}

size_t getsize_filelist(filelist_t *fl) {
  bool_t keep0 = 
    ( checkflag(fl->flags,FILELIST_NEEDSTDIN) ||
      (checkflag(fl->flags,FILELIST_EQ1) && (fl->num == 1)) ||
      (checkflag(fl->flags,FILELIST_MIN1) && (fl->num == 1)) ||
      (checkflag(fl->flags,FILELIST_MIN2) && (fl->num == 2)) );
  return fl->num + (keep0 ? 0 : (-1));
}

bool_t hasxpaths_filelist(filelist_t *fl) {
  return checkflag(fl->flags,FILELIST_XPATHS);
}

bool_t hasmultipaths_filelist(filelist_t *fl) {
  return checkflag(fl->flags,FILELIST_MULTIPATHS);
}
