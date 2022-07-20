This is a data visualizer that displays up to 12 time-series plots on a single
chart that can be scrolled horizontally and zoomed in or out. It is designed
to be relatively efficient when there are billions of points.

It was motivated by needing to see the analog data that the "readtape" program 
for magnetic tape data recover analyzes. (https://github.com/LenShustek/readtape)
I wasn't able to find a program that would work well for such very large datasets. 
The Saleae logic analyzer software that collects the analog signals does a pretty 
good job, but it requires that the original very large .logicdata files be preserved.

This program can read a CSV (comma separated value) file, where the first column
is the uniformly incremented timestamp for all the plots. The first two lines are
discarded as headers, but the number of items in them sets the number of plot lines.

It can also read the more compact TBIN file as defined for the readtape program.

Since our tape data tends to be smooth and well-sampled, by default we subsample by
using only every 3rd data point. You can change that with the tools/sampling menu.

You can save a subset of the data between specified times as a CSV or TBIN file. 
If subsampling is on, it will warn you about saving a lower-resolution file.

Here are the user controls:

zooming:    Wheel up or arrow up with the mouse in the plot window zooms in, 
            wheel down or arrow down zooms out.

scrolling:  Click the scrollbar arrows, click scrollbar whitespace, click and drag the
            box, click and drag whitespace in the plot area, or use left/right arrows.
            
values:     Hover the mouse over a point in the graph to display its value and time.

markers:    Place a marker on the plot by clicking the marker number, moving the mouse
                into plot window, then clicking again to place it where you want.
            Move a placed marker by clicking the circled marker number at the top of the line.
            Make a marker the delta time reference by doubleclicking the marker's time.
            Scroll to center a marker in the plot window by doubleclicking the marker's number.
            Scroll to the start or end of the plot by doubleclicking the L or R marker's name.

copy time:  Right clicking a marker's displayed time, or right clicking anywhere in the 
            plot window, copies the time to the Windows clipboard.

tools/goto:  
            Centers the plot on a specified time, and puts time marker 9 there.

File/Save.tbin or File/Save.csv:  
           The data between markers 1 and 2 is saved into a new file of the specified format.
           If saving .tbin and the data came from a .tbin file, its header is used.

tools/options
            dither sampled points: Randomly choose the point to draw a line to when we're
            skpping points closer together than the screen resolution. This somewhat reduces
            the Moire effect when zoomed out on a periodic waveform, but not entirely.

This is unabashedly a windows-only program for a little-endian 64-bit CPU with lots of virtual memory.

Len Shustek
July 2022