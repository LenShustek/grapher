//file: helptext.cpp
/******************************************************************************

   Put up a window with help text


See grapher.cpp for the consolidated change log

Copyright (C) 2018,2019,2022 Len Shustek
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
      rect.top = rect.left = 30;
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



/*  Justifiable help text. Replace \r\n with \\n"\r\n"bb  to turn it into the strings below.

This is a data visualizer that displays up to 12 time-series plots on a single
chart that can be scrolled horizontally and zoomed in or out. It is designed
to be relatively efficient when there are billions of points

It can read a CSV (comma separated value) file with the first column being the uniform timestamp.
The first two lines are discarded as headers, but the number of items sets the number of plot lines.
It can also read the more compact TBIN file as defined for the readtape program.

Since our tape data tends to be smooth and well-sampled, we subsample by using
only every 3rd data point. You can change that with the options/sampling menu.

You can save a subset of the data as a CSV or TBIN file. If subsampling is on,
it will warn you about saving a lower-resolution file.

Here are the user controls:

zooming:    Wheel up with the mouse in the plot window zooms in, wheel down zooms out.

scrolling:  You can click the scrollbar arrows, click scrollbar whitespace, click and drag 
            the scrollbar box, or click and drag whitespace in the plot area.

values:     Hover the mouse over a point in the graph to display its value.

markers:    Place by clicking on a marker number and moving the mouse into plot window, then click to place.
            Make a marker the delta time reference by doubleclicking the marker time.
            Make a marker visible in the plot window by doubleclicking the marker number.

File save .tbin or .csv:  The data between markers 1 and 2 is saved into a new file.
                          If the data came from a .tbin file, that header is used.

*/
const char helptext[] = {
"\n"
"  This is a data visualizer that displays up to 12 time-series plots on a single\n"
"  chart that can be scrolled horizontally and zoomed in or out. It is designed\n"
"  to be relatively efficient when there are billions of points\n"
"  \n"
"  It can read a CSV (comma separated value) file with the first column being the uniform timestamp.\n"
"  The first two lines are discarded as headers, but the number of items sets the number of plot lines.\n"
"  It can also read the more compact TBIN file as defined for the readtape program.\n"
"  \n"
"  Since our tape data tends to be smooth and well-sampled, we subsample by using\n"
"  only every 3rd data point. You can change that with the options/sampling menu.\n"
"  \n"
"  You can save a subset of the data as a CSV or TBIN file. If subsampling is on,\n"
"  it will warn you about saving a lower-resolution file.\n"
"  \n"
"  Here are the user controls:\n"
"  \n"
"  zooming:    Wheel up with the mouse in the plot window zooms in, wheel down zooms out.\n"
"  \n"
"  scrolling:  You can click the scrollbar arrows, click scrollbar whitespace, click and drag \n"
"              the scrollbar box, or click and drag whitespace in the plot area.\n"
"  \n"
"  values:     Hover the mouse over a point in the graph to display its value.\n"
"  \n"
"  markers:    Place by clicking on a marker number and moving the mouse into plot window, then click to place.\n"
"              Make a marker the delta time reference by doubleclicking the marker time.\n"
"              Make a marker visible in the plot window by doubleclicking the marker number.\n"
"  \n"
"  File save .tbin or .csv:  The data between markers 1 and 2 is saved into a new file.\n"
"                            If the data came from a .tbin file, that header is used.\n"
};
//*