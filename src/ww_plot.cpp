//file: ww_plot.cpp
/******************************************************************************

Generate the center plot window, and handle zoooming and scrolling


See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************/

#include "grapher.h"

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

#if 0 // this didn't work for eliminating Moire effects, so we'll try random numbers instead
int64_t gcd(int64_t a, int64_t b) { // Eudlid's greatest common divisor algorithm
   int64_t result = min(a, b);
   while (result > 0) {
      if (a % result == 0 && b % result == 0) break;
      result--; }
   return result; }

int64_t get_skip_increment(int64_t skipval) {
   // Try to find a pretty big number relatively prime to skipval that we will increment the modulus by for checking
   // whether to process a point. The effect of this is to almost randomly choose which of the skip+1 points to process
   // in each interval, which reduces the appearance of Moire patterns when the data is periodic.
   int64_t testval = skipval / 3;  // start with 1/3 the value
   while (1) {
      if (gcd(skipval, testval) == 1) break; // got it
      if (++testval >= skipval) testval = 2;
      assert(testval != skipval / 3, "can't find relatively prime skip value to %lld", skipval); }
   return testval; }
#endif

// This works somewhat for eliminating Moire effects, but not at all magnfications
uint64_t rand64() { // generate a 64-bit random number
// George Marsaglia's algorithm: https://en.wikipedia.org/wiki/Xorshift
   static uint64_t state = 12345;
   uint64_t x = state;
   x ^= x << 13;
   x ^= x >> 7;
   x ^= x << 17;
   return state = x; }

/* The plot graphing routine uses the newer D2D1 "Direct2D" API for drawing lots of lines, in an
   attempt to use hardware-accelerated immediate-mode graphics that are executed on the graphics card.
   I have no idea whether that is actually happening.

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
      plotdata.num_pts_plotted = numpts;
      plotdata.first_pt_plotted = startpt;
      uint64_t skip = numpts / (int)width  + 1; // if more points than pixels, start skipping some
      uint64_t skip_mod = 0; // initial modulus for determining which point in a skip interval to plot
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
      for (uint64_t pt = startpt; ; ++pt) { // **** for all the points: BE EFFICIENT HERE!
         D2D1_POINT_2F next_coords;
         next_coords.x = width * (float)(pt - startpt) / numpts;
         bool plot_this_point = (pt - startpt) % skip == skip_mod;
         if (DRAW_MINMAX || plot_this_point)  // if we're looking at this point
            for (int gr = 0; gr < plotdata.nseries; ++gr) { // for each plot series
               float dataval = plotdata.curblk->data[plotdata.curndx + gr]; // -plotdata.maxval .. +plotdata.maxval
               //assert(dataval <= plotdata.maxval, "data value %f bigger than max %f\n", dataval, plotdata.maxval);
               float midy = plotheight * gr + plotheight_div_2;
               next_coords.y = midy - plotheight_div_2 * dataval / plotdata.maxval;
               dassert(next_coords.y >= midy - plotheight_div_2,
                       "y coord %f too small at point %llu gr %d ndx %d value %f\n",
                       next_coords.y, plotdata.curpt, gr, plotdata.curndx, dataval);
               if (DRAW_MINMAX && !plot_this_point) { // if we're not drawing to this data point for performance
                  // accumulate min and max of what we're skipping, to draw later as one vertical line
                  if (max_coords[gr].y < next_coords.y) max_coords[gr].y = next_coords.y;
                  if (min_coords[gr].y > next_coords.y) min_coords[gr].y = next_coords.y; }
               else { // drawing to this data point
                  plot_ww.target->DrawLine(prev_coords[gr], next_coords, pBlackBrush, 1.0f);
                  if (DRAW_MINMAX && skip > 1 && pt > startpt) { // but first: draw vertical line between the values representing skipped points
                     max_coords[gr].x = min_coords[gr].x = (prev_coords[gr].x + next_coords.x) / 2;
                     //dlog("drawing minmax line for pt %llu gr %d at %f from %f to %f, prev.y %f, next.y %f\n",
                     //       pt, gr, max_coords[gr].x, min_coords[gr].y, max_coords[gr].y, prev_coords[gr].y, next_coords.y);
                     plot_ww.target->DrawLine(min_coords[gr], max_coords[gr], pBlackBrush, 0.5f); }
                  // initialize min/max for skipped points. Requires (pt-startpt)%skip == 0 when pt==startpt
                  if (DRAW_MINMAX) max_coords[gr].y = min_coords[gr].y = next_coords.y;
                  if (plotpoints) {
                     ellipse.point.x = next_coords.x;
                     ellipse.point.y = next_coords.y;
                     plot_ww.target->DrawEllipse(ellipse, pBlackBrush, 1.0, NULL); }
                  prev_coords[gr] = next_coords; } } //for all plots
         if (plot_this_point && skip > 10 && plotdata.do_dither) // maybe dither the modulus and hence the point we plot in the next skip interval
            skip_mod = rand64() % skip;
         if (pt >= endpt) break;
         get_nextpoint(); };
      draw_plot_markers();
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
   if (time >= timestart && time <= timeend) {
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

//
//      value popup window routines
// Note that we use the old GDI routines, not D2D1 (Direct 2D), so we write directly to the screen

#define VPOPUP_SIZE 150,40
#define VPOPUP_CIRCLE_DIAM 10
#define VPOPUP_CIRCLE_BOUND 16  // to allow for thickness; must be even
static struct {  // private state information for the vpopup window
   HDC memdc;
   HGDIOBJ mbitmap, prevobject;
   float xcoord, ycoord; } vpopup;

void vpopup_open(float xcoord, float ycoord, float dataval, double pttime) {
   assert(!vpopup_ww.initialized, "vpopup_already open");
   dlog("creating vpopup at %f,%f for value %lf time %lf\n", xcoord, ycoord, dataval, pttime);
   vpopup.xcoord = xcoord;
   vpopup.ycoord = ycoord;
   // (1) create popup window
   vpopup_ww.handle =
      CreateWindowEx(WS_EX_STATICEDGE, //WS_EX_CLIENTEDGE,
                     "ClassVpopup", "Vpopup", WS_POPUP | /*WS_BORDER*/ WS_DLGFRAME,
                     (int)xcoord, (int)ycoord - VPOPUP_CIRCLE_DIAM/2, VPOPUP_SIZE,
                     /*plot_ww.handle*/ NULL, (HMENU)0, HINST_THISCOMPONENT, NULL);
   ShowWindow(vpopup_ww.handle, SW_SHOW);
   // (2) draw the text in it
   HDC hdc = GetDC(vpopup_ww.handle);
   RECT  rect = { 0, 0, VPOPUP_SIZE };
   char msg[100], timestr[30];
   snprintf(msg, sizeof(msg), "%f\nat %s", dataval, showtime(pttime, timestr, sizeof(timestr)));
   DrawText(hdc, msg, -1, &rect, DT_NOCLIP | DT_CENTER);
   // (3) save the screen area around the point in the plot window
   HDC plothdc = GetDC(plot_ww.handle);
   vpopup.memdc = CreateCompatibleDC(plothdc);
   assert(vpopup.memdc, "CreateCompatibleDC failed");
   vpopup.mbitmap = CreateCompatibleBitmap(plothdc, VPOPUP_CIRCLE_BOUND, VPOPUP_CIRCLE_BOUND);
   assert(vpopup.mbitmap, "CreateCompatibleBitmap failed");
   vpopup.prevobject = SelectObject(vpopup.memdc, vpopup.mbitmap);
   assert(vpopup.prevobject, "vpopup_open SelectObject failed");
   BOOL ok = BitBlt(vpopup.memdc, 0, 0, VPOPUP_CIRCLE_BOUND, VPOPUP_CIRCLE_BOUND, // destination, rectangle size
                    plothdc, (int)xcoord - VPOPUP_CIRCLE_BOUND / 2, (int)ycoord - VPOPUP_CIRCLE_BOUND / 2, SRCCOPY);
   dlog("vpopup_open: saved rectangle at %d, %d\n ",
        (int)xcoord - VPOPUP_CIRCLE_BOUND / 2, (int)ycoord - VPOPUP_CIRCLE_BOUND / 2);
   assert(ok, "Bitblt save err: %08lX\n", GetLastError());
   dlog("vpopup_open: plothdc %08lX, memdc %08lX, mbitmap %08lX, prevobject %08lX\n",
        plothdc, vpopup.memdc, vpopup.mbitmap, vpopup.prevobject);
   // (4) draw a little circle at the actual spot
   SelectObject(plothdc, GetStockObject(DC_PEN));
   SetDCPenColor(plothdc, RGB(255, 0, 0));
   ok = Ellipse(plothdc,
                (int)xcoord - VPOPUP_CIRCLE_DIAM / 2, (int)ycoord - VPOPUP_CIRCLE_DIAM / 2,
                (int)xcoord + VPOPUP_CIRCLE_DIAM / 2, (int)ycoord + VPOPUP_CIRCLE_DIAM / 2);
   assert(ok, "vpopup_open: Ellpse failed");
   vpopup_ww.initialized = true; }

void vpopup_close(void) {
   TRACKMOUSEEVENT tme;  // cancel the hover timer
   tme.cbSize = sizeof(TRACKMOUSEEVENT);
   tme.dwFlags = TME_CANCEL | TME_HOVER;
   tme.hwndTrack = plot_ww.handle;
   TrackMouseEvent(&tme);
   if (vpopup_ww.initialized) { // if there's really a popup window
      dlog("closing vpopup window\n");
      HDC plothdc = GetDC(plot_ww.handle);
      assert(plothdc, "vpopup_close getDC failed");
      //SelectObject(vpopup.memdc, vpopup.mbitmap); // reselect the bitmap object
      BOOL ok = BitBlt(plothdc, (int)vpopup.xcoord - VPOPUP_CIRCLE_BOUND / 2, (int)vpopup.ycoord - VPOPUP_CIRCLE_BOUND / 2, // destination
                       VPOPUP_CIRCLE_BOUND, VPOPUP_CIRCLE_BOUND, // rectangle size
                       vpopup.memdc, 0, 0, SRCCOPY);
      dlog("vpopup_close: restored rectangle at %d, %d\n",
           (int)vpopup.xcoord - VPOPUP_CIRCLE_BOUND / 2, (int)vpopup.ycoord - VPOPUP_CIRCLE_BOUND / 2);
      assert(ok, "Bitblt restore err: %08lX\n", GetLastError());
      HGDIOBJ object = SelectObject(vpopup.memdc, vpopup.prevobject);  // restore the previous object
      assert(object, "vpopup_close SelectObject failed: %08lX", object);
      DeleteDC(vpopup.memdc); // delete memory DC
      ShowWindow(vpopup_ww.handle, SW_HIDE);
      DestroyWindow(vpopup_ww.handle);
      vpopup_ww.handle = NULL;
      vpopup_ww.initialized = false; } }

double get_graph_time(int xPos) { // get the time corresponding to the mouse position
   D2D1_SIZE_F canvas = plot_ww.target->GetSize();
   float width = canvas.width;
   return plotdata.leftedge_time + (float(xPos) / width) * (plotdata.rightedge_time - plotdata.leftedge_time); }

bool check_mouseon_graph(int xPos, int yPos) { // has mouse moved onto a graph line?
   vpopup_close();
   D2D1_SIZE_F canvas = plot_ww.target->GetSize();
   float height = canvas.height;
   float width = canvas.width;
   float plotheight = height / plotdata.nseries;  // height of each plot
   dlog("check_mouseon_graph(%d, %d), canvas %f x %f, plotheight %f\n", xPos, yPos, width, height, plotheight);
   double pttime = plotdata.leftedge_time + (float(xPos) / width) * (plotdata.rightedge_time - plotdata.leftedge_time);
   double delta_ns = (double)plotdata.timedelta_ns / 1e9;
   uint64_t samplenum = (uint64_t)((pttime - (double)plotdata.timestart_ns/1e9 + delta_ns) / delta_ns); // rounded up
   //dlog("  samplenum %llu from pttime %f, timestart %f, timedelta %f\n",
   //       samplenum, pttime, (double)plotdata.timestart_ns / 1e9, (double)plotdata.timedelta_ns / 1e9);
   assert(samplenum >= 0 && samplenum < plotdata.nvals, "check_mouseon_graph: bad #1 samplenum %llu for time %f\n", samplenum, pttime);
   assert(samplenum >= plotdata.first_pt_plotted && samplenum <= plotdata.first_pt_plotted + plotdata.num_pts_plotted,
          "check_mouseon_graph: bad #2 samplenum %llu for time %f\n", samplenum, pttime);
   float x_coord = (float)(samplenum - plotdata.first_pt_plotted)/plotdata.num_pts_plotted * width;
   dlog("   first pt %llu, samplenum %llu, width %f\n", plotdata.first_pt_plotted, samplenum, width);
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
      if (yPos > y_coord - MOUSE_FUZZ && yPos < y_coord + MOUSE_FUZZ) {
         vpopup_open(x_coord, y_coord, dataval, pttime);
         break; } }
   return vpopup_ww.initialized; }

// value popup window: message processing
LRESULT CALLBACK grapherApp::WndProcVpopup(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
   LRESULT result = 0;
   show_message_name(&vpopup_ww, message, wParam, lParam);
   result = DefWindowProc(hwnd, message, wParam, lParam);
   --vpopup_ww.recursion_level;
   return result; }

void zoom(struct window_t *ww, enum zoom_t type, bool center_on_cursor) {
   SCROLLINFO si;
   si.cbSize = sizeof(si);
   si.fMask = SIF_ALL;
   GetScrollInfo(ww->handle, SB_HORZ, &si);
   int old_nPage = si.nPage;
   dlog("zoom %d: scroll box pos %d, page size %d", type, si.nPos, si.nPage);
   if (type == ZOOM_IN) {
      if (plotdata.num_pts_plotted <= 10) // don't zoom in closer than 10 points per screen
         return;
      if (si.nPage >= 2) si.nPage = si.nPage * 2 / 3; }
   else if (type == ZOOM_OUT) {
      if (si.nPage < 2) si.nPage = 2;
      else if (si.nPage < (unsigned)si.nMax * 2 / 3) si.nPage = si.nPage * 3 / 2;
      else si.nPage = si.nMax; }
   dlog(" changed to %d\n", si.nPage);
   if (center_on_cursor) {
      dlog("change nPos from %d", si.nPos);
      si.nPos -= (int64_t)ww->mousex * ((int64_t)si.nPage - old_nPage) / (int64_t)(ww->canvas.right - ww->canvas.left);
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
         vpopup_close();  // close any vpopup and cancel hover timer
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

      case WM_MOUSEWHEEL: // grow/shrink scroll box from mouse wheel
         zoom(&plot_ww, GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? ZOOM_OUT : ZOOM_IN, true);
         break;

      case WM_HSCROLL:
         // see https://docs.microsoft.com/en-us/windows/win32/controls/about-scroll-bars
         SCROLLINFO si;
         si.cbSize = sizeof(si);
         si.fMask = SIF_ALL;
         GetScrollInfo(hwnd, SB_HORZ, &si);
         int scroll_amount, min_amount;
         min_amount = si.nMax / 100000; // 1%, pretty arbitrary
         switch (LOWORD(wParam)) {
         case SB_LINELEFT:  // User clicked the left arrow.
            scroll_amount = +(int)si.nPage / (plot_ww.canvas.right - plot_ww.canvas.left); // one pixel
            // Windows doesn't do this right when the page is really small compared to nMax...
            if (scroll_amount < min_amount) scroll_amount = min_amount;
            break;
         case SB_LINERIGHT: // User clicked the right arrow.
            scroll_amount =  -(int)si.nPage / (plot_ww.canvas.right - plot_ww.canvas.left); // one pixel
            if (scroll_amount > -min_amount) scroll_amount = -min_amount;
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
         dlog("scrollbar: adjusting nPos %d by %d, nPage is %d\n", si.nPos, scroll_amount, si.nPage);
         si.nPos -= scroll_amount;
         SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
         //It's pointless to actually scroll unless we implement partial plot window painting.
         //ScrollWindow(hwnd, scroll_amount, 0, NULL, NULL);
         RedrawWindow(plot_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
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
            // (xPos-mousex)/width is the fraction of the visible window to move
            // nPage is the number of scrolling units that are visible
            int scroll_amount = ((xPos - plot_ww.mousex) * (int64_t)si.nPage) / (int64_t)canvas.width;
            dlog("mousescroll xPos %d plot_ww.mousex %d si.nPage %d canvas.width %d si.nMax %d si.nMin %d\n",
                 xPos, plot_ww.mousex, si.nPage, (int)canvas.width, si.nMax, si.nMin);
            dlog("mousescroll at %d by %d\n", xPos, scroll_amount);
            if (scroll_amount == 0)
               //Either the mouse didn't really move, or it moved so little that the above calculation rounds to zero.
               //In the later case, we want to not update mousex, so that the movement will accumulate.
               break;
            si.nPos -= scroll_amount;  // adjust the current scrollbox position
            si.fMask = SIF_POS;
            SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
            //It's pointless to actually scroll unless we implement partial plot window painting.
            //ScrollWindow(hwnd, scroll_amount, 0, NULL, NULL);
            RedrawWindow(plot_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE); }
         else if (xPos != plot_ww.mousex || yPos != plot_ww.mousey) { // just a mouse movement
            vpopup_close();  // close the value popup if it's up
            TRACKMOUSEEVENT tme;  // start a new hover timer
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_HOVER /* | TME_LEAVE*/ ;
            tme.hwndTrack = plot_ww.handle;
            tme.dwHoverTime = 250; // msec before WM_MOUSEHOVER is generated
            TrackMouseEvent(&tme); }
         plot_ww.mousex = xPos;
         plot_ww.mousey = yPos; }
      break;

      case WM_MOUSEHOVER: {  // the mouse has been hovering for a while
         TRACKMOUSEEVENT tme;  // cancel the hover timer
         tme.cbSize = sizeof(TRACKMOUSEEVENT);
         tme.dwFlags = TME_CANCEL | TME_HOVER;
         tme.hwndTrack = plot_ww.handle;
         TrackMouseEvent(&tme);
         int xPos = GET_X_LPARAM(lParam);
         int yPos = GET_Y_LPARAM(lParam);
         if (check_mouseon_graph(xPos, yPos)) { // check for hovering on a plotline
            //TRACKMOUSEEVENT tme; // don't need this: moving the mouse will take it down
            //tme.cbSize = sizeof(TRACKMOUSEEVENT);
            //tme.dwFlags = TME_LEAVE; // request WM_MOUSELEAVE notifiation
            //tme.hwndTrack = plot_ww.handle;
            //TrackMouseEvent(&tme);
         } }
      break;

      case WM_MOUSELEAVE: // mouse has left the hover area
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

      case WM_RBUTTONDOWN: { // right click: copy time to clipboard
         copy_time(get_graph_time(/*xPos*/GET_X_LPARAM(lParam))); }
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

//*