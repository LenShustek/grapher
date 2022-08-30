//file: helptext.cpp
/******************************************************************************

   Put up a window with help text


See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************/

#include "grapher.h"

LRESULT CALLBACK WndProcHelp(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   dlog("help: message %s\n", msgname(message, false));
   LRESULT result = 0;
   switch (message) {

   case WM_CTLCOLORDLG:
      result = (INT_PTR)GetStockObject(WHITE_BRUSH);
      break;

     case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc;
      hdc = BeginPaint(hwnd, &ps);
      SelectFont(hdc, (HFONT)GetStockObject(ANSI_FIXED_FONT));
      RECT  rect;
      rect.top = rect.left = 5,5;
      GetClientRect(hwnd, &rect);
      DrawText(hdc, helptext, -1, &rect, DT_TOP | DT_NOCLIP);
      EndPaint(hwnd, &ps); }
   break;

   case WM_COMMAND: // button push
   case WM_CLOSE:   // x in upper right corner
      EndDialog(hwnd, 0);//destroy dialog window
      result = 1;
      break;
   default:
      result = DefWindowProc(hwnd, message, wParam, lParam); }
   return result; }

void showhelp(void) {
   DialogBox(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDD_HELP), main_ww.handle, WndProcHelp); }

/* 
An easy way to create this array initializer is to start with the text at the
beginning of grapher.cpp and replace  \r\n  with  \\n"\r\n"bb  
*/
const char helptext[] = {
"  This is a data visualizer that displays up to 12 time-series plots on a single\n"
"  chart that can be scrolled horizontally and zoomed in or out. It is designed\n"
"  to be relatively efficient when there are billions of points.\n"
"  \n"
"  This program can read a CSV (comma separated value) file, where the first column is\n"
"  the uniformly incremented timestamp for all the plots. The first two lines are discarded\n"
"  as headers, but the number of items in the second line sets the number of plot lines.\n"
"  \n"
"  It can also read the more compact TBIN file as defined for the readtape program.\n"
"  \n"
"  Since our tape data tends to be smooth and well-sampled, by default we subsample by\n"
"  using only every 3rd data point. You can change that with the tools/sampling menu.\n"
"  \n"
"  You can save a subset of the data between specified times as a CSV or TBIN file.\n"
"  If subsampling is on, it will warn you about saving a lower-resolution file.\n"
"  \n"
"  Here are the user controls:\n"
"  \n"
"  zooming:    Wheel up or arrow up with the mouse in the plot window zooms in,\n"
"              wheel down or arrow down zooms out.\n"
"  \n"
"  scrolling:  Click the scrollbar arrows, click scrollbar whitespace, click and drag the\n"
"              box, click and drag whitespace in the plot area, or use left/right arrows.\n"
"  \n"
"  values:     Hover the mouse over a point in the graph to display its value and time.\n"
"  \n"
"  markers:    Place a marker on the plot by clicking the marker number, moving the mouse\n"
"                  into plot window, then clicking again to place it where you want.\n"
"              Move a placed marker by clicking the circled marker number at the top of the line.\n"
"              Make a marker the delta time reference by doubleclicking the marker's time.\n"
"              Scroll to center a marker in the plot window by doubleclicking the marker's number.\n"
"              Scroll to the start or end of the plot by doubleclicking the L or R marker's name.\n"
"  \n"
"  copy time:  Right clicking a marker's displayed time or delta time, or right clicking\n"
"              anywhere in the plot window, copies the time to the Windows clipboard.\n"
"  \n"
"  tools/goto: Center the plot on a specified time, and puts time marker 9 there.\n"
"  \n"
"  tools/options\n"
"           'dither sampled points': Randomly choose the point to draw a line to when we're\n"
"               skipping points closer together than the screen resolution. This somewhat reduces\n"
"               the Moire effect when zoomed out on a periodic waveform, but not entirely.\n"
"           'store 16-bit integers': Store scaled signed 2-byte integers instead of 4-byte floats.\n"
"               This saves on virtual memory but, mysteriously, doesn't make plotting faster.\n"
"  \n"
"  File/Save.tbin or File/Save.csv:\n"
"             The data between markers 1 and 2 is saved into a new file of the specified format.\n"
"             If saving .tbin and the data came from a .tbin file, the original header is used.\n"
};
//*