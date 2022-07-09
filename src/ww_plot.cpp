//file: ww_plot.cpp
/******************************************************************************

Generate the center plot window, and handle zoooming and scrolling


See grapher.cpp for the consolidated change log

Copyright (C) 2018,2019,2022 Len Shustek
Released under the MIT License
******************************************************************************/

#include "grapher.h"

#if 0
void drawsine(int ncycles, int npts, int startpt, int endpt, D2D1_SIZE_F targetsize) {
   D2D1_POINT_2F prevpoint;
   dlog("drawsine: %d pts from %d to %d in width %lf height %lf\n", npts, startpt, endpt, targetsize.width, targetsize.height);
   prevpoint.x = 0;
   prevpoint.y = targetsize.height / 2;
   if (endpt > npts) endpt = npts;
   for (int pt = startpt; pt < endpt; ++pt) {
      float sinx = (float)sin((2 * 3.1415f*(float)pt * (float)ncycles) / (float)npts); // actual sine wave value
      D2D1_POINT_2F nextpoint;
      nextpoint.x = (float)targetsize.width * (float)(pt - startpt) / (endpt - startpt);
      nextpoint.y = (targetsize.height / 2) * (1 + sinx * pt / npts);  // gradually increasing sine wave
      //dlog("sinx %f, line from %f, %f to %f, %f\n", sinx, prevpoint.x, prevpoint.y, nextpoint.x, nextpoint.y);
      plot_ww.target->DrawLine(
         prevpoint, nextpoint,
         pBlackBrush, 1.0f);
      prevpoint = nextpoint; } }
#endif

#if 0
HRESULT grapherApp::drawplots() {
   HRESULT hr;
   if (!(plot_ww.target->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)) {
      static const char sc_helloWorld[] = "Hello, World!";
      dlog("drawplots()\n");
      // Retrieve the size of the render target.
      D2D1_SIZE_F canvas = plot_ww.target->GetSize();
      plot_ww.target->BeginDraw();
      plot_ww.target->SetTransform(D2D1::Matrix3x2F::Identity());
      plot_ww.target->Clear(D2D1::ColorF(D2D1::ColorF::White));
      plot_ww.target->DrawText(
         sc_helloWorld,
         ARRAYSIZE(sc_helloWorld) - 1,
         m_pTextFormat,
         D2D1::RectF(0, 0, canvas.width, canvas.height),
         pBlackBrush);
#if 0
#define NUMX 10
#define NUMY 10
      for (int ix = 0; ix < NUMX; ++ix)
         for (int iy = 0; iy < NUMY; ++iy)
            plot_ww.target->DrawLine(
               D2D1::Point2F(0.0f, 0.0f),
               D2D1::Point2F(1200.f*ix / NUMX, 800.f*iy / NUMY),
               pBlackBrush, 0.1f);
#endif
      SCROLLINFO si;
      si.cbSize = sizeof(si);
      si.fMask = SIF_ALL;
      GetScrollInfo(plot_ww.handle, SB_HORZ, &si);  // get scrollbar info
#define NCYCLES 1000
#define NPOINTS 1000000
      int startpt = si.nPos/*0..100*/ * NPOINTS / 100;
      if (si.nPage == 0) si.nPage = 1;
      int numpts = si.nPage/*0..100*/ * NPOINTS / 100;
      //dlog("canvas.width, height = %lf, %lf\n", canvas.width, canvas.height);
      drawsine(NCYCLES, NPOINTS, startpt, startpt + numpts, canvas);
      hr = plot_ww.target->EndDraw();
      if (hr == D2DERR_RECREATE_TARGET) {
         hr = S_OK;
         DiscardDeviceResources(); } }
   return hr; }
#endif

// For efficiency, create an index of the first points in each block so we can search for a point
// without causing thrashing because the data blocks have to be paged in as we search the linked list
void make_block_index(void) {
   assert(plotdata.blkindex = (struct blkindex_t *) malloc(plotdata.numblks * sizeof(struct blkindex_t)), "can't allocate block index");
   int ndx = 0;
   for (struct datablk_t *blkptr = plotdata.datahead; blkptr; blkptr = blkptr->next) {
      plotdata.blkindex[ndx].firstnum = blkptr->firstnum;
      plotdata.blkindex[ndx].count = blkptr->count;
      plotdata.blkindex[ndx++].blk = blkptr; } }

void get_point(uint64_t pt) { // set curpt/curblk to the desired point
   for (int ndx = 0; ndx < plotdata.numblks; ++ndx) {
      if (pt >= plotdata.blkindex[ndx].firstnum && pt < plotdata.blkindex[ndx].firstnum + plotdata.blkindex[ndx].count) {
         plotdata.curblk = plotdata.blkindex[ndx].blk;
         plotdata.curpt = pt;
         plotdata.curndx = (int)(plotdata.curpt - plotdata.curblk->firstnum) * plotdata.nseries;
         return; } }
   assert(false, "can't find point %lld out of %lld\n", pt, plotdata.nvals); }

void get_nextpoint(void) { // get the point following the current point
   if (++plotdata.curpt < plotdata.curblk->firstnum + plotdata.curblk->count)
      plotdata.curndx += plotdata.nseries;
   else {
      plotdata.curblk = plotdata.curblk->next; // move to next block
      plotdata.curndx = 0;
      dassert(plotdata.curblk, "can't find next point %lld", plotdata.curpt);
      dassert(plotdata.curblk->firstnum == plotdata.curpt,
              "first point in next block is %lld, not %lld", plotdata.curblk->firstnum, plotdata.curpt); } }

/* The plot graphing routine use the newer D2D1 "Direct2D" API to draw lots of lines, in an
   attempt to use hardware-accelerated immediate-mode graphics that are executed on the graphics card.
   I have no idea whether that is actually working.

   In any case we reduce the number of lines to be drawn by skipping points when the number is greater than the
   screen resolution. We draw a thin vertical line representing the min and max points we skipped as a visual
   indicator that the graph is "fuzzy" at that resolution. It clears up as you zoom in.

   When you zoom really closely so that there are fewer points shown, we draw circles at the actual points. */

#define DRAW_MINMAX true
#define PLOTDOT_SIZE 4

HRESULT grapherApp::drawplots() {
   HRESULT hr = 0;
   if (plotdata.data_valid && !(plot_ww.target->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED)) {
      // Retrieve the size of the render target.
      D2D1_SIZE_F canvas = plot_ww.target->GetSize();
      float height = canvas.height;
      float width = canvas.width;
      dlog("drawplots: canvas width %f, height %f\n", width, height);
      plot_ww.target->BeginDraw();
      plot_ww.target->SetTransform(D2D1::Matrix3x2F::Identity());
      plot_ww.target->Clear(D2D1::ColorF(D2D1::ColorF::White));
      SCROLLINFO si;
      si.cbSize = sizeof(si);
      si.fMask = SIF_ALL;
      GetScrollInfo(plot_ww.handle, SB_HORZ, &si);  // get scrollbar info
      uint64_t startpt = si.nPos * plotdata.nvals / si.nMax;
      if (startpt >= plotdata.nvals) startpt = plotdata.nvals - 1;
      if (si.nPage == 0) si.nPage = 1; // minimum scroll box size
      uint64_t numpts = plotdata.nvals * si.nPage / si.nMax;
      uint64_t endpt = startpt + numpts - 1;
      if (endpt >= plotdata.nvals) { // don't go beyond available data
         endpt = plotdata.nvals - 1;
         numpts = endpt - startpt + 1; }
      // need to do some edge case checks here for small nvals
      uint64_t skip = numpts / /*2500*/ (int)width  + 1; // if too many points, start skipping some
      bool plotpoints = width / numpts > PLOTDOT_SIZE; // plot dots at the points if there is space between them
      dlog("plotting every %llu of %llu points out of %llu (%llu to %llu) for %d series in window size %f by %f\n",
           skip, numpts, plotdata.nvals, startpt, endpt, plotdata.nseries, width, height);
      float plotheight = height / plotdata.nseries;  // height of each plot
      float plotheight_div_2 = plotheight / 2;
      D2D1_POINT_2F prev_coords[MAXSERIES];
      D2D1_POINT_2F max_coords[MAXSERIES], min_coords[MAXSERIES];
      D2D1_ELLIPSE ellipse = { 0, 0, PLOTDOT_SIZE/2, PLOTDOT_SIZE/2 };
      get_point(startpt); // find the starting point
      for (int gr = 0; gr < plotdata.nseries; ++gr) { // draw zero-lines
         float midy = plotheight * gr + plotheight_div_2;
         plot_ww.target->DrawLine(
            D2D1::Point2F(0.0f, midy), D2D1::Point2F(width, midy), pRedBrush, 0.5f);
         prev_coords[gr].x = 0; prev_coords[gr].y = midy;
         max_coords[gr].y = 0;  min_coords[gr].y = 1.e4; }
      plotdata.leftedge_time = (double)(plotdata.timestart_ns + plotdata.timedelta_ns * startpt) / 1e9;
      plotdata.rightedge_time = (double)(plotdata.timestart_ns + plotdata.timedelta_ns * endpt) / 1e9;
      dlog("  leftedge time %f, rightedge time %f\n", plotdata.leftedge_time, plotdata.rightedge_time);
      set_marker(0, plotdata.leftedge_time);
      set_marker(1, plotdata.rightedge_time);
      if (numpts > 5000000) { // if this is going to take a while
         HCURSOR hourglass = LoadCursor(NULL, IDC_WAIT);
         SetCursor(hourglass); }
      for (uint64_t pt = startpt; ; ++pt) { // **** for all the points: be efficient here!
         D2D1_POINT_2F next_coords;
         next_coords.x = width * (float)(pt - startpt) / numpts;
#if !DRAW_MINMAX
         if ((pt - startpt) % skip == 0)
#endif
            for (int gr = 0; gr < plotdata.nseries; ++gr) { // for each plot series
               float dataval = plotdata.curblk->data[plotdata.curndx+gr]; // -plotdata.maxval .. +plotdata.maxval
               //assert(dataval <= plotdata.maxval, "data value %f bigger than max %f\n", dataval, plotdata.maxval);
               float midy = plotheight * gr + plotheight_div_2;
               next_coords.y = midy - plotheight_div_2 * dataval / plotdata.maxval;
               dassert(next_coords.y >= midy - plotheight_div_2,
                       "y coord %f too small at point %llu gr %d ndx %d value %f\n",
                       next_coords.y, plotdata.curpt, gr, plotdata.curndx, dataval);
               if ((pt-startpt) % skip != 0) { // if we're skipping this data point for performance (never true if !MINMAX)
                  // accumulate min and max of what we're skipping, to draw as one vertical line
                  if (max_coords[gr].y < next_coords.y) max_coords[gr].y = next_coords.y;
                  if (min_coords[gr].y > next_coords.y) min_coords[gr].y = next_coords.y; }
               else { // doing this data point
                  plot_ww.target->DrawLine(prev_coords[gr], next_coords, pBlackBrush, 1.0f);
                  if (DRAW_MINMAX && skip > 1 && pt > startpt) { // draw vertical line between the points representing skipped point
                     max_coords[gr].x = min_coords[gr].x = (prev_coords[gr].x + next_coords.x) / 2;
                     //dlog("drawing minmax line for pt %llu gr %d at %f from %f to %f, prev.y %f, next.y %f\n",
                     //       pt, gr, max_coords[gr].x, min_coords[gr].y, max_coords[gr].y, prev_coords[gr].y, next_coords.y);
                     plot_ww.target->DrawLine(min_coords[gr], max_coords[gr], pBlackBrush, 0.5f); }
                  // initialize min/max for skipped points. Requires (pt-startpt)%skip == 0 when pt==startpt
                  max_coords[gr].y = min_coords[gr].y = next_coords.y;
                  if (plotpoints) {
                     ellipse.point.x = next_coords.x;
                     ellipse.point.y = next_coords.y;
                     plot_ww.target->DrawEllipse(ellipse, pBlackBrush, 1.0, NULL); }
                  prev_coords[gr] = next_coords; } }
         if (pt >= endpt) break;
         get_nextpoint(); };
      draw_markers();
      hr = plot_ww.target->EndDraw();
      // ... all sorts of things that didn't work, in various combinations, to repaint the marker window:
      //SendMessage(marker_ww.handle, WM_CHILDACTIVATE, 0, 0);
      //HDC DeviceContext = GetDC(marker_ww.handle);
      //SendMessage(marker_ww.handle, WM_ERASEBKGND, (WPARAM)DeviceContext, 0L);
      //RECT mr;
      //GetWindowRect(marker_ww.handle, &mr);
      //SendMessage(marker_ww.handle, WM_SIZE, SIZE_RESTORED, MAKELPARAM(mr.right - mr.left, mr.bottom - mr.top));
      //SendMessage(marker_ww.handle, WM_PAINT, 0, 0L);
      // see https://blog.katastros.com/a?ID=00400-02fde0a9-257d-4427-8761-4d789c3b7d1d
      // see https://blog.birost.com/a?ID=00100-d4acc7a0-756e-481f-8a25-8ac46444cd79
      RedrawWindow(marker_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE); // finally: THIS WORKS!
      if (hr == D2DERR_RECREATE_TARGET) {
         hr = S_OK;
         DiscardDeviceResources(); } }
   return hr; }

void center_plot_on(double time) { // center the plot on "time"
   double timestart = (double)plotdata.timestart_ns / 1e9;
   double timeend = timestart + (double)plotdata.timedelta_ns / 1e9 * plotdata.nvals;
   if (time >= timestart && time < timeend) {
      uint64_t centerpoint = // which point number we want centered
         (uint64_t)((time - timestart)*1e9) / plotdata.timedelta_ns;
      SCROLLINFO si;
      si.cbSize = sizeof(si);
      si.fMask = SIF_ALL;
      GetScrollInfo(plot_ww.handle, SB_HORZ, &si);  // get scrollbar info
      if (si.nPage == 0) si.nPage = 1; // minimum scroll box size
      uint64_t numpts = plotdata.nvals * si.nPage / si.nMax; // number of points we plot in the window
      if (centerpoint < numpts / 2) si.nPos = 0;
      else {
         uint64_t startpt = centerpoint - numpts / 2; // the first point we want plotted
         si.nPos = (int) (startpt * si.nMax / plotdata.nvals);
         dlog("centering time %f pt %llu, showing %llu pts in the window starting with %llu, so nPos is %d\n",
              time, centerpoint, numpts, startpt, si.nPos); //
      }
      si.fMask = SIF_POS;
      SetScrollInfo(plot_ww.handle, SB_HORZ, &si, true);
      RedrawWindow(plot_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE); } }

static bool vpopup_showing = false; // are we showing a voltage popup window?

void vpopup_close(void) {
   if (vpopup_showing) { // if there's already a popup window
      dlog("closing vpopup window\n");
      //CloseWindow(vpopup_ww.handle);
      ShowWindow(vpopup_ww.handle, SW_HIDE);
      DestroyWindow(vpopup_ww.handle);
      vpopup_ww.handle = NULL;
      vpopup_showing = false; } }

bool check_mouseon_graph(int xPos, int yPos) { // has mouse moved onto a graph line?
   vpopup_close();
   D2D1_SIZE_F canvas = plot_ww.target->GetSize();
   float height = canvas.height;
   float width = canvas.width;
   float plotheight = height / plotdata.nseries;  // height of each plot
   //dlog("check_mouseon_graph(%d, %d), canvas %f x %f, plotheight %f\n", xPos, yPos, width, height, plotheight);
   double pttime = plotdata.leftedge_time + (float(xPos) / width) * (plotdata.rightedge_time - plotdata.leftedge_time);
   uint64_t samplenum = (uint64_t)((pttime - (double)plotdata.timestart_ns/1e9) / ((double)plotdata.timedelta_ns/1e9));
   //dlog("  samplenum %llu from pttime %f, timestart %f, timedelta %f\n",
   //       samplenum, pttime, (double)plotdata.timestart_ns / 1e9, (double)plotdata.timedelta_ns / 1e9);
   assert(samplenum >= 0 && samplenum < plotdata.nvals, "check_mouseon_graph: bad samplenum %llu for time %f\n", samplenum, pttime);
   get_point(samplenum);
   int ptnum = (int)(samplenum - plotdata.curblk->firstnum); // which of the groups in the block, each with data for all the series
   for (int gr = 0; gr < plotdata.nseries; ++gr) { // for each plot series
      int ndx = ptnum * plotdata.nseries /* get to the right group */ + gr /* then to the value for this plot */;
      //dlog("  pttim %f, samplenum %llu, blk starts at %llu, ptnum %d ndx %d\n", pttime, samplenum, plotdata.curblk->firstnum, ptnum, ndx);
      assert(ndx >= 0 && ndx < NDATABLK * plotdata.nseries,
             "check_mouseon_graph: bad ndx %d to data\n", ndx);
      float dataval = plotdata.curblk->data[ndx]; // -plotdata.maxval .. +plotdata.maxval
      assert(dataval <= plotdata.maxval, "check_mouseon_graph: data value %f bigger than max %f\n", dataval, plotdata.maxval);
      float midy = plotheight * gr + plotheight / 2;
      float y_coord = midy - plotheight / 2 * dataval / plotdata.maxval; // where the point was plotted
      //dlog("  plot %d dataval %f of max %f plotted at y %f, centerline %f\n", gr, dataval, plotdata.maxval, y_coord, midy);
      assert(y_coord >= midy - plotheight / 2,
             "check_mouseon_graph: y coord %f too small at point %llu gr %d ptnum %d ndx %d value %f\n",
             y_coord, plotdata.curpt, gr, ptnum, ndx, dataval);
#define MOUSE_FUZZ 10 // how close, in pixels, we need to be to the graph line
      if (yPos > y_coord - MOUSE_FUZZ
            && yPos < y_coord + MOUSE_FUZZ) {
         dlog("creating vpopup for graph %d at %d,%d\n", gr, xPos, yPos);
         //ShowWindow(vpopup_ww.handle, SW_SHOWNORMAL);
         //UpdateWindow(vpopup_ww.handle); //
         vpopup_ww.handle = CreateWindowEx(WS_EX_STATICEDGE, //WS_EX_CLIENTEDGE,
                                           "ClassVpopup", "Vpopup", WS_POPUP | WS_BORDER,
                                           xPos, yPos, 100, 25,
                                           /*plot_ww.handle*/ NULL, (HMENU)0, HINST_THISCOMPONENT, NULL);
         ShowWindow(vpopup_ww.handle, SW_SHOW);
         HDC hdc = GetDC(vpopup_ww.handle);
         RECT  rect = {0, 0, 100, 25 };
         char value[20];
         snprintf(value, sizeof(value), "%f", dataval);
         DrawText(hdc, value, -1, &rect, DT_SINGLELINE | DT_NOCLIP | DT_CENTER | DT_VCENTER);
         vpopup_showing = true; } }
   return vpopup_showing; }

LRESULT CALLBACK grapherApp::WndProcVpopup(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   LRESULT result = 0;
   show_message_name(&plot_ww, message, wParam, lParam);
   result = DefWindowProc(hwnd, message, wParam, lParam);
   --plot_ww.recursion_level;
   return result; }

void zoom(struct window_t *ww, enum zoom_t type, bool center_on_cursor) {
   SCROLLINFO si;
   si.cbSize = sizeof(si);
   si.fMask = SIF_ALL;
   GetScrollInfo(ww->handle, SB_HORZ, &si);
   int old_nPage = si.nPage;
   dlog("zoom %d: scroll box pos %d, page size %d", type, si.nPos, si.nPage);
   if (type == ZOOM_IN) {
      if (si.nPage >= 2) si.nPage = si.nPage * 2 / 3; }
   else if (type == ZOOM_OUT) {
      if (si.nPage < 2) si.nPage = 2;
      else if (si.nPage < (unsigned)si.nMax * 2 / 3) si.nPage = si.nPage * 3 / 2;
      else si.nPage = si.nMax; }
   dlog(" changed to %d\n", si.nPage);
   if (center_on_cursor) {
      dlog("change nPos from %d", si.nPos);
      si.nPos -= ww->mousex * ((int)si.nPage - old_nPage) / (ww->canvas.right - ww->canvas.left);
      dlog(" to %d with mouse at %d out of %d\n", si.nPos, ww->mousex, ww->canvas.right - ww->canvas.left);
      si.fMask = SIF_PAGE | SIF_POS; }
   else si.fMask = SIF_PAGE;
   SetScrollInfo(ww->handle, SB_HORZ, &si, true);
   SendMessage(ww->handle, WM_PAINT, 0, 0L); }

// the plot window message handler

LRESULT CALLBACK grapherApp::WndProcPlot(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   show_message_name(&plot_ww, message, wParam, lParam);
   LRESULT result = 0;
   if (plotdata.data_valid) switch (message) {

      //case WM_CREATE:
      //   break;

      case WM_PAINT:
      case WM_DISPLAYCHANGE: {
         PAINTSTRUCT ps;
         BeginPaint(hwnd, &ps);
         app->drawplots();
         EndPaint(hwnd, &ps); }
      break;

      case WM_KEYDOWN: {  // control scroll box from keystrokes
         // See https://docs.microsoft.com/en-us/windows/win32/controls/create-a-keyboard-interface-for-standard-scroll-bars
         WORD wScrollNotify = 0xffff;
         switch (wParam) {
         case VK_UP:
            zoom(&plot_ww, ZOOM_IN, false);
            break;
         case VK_DOWN:
            zoom(&plot_ww, ZOOM_OUT, false);
            break;
         case VK_LEFT:
            wScrollNotify = SB_LINELEFT;
            break;
         case VK_RIGHT:
            wScrollNotify = SB_LINERIGHT;
            break;
         case VK_HOME:
            wScrollNotify = SB_TOP;
            break;
         case VK_END:
            wScrollNotify = SB_BOTTOM;
            break; }
         if (wScrollNotify != 0xffff)
            SendMessage(hwnd, WM_HSCROLL, MAKELONG(wScrollNotify, 0), 0L); }
      break;

      case WM_HSCROLL:
         // see https://docs.microsoft.com/en-us/windows/win32/controls/about-scroll-bars
         SCROLLINFO si;
         si.cbSize = sizeof(si);
         si.fMask = SIF_ALL;
         GetScrollInfo(hwnd, SB_HORZ, &si);
         int scroll_amount;
         switch (LOWORD(wParam)) {
         case SB_LINELEFT:  // User clicked the left arrow.
            scroll_amount = 1;
            break;
         case SB_LINERIGHT:
            scroll_amount = -1;// User clicked the right arrow.
            break;
         case SB_PAGELEFT: // User clicked the scroll bar shaft left of the scroll box.
            scroll_amount = si.nPage;
            break;
         case SB_PAGERIGHT:// User clicked the scroll bar shaft right of the scroll box.
            scroll_amount = -(int)si.nPage;
            break;
         case SB_THUMBTRACK: // User dragged the scroll box.
            scroll_amount = si.nPos - si.nTrackPos;
            break;
         default:
            scroll_amount = 0;
            break; }
         si.fMask = SIF_POS;
         si.nPos -= scroll_amount;
         SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
         ScrollWindow(hwnd, scroll_amount, 0, NULL, NULL);
         result = 0;
         break;

      case WM_MOUSEMOVE: { // if moving the mouse in this window
         int xPos = GET_X_LPARAM(lParam);
         int yPos = GET_Y_LPARAM(lParam);
         D2D1_SIZE_F canvas = plot_ww.target->GetSize();
         if (marker_tracked >= 0) { // and we are tracking a cursor
            double newtime = plotdata.leftedge_time + (float(xPos) / canvas.width)*(plotdata.rightedge_time - plotdata.leftedge_time);
            dlog("move marker %d at xPos %d to time %f\n", marker_tracked, xPos, newtime);
            set_marker(marker_tracked, newtime);
            RedrawWindow(plot_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE); }
         else if (wParam & MK_LBUTTON) { // mouse move with left button: scroll horizontally
            SCROLLINFO si;
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd, SB_HORZ, &si);
            // (xPos-mousex)/width is the fraction of the visible window (nPage pixels) to move
            int scroll_amount = ((xPos - plot_ww.mousex) * (int)si.nPage) / (int)canvas.width;
            dlog("xPos %d plot_ww.mousex %d si.nPage %d canvas.width %d si.nMax %d si.nMin %d\n",
                 xPos, plot_ww.mousex, si.nPage, (int)canvas.width, si.nMax, si.nMin);
            dlog("scroll at %d by %d\n", xPos, scroll_amount);
            si.nPos -= scroll_amount;
            si.fMask = SIF_POS;
            SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
            ScrollWindow(hwnd, scroll_amount, 0, NULL, NULL); }
         else if (xPos != plot_ww.mousex || yPos != plot_ww.mousey) {
            vpopup_close();
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_HOVER /* | TME_LEAVE*/ ;
            tme.hwndTrack = plot_ww.handle;
            tme.dwHoverTime = 500; // msec before WM_MOUSEHOVER is generated
            TrackMouseEvent(&tme); }
         plot_ww.mousex = xPos;
         plot_ww.mousey = yPos; }
      break;

      case WM_MOUSEWHEEL: // grow/shrink scroll box from mouse wheel
         zoom(&plot_ww, GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? ZOOM_OUT : ZOOM_IN, true);
         break;

      case WM_MOUSEHOVER: {
         // cancel hover request
         TRACKMOUSEEVENT tme;
         tme.cbSize = sizeof(TRACKMOUSEEVENT);
         tme.dwFlags = TME_CANCEL | TME_HOVER;
         tme.hwndTrack = plot_ww.handle;
         TrackMouseEvent(&tme);
         int xPos = GET_X_LPARAM(lParam);
         int yPos = GET_Y_LPARAM(lParam);
         if (check_mouseon_graph(xPos, yPos)) { // if we showed a value popup window
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = plot_ww.handle;
            TrackMouseEvent(&tme); } }
      break;

      case WM_MOUSELEAVE:
         vpopup_close();
         break;

      case WM_LBUTTONDOWN: { // left click
         if (marker_tracked >= 0)
            marker_tracked = -1;  // stop tracking a marker
         else { // check if we should start tracking a marker
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            check_marker_plot_click(xPos, yPos); } }
      break;

      case WM_SIZE: {
         UINT width = LOWORD(lParam);
         UINT height = HIWORD(lParam);
         do_resize(&plot_ww, width, height); }
      result = 0;
      break;

      case WM_ERASEBKGND:
         if (plotdata.data_valid) result = 0; // don't erase if we're showing data
         else DefWindowProc(hwnd, message, wParam, lParam);
         break;

      default:
         result = DefWindowProc(hwnd, message, wParam, lParam); }
   else // no data in our window
      result = DefWindowProc(hwnd, message, wParam, lParam);

   --plot_ww.recursion_level;
   return result; }

void grapherApp::create_plot_window(HWND handle) {
   GetClientRect(handle, &plot_ww.canvas); // get the size of the application client window
   dlog("creating plot window within %d,%d to %d,%d\n", plot_ww.canvas.left, plot_ww.canvas.top, plot_ww.canvas.right, plot_ww.canvas.bottom);
   dlog("  x,y = %ld, %ld  width %ld,  height %ld\n",
        LABELWINDOW_WIDTH, 0, // x,y position
        plot_ww.canvas.right - plot_ww.canvas.left - LABELWINDOW_WIDTH - MARKERWINDOW_WIDTH, // width, height: scale from DPI to pixels
        plot_ww.canvas.bottom - plot_ww.canvas.top); // height in pixels
   plot_ww.initialized = true; // do early so messages to main window don't cause this to be called again
   plot_ww.handle = CreateWindowEx(  // create the plot child window
                       0, // extended styles
                       "ClassPlot",  // registered class name
                       NULL, // window name
                       /*WS_OVERLAPPEDWINDOW +*/ WS_HSCROLL + WS_CHILD + WS_BORDER, // style
                       LABELWINDOW_WIDTH, 0, // x,y position
                       plot_ww.canvas.right - plot_ww.canvas.left - LABELWINDOW_WIDTH - MARKERWINDOW_WIDTH, // width, height: scale from DPI to pixels
                       plot_ww.canvas.bottom - plot_ww.canvas.top, // height in pixels
                       handle, // parent
                       NULL, // menu or child
                       HINST_THISCOMPONENT, // application instance
                       NULL); // context
   if (plot_ww.handle) {
      SetFocus(plot_ww.handle);
      CreateWindowResources(&plot_ww);
      make_fake_data();  // invent a dataset to display
      ShowWindow(plot_ww.handle, SW_SHOWNORMAL);
      UpdateWindow(plot_ww.handle); } }

#if 0
void grapherApp::create_Vpopup_window(HWND handle) {
   vpopup_ww.handle = CreateWindowEx(  // create the voltage popup window
                         0, // extended styles
                         "ClassVpopup",  // registered class name
                         NULL, // window name
                         WS_OVERLAPPEDWINDOW, // style
                         0, 0, // x,y position
                         100, 50, // width, height
                         NULL, // parent
                         NULL, // menu or child
                         HINST_THISCOMPONENT, // application instance
                         NULL); // context
   if (vpopup_ww.handle) {
      CreateWindowResources(&vpopup_ww); } }
#endif

//*