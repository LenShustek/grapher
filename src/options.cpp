//file: options.cpp
/******************************************************************************

Display and get configuration options

See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************///

#include "grapher.h"

//*********************   tools/sampling   ********************************

int sampling = DEFAULT_SAMPLING;

LRESULT CALLBACK WndProcSampling(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   dlog("Sampling dialog: message %s\n", msgname(message, false));
   LRESULT result = true;
   switch (message) {

   case WM_INITDIALOG: {
      char msg[60];
      snprintf(msg, sizeof(msg), "sampling is currently %d", sampling);
      SetDlgItemText(hwnd, IDC_SAMPLING_MSG, msg); }
   break;

   case WM_COMMAND: {
      WORD control_id = LOWORD(wParam);
      WORD notif_code = HIWORD(wParam);
      dlog("options: notif_code %d, control_id %d\n", notif_code, control_id);
      switch (control_id) {
      case IDOK: { // OK button
         BOOL success;
         int value = GetDlgItemInt(hwnd, IDC_SAMPLING_VALUE, &success, false);
         if (success && value > 0) {
            sampling =value;
            dlog("options: set sampling to %d\n", sampling);
            char msg[50];
            snprintf(msg, sizeof(msg), "sampling set to %d", sampling);
            MessageBox(hwnd, msg, "Info", MB_TASKMODAL);
            goto close_sampling; }
         else MessageBox(hwnd, "bad number", NULL, MB_ICONHAND | MB_TASKMODAL); }
      break;

      case IDCANCEL: // Cancel button
         goto close_sampling; } }
   break; // command

   case WM_CLOSE:   // x in upper right corner
close_sampling:
      EndDialog(hwnd, 0);//destroy dialog window
      break;

   default:
      result = false; // we didn't process this message
      // result = DefWindowProc(hwnd, message, wParam, lParam); // don't do this for dialog boxes!
      // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-dlgproc
   }
   return result; }

void set_tools_sampling(void) {
   DialogBox(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDD_SAMPLING), main_ww.handle, WndProcSampling); }

//*********************   tools/goto   ********************************

LRESULT CALLBACK WndProcGoto(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   dlog("Goto dialog: message %s\n", msgname(message, false));
   LRESULT result = true;
   switch (message) {

   case WM_INITDIALOG: {
      char msg[60];
      snprintf(msg, sizeof(msg), "Enter a time between %lf and %lf",
               plotdata.timestart, plotdata.timeend );
      SetDlgItemText(hwnd, IDC_GOTO_MSG, msg); }
   break;

   case WM_COMMAND: {
      WORD control_id = LOWORD(wParam);
      WORD notif_code = HIWORD(wParam);
      dlog("Goto: notif_code %d, control_id %d\n", notif_code, control_id);
      switch (control_id) {
      case IDOK: { // OK button
         char numbuf[30];
         int nchars = GetDlgItemText(hwnd, IDC_GOTO_TIME, numbuf, sizeof(numbuf));
         double timeval;
         if (nchars && sscanf(numbuf, "%lf", &timeval)
               && timeval >= plotdata.timestart && timeval <= plotdata.timeend) {
            dlog("GOTO %lf\n", timeval);
            center_plot_on(timeval); // center the graph on this time value
            set_marker(NUM_MARKERS - 1, timeval);// place the last marker there
            RedrawWindow(plot_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            goto close_goto; }
         else MessageBox(hwnd, "bad value", NULL, MB_ICONHAND | MB_TASKMODAL); }
      break;

      case IDCANCEL: // Cancel button
         goto close_goto; } }
   break; // command

   case WM_CLOSE:   // x in upper right corner
close_goto:
      EndDialog(hwnd, 0);//destroy dialog window
      break;

   default:
      result = false; // we didn't process this message
      // result = DefWindowProc(hwnd, message, wParam, lParam); // don't do this for dialog boxes!
      // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-dlgproc
   }
   return result; }

void set_tools_goto(void) {
   DialogBox(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDD_GOTO), main_ww.handle, WndProcGoto); }

//*********************   tools/options   ********************************

LRESULT CALLBACK WndProcoptions(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   dlog("options dialog: message %s\n", msgname(message, false));
   LRESULT result = true;
   switch (message) {

   case WM_INITDIALOG: {
      CheckDlgButton(hwnd, IDC_OPTIONS_DITHER,
         plotdata.do_dither ? BST_CHECKED : BST_UNCHECKED); 
      CheckDlgButton(hwnd, IDC_OPTIONS_STORE_INTS,
         option_store_ints ? BST_CHECKED : BST_UNCHECKED); }
   break;

   case WM_COMMAND: {
      WORD control_id = LOWORD(wParam);
      WORD notif_code = HIWORD(wParam);
      dlog("options: notif_code %d, control_id %d\n", notif_code, control_id);
      switch (control_id) {
      case IDOK: { // OK button
         UINT result = IsDlgButtonChecked(hwnd, IDC_OPTIONS_DITHER);
         if (result == BST_CHECKED) plotdata.do_dither = TRUE;
         if (result == BST_UNCHECKED) plotdata.do_dither = FALSE;
         result = IsDlgButtonChecked(hwnd, IDC_OPTIONS_STORE_INTS);
         if (result == BST_CHECKED) option_store_ints = TRUE;
         if (result == BST_UNCHECKED) option_store_ints = FALSE;
         goto close_options; }
      break;

      case IDCANCEL: // Cancel button
         goto close_options; } }
   break; // command

   case WM_CLOSE:   // x in upper right corner
close_options:
      EndDialog(hwnd, 0);//destroy dialog window
      break;

   default:
      result = false; // we didn't process this message
      // result = DefWindowProc(hwnd, message, wParam, lParam); // don't do this for dialog boxes!
      // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-dlgproc
   }
   return result; }

void set_tools_options(void) {
   DialogBox(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDD_OPTIONS), main_ww.handle, WndProcoptions); }