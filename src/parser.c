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
#include "mysignal.h"
#include "myerror.h"
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

bool_t stop_parser(parser_t *parser, bool_t abort) {
  if( parser ) {
    XML_Bool how = abort ? XML_FALSE : XML_TRUE;
    parser->cur.rstatus = XML_StopParser(parser->p, how);
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
  if( checkflag(r,PARSER_ABORT) ) {
    stop_parser(parser, TRUE);
  } else if( checkflag(r,PARSER_STOP) ) {
    stop_parser(parser, FALSE);
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

void XMLCALL xml_startdoctypedeclhandler(void *userdata,
					 const XML_Char *doctypeName,
					 const XML_Char *sysid,
					 const XML_Char *pubid,
					 int has_internal_subset) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.start_doctypedecl ) {
    exec_result( parser->callbacks.start_doctypedecl(parser->user, doctypeName, sysid, pubid, has_internal_subset), parser );
  }
}

void XMLCALL xml_enddoctypedeclhandler(void *userdata) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.end_doctypedecl ) {
    exec_result( parser->callbacks.end_doctypedecl(parser->user), parser );
  }
}

void XMLCALL xml_entitydeclhandler(void *userdata, 
				   const XML_Char *entityName, 
				   int is_parameter_entity, 
				   const XML_Char *value, 
				   int value_length, 
				   const XML_Char *base, 
				   const XML_Char *systemId,
				   const XML_Char *publicId, 
				   const XML_Char *notationName) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.entitydecl ) {
    exec_result( parser->callbacks.entitydecl(parser->user, entityName, is_parameter_entity, value, value_length, base, systemId, publicId, notationName), parser );
  }
}

/* depending on what was read in the prolog, entities are 
   returned either ask skipped or as external, but we don't make the 
   distinction and pass everything to the default parser */
void XMLCALL xml_skippedentityhandler(void *userdata,
				      const XML_Char *entityName,
				      int is_parameter_entity) {
  parser_t *parser = (parser_t *)userdata;
  if( parser && parser->callbacks.dfault ) {
    XML_DefaultCurrent(parser->p);
  }
}

int XMLCALL xml_externalentityrefhandler(XML_Parser dummy,
					  const XML_Char *context,
					  const XML_Char *base,
					  const XML_Char *systemId,
					  const XML_Char *publicId) {
  parser_t *parser = (parser_t *)dummy;
  if( parser && parser->callbacks.dfault ) {
    XML_DefaultCurrent(parser->p);
  }
  return XML_STATUS_OK;
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
    /* if start_cdata, always set endcdatahandler */
    XML_SetEndCdataSectionHandler(parser->p, parser->callbacks.start_cdata ?
				  xml_endcdatahandler : NULL);
    XML_SetDefaultHandler(parser->p, parser->callbacks.dfault ?
    			  xml_defaulthandler : NULL);


    XML_SetStartDoctypeDeclHandler(parser->p, parser->callbacks.start_doctypedecl ?
				   xml_startdoctypedeclhandler : NULL);
    /* if start_doctypedecl, always set enddoctypedeclhandler */
    XML_SetEndDoctypeDeclHandler(parser->p, parser->callbacks.start_doctypedecl ?
				 xml_enddoctypedeclhandler : NULL);
    XML_SetEntityDeclHandler(parser->p, parser->callbacks.entitydecl ?
			     xml_entitydeclhandler : NULL);

    /* what to do about entities: there can be internal and external
     * entities, which expat treats differently. We want all our
     * entities to stay unprocessed, otherwise we'll get well formedness
     * errors. When a handler is called, the raw string is passed to
     * the default handler (provided it exists). */
    XML_UseForeignDTD(parser->p, XML_TRUE);
    XML_SetParamEntityParsing(parser->p, XML_PARAM_ENTITY_PARSING_NEVER);
    XML_SetExternalEntityRefHandler(parser->p,
				    xml_externalentityrefhandler);
    XML_SetExternalEntityRefHandlerArg(parser->p, parser);
    XML_SetSkippedEntityHandler(parser->p, xml_skippedentityhandler);


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
    parser->callbacks.start_doctypedecl = callbacks->start_doctypedecl;
    parser->callbacks.end_doctypedecl = callbacks->end_doctypedecl;
    parser->callbacks.entitydecl = callbacks->entitydecl;
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

bool_t aborted_parser(parser_t *parser) {
  return parser && 
    (parser->cur.rstatus == XML_STATUS_ERROR) &&
    (XML_GetErrorCode(parser->p) == XML_ERROR_ABORTED);
}

bool_t suspended_parser(parser_t *parser) {
  return parser && (parser->cur.pstatus == XML_SUSPENDED);
}

bool_t busy_parser(parser_t *parser) {
  return parser && (parser->cur.pstatus == XML_PARSING);
}

bool_t audit_parser(parser_t *parser) {
  /* since a parser is used by nearly all xml-coreutils,
     and the do_parser() functions call audit_parser(),
     this is a great place to process signals */
  process_pending_signal();

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
      errormsg(E_ERROR, "can't allocate read buffer (%lu bytes)\n", 
	       (unsigned long)n);
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

