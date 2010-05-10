#include "common.h"
#include "entities.h"
#include "myerror.h"

char_t escc = '\\';
wchar_t Lescc = L'\\';
char_t xpath_magic[2] = ":"; 

char_t xpath_delims[2] = "/"; /* begin tag */
char_t xpath_att_names[2] = "@"; /* begin attribute name */
char_t xpath_att_values[2] = "="; /* begin attribute value */
char_t xpath_pis[2] = "?"; /* begin processing instruction */
char_t xpath_pred_starts[2] = "["; /* begin predicate */
char_t xpath_pred_ends[2] = "]"; /* end predicate */
char_t xpath_starts[2] = "["; /* begin XPATH path fragment */
char_t xpath_ends[2] = "]"; /* end XPATH path fragment */
char_t xpath_specials[9] = "[]=@/"; /* next element delimiter */
char_t xpath_ptags[4] = "/.."; /* used for normalization */
char_t xpath_ctags[3] = "/."; /* used for normalization */
char_t xpath_tag_delims[4] = "@[]"; /* delimiteter for a tag decoration */

/* assume ordering newspecials = "delim,att-sign" */
void redefine_xpath_specials(const char_t *newspecials) {
  if( newspecials ) {
    if( newspecials[0] ) {
      if( isascii(newspecials[0]) && !xml_namechar(newspecials[0]) ) {
	xpath_delims[0] = newspecials[0];
	xpath_specials[4] = newspecials[0];
	xpath_ptags[0] = newspecials[0];
	xpath_ctags[0] = newspecials[0];
      } else {
	errormsg(E_WARNING, 
		 "cannot replace XPATH separator '%c' with '%c'.\n",
		 xpath_delims[0], newspecials[0]);
      }
    } 

    if( newspecials[0] && newspecials[1] ) {
      if( isascii(newspecials[1]) && !xml_namechar(newspecials[1]) &&
	  (newspecials[1] != xpath_delims[0]) ) {
	xpath_att_names[0] = newspecials[1];
	xpath_specials[3] = newspecials[1];
      } else {
	errormsg(E_WARNING, 
		 "cannot replace XPATH attribute separator '%c' with '%c'.\n",
		 xpath_att_names[0], newspecials[1]);
      }
    } 
  }
}


