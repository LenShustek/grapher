//file: options.cpp
/******************************************************************************

Display and get configuration options

See grapher.cpp for the consolidated change log

Copyright (C) 2018,2019,2022 Len Shustek
Released under the MIT License
******************************************************************************///

#include "grapher.h"

int sampling = DEFAULT_SAMPLING;

LRESULT CALLBACK WndProcOption(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   dlog("options: message %s\n", msgname(message, false));
   LRESULT result = false;
   switch (message) {

   case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      char msg[30];
      int msglen = snprintf(msg, sizeof(msg), "sampling = %d", sampling);
      TextOut(hdc, 10, 10, msg, msglen);
      EndPaint(hwnd, &ps); }
   result = true;
   break;

   case WM_COMMAND: {
      WORD control_id = LOWORD(wParam);
      WORD notif_code = HIWORD(wParam);
      dlog("options: notif_code %d, control_id %d\n", notif_code, control_id);
      switch (control_id) {
      case IDOK: { // OK button
         BOOL success;
         int value = GetDlgItemInt(hwnd, IDC_EDIT1, &success, false);
         if (success) {
            sampling =value;
            dlog("options: set sampling to %d\n", sampling);
            char msg[50];
            snprintf(msg, sizeof(msg), "sampling set to %d", sampling);
            MessageBox(hwnd, msg, "Info", MB_TASKMODAL); }
         else MessageBox(hwnd, "bad number", NULL, MB_ICONHAND | MB_TASKMODAL); }
      // fall into cancel to end dialog
      case IDCANCEL: // Cancel button
         EndDialog(hwnd, 0);//destroy dialog window
         //result = 1;
      } }
   result = true;
   break;

   case WM_CLOSE:   // x in upper right corner
      EndDialog(hwnd, 0);//destroy dialog window
      result = true;
      break;

      //default:
      // result = DefWindowProc(hwnd, message, wParam, lParam); // don't do this for dialog boxes!
      // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-dlgproc
   }
   return result; }

void set_option_sampling(void) {
   DialogBox(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDD_OPTION), main_ww.handle, WndProcOption); }