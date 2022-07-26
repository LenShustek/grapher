//file: readfile.cpp
/******************************************************************************

Handle reading and writing of CSV and TBIN files


See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************/#include "grapher.h"
#include "csvtbin.h"

// Compiled with Microsoft Visual Studio Community 2017
// Info on creating icons:
// https://stackoverflow.com/questions/40933304/how-to-create-an-icon-for-visual-studio-with-just-mspaint-and-visual-studio
// https://docs.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)

#if defined(_WIN32) // there is NO WAY to do this in an OS-independent fashion!
#define ftello _ftelli64
#define fseeko _fseeki64
#endif

#include <commdlg.h> // old-style Windows API
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

FILE *inf;
uint64_t filesize;
uint64_t file_nvals;

bool progressbar_open = false;
int progressbar_last_percent = 0;
INITCOMMONCONTROLSEX progressbar_types = { sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS } ;
HWND progressbar_handle;

void progressbar_show(int percent) {
   if (!progressbar_open) {
      InitCommonControlsEx(&progressbar_types);
      progressbar_handle = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
                                          WS_CHILD | WS_VISIBLE, 100, 200, 600, 25, // x, y, width, height
                                          plot_ww.handle, (HMENU)0, HINST_THISCOMPONENT, NULL);
      SendMessage(progressbar_handle, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); // default is 0..100
      SendMessage(progressbar_handle, PBM_SETSTEP, (WPARAM)1, 0);
      progressbar_last_percent = 0;
      progressbar_open = true; }
   if (percent != progressbar_last_percent) {
      progressbar_last_percent = percent;
      dlog("progress percent %d\n", percent);
      SendMessage(progressbar_handle, PBM_STEPIT, 0, 0); } }

void progressbar_hide(void) {
   if (progressbar_open) {
      DestroyWindow(progressbar_handle);
      progressbar_open = false; } }

struct datablk_t * alloc_data(void) { // add another data block
   struct datablk_t *newblk;
   int blksize = sizeof(struct datablk_t) // header as declared holds only one value
                 + plotdata.nseries * sizeof(float) * NDATABLK;
   assert(newblk = (struct datablk_t *)malloc(blksize), "can't allocate data block");
   ++plotdata.numblks;
   newblk->next = newblk->prev = NULL;
   newblk->count = 0;
   newblk->firstnum = plotdata.nvals;
   if (plotdata.datahead == NULL) {
      plotdata.datahead = newblk;  // this is the first block
      newblk->firstnum = 0; }
   else {  // add on to the tail
      newblk->prev = plotdata.datatail;
      plotdata.datatail->next = newblk;
      newblk->firstnum = plotdata.datatail->firstnum + plotdata.datatail->count; }
   plotdata.datatail = newblk;
   return newblk; }

void add_data(float *datapoints) {  // add plotdata.nseries values
   struct datablk_t *blk;
   if (plotdata.datahead== NULL  // if we haven't allocated anything yet
         || plotdata.datatail->count == NDATABLK) // or if the last block is full,
      blk = alloc_data(); // then allocate another data block
   else blk = plotdata.datatail; // otherwise continue filling the last block
   int startpos = blk->count * plotdata.nseries; // add after all the previous data
   for (int ndx = 0; ndx < plotdata.nseries; ++ndx)  // add the new values
      blk->data[startpos+ndx] = datapoints[ndx];  // (would memcpy() be faster?)
   ++blk->count; }

void dump_blocks(void) {
   dlog("%d series %llu points %d blocks %f maxval\n",
        plotdata.nseries, plotdata.nvals, plotdata.numblks, plotdata.maxval);
   struct datablk_t *prev = NULL;
   int blknum = 0;
   for (struct datablk_t *blk = plotdata.datahead; blk; blk = blk->next) {
      dlog("block %d has points %llu through %llu\n",
           ++blknum, blk->firstnum, blk->firstnum + blk->count - 1);
      if(blk->prev != prev) dlog("   previous link bad\n");
      prev = blk; } }

void show_file_info(void) {
   dlog("stored %llu data points and allocated %d blocks\n", plotdata.nvals, plotdata.numblks);
   char msg[500], sample_info[100];
   char nfilevals[30], nplotvals[30], numblks[30], filemsg[30], memmsg[30];
   //dump_blocks();
   if (plotdata.sampling > 1 && plotdata.source != DATA_FAKE)
      snprintf(sample_info, sizeof(sample_info), "We stored 1 out of every %d data points, or %s.\n",
               plotdata.sampling, u64commas(plotdata.nvals, nplotvals));
   else sample_info[0] = 0;
   snprintf(msg, sizeof(msg),
            "The %s file had %s points for each of %d plots.\n"
            "%s" // sampling info
            "We allocated %s data blocks taking %s in virtual memory.\n"
            "The largest value from time %f to %f is %.3f.\n",
            format_memsize(filesize, filemsg), u64commas(file_nvals, nfilevals), plotdata.nseries,
            sample_info,
            u32commas(plotdata.numblks, numblks),
            /**/format_memsize(plotdata.numblks*(uint64_t)(sizeof(struct datablk_t)+ plotdata.nseries * NDATABLK*sizeof(float)), memmsg),
            plotdata.timestart, plotdata.timeend, plotdata.maxval);
   MessageBoxA(NULL, msg, "Info", 0); }

void finish_file(const char *filename) {
   make_block_index();
   clear_markers();
   plotdata.timestart = (double)plotdata.timestart_ns / 1e9;
   plotdata.timeend = (double)(plotdata.timestart_ns + plotdata.timedelta_ns*(plotdata.nvals-1)) / 1e9;
   SetWindowText(main_ww.handle, filename); // put the filename on the title of the main window
   HMENU menu;
   menu = GetMenu(main_ww.handle); // un-grey file/save and close menu items
   EnableMenuItem(menu, ID_FILE_SAVE_TBIN, MF_BYCOMMAND | MF_ENABLED);
   EnableMenuItem(menu, ID_FILE_SAVE_CSV, MF_BYCOMMAND | MF_ENABLED);
   EnableMenuItem(menu, ID_FILE_CLOSEDATA, MF_BYCOMMAND | MF_ENABLED);
   EnableMenuItem(menu, ID_FILE_FILEINFO, MF_BYCOMMAND | MF_ENABLED);
   EnableMenuItem(menu, ID_TOOLS_GOTO, MF_BYCOMMAND | MF_ENABLED);
   if (plotdata.source != DATA_FAKE) show_file_info();
   SCROLLINFO si;
   si.cbSize = sizeof(si);
#if 0
   // change from 0..100 range for the horizontal scroll box to something larger based on the number of
   // data points, so that we can zoom really close
#define NUM_PTS_ACROSS 25
   uint64_t maxval = plotdata.nvals / NUM_PTS_ACROSS; // try for NUM_PTS_ACROSS points across the screen
   if (maxval < NUM_PTS_ACROSS) maxval = NUM_PTS_ACROSS; // but bound it
   if (maxval > MAXINT32 / 2) maxval = MAXINT32 / 2;
   si.nMax = (int) maxval;
#endif 0
   si.nMax = 10000000;  // a very large value, since these are unitless, allows finer resolution
   // (Using MAXINT causes Windows to fail to draw the scroll box properly, presumably because of overflows.)
   si.nMin = 0;
#define MANY_SAMPLES 1000000L
   si.nPage = plotdata.nvals > MANY_SAMPLES ?  // if we have a lot of samples
              (UINT) ((uint64_t)MANY_SAMPLES * si.nMax / plotdata.nvals)  // then zoom in to display the first batch
              : si.nMax; // otherwise set the initial page size to max, ie zoom all the way out
   dlog("set initial scroll nPage to %d out of %d, given %lld values\n", si.nPage, si.nMax, plotdata.nvals);
   si.fMask = SIF_RANGE + SIF_PAGE;
   SetScrollInfo(plot_ww.handle, SB_HORZ, &si, true);
   plotdata.data_valid = true;
   //SendMessage(plot_ww.handle, WM_PAINT, 0, 0L);  // these don't work!
   //SendMessage(marker_ww.handle, WM_PAINT, 0, 0L);
   //SendMessage(label_ww.handle, WM_PAINT, 0, 0L);
   RedrawWindow(plot_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE); // plot window will redraw marker window
   RedrawWindow(label_ww.handle, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
   si.fMask = SIF_ALL;
   GetScrollInfo(plot_ww.handle, SB_HORZ, &si);  // get scrollbar info to check it
   dlog("plot scrollbar: nMin %d nMax %d nPage %d nPos %d\n",
        si.nMin, si.nMax, si.nPage, si.nPos); }

void discard_data(void) {
   plotdata.data_valid = false;
   for (struct datablk_t *blk = plotdata.datahead; blk; ) {
      struct datablk_t *nextblk = blk->next;
      free(blk);
      blk = nextblk; }
   free(plotdata.blkindex);
   plotdata.blkindex = NULL;
   plotdata.datahead = plotdata.datatail = NULL;
   plotdata.numblks = 0;
   plotdata.nvals = file_nvals = 0;
   plotdata.maxval = 0.0f;
   SendMessage(label_ww.handle, WM_ERASEBKGND, (WPARAM)GetDC(label_ww.handle), 0);
   SendMessage(plot_ww.handle, WM_ERASEBKGND, (WPARAM)GetDC(plot_ww.handle), 0);
   SendMessage(marker_ww.handle, WM_ERASEBKGND, (WPARAM)GetDC(marker_ww.handle), 0);
   HMENU menu;
   menu = GetMenu(main_ww.handle); // grey out file save and close menu items
   EnableMenuItem(menu, ID_FILE_SAVE_TBIN, MF_BYCOMMAND | MF_GRAYED);
   EnableMenuItem(menu, ID_FILE_SAVE_CSV, MF_BYCOMMAND | MF_GRAYED);
   EnableMenuItem(menu, ID_FILE_CLOSEDATA, MF_BYCOMMAND | MF_GRAYED);
   EnableMenuItem(menu, ID_FILE_FILEINFO, MF_BYCOMMAND | MF_GRAYED);
   EnableMenuItem(menu, ID_TOOLS_GOTO, MF_BYCOMMAND | MF_GRAYED);
   SetWindowText(main_ww.handle, "grapher"); // put the application name on the title of the main window
}

void make_fake_data(void) {  // make fake data: 2 sine waves, one increasing, one decreasing
   int ncycles = 100;
   int npts = 100001;
   discard_data();
   plotdata.source = DATA_FAKE;
   plotdata.sampling = 1;
   plotdata.nseries = 2;
   plotdata.maxval = 1.0f;
   plotdata.timestart_ns = 0;
   plotdata.timedelta_ns = 1000000;  // 1 msec per point
   plotdata.nvals = file_nvals = npts;
   strcpy(plotdata.labels[0], "fake data 1");
   strcpy(plotdata.labels[1], "fake data 2");
   float series[2];
   for (int pt = 0; pt < npts; ++pt) {
      float sinx = (float)sin((2 * 3.141592f*(float)pt * (float)ncycles) / (float)npts); // actual sine wave value
      series[0] = sinx * pt / npts;         // gradually increasing sine wave
      series[1] = sinx * (npts-pt) / npts;  // gradually decreasing sine wave
      add_data(series); }
   finish_file("fake data"); }

void get_filesize(void) {
   assert(fseeko(inf, 0, SEEK_END) == 0, "fseek to end failed");
   assert((filesize = ftello(inf)) >= 0, "ftell for filesize failed");
   assert(fseeko(inf, 0, SEEK_SET) == 0, "fseek to start failed");
   dlog("filesize: %lld\n", filesize); }

struct tbin_hdr_t tbin_hdr;
struct tbin_hdrext_trkorder_t tbin_hdrext_trkorder;
struct tbin_dat_t tbin_dat;

void read_tbin_file(char *filename) {
   assert(inf = fopen(filename, "rb"), "can't open file");
   get_filesize();
   assert(fread(&tbin_hdr, sizeof(tbin_hdr), 1, inf) == 1, "can't read .tbin header");
   assert(strcmp(tbin_hdr.tag, HDR_TAG) == 0, ".tbin file missing hdr tag");
   assert(tbin_hdr.u.s.format == TBIN_FILE_FORMAT, "bad .tbin file header version");
   assert(tbin_hdr.u.s.tbinhdrsize == sizeof(tbin_hdr), "bad .tbin header size");
   if (tbin_hdr.u.s.flags & TBIN_TRKORDER_INCLUDED) { // optional track order header
      assert(fread(&tbin_hdrext_trkorder, sizeof(tbin_hdrext_trkorder), 1, inf) == 1, "can't read .tbin trkorder header extension");
      assert(strcmp(tbin_hdrext_trkorder.tag, HDR_TRKORDER_TAG) == 0, ".tbin file missing trkorder header tag"); }
   assert(fread(&tbin_dat, sizeof(tbin_dat), 1, inf) == 1, "can't read .tbin dat");
   assert(strcmp(tbin_dat.tag, DAT_TAG) == 0, ".tbin file missing DAT tag");
   assert(tbin_dat.sample_bits == 16, "we support only 16 bits/sample");
   discard_data(); // discard any previous data
   plotdata.source = DATA_TBIN;
   plotdata.timestart_ns = tbin_dat.tstart;
   plotdata.sampling = sampling;
   //plotdata.timestart = (double)tbin_dat.tstart / 1e9;
   plotdata.maxval = tbin_hdr.u.s.maxvolts;
   plotdata.timedelta_ns = tbin_hdr.u.s.tdelta * sampling; // account for datapoints we will skip
   //plotdata.timedelta = (double)tbin_hdr.u.s.tdelta / 1e9;
   plotdata.nseries = tbin_hdr.u.s.ntrks;
   assert(plotdata.nseries <= MAXSERIES, "too many data series: %d", plotdata.nseries);
   for (int trk = 0; trk < plotdata.nseries; ++trk)
      snprintf(plotdata.labels[trk], MAXLABEL, "track %d", trk);
   int16_t tbin_voltages[MAXSERIES];
   float datapoints[MAXSERIES];
   //uint64_t timenow = plotdata.timestart_ns;
   HCURSOR hourglass = LoadCursor(NULL, IDC_WAIT);
   SetCursor(hourglass);
   uint64_t nbytes = 0; // number of bytes read (approx; doesn't include header)
   for (plotdata.nvals = 0; ; ++plotdata.nvals) {
      for (int skip = 0; skip < sampling; ++skip) { // read "sampling" sets of data
         assert(fread(&tbin_voltages[0], 2, 1, inf) == 1, "can't read .tbin data for head 0");
         if (tbin_voltages[0] == -32768 /*0x8000*/)  // end of file marker
            goto tbin_done;
         assert(fread(&tbin_voltages[1], 2, plotdata.nseries - 1, inf) == plotdata.nseries - 1,
                "can't read .tbin data for heads 1.. ");
         nbytes += plotdata.nseries * 2;
         ++file_nvals; }
      for (int series = 0; series < plotdata.nseries; ++series) { // convert from tbin binary to float
         datapoints[series] = (float)tbin_voltages[series] / 32767 * tbin_hdr.u.s.maxvolts; }
      add_data(datapoints);
      //timenow += plotdata.timedelta_ns;
      if (filesize > 10000000L)
         progressbar_show((int)(nbytes * 100 / filesize)); }
tbin_done:
   progressbar_hide();
   finish_file(filename); }

void read_csv_file(char *filename) {
#define MAXLINE 400
   char line[MAXLINE + 1];
   if (inf = fopen(filename, "r")) {
      get_filesize();
      // first two (why?) lines in the input file are headers from Saleae
      assert(fgets(line, MAXLINE, inf) != NULL, "Can't read first CSV title line");
      assert(fgets(line, MAXLINE, inf) != NULL, "Can't read second CSV title line");
      discard_data(); // discard any previous data
      plotdata.source = DATA_CSV;
      plotdata.nseries = 0;
      plotdata.sampling = sampling;
      for (int i = 0; line[i]; ++i) if (line[i] == ',') {
            snprintf(plotdata.labels[plotdata.nseries], MAXLABEL, "plot %d", plotdata.nseries);
            ++plotdata.nseries; }
      assert(plotdata.nseries < MAXSERIES, "too many plots: %d\n", plotdata.nseries);
      dlog("from CSV file: %d series\n", plotdata.nseries);
      // For CSV files, set sample_deltat by reading the first 10,000 samples, because Saleae timestamps
      // are only given to 0.1 usec. If the sample rate is, say, 3.125 Mhz, or 0.32 usec between samples,
      // then all the timestamps are off by either +0.08 usec or -0.02 usec!
      uint64_t curpos = ftello(inf);
      int linecounter = 0;
      assert(fgets(line, MAXLINE, inf), "can't read first CSV data line");
      char *linep = line;
      double first_timestamp = scanfast_double(&linep);
      while (fgets(line, MAXLINE, inf) && ++linecounter < 10000);
      linep = line;
      double timestamp = scanfast_double(&linep); // get the timestamp of the last line we read
      double sample_deltat = (timestamp - first_timestamp) / (linecounter - 1);
      plotdata.timestart_ns = (uint64_t)(first_timestamp*1e9);
      plotdata.timedelta_ns = (uint64_t)(sample_deltat*1e9) * sampling; // account for samples we will skip
      dlog("start %f (%llu ns), delta %f (%llu ns)\n",
           first_timestamp, plotdata.timestart_ns, sample_deltat, plotdata.timedelta_ns);
      assert(fseeko(inf, curpos, SEEK_SET) == 0, "can't seek back in csv file");
      line[MAXLINE - 1] = 0;
      HCURSOR hourglass = LoadCursor(NULL, IDC_WAIT);
      SetCursor(hourglass);
      /* sscanf is excruciately slow and was taking 90% of the processing time!
      The special-purpose scan routines are about 25 times faster, but do no error checking. */
      uint64_t nbytes = 0; // number of bytes read (approx; doesn't include header lines)
      int skipped = 0;
      while (fgets(line, MAXLINE, inf)) {  // read all the rest of the data lines until endfile
         nbytes += strlen(line) + 2; // +2 for CRLF
         ++file_nvals;
         if (++skipped >= sampling) { // process only every "sampling" lines
            skipped = 0;
            linep = line;
            float sampletime = scanfast_float(&linep);  // get (and ignore) the time of this sample
            float datapoints[MAXSERIES];
            for (int series = 0; series < plotdata.nseries; ++series) {  // read values for all series
               float datapoint = scanfast_float(&linep);
               datapoints[series] = datapoint;
               if (datapoint < 0) {
                  if (-datapoint > plotdata.maxval) plotdata.maxval = -datapoint; }
               else if (datapoint > plotdata.maxval) plotdata.maxval = datapoint; }
            add_data(datapoints); // add nseries data points
            ++plotdata.nvals;
            if (filesize > 10000000L)
               progressbar_show((int)(nbytes * 100 / filesize)); } }
      progressbar_hide();
      finish_file(filename); }
   else MessageBoxA(NULL, _strerror(NULL), "Error", 0); // can't open file
}

void open_file(HWND hwnd) {
   OPENFILENAME ofn;       // common dialog box structure
   char filename[MAXPATH];      // buffer for file name
   ZeroMemory(&ofn, sizeof(ofn));
   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFile = filename;
   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
   // use the contents of filename to initialize itself.
   ofn.lpstrFile[0] = '\0';
   ofn.nMaxFile = sizeof(filename);
   ofn.lpstrFilter = ".csv,.tbin\0*.csv;*.tbin\0All\0*.*\0";
   ofn.nFilterIndex = 1;
   ofn.lpstrFileTitle = NULL;
   ofn.nMaxFileTitle = 0;
   ofn.lpstrInitialDir = NULL;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
   if (GetOpenFileName(&ofn) == TRUE) {
      dlog("got file: %s\n", ofn.lpstrFile);
      //dlog("in hex: ");
      //for (LPWSTR ptr = ofn.lpstrFile; *ptr; ++ptr) dlog("%04X ", *ptr);
      //dlog("\n");
      char filename[MAXPATH];
      strncpy(filename, ofn.lpstrFile, sizeof(filename));
      //wchar_to_ascii(ofn.lpstrFile, filename, sizeof(filename));
      //dlog("or in ASCII: %s\n", filename);
      size_t namelen = strlen(filename);
      assert(namelen > 4, "filename too short");
      if (strcmp(filename + namelen - 4, ".csv") == 0)
         read_csv_file(filename);
      else if (strcmp(filename + namelen - 5, ".tbin") == 0)
         read_tbin_file(filename);
      else assert(false, "file not .csv or .tbin"); } }

FILE *outf;

uint64_t convert_time_to_point(double time) {
   return ((uint64_t)(time*1e9) - plotdata.timestart_ns) / plotdata.timedelta_ns; }

void write_csv_file(uint64_t starttime, uint64_t lastpt) {
   uint64_t pnt = plotdata.curpt;
   uint64_t firstpt = plotdata.curpt;
   uint64_t numpts = lastpt - firstpt;
   for (int i = 0; i < 2; ++i) { // write the header line twice, like Salea
      fprintf(outf, "time, ");
      for (int gr = 0; gr < plotdata.nseries; ++gr)
         fprintf(outf, "data %d%s", gr,
                 gr == plotdata.nseries - 1 ? "\n" : ", "); }
   HCURSOR hourglass = LoadCursor(NULL, IDC_WAIT);
   SetCursor(hourglass);
   uint64_t timenow = starttime;
   do { // for all data points
      // The %f floating-point display formatting is quite slow. If the -read option is ever used for production, we
      // should write fast special-purpose routines, as we did for parsing floating-point input. Could be 10x faster!
      fprintf(outf, "%12.8lf, ", (double)timenow / 1e9); // the timestamp column
      timenow += plotdata.timedelta_ns;
      for (int gr = 0; gr < plotdata.nseries; ++gr) // all the data columns
         fprintf(outf, "%f%s", plotdata.curblk->data[plotdata.curndx+gr],
                 gr == plotdata.nseries - 1 ? "\n" : ", ");
      if (numpts > 1000000UL)
         progressbar_show((int)((pnt - firstpt) * 100 / numpts));
      ++pnt;
      get_nextpoint(); }
   while (pnt < lastpt);
   fclose(outf);
   progressbar_hide();
   char msg[100], num[30];
   snprintf(msg, sizeof(msg), "wrote %s samples\n", u64commas(lastpt - firstpt, num));
   MessageBoxA(NULL, msg, "Info", 0); }

void write_tbin_file(uint64_t starttime, uint64_t lastpt) {
   uint64_t pnt = plotdata.curpt;
   uint64_t firstpt = plotdata.curpt;
   uint64_t numpts = lastpt - firstpt;
   dlog("writing pts %llu to %llu starting time %f\n", firstpt, lastpt, (double)starttime/1e9);
   if (plotdata.source != DATA_TBIN) { // create the tbin header
      ZeroMemory(&tbin_hdr, sizeof(tbin_hdr));
      strcpy(tbin_hdr.tag, HDR_TAG);
      strncpy_s(tbin_hdr.descr, "file created by grapher", sizeof(tbin_hdr.descr));
      tbin_hdr.u.s.format = TBIN_FILE_FORMAT;
      tbin_hdr.u.s.flags = TBIN_NO_REORDER;
      tbin_hdr.u.s.tbinhdrsize = sizeof(tbin_hdr);
      {
         time_t converted_time;
         time(&converted_time);
         struct tm *ptm = localtime(&converted_time);
         tbin_hdr.u.s.time_converted = *ptm; }  // structure copy!
      tbin_hdr.u.s.ntrks = plotdata.nseries;
      tbin_hdr.u.s.tdelta = (uint32_t) plotdata.timedelta_ns;
      tbin_hdr.u.s.maxvolts = plotdata.maxval; }
   assert(fwrite(&tbin_hdr, sizeof(tbin_hdr), 1, outf) == 1, "can't write tbin header: %s", _strerror(NULL));
   ZeroMemory(&tbin_dat, sizeof(tbin_dat));
   strcpy(tbin_dat.tag, DAT_TAG);
   tbin_dat.sample_bits = 16;
   tbin_dat.tstart = starttime;
   assert(fwrite(&tbin_dat, sizeof(tbin_dat), 1, outf) == 1, "can't write tbin data header: %s", _strerror(NULL));
   HCURSOR hourglass = LoadCursor(NULL, IDC_WAIT);
   SetCursor(hourglass);
   float fsample, round;  // this conversion code was cribbed from csvtbin.c
   int32_t sample;
   long long count_toosmall = 0, count_toobig = 0;
   do { // for all data points
      int16_t outbuf[MAXSERIES]; // accumulate data for all graphs, for faster writing
      for (int gr = 0; gr < plotdata.nseries; ++gr) { // for all plots at that datapoint
         fsample = plotdata.curblk->data[plotdata.curndx + gr];
         if (fsample < 0) round = -0.5;  else round = 0.5;  // (int) truncates towards zero
         sample = (int)((fsample / tbin_hdr.u.s.maxvolts * 32767) + round);
         //dlog("%6d %9.5f, ", sample, fsample);
         if (sample < -32767) {
            sample = -32767; ++count_toosmall; }
         if (sample > 32767) {
            dlog("too big at pt %llu, sample %d\n", pnt, sample);
            sample = 32767;  ++count_toobig; }
         outbuf[gr] = (int16_t)sample; }
      assert(fwrite(outbuf, 2, plotdata.nseries, outf) == plotdata.nseries,
             "can't write tbin data: %s", _strerror(NULL));
      if (numpts > 10000000UL)
         progressbar_show((int)((pnt - firstpt) * 100 / numpts));
      ++pnt;
      get_nextpoint(); }
   while (pnt < lastpt);
   uint16_t endmarker = 0x8000;
   assert(fwrite(&endmarker, 1, 2, outf) == 2,
          "can't write tbin endmarker: %s", _strerror(NULL));
   fclose(outf);
   progressbar_hide();
   char msg[100], num[30];
   snprintf(msg, sizeof(msg), "wrote %s samples\n%llu too small, %llu too big",
            u64commas(lastpt - firstpt, num), count_toosmall, count_toobig);
   MessageBoxA(NULL, msg, "Info", 0); }

void save_file(HWND hwnd, enum datasource_t type) {
   const char *ext = type == DATA_CSV ? ".csv" : type == DATA_TBIN ? ".tbin" : "???";
   OPENFILENAME ofn;       // common dialog box structure
   char filename[MAXPATH];      // buffer for file name
   if (!markers[2].timeset) {
      MessageBoxA(NULL, "Marker 1 not set", "Error", 0);
      return; }
   if (!markers[3].timeset) {
      MessageBoxA(NULL, "Marker 2 not set", "Error", 0);
      return; }
   if (!plotdata.data_valid) {
      MessageBoxA(NULL, "No data", "Error", 0);
      return; }
   if (plotdata.sampling != 1) {
      char msg[50];
      snprintf(msg, sizeof(msg), "data was sampled at %d\nsave a low-resolution file?", plotdata.sampling);
      if (MessageBoxA(NULL, msg, "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
         return; }
   ZeroMemory(&ofn, sizeof(ofn));
   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hwnd;
   ofn.lpstrFile = filename;
   ofn.lpstrFile[0] = '\0';
   ofn.nMaxFile = sizeof(filename);
   ofn.lpstrFilter = ext;
   ofn.nFilterIndex = 1;
   ofn.lpstrFileTitle = NULL;
   ofn.nMaxFileTitle = 0;
   ofn.lpstrInitialDir = NULL;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
   if (GetSaveFileName(&ofn) == TRUE) {
      int length = (int)strlen(ofn.lpstrFile);
      if (length > 5 && strcmp(filename + length - strlen(ext), ext) != 0) // if extension wasn't given
         strcat(filename, ext); // add it
      dlog("save file name: %s\n", filename);
      if (GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) {
         char msg[MAXPATH];
         snprintf(msg, sizeof(msg), "file exists:\n%s\nOverwrite?", filename);
         if (MessageBox(main_ww.handle, msg, NULL, MB_OKCANCEL) == IDCANCEL)
            return; } // abort the save
      uint64_t firstpt, lastpt;
      double starttime;
      if (markers[2].time < markers[3].time) {
         firstpt = convert_time_to_point(starttime = markers[2].time);
         lastpt = convert_time_to_point(markers[3].time); }
      else {
         firstpt = convert_time_to_point(starttime = markers[3].time);
         lastpt = convert_time_to_point(markers[2].time); }
      get_point(firstpt);
      if (outf = fopen(filename, type == DATA_CSV ? "w" : "wb")) {
         if (type == DATA_CSV) write_csv_file((uint64_t)(starttime*1e9), lastpt);
         else if (type == DATA_TBIN) write_tbin_file((uint64_t)(starttime*1e9), lastpt);
         else assert(false, "bad data type in save_file()"); }
      else MessageBoxA(NULL, _strerror(NULL), "Error", 0); // can't open file for writing
   } }

//*