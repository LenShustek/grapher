//file: ww_labels.cpp
/******************************************************************************

Manage the left window with the plot labels

See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************///

#include "grapher.h"

void show_labels(HWND hwnd) {  // show labels in the left window
   char labelname[20];
   PAINTSTRUCT ps;
   HDC hdc = BeginPaint(hwnd, &ps);
   int plotheight = (label_ww.canvas.bottom - label_ww.canvas.top) / plotdata.nseries;
   plotheight -=1;  // ??? experimentally deduced correction for proper vertical alignment
   int ycoord = plotheight / 2 - 10; // ditto for -x
   int rc;
   dlog("in show_labels, valid_data  = %d, nseries = %d\n", plotdata.data_valid, plotdata.nseries);
   if (plotdata.data_valid) for (int ndx = 0; ndx < plotdata.nseries; ++ndx) {
         RECT  rect;
         GetClientRect(hwnd, &rect);
         SetTextColor(hdc, LABEL_COLOR);
         rect.left = 5;
         rect.top = ycoord;
         strncpy_s(labelname, plotdata.labels[ndx], sizeof(labelname));
         rc = DrawText(hdc, labelname, -1, &rect, DT_TOP | DT_SINGLELINE | DT_NOCLIP);
         ycoord += plotheight; }
   EndPaint(hwnd, &ps); }

//
// the label window message handler
//
LRESULT CALLBACK grapherApp::WndProcLabel(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   show_message_name(&label_ww, message, wParam, lParam);
   LRESULT result = 0;
   switch (message) {

   case WM_CREATE:
      break;

   case WM_PAINT:
   case WM_DISPLAYCHANGE: {
      //PAINTSTRUCT ps;
      //BeginPaint(hwnd, &ps);
      show_labels(hwnd);
      //EndPaint(hwnd, &ps);
   }
   break;

   case WM_SIZE: {
      UINT width = LOWORD(lParam);
      UINT height = HIWORD(lParam);
      do_resize(&label_ww, width, height); }
   break;

   case WM_LBUTTONDOWN: {
      int xPos = GET_X_LPARAM(lParam);
      int yPos = GET_Y_LPARAM(lParam);
      //if (label_click(xPos, yPos))
      //   RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
   }
   break;

   default:
      result = DefWindowProc(hwnd, message, wParam, lParam); }
   --label_ww.recursion_level;
   return result; }


void grapherApp::create_label_window(HWND handle) {
   GetClientRect(handle, &label_ww.canvas); // get the size of the application client window
   dlog("creating label window within %d,%d to %d,%d\n", label_ww.canvas.left, label_ww.canvas.top, label_ww.canvas.right, label_ww.canvas.bottom);
   dlog("  x,y = %ld, %ld  width %ld,  height %ld\n",
          0, 0, // x,y position
          LABELWINDOW_WIDTH, label_ww.canvas.bottom - label_ww.canvas.top); // width, height in pixels
   label_ww.initialized = true; // do early so messages to main window don't cause this to be called again
   label_ww.handle = CreateWindowEx(  // create the label child window
                        0, // extended styles
                        "ClassLabel",  // registered class name
                        "Labels", // window name
                        /*WS_OVERLAPPEDWINDOW |*/  WS_CHILD /*| WS_CAPTION*/ | WS_BORDER, // style
                        0, 0, // x,y position
                        LABELWINDOW_WIDTH, // width in pixels
                        label_ww.canvas.bottom - label_ww.canvas.top, // height in pixels
                        handle, // parent
                        NULL, // menu or child
                        HINST_THISCOMPONENT, // application instance
                        NULL); // context
   if (label_ww.handle) {
      // WS_CAPTION doesn't leave enough room for labels when the window is small
      //SetWindowTextA(label_ww.handle, "plots");  
      CreateWindowResources(&label_ww);
      ShowWindow(label_ww.handle, SW_SHOWNORMAL);
      UpdateWindow(label_ww.handle);
      //BringWindowToTop(label_ww.handle); // doesn't work
   } }
//*