//file: grapher.cpp
/********************************************************************************

This is a data visualizer that displays up to 12 time-series plots on a single
chart that can be scrolled horizontally and zoomed in or out. It is designed
to be relatively efficient when there are billions of points.

This was motivated by needing to see the magnetic tape data that the "readtape"
program analyzes, and I wasn't able to find a program that would work well for
such large datasets. The Saleae logic analyzer software does a pretty good job,
but it requires the original very large .logicdata files to be preserved.

This program can read a CSV (comma separated value) file with the first column
being the uniformly incremented timestamp for all the plots. The first two lines
are discarded as headers, but the number of items sets the number of plot lines.

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

This is unabashedly a windows-only program for a little-endian 64-bit CPU.

Len Shustek
July 2022
*******************************************************************************
---CHANGE LOG ---
 8 Jul 2022, L. Shustek, V1.0
  first version

 */
#define VERSION "0.1"

/* TODO:
- consider dithering the choice of points to plot when we are not displaying some,
  in order to avoid moire patterns with periodic data. But needs to be fast!
- maybe add some time annotations on the top
- maybe allow plots to be reordered by dragging the names

done
- add subsampling, and an options menu to change it
- add source file headers
- ask for permission to rewrite files
- put filename on main window title
- change dlog to a decent logging system
- cleanup leftover C++ crap and #includes
 -read CSV files
- save data between markers in CSV or TBIN format
- zoom with scrollwheel centers on the cursor
- doubleclick on marker name to make it visible in the plot window
- show plot names in the left window
- grey out inappropriate file menu items

*/
/******************************************************************************
Copyright (C) 2022 Len Shustek

The MIT License (MIT): Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

#include "grapher.h"

// globals (C++ be damned!)

HINSTANCE hInst;           // current instance
grapherApp *app;           // C++ crap left over from the skeleton I stole
ID2D1Factory *pD2DFactory;
//IWICImagingFactory *m_pWICFactory;
//IDWriteFactory *m_pDWriteFactory;
//IDWriteTextFormat *m_pTextFormat;
ID2D1SolidColorBrush *pBlackBrush, *pRedBrush;
IDWriteTextFormat *pTextFormat;

struct window_t
   main_ww = { "main_ww", 0 },  // the main application window, of which the rest are children
label_ww = {"label_ww", 0 },    // the left window with plot graph labels
plot_ww = { "plot_ww", 0 },     // the big center plot window
marker_ww = { "marker_ww", 0 }, // the right window with timing marker info
vpopup_ww = { "vpopup_ww", 0 }; // the value popup when the mouse hovers in the plot window
struct plotdata_t plotdata = { 0 };

//
// Main entry
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#if DEBUG
   if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) { // allow printf to work
      freopen("CONOUT$", "w", stdout);
      freopen("CONOUT$", "w", stderr); }
   dlog("\nHello, there\n");
#endif
#if 0 // test the new version of format_memsize that produces xxxx.x
   char testbuf[30];
#define TESTMEM(val) dlog("val %llu becomes %s\n", val, format_memsize(val,testbuf))
   TESTMEM(0ULL);
   TESTMEM(512ULL);
   TESTMEM(1023ULL);
   TESTMEM(1024ULL);
   TESTMEM(1025ULL);
   TESTMEM((1024ULL + 101));
   TESTMEM((1024ULL + 102));
   TESTMEM((1024ULL + 103));
   TESTMEM(2559ULL);
   TESTMEM(2560ULL);
   TESTMEM(2561ULL);

   TESTMEM(1024ULL*512);
   TESTMEM(1024ULL*1023);
   TESTMEM(1024ULL*1024);
   TESTMEM(1024ULL*1025);
   TESTMEM(1024ULL*(1024 + 101));
   TESTMEM(1024ULL*(1024 + 102));
   TESTMEM(1024ULL*(1024 + 103));
   TESTMEM(1024ULL*2559);
   TESTMEM(1024ULL*2560);
   TESTMEM(1024ULL*2561);

   TESTMEM(1024ULL*1024*512);
   TESTMEM(1024ULL*1024*1023);
   TESTMEM(1024ULL*1024*1024);
   TESTMEM(1024ULL*1024*1025);
   TESTMEM(1024ULL*1024*(1024 + 101));
   TESTMEM(1024ULL*1024*(1024 + 102));
   TESTMEM(1024ULL*1024*(1024 + 103));
   TESTMEM(1024ULL*1024*2559);
   TESTMEM(1024ULL*1024*2560);
   TESTMEM(1024ULL*1024*2561);
#endif
   hInst = hInstance; // Store instance handle in our global variable
   // Ignore the return value because we want to run the program even in the
   // unlikely event that HeapSetInformation fails.
   HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
   if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) { // initialize COM library
      grapherApp applic;
      if (SUCCEEDED(applic.Initialize())) {
         MSG msg;
         while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg); } }
      CoUninitialize(); }
   return 0; }

//
// Initialize members.
//
grapherApp::grapherApp() :
   //m_hwnd(NULL),
   m_pD2DFactory(NULL),
   m_pDWriteFactory(NULL),
   //m_pRenderTarget(NULL),
   //m_pBlackBrush(NULL),
   m_pTextFormat(NULL) {}
//
// Release resources.
//
grapherApp::~grapherApp() {
   SafeRelease(&m_pD2DFactory);
   SafeRelease(&m_pDWriteFactory);
   //SafeRelease(&m_pRenderTarget);
   SafeRelease(&m_pTextFormat);
   //SafeRelease(&m_pBlackBrush);
}

//
// Create the application window and initializes device-independent resources.
//
HRESULT grapherApp::Initialize() {
   HRESULT hr;
   // Initialize device-indpendent resources, such
   // as the Direct2D factory.
   hr = CreateDeviceIndependentResources();
   if (SUCCEEDED(hr)) { // Register the window classes
      WNDCLASSEX windowclass = { sizeof(WNDCLASSEX) };   // main application window class
      windowclass.style         = CS_HREDRAW | CS_VREDRAW;
      windowclass.lpfnWndProc   = grapherApp::WndProc;
      windowclass.cbClsExtra    = 0;
      windowclass.cbWndExtra    = sizeof(LONG_PTR);
      windowclass.hInstance     = HINST_THISCOMPONENT;
      windowclass.hbrBackground = NULL;
      windowclass.lpszMenuName  = MAKEINTRESOURCE(IDC_GRAPHER);
      windowclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
      windowclass.lpszClassName = "ClassApp";
      RegisterClassEx(&windowclass);

      windowclass.lpfnWndProc = grapherApp::WndProcPlot;  // plot area window class
      windowclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // draws background when window is maximized
      windowclass.style = CS_HREDRAW | CS_VREDRAW;
      windowclass.lpszMenuName = NULL; // no menu
      windowclass.lpszClassName = "ClassPlot";
      RegisterClassEx(&windowclass);

      windowclass.lpfnWndProc = grapherApp::WndProcMarker;  // marker area window class
      windowclass.hbrBackground = (HBRUSH) (COLOR_WINDOW+1); // draws background when window is maximized
      windowclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
      windowclass.lpszMenuName = NULL; // no menu
      windowclass.lpszClassName = "ClassMarker";
      RegisterClassEx(&windowclass);

      windowclass.lpfnWndProc = grapherApp::WndProcLabel;  // label area window class
      windowclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // draws background when window is maximized
      windowclass.style = CS_HREDRAW | CS_VREDRAW;
      windowclass.lpszMenuName = NULL; // no menu
      windowclass.lpszClassName = "ClassLabel";
      RegisterClassEx(&windowclass);

      windowclass.lpfnWndProc = grapherApp::WndProcVpopup;  // voltage popup window class
      windowclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // draws background when window is maximized
      windowclass.style = CS_HREDRAW | CS_VREDRAW;
      windowclass.lpszMenuName = NULL; // no menu
      windowclass.lpszClassName = "ClassVpopup";
      RegisterClassEx(&windowclass);

      FLOAT dpiX, dpiY;
      pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);  // get DPI
      dlog("dpiX %f, dpiY %f\n", dpiX, dpiY);
      dlog("sizeof(ptr)=%d\n", (int)sizeof(&dpiX));
      //NB: "Before returning, CreateWindow sends a WM_CREATE message to the window procedure."
      main_ww.handle = CreateWindowEx(  // create the main application window
                          0, // extended styles
                          "ClassApp",  // registered class name
                          "grapher", // window name
                          WS_OVERLAPPEDWINDOW, // style
                          CW_USEDEFAULT, CW_USEDEFAULT, // x,y position
                          static_cast<UINT>(ceil(640.f * dpiX / 96.f)), // width, height: scale from DPI to pixels
                          static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
                          NULL, // parent
                          NULL, // menu or child
                          HINST_THISCOMPONENT, // application instance
                          this ); // context
      dlog("created main window handle %llX\n", (uint64_t) main_ww.handle);
      app = this;
      hr = main_ww.handle ? S_OK : E_FAIL;
      if (SUCCEEDED(hr)) {
         main_ww.initialized = true;
         CreateWindowResources(&main_ww);
         ShowWindow(main_ww.handle, SW_SHOWMAXIMIZED);
         UpdateWindow(main_ww.handle); } }
   return hr; }

// Create resources which are not bound
// to any device. Their lifetime effectively extends for the
// duration of the app. These resources include the Direct2D and
// DirectWrite factories,  and a DirectWrite Text Format object
// (used for identifying particular font characteristics).

HRESULT grapherApp::CreateDeviceIndependentResources() {
   static const WCHAR msc_fontName[] = L"Verdana";
   static const FLOAT msc_fontSize = 12;
   HRESULT hr;
   ID2D1GeometrySink *pSink = NULL;
   // Create a Direct2D factory.
   hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
   if (SUCCEEDED(hr)) {
      // Create a DirectWrite factory.
      hr = DWriteCreateFactory(
              DWRITE_FACTORY_TYPE_SHARED,
              __uuidof(m_pDWriteFactory),
              reinterpret_cast<IUnknown **>(&m_pDWriteFactory)); }
   if (SUCCEEDED(hr)) {
      // Create a DirectWrite text format object.
      hr = m_pDWriteFactory->CreateTextFormat(
              msc_fontName, NULL,
              DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
              msc_fontSize,
              L"", //locale
              &pTextFormat); }
   if (SUCCEEDED(hr)) {
      // Center the text horizontally and vertically.
      pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
      pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER); }
   SafeRelease(&pSink);
   return hr; }

// Create window-specific resources

HRESULT grapherApp::CreateWindowResources(struct window_t *ww) {
   HRESULT hr = 0;
   dlog("CreateWindowResources(%s), handle %llX, target %llX\n",
        ww->name, (uint64_t)ww->handle, (uint64_t)ww->target);
   if (!ww->target) { // not done yet
      RECT rc;
      GetClientRect(ww->handle, &rc);
      dlog("  left %ld, top %ld, right %ld, bottom %ld, width %ld, height %ld\n",
           rc.left, rc.top, rc.right, rc.bottom, rc.right - rc.left, rc.bottom - rc.top);
      D2D1_SIZE_U size = D2D1::SizeU(
                            rc.right - rc.left,
                            rc.bottom - rc.top);
      // Create a Direct2D render target.
      hr = pD2DFactory->CreateHwndRenderTarget(
              D2D1::RenderTargetProperties(),
              D2D1::HwndRenderTargetProperties(ww->handle, size),
              &ww->target);
      dlog("created target %llX, err %08X\n", (uint64_t)ww->target, hr);
      if (SUCCEEDED(hr)) {
         // Create colored brushes
         hr = ww->target->CreateSolidColorBrush(
                 D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush);
         hr = ww->target->CreateSolidColorBrush(
                 D2D1::ColorF(D2D1::ColorF::Red), &pRedBrush); } }
   return hr; }

//  Discard device-specific resources which need to be recreated
//  when a Direct3D device is lost

void grapherApp::DiscardDeviceResources() {
   //SafeRelease(&m_pRenderTarget);
   //SafeRelease(&m_pBlackBrush);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   UNREFERENCED_PARAMETER(lParam);
   switch (message) {
   case WM_INITDIALOG:
      return (INT_PTR)TRUE;
   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
         EndDialog(hDlg, LOWORD(wParam));
         return (INT_PTR)TRUE; }
      break; }
   return (INT_PTR)FALSE; }

//
//  Process a WM_SIZE message and resize the render target appropriately.
//
void do_resize(struct window_t *ww, UINT width, UINT height) {
   if (ww->target) {
      D2D1_SIZE_U size;
      size.width = width;
      size.height = height;
      // Note: This method can fail, but it's okay to ignore the
      // error here -- it will be repeated on the next call to EndDraw.
      ww->target->Resize(size);
      GetClientRect(ww->handle, &ww->canvas); // get the size of the application client window
      dlog("%s window resize: %d,%d to %d,%d\n", ww->name, ww->canvas.left, ww->canvas.top, ww->canvas.right, ww->canvas.bottom); } }

//
// The main application window message handler.
//
LRESULT CALLBACK grapherApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   LRESULT result = 0;
   //if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) { // allow printf to work
   //   freopen("CONOUT$", "w", stdout);
   //   freopen("CONOUT$", "w", stderr); }
   show_message_name(&main_ww, message, wParam, lParam);

   if (message == WM_CREATE) {
      LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
      grapherApp *pgrapherApp = (grapherApp *)pcs->lpCreateParams;
      ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pgrapherApp) );
      result = 1; }
   else {
      grapherApp *pgrapherApp = reinterpret_cast<grapherApp *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
      if (main_ww.initialized) {
         if (!plot_ww.initialized) create_plot_window(hwnd);
         if (!marker_ww.initialized) create_marker_window(hwnd);
         if (!label_ww.initialized) create_label_window(hwnd); }
      if (pgrapherApp) {
         switch (message) {

         case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) { // Parse the menu selections:
            case IDM_ABOUT:
               char msg[100];
               snprintf(msg, sizeof(msg), "Version %s\nCopyright (c) 2022, Len Shustek", VERSION);
               MessageBox(NULL, msg, "About", 0);
               //DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
               break;
            case IDM_EXIT:
               DestroyWindow(hwnd);
               break;
            case ID_HELP_HELP:
               showhelp();
               break;
            case ID_OPTIONS_SAMPLING:
               set_option_sampling();
               break;
            case ID_FILE_OPEN:
               open_file(hwnd);
               break;
            case ID_FILE_CLOSEDATA:
               discard_data();
               break;
            case ID_FILE_SAVE_TBIN:
               save_file(hwnd, DATA_TBIN);
               break;
            case ID_FILE_SAVE_CSV:
               save_file(hwnd, DATA_CSV);
               break; } } // close switch(command)
         break;

         case WM_SIZE: {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            do_resize(&main_ww, width, height);
            //***** gotta fix this crap: positions and size are in too many places!
            if (plot_ww.initialized)
               SetWindowPos(plot_ww.handle, HWND_TOP,
                            LABELWINDOW_WIDTH, 0, // x,y position
                            width - LABELWINDOW_WIDTH - MARKERWINDOW_WIDTH, height, SWP_NOZORDER);
            if (marker_ww.initialized)
               SetWindowPos(marker_ww.handle, HWND_TOP,
                            width - MARKERWINDOW_WIDTH, 0, // x,y position
                            MARKERWINDOW_WIDTH, height, SWP_NOZORDER);
            if (label_ww.initialized)
               SetWindowPos(label_ww.handle, HWND_TOP,
                            0, 0, // x,y position
                            LABELWINDOW_WIDTH, height, SWP_NOZORDER); }
         break;

         case WM_PAINT:
         case WM_DISPLAYCHANGE: {
            static bool painted = false; // TEMP: paint only once
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            //if (!painted) pgrapherApp->OnRender();
            EndPaint(hwnd, &ps);
            painted = true; }
         break;

         case WM_SETFOCUS:  // if we get focus, turn it back to plot window  (need to do others too?)
            SetFocus(plot_ww.handle);
            break;

         case WM_DESTROY:
            PostQuitMessage(0);
            result = 1;
            break;

         default:
            result = DefWindowProc(hwnd, message, wParam, lParam); } }
      else result = DefWindowProc(hwnd, message, wParam, lParam); } // !pgrapherApp
   --main_ww.recursion_level;
   return result; }
//*