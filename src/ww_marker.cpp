//file: ww_marker.cpp
/******************************************************************************

Manage the right window with all the time markers

See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************///
// Marker window
//

#include "grapher.h"
#define MARKER_YSPACING 30      // vertical spacing in the marker window
#define MARKER_X_NUMBERAREA 30  // where the marker's time starts (approx)
#define MARKER_X_DELTAAREA 110  // where the marker's delta time starts (approx)
#define MARKER_CIRCLE_DIAM 20   // how big the circle is at the top of the plot window marker vertical line

struct markers_t markers[NUM_MARKERS] = {
   {"L", RGB(100,50,50) },
   {"R", RGB(100,50,50) } };
#define MARKER_LEFT 0
#define MARKER_RIGHT 1

#define MARKER_DEFAULT_COLOR RGB(50, 50, 100)
#define MARKER_BOLDEN 0x808080

int marker_reference = 0;  // which marker ndx is the delta time reference
int marker_tracked = -1;   // which marker is being tracked by the cursor in the plot window, if any

void clear_markers(void) {
   int y = 30; // leave room for title
   for (int ndx = 0; ndx < NUM_MARKERS; ++ndx) {
      markers[ndx].timeset = false;
      markers[ndx].markerww_y_coord = y;
      y += MARKER_YSPACING; }
   marker_reference = 0; }

void set_marker(int ndx, double time) {
   markers[ndx].time = time;
   markers[ndx].timeset = true; }


void show_markers(HWND hwnd) {  // show markers in the right markers windows
   char buf[40];
   char markername[20];
   char timestr[30], deltastr[30];
   PAINTSTRUCT ps;
   RECT  rect;
   HDC hdc = BeginPaint(hwnd, &ps);
   SelectFont(hdc, (HFONT)GetStockObject(ANSI_FIXED_FONT));
   SetTextCharacterExtra(hdc, -1); // narrow character spacing
   GetClientRect(hwnd, &rect);
   rect.top = 5;
   SetTextColor(hdc, LABEL_COLOR);
   DrawText(hdc, "      time           delta", -1, &rect, DT_SINGLELINE | DT_NOCLIP);
   int rc;
   int markernum = 1;
   for (int ndx = 0; ndx < NUM_MARKERS; ++ndx) {
      GetClientRect(hwnd, &rect);
      DWORD color = markers[ndx].color;
      SetTextColor(hdc, (color ? color : MARKER_DEFAULT_COLOR));
      rect.left = 5;
      rect.top = markers[ndx].markerww_y_coord;
      if (markers[ndx].name)
         snprintf(markername, sizeof(markername), "%s:", markers[ndx].name);
      else
         snprintf(markername, sizeof(markername), "%d:", markernum++);
      if (markers[ndx].timeset) {
         sprintf_s(buf, sizeof(buf), "%s %13s %14s", markername, // (only delta can be negative)
                   showtime(markers[ndx].time, timestr, sizeof(timestr)),
                   showtime(markers[ndx].time - markers[marker_reference].time, deltastr, sizeof(deltastr))); }
      else
         sprintf_s(buf, sizeof(buf), "%s", markername);
      rc = DrawText(hdc, buf, -1, &rect, DT_SINGLELINE | DT_NOCLIP); }
   EndPaint(hwnd, &ps); }

void draw_plot_markers(void) { // Draw markers in the plot window as vertical lines topped with circled numbers.
   //We use D2D1 to draw circled letters and lines into the same double-buffer that we are using to draw the plots.
   //But there is apparently no ASCII version of D2D1 DrawText (?!?), so we have to use WCHARs in this function.
   // Note: plot_ww.target->BeginDraw() has already been called.
   int markernum = 1;
   for (int ndx = 0; ndx < NUM_MARKERS; ++ndx) {
      WCHAR markername[20];
      if (markers[ndx].name)
         swprintf(markername, sizeof(markername) / 2, L"%hs", markers[ndx].name);
      else
         swprintf(markername, sizeof(markername) / 2, L"%d", markernum++);
      if (!markers[ndx].name   // a change: only do markers with a generated name, ie not R or L
            && markers[ndx].timeset  // and only those set with a time within the windows
            && markers[ndx].time >= plotdata.leftedge_time
            && markers[ndx].time <= plotdata.rightedge_time) {
         markers[ndx].visible = true;
         D2D1_SIZE_F canvas = plot_ww.target->GetSize();
         D2D1_POINT_2F top, bottom;
         top.y = MARKER_CIRCLE_DIAM;
         bottom.y = canvas.height;
         top.x = bottom.x =
                    (float)(canvas.width * (markers[ndx].time - plotdata.leftedge_time) / (plotdata.rightedge_time - plotdata.leftedge_time));
         markers[ndx].plotww_x_coord = (int)top.x;
         plot_ww.target->DrawLine(top, bottom, pRedBrush, 1.0f);
         D2D1_ELLIPSE  ellipse = D2D1::Ellipse(D2D1::Point2F(top.x, MARKER_CIRCLE_DIAM / 2), MARKER_CIRCLE_DIAM / 2, MARKER_CIRCLE_DIAM / 2);
         plot_ww.target->DrawEllipse(ellipse, pRedBrush, 0.5f);
         plot_ww.target->DrawText(markername, (UINT)wcslen(markername), pTextFormat,
                                  D2D1::RectF(top.x - MARKER_CIRCLE_DIAM / 2, 0, top.x + MARKER_CIRCLE_DIAM / 2, MARKER_CIRCLE_DIAM),
                                  pRedBrush); }
      else markers[ndx].visible = false; } }

enum { SINGLECLICK, DOUBLECLICK };  // click
enum { LEFTCLICK, RIGHTCLICK };     // button

bool marker_click(int xpos, int ypos, int click, int button) {
   // See if we clicked on a marker's number or time in the marker window
   int selected_marker = -1;
   int ndx;
   for (ndx = 0; ndx < NUM_MARKERS; ++ndx) {
      if (ypos > markers[ndx].markerww_y_coord
            && ypos < markers[ndx].markerww_y_coord + MARKER_YSPACING) {
         selected_marker = ndx;
         break; } }
   if (selected_marker == -1) {
      dlog("marker click no match at %d, %d\n", xpos, ypos);
      return false; }
   dlog("marker %d click, xPos %d\n", ndx, xpos);
   if (xpos < MARKER_X_NUMBERAREA) { // click was on the marker's number
      if (button == LEFTCLICK) { // by the left button
         if (click == DOUBLECLICK) { // if number was double clicked
            marker_tracked = -1; // cancel any tracking of this marker
            if (ndx == MARKER_LEFT) // special case for L/R: scroll to left/right of whole plot
               center_plot_on(plotdata.timestart);
            else if (ndx == MARKER_RIGHT)
               center_plot_on(plotdata.timeend);
            else { // scroll the plot so the clicked-on marker is in the center
               if (markers[ndx].timeset)
                  center_plot_on(markers[ndx].time); } }
         else { // number was single clicked
            marker_tracked = ndx;  // then start tracking this marker in the plot window
            dlog("tracking marker %d\n", ndx); } } }
   else { // click was in the time area:
      if (button == LEFTCLICK && click == DOUBLECLICK && markers[ndx].timeset) { // maybe make this the reference marker
         marker_reference = ndx;
         RedrawWindow(marker_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE); }
      else if (button == RIGHTCLICK && markers[ndx].timeset) { // right click time area: copy to clipboard
         copy_time(xpos < MARKER_X_DELTAAREA ?  markers[ndx].time // either the marker's time
                   : markers[ndx].time - markers[marker_reference].time);   // or the marker's delta time
      } }
   return true; }

void check_marker_plot_click(int xPos, int yPos) { // see if we just clicked on a marker name in the plot window
   D2D1_SIZE_F canvas = plot_ww.target->GetSize();
   for (int ndx = 0; ndx < NUM_MARKERS; ++ndx)
      if (markers[ndx].visible) {
         int xcoord = markers[ndx].plotww_x_coord;
         dlog("checking marker at x %d for click at %d, %d\n", xcoord, xPos, yPos);
         if (yPos < MARKER_CIRCLE_DIAM
               && xPos > xcoord - MARKER_CIRCLE_DIAM / 2
               && xPos < xcoord + MARKER_CIRCLE_DIAM / 2) {
            marker_tracked = ndx;  // start tracking this marker with the mouse
            dlog("starting to track marker %d\n", ndx);
            RedrawWindow(plot_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE); } } }

//
// the marker window message handler
//
LRESULT CALLBACK grapherApp::WndProcMarker(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   show_message_name(&marker_ww, message, wParam, lParam);
   LRESULT result = 0;
   switch (message) {

   case WM_CREATE:
      break;

   case WM_PAINT:
   case WM_DISPLAYCHANGE:
      show_markers(hwnd);
      break;

   case WM_SIZE: {
      UINT width = LOWORD(lParam);
      UINT height = HIWORD(lParam);
      do_resize(&marker_ww, width, height); }
   break;

   case WM_LBUTTONDOWN: {  // first left button click
      int xPos = GET_X_LPARAM(lParam);
      int yPos = GET_Y_LPARAM(lParam);
      marker_click(xPos, yPos, SINGLECLICK, LEFTCLICK); }
   break;

   case WM_LBUTTONDBLCLK: {  // double left button click
      int xPos = GET_X_LPARAM(lParam);
      int yPos = GET_Y_LPARAM(lParam);
      marker_click(xPos, yPos, DOUBLECLICK, LEFTCLICK); }
   break;

   case WM_RBUTTONDOWN: { // right button click: copy time
      int xPos = GET_X_LPARAM(lParam);
      int yPos = GET_Y_LPARAM(lParam);
      marker_click(xPos, yPos, SINGLECLICK, RIGHTCLICK); }
   break;

   default:
      result = DefWindowProc(hwnd, message, wParam, lParam); }
   --marker_ww.recursion_level;
   return result; }

void grapherApp::create_marker_window(HWND handle) {
   GetClientRect(handle, &marker_ww.canvas); // get the size of the application client window
   dlog("creating marker window within %d,%d to %d,%d\n", marker_ww.canvas.left, marker_ww.canvas.top, marker_ww.canvas.right, marker_ww.canvas.bottom);
   dlog("  x,y = %ld, %ld  width %ld,  height %ld\n",
        marker_ww.canvas.right - MARKERWINDOW_WIDTH, 0, // x,y position
        MARKERWINDOW_WIDTH, marker_ww.canvas.bottom - marker_ww.canvas.top); // width, height in pixels
   marker_ww.initialized = true; // do early so messages to main window don't cause this to be called again
   marker_ww.handle =
      CreateWindowEx(  // create the marker child window
         0, // extended styles
         "ClassMarker",  // registered class name
         "Markers", // window name
         /*WS_OVERLAPPEDWINDOW |*/  WS_CHILD /*| WS_CAPTION*/ | WS_BORDER, // style
         marker_ww.canvas.right - MARKERWINDOW_WIDTH, 0, // x,y position
         MARKERWINDOW_WIDTH, // width in pixels
         marker_ww.canvas.bottom - marker_ww.canvas.top, // height in pixels
         handle, // parent
         NULL, // menu or child
         HINST_THISCOMPONENT, // application instance
         NULL); // context
   if (marker_ww.handle) {
      clear_markers();
      // We omit WS_CAPTION and a name to have thin borders and leave maximum room for marker info
      //SetWindowTextA(marker_ww.handle, "markers");
      CreateWindowResources(&marker_ww);
      ShowWindow(marker_ww.handle, SW_SHOWNORMAL);
      UpdateWindow(marker_ww.handle);
      //BringWindowToTop(marker_ww.handle); // doesn't work
   } }


