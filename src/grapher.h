//file: grapher.h
/******************************************************************************

Global symbols


See grapher.cpp for the consolidated change log

Copyright (C) 2022 Len Shustek
Released under the MIT License
******************************************************************************/
#pragma once

#include "appstuff.h"

#ifdef _DEBUG  // set by Visual Studio for the DEBUG configuration
#define DEBUG true
#else
#define DEBUG false
#endif
#define DEBUG_SHOW_MESSAGES false  // show window messages in log?

#define DEFAULT_SAMPLING 3     // works well for readtape mag tape data
#define DEFAULT_DITHER true    // even though it's only sometimes effective

#define MAXSERIES 12           // maximum number of data series we plot
#define NDATABLK 5000          // number of samples in a data block
#define MAXLABEL 20            // maximum length of a label name for a plot
#define NUM_MARKERS 11         // time markers: L, R, 1..9
#define LABELWINDOW_WIDTH 80
#define MARKERWINDOW_WIDTH 200
#define LABEL_COLOR RGB(100, 100, 100)
#define MAXPATH 300

#if DEBUG 
#define dassert(...) {assert(__VA_ARGS__);}
#define dlog(...) {debuglog(__VA_ARGS__);}
#else
#define dassert(...) {}
#define dlog(...) {}
#endif


extern grapherApp *app;
enum datasource_t { DATA_CSV, DATA_TBIN, DATA_FAKE };
enum zoom_t { ZOOM_IN, ZOOM_OUT };

struct datablk_t {   // a block of up to NDATABLK data points for plotdata.nseries series
   struct datablk_t *next, *prev;
   uint64_t firstnum;  // number of the first datum in this block
   int count;          // how many data are in the block, 0..NDATABLK
   float data[1];      // the data of this block: plotdata.nseries * NDATABLK numbers
};

struct blkindex_t {   // index entry
   struct datablk_t *blk;  // pointer to a block
   uint64_t firstnum;      // the first point number in that block
   int count;              // how many data are in the block, 0..NDATABLK
};

struct plotdata_t {   // info about our dataset
   bool data_valid; 
   enum datasource_t source;     // where we got the data from
   uint64_t timestart_ns, timedelta_ns;
   double timestart, timeend;    // time of the first and last points 
   int sampling;                 // what sampling was used when we read the file
   bool do_dither;               // whether we should do dithering of the point being displayed
   float maxval;                 // max of absolute values
   int nseries;                  // number of plots we draw
   uint64_t nvals;               // number of values in the dataset
   int numblks;                  // number of blocks it takes to store them
   struct datablk_t *datahead, *datatail; // ptrs to the head and tail of the data block chain
   struct blkindex_t *blkindex;  // pointer to the block index with plotdata.numblks entries
   uint64_t curpt;               // the current point number we're looking at
   struct datablk_t *curblk;     // the block it's in
   int curndx;                   // the index in data[] to the plotdata.nseries data values for that point
   uint64_t num_pts_plotted;     // the number of points we last plotted
   uint64_t first_pt_plotted;    // the first point we last plotted
   double leftedge_time, rightedge_time; // time in seconds displayed in the window

   char labels[MAXSERIES][MAXLABEL];  // labels for the plots
};
extern struct plotdata_t plotdata;

struct window_t {  // info about our windows
   const char *name;
   bool initialized;
   HWND handle;
   RECT canvas;                     // the current entire window drawing canvas
   ID2D1HwndRenderTarget *target;   // permanent D2D1 render target
   int mousex, mousey;              // last reported mouse position
   int recursion_level;             // WndProcxxx() recursion 
};
extern struct window_t main_ww, label_ww, plot_ww, marker_ww, vpopup_ww;

struct markers_t {
   const char *name;
   DWORD color;
   bool timeset;  // has this marker's time been set?
   bool visible;  // is it visible in the plot window?
   int markerww_y_coord;   // it's position in the marker window
   int plotww_x_coord;     // if it's visible on the plot, it's position there
   double time;
};
extern struct markers_t markers[];

void debuglog(const char* msg, ...);
void showhelp(void);
void show_message_name(struct window_t *ww, UINT message, WPARAM wParam, LPARAM lParam);
const char *msgname(unsigned message, bool ignoresome);
void do_resize(struct window_t *ww, UINT width, UINT height);
void open_file(HWND);
void save_file(HWND hwnd, enum datasource_t type);
void discard_data(void);
void show_file_info(void);
void make_block_index(void);
void get_point(uint64_t pt);
void get_nextpoint(void);
void wchar_to_ascii(LPWSTR src, char *dst, int buflen);
void assert(bool b, const char *msg, ...);
char *u32commas(uint32_t n, char buf[14]);
char *u64commas(uint64_t n, char buf[26]);
void make_fake_data(void);
void clear_markers(void);
void set_marker(int ndx, double time);
void draw_plot_markers(void);
void check_marker_plot_click(int xPos, int yPos);
void center_plot_on(double time);
char *showtime(double time, char *buf, int bufsize);
char *format_memsize(uint64_t val, char *buf);
float scanfast_float(char **p);
double scanfast_double(char **p);
void set_tools_sampling(void);
void set_tools_goto(void);
void set_tools_options(void);
void copy_time(double time);

extern int marker_tracked;  // which marker, if any, the mouse is tracking in the plot window
extern int sampling;
extern const char helptext[];

extern ID2D1SolidColorBrush *pBlackBrush, *pRedBrush;
extern IDWriteTextFormat *pTextFormat;

//*