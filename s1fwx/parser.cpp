#include "include.h"
#include "parser.h"


// -----------------------------------------------------------------------------------------------
PARSER_CMD PARSER::ParserCmdList[_PARSER_CMD_SIZE_] =
{
  {NULL,  {PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"",    {PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"//",  {PARSER_PARAM_ANY,   PARSER_PARAM_ANY,   PARSER_PARAM_ANY,   PARSER_PARAM_ANY  }},

  {"[AFI]", {PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"[FW]",  {PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"[EOF]", {PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},

  {"VID",   {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"PID",   {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"VER",   {PARSER_PARAM_VER,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"DATE",  {PARSER_PARAM_DATE,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},

  {"INF",               {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"INF_DEVICE",        {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"INF_MANUFACTURER",  {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},

  {"USB_ATTR",  {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"USB_IDENT", {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"USB_REV",   {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},

  {"SET_RTCRATE",         {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_DISPLAYCONTRAST", {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_LIGHTTIME",       {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_STANDBYTIME",     {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_SLEEPTIME",       {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_LANGUAGEID",      {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_REPLAYMODE",      {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_ONLINEMODE",      {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_BATTERYTYPE",     {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"SET_RADIODISABLE",    {PARSER_PARAM_UINT,  PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},

  {"PATH",    {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"FILE",    {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"FILE",    {PARSER_PARAM_STR,   PARSER_PARAM_STR,   PARSER_PARAM_STR,   PARSER_PARAM_UINT}},
  {"BREC",    {PARSER_PARAM_STR,   PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"FWSC",    {PARSER_PARAM_STR,   PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"ADFU",    {PARSER_PARAM_STR,   PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
  {"FWIMAGE", {PARSER_PARAM_STR,   PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY, PARSER_PARAM_EMPTY}},
};




// -----------------------------------------------------------------------------------------------
int PARSER::parse_script(LP_AFI_HEAD lpAFI, LPSTR *lppAfiFileList, LP_FW_HEAD lpFW, LPSTR *lppFwFileList)
{
  char szPath[MAX_PATH] = "";
  int afi_file_cnt = 0;
  int fw_file_cnt = 0;

  if(lpAFI) {
    ZeroMemory(lpAFI, sizeof(AFI_HEAD));
    memcpy(lpAFI->id, AFI_ID, sizeof(lpAFI->id));
    //if(lppAfiFileList) ZeroMemory(lppAfiFileList, sizeof(LPSTR)*AFI_ENTRIES);
  }

  if(lpFW) {
    ZeroMemory(lpFW, sizeof(FW_HEAD));
    lpFW->id = FIRMWARE_ID;
    COMVAL_INIT(&lpFW->comval)
    //if(lppFwFileList) ZeroMemory(lppFwFileList, sizeof(LPSTR)*FIRMWARE_ENTRIES);
  }

  for(int state = 0;;)
  {
    if(!this->next()) {

      this->error(PARSER_ERR_UEOF);
      return ERROR_SUCCESS;

    }

    if(this->cmd == PARSER_CMD_EMPTYLINE);
    else if(this->cmd == PARSER_CMD_COMMENT);
    else if(this->cmd == PARSER_CMD__EOF_) return ERROR_SUCCESS;
    else if(this->cmd == PARSER_CMD__AFI_) state = 1;
    else if(this->cmd == PARSER_CMD__FW_) state = 2;
    else if(state == 2 && lpFW) switch(this->cmd) {  //listen fw state

      case PARSER_CMD_VID:  this->cpy(&lpFW->vendor_id, sizeof(lpFW->vendor_id), &this->param[0].val.u, sizeof(uint32)); break;
      case PARSER_CMD_PID:  this->cpy(&lpFW->product_id, sizeof(lpFW->product_id), &this->param[0].val.u, sizeof(uint32)); break;
      case PARSER_CMD_VER:  this->cpy(&lpFW->version_id, sizeof(lpFW->version_id), &this->param[0].val.ver, sizeof(this->param[0].val.ver)); break;
      case PARSER_CMD_DATE: this->cpy(&lpFW->date, sizeof(lpFW->date), &this->param[0].date, sizeof(this->param[0].date)); break;

      case PARSER_CMD_INF:              this->cpy(lpFW->info.global, sizeof(lpFW->info.global), this->param[0].str, this->param[0].val.strlen); break;
      case PARSER_CMD_INF_DEVICE:       this->cpy(lpFW->info.device, sizeof(lpFW->info.device), this->param[0].str, this->param[0].val.strlen); break;
      case PARSER_CMD_INF_MANUFACTURER: this->cpy(lpFW->info.manufacturer, sizeof(lpFW->info.manufacturer), this->param[0].str, this->param[0].val.strlen); break;

      case PARSER_CMD_USB_ATTR:   this->cpy(lpFW->usb.attr, sizeof(lpFW->usb.attr), this->param[0].str, this->param[0].val.strlen); break;
      case PARSER_CMD_USB_IDENT:  this->cpy(lpFW->usb.ident, sizeof(lpFW->usb.ident), this->param[0].str, this->param[0].val.strlen); break;
      case PARSER_CMD_USB_REV:    this->cpy(lpFW->usb.rev, sizeof(lpFW->usb.rev), this->param[0].str, this->param[0].val.strlen); break;

      case PARSER_CMD_SET_RTCRATE:          this->cpy(&lpFW->comval.rtc_rate, sizeof(lpFW->comval.rtc_rate), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_DISPLAYCONTRAST:  this->cpy(&lpFW->comval.display_contrast, sizeof(lpFW->comval.display_contrast), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_LIGHTTIME:        this->cpy(&lpFW->comval.light_time, sizeof(lpFW->comval.light_time), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_STANDBYTIME:      this->cpy(&lpFW->comval.standby_time, sizeof(lpFW->comval.standby_time), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_SLEEPTIME:        this->cpy(&lpFW->comval.sleep_time, sizeof(lpFW->comval.sleep_time), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_LANGUAGEID:       this->cpy(&lpFW->comval.language_id, sizeof(lpFW->comval.language_id), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_REPLAYMODE:       this->cpy(&lpFW->comval.replay_mode, sizeof(lpFW->comval.replay_mode), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_ONLINEMODE:       this->cpy(&lpFW->comval.online_mode, sizeof(lpFW->comval.online_mode), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_BATTERYTYPE:      this->cpy(&lpFW->comval.battery_type, sizeof(lpFW->comval.battery_type), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;
      case PARSER_CMD_SET_RADIODISABLE:     this->cpy(&lpFW->comval.radio_disable, sizeof(lpFW->comval.radio_disable), &this->param[0].val.u, sizeof(this->param[0].val.u)); break;

      case PARSER_CMD_PATH:
        szPath[MAX_PATH-2] = szPath[MAX_PATH-1] = 0;
        this->cpy(szPath, MAX_PATH-2, this->param[0].str, this->param[0].val.strlen);
        if(szPath[0] && szPath[strlen(szPath)-1] != '\\') strcat(szPath, "\\");
        break;

      case PARSER_CMD_FILE:
        // insert filename into list
        if(fw_file_cnt >= FIRMWARE_ENTRIES) this->error(PARSER_ERR_TOO_MANY_FILES);
        else do {
          // build filename from path and param
          char *fname = new char[strlen(szPath) + this->param[0].val.strlen + 16];
          if(!fname) return ERROR_NOT_ENOUGH_MEMORY;
          strcpy(fname, szPath);
          this->cpy(&fname[strlen(fname)], this->param[0].val.strlen + 1, this->param[0].str, this->param[0].val.strlen);
          // fill entry (filename, extension)
          char fname_s[MAX_PATH+1];
          if(!GetShortPathName(fname, fname_s, sizeof(fname_s))) {
            this->error(PARSER_ERR_FILE_ACCESS);
            break;  //return GetLastError();
          }
          char *f8 = strrchr(fname_s, '\\');
          if(!f8) f8 = strrchr(fname_s, ':');
          if(!f8) f8 = fname_s; else f8++;
          char *fe = strrchr(f8, '.');
          if(fe) {*fe = 0; fe++;}
          this->cpy(lpFW->entry[fw_file_cnt].filename, 8, f8, strlen(f8), ' ');
          if(fe) this->cpy(lpFW->entry[fw_file_cnt].extension, 3, fe, strlen(fe), ' ');
          // add file to list if available
          if(lppFwFileList) lppFwFileList[fw_file_cnt] = fname;
          else delete fname;
          // inc counter
          fw_file_cnt++;
        } while(0);
        break;

      default: this->error(PARSER_ERR_SYNTAX);

    } else if(state == 1 && lpAFI) switch(this->cmd) {  //listen afi state

      case PARSER_CMD_VID: this->cpy(&lpAFI->vendor_id, sizeof(lpAFI->vendor_id), &this->param[0].val.u, sizeof(uint32)); break;
      case PARSER_CMD_PID: this->cpy(&lpAFI->product_id, sizeof(lpAFI->product_id), &this->param[0].val.u, sizeof(uint32)); break;
      case PARSER_CMD_VER: this->cpy(&lpAFI->version_id, sizeof(lpAFI->version_id), &this->param[0].val.ver, sizeof(this->param[0].val.ver)); break;
      case PARSER_CMD_DATE: this->cpy(&lpAFI->date, sizeof(lpAFI->date), &this->param[0].date, sizeof(this->param[0].date)); break;

      case PARSER_CMD_PATH:
        szPath[MAX_PATH-2] = szPath[MAX_PATH-1] = 0;
        this->cpy(szPath, MAX_PATH-2, this->param[0].str, this->param[0].val.strlen);
        if(szPath[0] && szPath[strlen(szPath)-1] != '\\') strcat(szPath, "\\");
        break;

      case PARSER_CMD_FILE_AFI:
      case PARSER_CMD_BREC:
      case PARSER_CMD_FWSC:
      case PARSER_CMD_ADFU:
      case PARSER_CMD_FWIMAGE:
        // insert filename into list
        if(afi_file_cnt >= AFI_ENTRIES) this->error(PARSER_ERR_TOO_MANY_FILES);
        else do {
          // build filename from path and param
          char *fname = new char[strlen(szPath) + this->param[0].val.strlen + 16];
          if(!fname) return ERROR_NOT_ENOUGH_MEMORY;
          strcpy(fname, szPath);
          this->cpy(&fname[strlen(fname)], this->param[0].val.strlen + 1, this->param[0].str, this->param[0].val.strlen);
          // fill entry (filename, extension)
          char fname_s[MAX_PATH+1];
          if(!GetShortPathName(fname, fname_s, sizeof(fname_s))) {
            this->error(PARSER_ERR_FILE_ACCESS);
            break;  //return GetLastError();
          }
          char *f8 = strrchr(fname_s, '\\');
          if(!f8) f8 = strrchr(fname_s, ':');
          if(!f8) f8 = fname_s; else f8++;
          char *fe = strrchr(f8, '.');
          if(fe) {*fe = 0; fe++;}
          this->cpy(lpAFI->entry[afi_file_cnt].filename, 8, f8, strlen(f8), ' ');
          if(fe) this->cpy(lpAFI->entry[afi_file_cnt].extension, 3, fe, strlen(fe), ' ');
          // fill entry (additional informations)
          switch(this->cmd) {
            case PARSER_CMD_FILE_AFI:
              this->cpy(lpAFI->entry[afi_file_cnt].desc, 4, this->param[1].str, this->param[1].val.strlen);
              this->cpy(&lpAFI->entry[afi_file_cnt].type, 1, this->param[2].str, this->param[2].val.strlen);
              this->cpy(&lpAFI->entry[afi_file_cnt].download_addr, sizeof(lpAFI->entry[afi_file_cnt].download_addr), &this->param[3].val.u, sizeof(uint32));
              break;

            case PARSER_CMD_BREC:
              this->cpy(lpAFI->entry[afi_file_cnt].desc, 4, this->param[1].str, this->param[1].val.strlen);
              lpAFI->entry[afi_file_cnt].type = 'B';
              lpAFI->entry[afi_file_cnt].download_addr = AFI_ENTRY_DLADR_BREC;
              break;
            case PARSER_CMD_FWSC:
              this->cpy(lpAFI->entry[afi_file_cnt].desc, 4, this->param[1].str, this->param[1].val.strlen);
              lpAFI->entry[afi_file_cnt].type = 'F';
              lpAFI->entry[afi_file_cnt].download_addr = AFI_ENTRY_DLADR_FWSC;
              break;
            case PARSER_CMD_ADFU:
              this->cpy(lpAFI->entry[afi_file_cnt].desc, 4, this->param[1].str, this->param[1].val.strlen);
              lpAFI->entry[afi_file_cnt].type = 'U';
              lpAFI->entry[afi_file_cnt].download_addr = AFI_ENTRY_DLADR_ADFU;
              break;

            case PARSER_CMD_FWIMAGE:
              memset(lpAFI->entry[afi_file_cnt].desc, 0, 4);
              lpAFI->entry[afi_file_cnt].type = 'I';
              lpAFI->entry[afi_file_cnt].download_addr = AFI_ENTRY_DLADR_FW;
              break;
          }
          // add file to list if available
          if(lppAfiFileList) lppAfiFileList[afi_file_cnt] = fname;
          else delete fname;
          // inc counter
          afi_file_cnt++;
        } while(0);
        break;

      default: this->error(PARSER_ERR_SYNTAX);

    } else if(state == 0) this->error(PARSER_ERR_SYNTAX); //listen state
  }

  return ERROR_SUCCESS;
}
