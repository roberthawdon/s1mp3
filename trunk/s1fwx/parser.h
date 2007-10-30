#ifndef _PARSER_H_
#define _PARSER_H_

#include "heads.h"


// -----------------------------------------------------------------------------------------------
#define PARSER_MAX_PARAMS 4


// -----------------------------------------------------------------------------------------------
enum PARSER_CMD_ID {
  PARSER_CMD_UNKNOWN,
  PARSER_CMD_EMPTYLINE,
  PARSER_CMD_COMMENT,

  PARSER_CMD__AFI_,
  PARSER_CMD__FW_,
  PARSER_CMD__EOF_,

  PARSER_CMD_VID,
  PARSER_CMD_PID,
  PARSER_CMD_VER,
  PARSER_CMD_DATE,

  PARSER_CMD_INF,
  PARSER_CMD_INF_DEVICE,
  PARSER_CMD_INF_MANUFACTURER,

  PARSER_CMD_USB_ATTR,
  PARSER_CMD_USB_IDENT,
  PARSER_CMD_USB_REV,

  PARSER_CMD_SET_RTCRATE,
  PARSER_CMD_SET_DISPLAYCONTRAST,
  PARSER_CMD_SET_LIGHTTIME,
  PARSER_CMD_SET_STANDBYTIME,
  PARSER_CMD_SET_SLEEPTIME,
  PARSER_CMD_SET_LANGUAGEID,
  PARSER_CMD_SET_REPLAYMODE,
  PARSER_CMD_SET_ONLINEMODE,
  PARSER_CMD_SET_BATTERYTYPE,
  PARSER_CMD_SET_RADIODISABLE,

  PARSER_CMD_PATH,
  PARSER_CMD_FILE,
  PARSER_CMD_FILE_AFI,
  PARSER_CMD_BREC,
  PARSER_CMD_FWSC,
  PARSER_CMD_ADFU,
  PARSER_CMD_FWIMAGE,

  _PARSER_CMD_SIZE_
};


// -----------------------------------------------------------------------------------------------
enum PARSER_PARAM_ID {
  PARSER_PARAM_EMPTY,
  PARSER_PARAM_STR,
  PARSER_PARAM_VER,
  //PARSER_PARAM_INT,
  PARSER_PARAM_UINT,
  PARSER_PARAM_DATE,
  PARSER_PARAM_ANY,
};

typedef struct _PARSER_CMD {
  const char *cmd;
  PARSER_PARAM_ID param[PARSER_MAX_PARAMS];
} PARSER_CMD, *LP_PARSER_CMD;

typedef struct _PARSER_PARAM {
  PARSER_PARAM_ID id;
  LPSTR str;
  union {uint32 u; int32 i; uint32 strlen; uint16 ver;} val;
  struct {uint8 year[2], month, day;} date;
} PARSER_PARAM, *LP_PARSER_PARAM;

// -----------------------------------------------------------------------------------------------
enum PARSER_ERR {
  PARSER_ERR_UNKNOWN,
  PARSER_ERR_SYNTAX,
  PARSER_ERR_UEOF,
  PARSER_ERR_TOO_MANY_FILES,
  PARSER_ERR_FILE_ACCESS,
};




// -----------------------------------------------------------------------------------------------
class PARSER
{
public:
  int parse_script(LP_AFI_HEAD, LPSTR *, LP_FW_HEAD, LPSTR *);


  PARSER(FILE *fh) {
    this->fh = fh;
    line = 0;
    cmd = PARSER_CMD_UNKNOWN; 
    param_cntr = 0;
    ZeroMemory(param, sizeof(param));
  }

  ~PARSER() {fclose(fh);}


  void error(PARSER_ERR ec)
  {
    fprintf(stderr, "parser(%u) : ", line);
    switch(ec) {
      case PARSER_ERR_SYNTAX: fprintf(stderr, "syntax error\n"); break;
      case PARSER_ERR_UEOF:   fprintf(stderr, "unexpected end of file\n"); break;
      case PARSER_ERR_TOO_MANY_FILES: fprintf(stderr, "too many files\n"); break;
      case PARSER_ERR_FILE_ACCESS: fprintf(stderr, "failed to access file\n"); break;
      default: fprintf(stderr, "internal error\n");
    }
  }

  bool parse_param(int nIndex, char **ppPos, PARSER_PARAM_ID ppID, LP_PARSER_PARAM lpPP)
  {
    *ppPos = skip(*ppPos);
    if(nIndex > 0 && ppID != PARSER_PARAM_EMPTY /*&& ppID != PARSER_PARAM_ANY*/) {
      // test for seperator if its not a first or empty param
      if(**ppPos != ',') return false;
      *ppPos = skip((*ppPos) + 1);
    }
    switch(lpPP->id = ppID) {
      case PARSER_PARAM_EMPTY:    //no more params allowed, test for string end
        return(**ppPos == 0);
      case PARSER_PARAM_STR:      //parse string
        if(**ppPos != '"') return false;
        lpPP->str = ++(*ppPos);
        for(lpPP->val.strlen = 0; **ppPos && **ppPos != '"'; (*ppPos)++) lpPP->val.strlen++;
        if(**ppPos != '"') return false;
        (*ppPos)++;
        return true;
      case PARSER_PARAM_DATE:     //parse for date [MM/DD/YYYY]
        {
          ZeroMemory(&lpPP->date, sizeof(lpPP->date));
          lpPP->date.month = (uint8)strtoul(*ppPos, ppPos, 16);
          if(!*ppPos || **ppPos != '/') return false;
          lpPP->date.day = (uint8)strtoul(++(*ppPos), ppPos, 16);
          if(!*ppPos || **ppPos != '/') return false;
          uint16 year = (uint16)strtoul(++(*ppPos), ppPos, 16);
          lpPP->date.year[0] = (uint8)(year >> 8);
          lpPP->date.year[1] = (uint8)(year);
          return(*ppPos != NULL);
        }
      case PARSER_PARAM_VER:      //parse for version
        {
          uint16 val = (uint16)strtoul(*ppPos, ppPos, 10);
          lpPP->val.ver = ((val>15)?15:val)<<4;
          if(!*ppPos) return false;
          if(**ppPos == '.') {
            val = (uint16)strtoul(++(*ppPos), ppPos, 10);
            lpPP->val.ver |= (val>15)?15:val;
            if(!*ppPos) return false;
            if(**ppPos == '.') {
              val = (uint16)strtoul(++(*ppPos), ppPos, 16);
              lpPP->val.ver |= ((val>255)?255:val)<<8;
              if(!*ppPos) return false;
            }
          }
        }
        return true;
      case PARSER_PARAM_UINT:     //parse unsigned value
        lpPP->val.u = strtoul(*ppPos, ppPos, 0);
        return(*ppPos != NULL);
      //case PARSER_PARAM_INT:    //parse signed value
        //lpPP->val.i = strtol(*ppPos, ppPos, 0);
        //return(*ppPos != NULL);
      case PARSER_PARAM_ANY:      //skip any content
        (*ppPos) += strlen(*ppPos); return true;
      default: return false;
    }
  }
    
  bool parse_cmd(LP_PARSER_CMD lpCmd)
  {
    if(!lpCmd->cmd) return false;
    else if(lpCmd->param[0] == PARSER_PARAM_EMPTY) return cmp(lpCmd->cmd, line_buf);
    else if(cmp(lpCmd->cmd, line_buf, strlen(lpCmd->cmd))) {
      if(lpCmd->param[0] == PARSER_PARAM_ANY) return true;
      char *pos = line_buf + strlen(lpCmd->cmd);
      pos = skip(pos);
      if(*pos != '=') return false;
      pos++;
      for(param_cntr=0; param_cntr<PARSER_MAX_PARAMS; param_cntr++)
        if(!parse_param(param_cntr, &pos, lpCmd->param[param_cntr], &param[param_cntr])) return false;
      pos = skip(pos);
      return(*pos == 0);
    }
    return false;
  }


  bool next()
  {
    if(feof(fh)) return false;

    fgets(line_buf, sizeof(line_buf), fh);
    if(strchr(line_buf, '\n')) line++;
    trunc(line_buf);
    strcpy(line_buf, skip(line_buf));

    cmd = _PARSER_CMD_SIZE_;
    param_cntr = 0;
    ZeroMemory(param, sizeof(param));
    while(cmd != PARSER_CMD_UNKNOWN) if(parse_cmd(&ParserCmdList[--cmd])) break;
    return true;
  }

  bool cmp(const char *a, char *b)
    {return _stricmp(a, b)==0;}
  bool cmp(const char *a, char *b, size_t l)
    {return _strnicmp(a, b, l)==0;}

  void cpy(LPVOID dest, size_t dest_len, LPVOID src, size_t src_len, int init=0)
    {if(dest) {memset(dest, init, dest_len); if(src) memcpy(dest, src, (src_len>dest_len)?dest_len:src_len);}}

  char *skip(char *a) {while(*a == ' ' || *a == '\t') a++; return a;}
  void trunc(char *a) {
    if(*a) for(char *b = strrchr(a, 0)-1; a <= b && (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r'); b--) *b = 0;
  }


  FILE *fh;
  uint32 line;
  char line_buf[1024];
  int cmd;  //PARSER_CMD_ID

  uint32 param_cntr;
  PARSER_PARAM param[PARSER_MAX_PARAMS];

  static PARSER_CMD ParserCmdList[_PARSER_CMD_SIZE_];
};


#endif