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

#include "common.h"
#include "error.h"
#include "parser.h"
#include <string.h>

extern char *inputfile;

bool_t create_parser(parser_t *parser, void *ud) {
  if( parser ) {
    parser->p = XML_ParserCreate(NULL);
    if( !parser->p ) {
      errormsg(E_FATAL, "no available memory for parser\n");
      parser->cur.rstatus = XML_STATUS_ERROR;
      return FALSE;
    }
    memset(&parser->callbacks, 0, sizeof(callback_t));
    XML_SetUserData(parser->p, parser); /* no need to free this later */
    parser->user = ud; /* user data, we don't free this either */
    parser->cur.rstatus = XML_STATUS_OK;
    return TRUE;
  }
  return FALSE;
}

bool_t reset_parser(parser_t *parser) {
  if( parser && parser->p ) {
    parser->cur.rstatus = XML_ParserReset(parser->p, NULL);
    XML_SetUserData(parser->p, parser); /* no need to free this later */
    reset_handlers_parser(parser);
    return (bool_t)(parser->cur.rstatus == XML_STATUS_OK);
  }
  return FALSE;
}

bool_t stop_parser(parser_t *parser) {
  if( parser ) {
    parser->cur.rstatus = XML_StopParser(parser->p, XML_TRUE);
    return (bool_t)(parser->cur.rstatus == XML_STATUS_OK);
  }
  return FALSE;
}

bool_t restart_parser(parser_t *parser) {
  if( parser ) {
    parser->cur.rstatus = XML_ResumeParser(parser->p);
    return (bool_t)(parser->cur.rstatus == XML_STATUS_OK);
  }
  return FALSE;
}

bool_t free_parser(parser_t *parser) {
  if( parser && parser->p ) {
    XML_ParserFree(parser->p);
    parser->p = NULL;
    if( parser->user ) { 
      parser->user = NULL;
    }
    return TRUE;
  }
  return FALSE;
}

void exec_result(result_t r, parser_t *parser) {
  if( checkflag(r,PARSER_STOP) ) {
    stop_parser(parser);
  } else if( checkflag(r,PARSER_DEFAULT) && parser->callbacks.dfault) {
    XML_DefaultCurrent(parser->p);
  }
}

void XMLCALL xml_startelementhandler(void *userdata, const XML_Char *name, const XML_Char **atts) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.start_tag ) {
    exec_result( parser->callbacks.start_tag(parser->user, name, atts), parser );
  }
}

void XMLCALL xml_endelementhandler(void *userdata, const XML_Char *name) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.end_tag ) {
    exec_result( parser->callbacks.end_tag(parser->user, name), parser );
  }
}

void XMLCALL xml_characterdatahandler(void *userdata, const XML_Char *s, int len) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.chardata ) {
    exec_result( parser->callbacks.chardata(parser->user, s, (size_t)len), parser );
  }
}

void XMLCALL xml_processinginstructionhandler(void *userdata, const XML_Char *target, const XML_Char *data) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.pidata ) {
    exec_result( parser->callbacks.pidata(parser->user, target, data), parser );
  }
}

void XMLCALL xml_commenthandler(void *userdata, const XML_Char *data) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.comment ) {
    exec_result( parser->callbacks.comment(parser->user, data), parser );
  }
}

void XMLCALL xml_startcdatahandler(void *userdata) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.start_cdata ) {
    exec_result( parser->callbacks.start_cdata(parser->user), parser );
  }
}

void XMLCALL xml_endcdatahandler(void *userdata) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.end_cdata ) {
    exec_result( parser->callbacks.end_cdata(parser->user), parser );
  }
}

void XMLCALL xml_defaulthandler(void *userdata, const XML_Char *s, int len) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.dfault ) {
    exec_result( parser->callbacks.dfault(parser->user, s, (size_t)len), parser );
  }
}

bool_t reset_handlers_parser(parser_t *parser) {
  if( parser ) {
    XML_SetStartElementHandler(parser->p, parser->callbacks.start_tag ?
			       xml_startelementhandler : NULL);
    XML_SetEndElementHandler(parser->p, parser->callbacks.end_tag ?
			     xml_endelementhandler : NULL);
    XML_SetCommentHandler(parser->p, parser->callbacks.comment ?
			  xml_commenthandler : NULL);
    XML_SetCharacterDataHandler(parser->p, parser->callbacks.chardata ?
				xml_characterdatahandler : NULL);
    XML_SetProcessingInstructionHandler(parser->p, parser->callbacks.pidata ?
					xml_processinginstructionhandler : NULL);
    XML_SetStartCdataSectionHandler(parser->p, parser->callbacks.start_cdata ?
			  xml_startcdatahandler : NULL);
    XML_SetEndCdataSectionHandler(parser->p, parser->callbacks.end_cdata ?
			  xml_endcdatahandler : NULL);
    XML_SetDefaultHandler(parser->p, parser->callbacks.dfault ?
			  xml_defaulthandler : NULL);
    return TRUE;
  }
  return FALSE;
}

bool_t setup_parser(parser_t *parser, callback_t *callbacks) {
  if( parser && callbacks ) {
    parser->callbacks.start_tag = callbacks->start_tag;
    parser->callbacks.end_tag = callbacks->end_tag;
    parser->callbacks.chardata = callbacks->chardata;
    parser->callbacks.pidata = callbacks->pidata;
    parser->callbacks.comment = callbacks->comment;
    parser->callbacks.start_cdata = callbacks->start_cdata;
    parser->callbacks.end_cdata = callbacks->end_cdata;
    parser->callbacks.dfault = callbacks->dfault;
    return reset_handlers_parser(parser);
  }
  return FALSE;
}

enum XML_Status get_pstatus(parser_t *parser) {
  XML_ParsingStatus status;
  XML_GetParsingStatus(parser->p, &status);
  return status.parsing;
}

bool_t ok_parser(parser_t *parser) {
  return parser && (parser->cur.rstatus == XML_STATUS_OK);
}

bool_t suspended_parser(parser_t *parser) {
  return parser && (parser->cur.pstatus == XML_SUSPENDED);
}

bool_t busy_parser(parser_t *parser) {
  return parser && (parser->cur.pstatus == XML_PARSING);
}

bool_t audit_parser(parser_t *parser) {
  if( parser ) {
    parser->cur.pstatus = get_pstatus(parser);
    parser->cur.lineno = XML_GetCurrentLineNumber(parser->p);
    parser->cur.colno = XML_GetCurrentColumnNumber(parser->p);
    parser->cur.length = XML_GetCurrentByteCount(parser->p);
    parser->cur.byteno = XML_GetCurrentByteIndex(parser->p);
    return (bool_t)(parser->cur.rstatus == XML_STATUS_OK);
  }
  return FALSE;
}

bool_t do_parser(parser_t *parser, size_t nbytes) {
  int n, fin;
  if( parser ) {
    n = (nbytes < 0) ? 0 : nbytes;
    fin = (nbytes < 0);
    parser->cur.rstatus = XML_ParseBuffer(parser->p, n, fin);
    return audit_parser(parser);
  }
  return FALSE;
}

bool_t do_parser2(parser_t *parser, const byte_t *buf, size_t nbytes) {
  if( parser && buf && (nbytes >= 0)) {
    parser->cur.rstatus = XML_Parse(parser->p, (char *)buf, (int)nbytes, 0);
    return audit_parser(parser);
  }
  return FALSE;
}

const char_t *error_message_parser(parser_t *parser) {
  if( parser && (parser->cur.rstatus == XML_STATUS_ERROR) ) {
    return XML_ErrorString(XML_GetErrorCode(parser->p));
  }
  return "";
}


byte_t *getbuf_parser(parser_t *parser, size_t n) {
  byte_t *buf;
  if( parser ) {
    /* parser will free this automatically */
    buf = (byte_t *)XML_GetBuffer(parser->p, n);
    if( !buf ) {
      errormsg(E_ERROR, "can't allocate read buffer (%d bytes)\n", n);
      return (byte_t *)NULL;
    }
    return buf;
  }
  return (byte_t *)NULL;
}

void freebuf_parser(parser_t *parser, byte_t *buf) {
  if( parser ) {
    XML_MemFree(parser->p, buf);
  }
}
