//file: utils.cpp
/******************************************************************************

Various utility routines for conversions, formatting, messages, tec.


See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************/

#include "grapher.h"

const char *msgname(unsigned message, bool ignoresome) {  // translate message numbers to readable names
   static struct {
      unsigned code; const char *name; }   msgnames[] = {
      {0,"WM_NULL" }, {1,"WM_CREATE" }, {2,"WM_DESTROY" }, {3,"WM_MOVE" }, {5,"WM_SIZE" }, {6,"WM_ACTIVATE" },
      {7,"WM_SETFOCUS" }, {8,"WM_KILLFOCUS" }, {10,"WM_ENABLE" }, {11,"WM_SETREDRAW" }, {12,"WM_SETTEXT" },
      {13,"WM_GETTEXT" }, {14,"WM_GETTEXTLENGTH" }, {15,"WM_PAINT" }, {16,"WM_CLOSE" },
      {17,"WM_QUERYENDSESSION" }, {18,"WM_QUIT" }, {19,"WM_QUERYOPEN" }, {20,"WM_ERASEBKGND" },
      {21,"WM_SYSCOLORCHANGE" }, {22,"WM_ENDSESSION" }, {24,"WM_SHOWWINDOW" }, {25,"WM_CTLCOLOR" },
      {26,"WM_WININICHANGE" }, {27,"WM_DEVMODECHANGE" }, {28,"WM_ACTIVATEAPP" }, {29,"WM_FONTCHANGE" },
      {30,"WM_TIMECHANGE" }, {31,"WM_CANCELMODE" }, {32,"WM_SETCURSOR" }, {33,"WM_MOUSEACTIVATE" },
      {34,"WM_CHILDACTIVATE" }, {35,"WM_QUEUESYNC" }, {36,"WM_GETMINMAXINFO" }, {38,"WM_PAINTICON" },
      {39,"WM_ICONERASEBKGND" }, {40,"WM_NEXTDLGCTL" }, {42,"WM_SPOOLERSTATUS" }, {43,"WM_DRAWITEM" },
      {44,"WM_MEASUREITEM" }, {45,"WM_DELETEITEM" }, {46,"WM_VKEYTOITEM" }, {47,"WM_CHARTOITEM" },
      {48,"WM_SETFONT" }, {49,"WM_GETFONT" }, {50,"WM_SETHOTKEY" }, {51,"WM_GETHOTKEY" },
      {55,"WM_QUERYDRAGICON" }, {57,"WM_COMPAREITEM" }, {61,"WM_GETOBJECT" }, {65,"WM_COMPACTING" },
      {68,"WM_COMMNOTIFY" }, {70,"WM_WINDOWPOSCHANGING" }, {71,"WM_WINDOWPOSCHANGED" }, {72,"WM_POWER" },
      {73,"WM_COPYGLOBALDATA" }, {74,"WM_COPYDATA" }, {75,"WM_CANCELJOURNAL" }, {78,"WM_NOTIFY" },
      {80,"WM_INPUTLANGCHANGEREQUEST" }, {81,"WM_INPUTLANGCHANGE" }, {82,"WM_TCARD" }, {83,"WM_HELP" },
      {84,"WM_USERCHANGED" }, {85,"WM_NOTIFYFORMAT" }, {123,"WM_CONTEXTMENU" }, {124,"WM_STYLECHANGING" },
      {125,"WM_STYLECHANGED" }, {126,"WM_DISPLAYCHANGE" }, {127,"WM_GETICON" }, {128,"WM_SETICON" },
      {129,"WM_NCCREATE" }, {130,"WM_NCDESTROY" }, {131,"WM_NCCALCSIZE" }, {132,"WM_NCHITTEST" },
      {133,"WM_NCPAINT" }, {134,"WM_NCACTIVATE" }, {135,"WM_GETDLGCODE" }, {136,"WM_SYNCPAINT" },
      {160,"WM_NCMOUSEMOVE" }, {161,"WM_NCLBUTTONDOWN" }, {162,"WM_NCLBUTTONUP" },
      {163,"WM_NCLBUTTONDBLCLK" }, {164,"WM_NCRBUTTONDOWN" }, {165,"WM_NCRBUTTONUP" },
      {166,"WM_NCRBUTTONDBLCLK" }, {167,"WM_NCMBUTTONDOWN" }, {168,"WM_NCMBUTTONUP" },
      {169,"WM_NCMBUTTONDBLCLK" }, {171,"WM_NCXBUTTONDOWN" }, {172,"WM_NCXBUTTONUP" },
      {173,"WM_NCXBUTTONDBLCLK" }, {176,"EM_GETSEL" }, {177,"EM_SETSEL" }, {178,"EM_GETRECT" },
      {179,"EM_SETRECT" }, {180,"EM_SETRECTNP" }, {181,"EM_SCROLL" }, {182,"EM_LINESCROLL" },
      {183,"EM_SCROLLCARET" }, {185,"EM_GETMODIFY" }, {187,"EM_SETMODIFY" }, {188,"EM_GETLINECOUNT" },
      {189,"EM_LINEINDEX" }, {190,"EM_SETHANDLE" }, {191,"EM_GETHANDLE" }, {192,"EM_GETTHUMB" },
      {193,"EM_LINELENGTH" }, {194,"EM_REPLACESEL" }, {195,"EM_SETFONT" }, {196,"EM_GETLINE" },
      {197,"EM_LIMITTEXT" }, {197,"EM_SETLIMITTEXT" }, {198,"EM_CANUNDO" }, {199,"EM_UNDO" },
      {200,"EM_FMTLINES" }, {201,"EM_LINEFROMCHAR" }, {202,"EM_SETWORDBREAK" }, {203,"EM_SETTABSTOPS" },
      {204,"EM_SETPASSWORDCHAR" }, {205,"EM_EMPTYUNDOBUFFER" }, {206,"EM_GETFIRSTVISIBLELINE" },
      {207,"EM_SETREADONLY" }, {209,"EM_SETWORDBREAKPROC" }, {209,"EM_GETWORDBREAKPROC" },
      {210,"EM_GETPASSWORDCHAR" }, {211,"EM_SETMARGINS" }, {212,"EM_GETMARGINS" }, {213,"EM_GETLIMITTEXT" },
      {214,"EM_POSFROMCHAR" }, {215,"EM_CHARFROMPOS" }, {216,"EM_SETIMESTATUS" }, {217,"EM_GETIMESTATUS" },
      {224,"SBM_SETPOS" }, {225,"SBM_GETPOS" }, {226,"SBM_SETRANGE" }, {227,"SBM_GETRANGE" },
      {228,"SBM_ENABLE_ARROWS" }, {230,"SBM_SETRANGEREDRAW" }, {233,"SBM_SETSCROLLINFO" },
      {234,"SBM_GETSCROLLINFO" }, {235,"SBM_GETSCROLLBARINFO" }, {240,"BM_GETCHECK" }, {241,"BM_SETCHECK" },
      {242,"BM_GETSTATE" }, {243,"BM_SETSTATE" }, {244,"BM_SETSTYLE" }, {245,"BM_CLICK" },
      {246,"BM_GETIMAGE" }, {247,"BM_SETIMAGE" }, {248,"BM_SETDONTCLICK" }, {255,"WM_INPUT" },
      {256,"WM_KEYDOWN" }, {256,"WM_KEYFIRST" }, {257,"WM_KEYUP" }, {258,"WM_CHAR" }, {259,"WM_DEADCHAR" },
      {260,"WM_SYSKEYDOWN" }, {261,"WM_SYSKEYUP" }, {262,"WM_SYSCHAR" }, {263,"WM_SYSDEADCHAR" },
      {264,"WM_KEYLAST" }, {265,"WM_UNICHAR" }, {265,"WM_WNT_CONVERTREQUESTEX" }, {266,"WM_CONVERTREQUEST" },
      {267,"WM_CONVERTRESULT" }, {268,"WM_INTERIM" }, {269,"WM_IME_STARTCOMPOSITION" },
      {270,"WM_IME_ENDCOMPOSITION" }, {271,"WM_IME_COMPOSITION" }, {271,"WM_IME_KEYLAST" },
      {272,"WM_INITDIALOG" }, {273,"WM_COMMAND" }, {274,"WM_SYSCOMMAND" }, {275,"WM_TIMER" },
      {276,"WM_HSCROLL" }, {277,"WM_VSCROLL" }, {278,"WM_INITMENU" }, {279,"WM_INITMENUPOPUP" },
      {280,"WM_SYSTIMER" }, {287,"WM_MENUSELECT" }, {288,"WM_MENUCHAR" }, {289,"WM_ENTERIDLE" },
      {290,"WM_MENURBUTTONUP" }, {291,"WM_MENUDRAG" }, {292,"WM_MENUGETOBJECT" }, {293,"WM_UNINITMENUPOPUP" },
      {294,"WM_MENUCOMMAND" }, {295,"WM_CHANGEUISTATE" }, {296,"WM_UPDATEUISTATE" }, {297,"WM_QUERYUISTATE" },
      {306,"WM_CTLCOLORMSGBOX" }, {307,"WM_CTLCOLOREDIT" }, {308,"WM_CTLCOLORLISTBOX" },
      {309,"WM_CTLCOLORBTN" }, {310,"WM_CTLCOLORDLG" }, {311,"WM_CTLCOLORSCROLLBAR" },
      {312,"WM_CTLCOLORSTATIC" }, {512,"WM_MOUSEMOVE" }, {513,"WM_LBUTTONDOWN" },
      {514,"WM_LBUTTONUP" }, {515,"WM_LBUTTONDBLCLK" }, {516,"WM_RBUTTONDOWN" }, {517,"WM_RBUTTONUP" },
      {518,"WM_RBUTTONDBLCLK" }, {519,"WM_MBUTTONDOWN" }, {520,"WM_MBUTTONUP" }, {521,"WM_MBUTTONDBLCLK" },
      {521,"WM_MOUSELAST" }, {522,"WM_MOUSEWHEEL" }, {523,"WM_XBUTTONDOWN" }, {524,"WM_XBUTTONUP" },
      {525,"WM_XBUTTONDBLCLK" }, {528,"WM_PARENTNOTIFY" }, {529,"WM_ENTERMENULOOP" },
      {530,"WM_EXITMENULOOP" }, {531,"WM_NEXTMENU" }, {532,"WM_SIZING" }, {533,"WM_CAPTURECHANGED" },
      {534,"WM_MOVING" }, {536,"WM_POWERBROADCAST" }, {537,"WM_DEVICECHANGE" }, {544,"WM_MDICREATE" },
      {545,"WM_MDIDESTROY" }, {546,"WM_MDIACTIVATE" }, {547,"WM_MDIRESTORE" }, {548,"WM_MDINEXT" },
      {549,"WM_MDIMAXIMIZE" }, {550,"WM_MDITILE" }, {551,"WM_MDICASCADE" }, {552,"WM_MDIICONARRANGE" },
      {553,"WM_MDIGETACTIVE" }, {560,"WM_MDISETMENU" }, {561,"WM_ENTERSIZEMOVE" }, {562,"WM_EXITSIZEMOVE" },
      {563,"WM_DROPFILES" }, {564,"WM_MDIREFRESHMENU" }, {640,"WM_IME_REPORT" }, {641,"WM_IME_SETCONTEXT" },
      {642,"WM_IME_NOTIFY" }, {643,"WM_IME_CONTROL" }, {644,"WM_IME_COMPOSITIONFULL" },
      {645,"WM_IME_SELECT" }, {646,"WM_IME_CHAR" }, {648,"WM_IME_REQUEST" }, {656,"WM_IMEKEYDOWN" },
      {656,"WM_IME_KEYDOWN" }, {657,"WM_IMEKEYUP" }, {657,"WM_IME_KEYUP" }, {672,"WM_NCMOUSEHOVER" },
      {673,"WM_MOUSEHOVER" }, {674,"WM_NCMOUSELEAVE" }, {675,"WM_MOUSELEAVE" }, {768,"WM_CUT" },
      {769,"WM_COPY" }, {770,"WM_PASTE" }, {771,"WM_CLEAR" }, {772,"WM_UNDO" }, {773,"WM_RENDERFORMAT" },
      {774,"WM_RENDERALLFORMATS" }, {775,"WM_DESTROYCLIPBOARD" }, {776,"WM_DRAWCLIPBOARD" },
      {777,"WM_PAINTCLIPBOARD" }, {778,"WM_VSCROLLCLIPBOARD" }, {779,"WM_SIZECLIPBOARD" },
      {780,"WM_ASKCBFORMATNAME" }, {781,"WM_CHANGECBCHAIN" }, {782,"WM_HSCROLLCLIPBOARD" },
      {783,"WM_QUERYNEWPALETTE" }, {784,"WM_PALETTEISCHANGING" }, {785,"WM_PALETTECHANGED" },
      {786,"WM_HOTKEY" }, {791,"WM_PRINT" }, {792,"WM_PRINTCLIENT" }, {793,"WM_APPCOMMAND" },
      {856,"WM_HANDHELDFIRST" }, {863,"WM_HANDHELDLAST" }, {864,"WM_AFXFIRST" }, {895,"WM_AFXLAST" },
      {896,"WM_PENWINFIRST" }, {897,"WM_RCRESULT" }, {898,"WM_HOOKRCRESULT" }, {899,"WM_GLOBALRCCHANGE" },
      {899,"WM_PENMISCINFO" }, {900,"WM_SKB" }, {901,"WM_HEDITCTL" }, {901,"WM_PENCTL" }, {902,"WM_PENMISC" },
      {903,"WM_CTLINIT" }, {904,"WM_PENEVENT" }, {911,"WM_PENWINLAST" }, {1024,"WM_USER" }, {MAXUINT,"" } };

   static unsigned ignore[] = { 32 /*WM_SETCURSOR*/, 160 /*WM_NCMOUSEMOVE*/,
                                145, 146, 147, // what are these?
                                132 /*WM_NCHITTEST*/,
                                512 /*WM_MOUSEMOVE*/,
                                MAXUINT };
   int ndx;
   if (ignoresome) for (ndx = 0; ignore[ndx] != MAXUINT; ++ndx)
         if (ignore[ndx] == message) return NULL;
   for (ndx = 0; msgnames[ndx].code != MAXUINT; ++ndx)
      if (msgnames[ndx].code == message) break;
   if (msgnames[ndx].code == MAXUINT) {
      static char number[20];
      sprintf(number, "%u", message);
      return number; }
   return msgnames[ndx].name; }

void show_message_name(struct window_t *ww, UINT message, WPARAM wParam, LPARAM lParam) {
   if (DEBUG_SHOW_MESSAGES) {
      ++ww->recursion_level;
      const char *msg = msgname(message, true);
      SYSTEMTIME time;
      GetSystemTime(&time);
      if (msg) dlog("%lu: %s(%d) message %s (%08X, %016llX)\n",
                       time.wSecond * 1000 + time.wMilliseconds, ww->name, ww->recursion_level, msg,
                       (unsigned)wParam, (unsigned long long)lParam); } }

// convert a wide-character ASCII string to normal single-byte chars
void wchar_to_ascii(LPWSTR src, char *dst, int buflen) {
   for (; *src && buflen>1; ++src, --buflen)
      *dst++ = *src >= 0x20 && *src <= 0x7e ? (char)*src : '?';
   *dst = 0; }

// Debugging log routines

FILE *rlogf = NULL;
static void vlog(const char *msg, va_list args) {
   va_list args2;
   va_copy(args2, args);
   vfprintf(stdout, msg, args);  // to the console
   if (!rlogf)
      rlogf = fopen("grapher.log", "w");
   vfprintf(rlogf, msg, args2); // and the log file
   va_end(args2); }

void debuglog(const char* msg, ...) { // debugging log
   static FILE *debugf;
   va_list args;
   va_start(args, msg);
   vlog(msg, args);
   va_end(args); }

void assert(bool b, const char *msg, ...) {
   if (!b) {
      static bool in_assert = false;
      if (in_assert) exit(16);
      in_assert = true;
      va_list args;
      va_start(args, msg);
      vlog(msg, args);
      debuglog("\n");
      char buf[200];
      vsnprintf(buf, sizeof(buf), msg, args);
      va_end(args);
      int msgboxID = MessageBoxA(
                        NULL, // window handle
                        buf,
                        "FATAL ERROR",
                        MB_ICONEXCLAMATION );
      exit(8); } }

// Microsoft Visual Studio C doesn't support the wonderful POSIX %' format
// specifier for nicely displaying big numbers with commas separating thousands, millions, etc.
// So here are a couple of special-purpose routines for that.

char *u32commas(uint32_t n, char buf[14]) { // 32 bits, max: 2,147,483,647
   char *p = buf + 13;  int ctr = 4;
   *p-- = '\0';
   if (n == 0)  *p-- = '0';
   else while (n > 0) {
         if (--ctr == 0) {
            *p-- = ',';  ctr = 3; }
         *p-- = n % 10 + '0';
         n = n / 10; }
   return p + 1; }

char *u64commas(uint64_t n, char buf[26]) { // 64 bits, max: 9,223,372,036,854,775,807
   char *p = buf + 25; int ctr = 4;
   *p-- = '\0';
   if (n == 0)  *p-- = '0';
   else while (n > 0) {
         if (--ctr == 0) {
            *p-- = ',';  ctr = 3; }
         *p-- = n % 10 + '0';
         n = n / 10; }
   return p + 1; }

char *showtime(double time, char *buf, int bufsize) {
   // buf must be at least 15 chars
   double abstime = time < 0 ? -time : time;
   if (abstime == 0)
      strcpy(buf, "0.000000   "); // 3 spaces for "no units"
   else if (abstime < 1e-6)
      sprintf_s(buf, bufsize, "%.6f ns", time * 1e9);
   else if (abstime < 1e-3)
      sprintf_s(buf, bufsize, "%.6f us", time * 1e6);
   else if (abstime < 1.0f)
      sprintf_s(buf, bufsize, "%.6f ms", time * 1e3);
   else
      sprintf_s(buf, bufsize, "%.6f s ", time);
   return buf; }

char *format_memsize(uint64_t val, char *buf) { // format as xxx.x KB, MB, or GB
   // WARNING: buffer should be at least 30 chars
   if (val < 1024)
      snprintf(buf, 30, "%llu bytes", val);
#define MEMCONV(val,bits) val>>bits,(uint32_t)(((val&((1<<bits)-1))*10)>>bits)
   else if (val < 1024 * 1024) snprintf(buf, 30, "%llu.%1u KB",MEMCONV(val,10));
   else if (val < 1024 * 1024 * 1024) snprintf(buf, 30, "%llu.%1u MB", MEMCONV(val,20));
   else snprintf(buf, 30, "%llu.%1u GB", MEMCONV(val, 30));
#undef MEMCONV
   return buf; }

// These routines are *way* faster than using sscanf!

float scanfast_float(char **p) { // *** fast scanning routines for CSV numbers
   float n = 0;
   bool negative = false;
   while (**p == ' ' || **p == ',')++*p; //skip leading blanks, comma
   if (**p == '-') { // optional minus sign
      ++*p; negative = true; }
   while (isdigit(**p)) n = n * 10 + (*(*p)++ - '0'); //accumulate left of decimal point
   if (**p == '.') { // skip decimal point
      float divisor = 10;
      ++*p;
      while (isdigit(**p)) { //accumulate right of decimal point
         n += (*(*p)++ - '0') / divisor;
         divisor *= 10; } }
   return negative ? -n : n; }

double scanfast_double(char **p) {
   double n = 0;
   bool negative = false;
   while (**p == ' ' || **p == ',')++*p;  //skip leading blanks, comma
   if (**p == '-') { // optional minus sign
      ++*p; negative = true; }
   while (isdigit(**p)) n = n * 10 + (*(*p)++ - '0'); //accumulate left of decimal point
   if (**p == '.') {
      double divisor = 10;
      ++*p;
      while (isdigit(**p)) { //accumulate right of decimal point
         n += (*(*p)++ - '0') / divisor;
         divisor *= 10; } }
   return negative ? -n : n; }

// MessageBox with auto-timeout

//This is apparently an undocumented Windows function that has been around since Windows XP in 2001.
//https://stackoverflow.com/questions/3091300/messagebox-with-timeout-or-closing-a-messagebox-from-another-thread
int DU_MessageBoxTimeout(HWND hWnd, const char* sText, const char* sCaption, UINT uType, DWORD dwMilliseconds) {
   // Displays a message box, and dismisses it after the specified timeout.
   typedef int(__stdcall *MSGBOXAPI)(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
   int iResult;
   HMODULE hUser32 = LoadLibraryA("user32.dll");
   if (hUser32) {
      auto MessageBoxTimeoutA = (MSGBOXAPI)GetProcAddress(hUser32, "MessageBoxTimeoutA");
      iResult = MessageBoxTimeoutA(hWnd, sText, sCaption, uType, 0, dwMilliseconds);
      FreeLibrary(hUser32); }
   else iResult = MessageBoxA(hWnd, sText, sCaption, uType);  // oups, fallback to the standard function!
   return iResult; }

void copy_time(double time) { // copy a time as text to the Windows clipboard
   char clipboard[25];
   int length = 1 + snprintf(clipboard, sizeof(clipboard), "%.8lf", time);
   dlog("to clipboard: %s\n", clipboard);
   if (OpenClipboard(NULL)) {
      EmptyClipboard();
      HGLOBAL global;
      if (global = GlobalAlloc(GMEM_MOVEABLE, length)) {
         char *dst = (char *)GlobalLock(global);
         memcpy(dst, clipboard, length);
         GlobalUnlock(global);
         SetClipboardData(CF_TEXT, global); }
      //"If SetClipboardData succeeds, the system owns the object identified by the hMem parameter.
      // The application may not write to or free the data once ownership has been transferred to the system."
      CloseClipboard(); }
   char msg[100];
   snprintf(msg, sizeof(msg), "copied to clipboard: %s", clipboard);
   //MessageBoxA(NULL, msg, "Info", 0);  // can we get this to timeout?
   DU_MessageBoxTimeout(NULL, msg, "Info", 0, 2000);  // yes!
}

//*