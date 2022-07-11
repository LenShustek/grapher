This is a data visualizer that displays up to 12 time-series plots on a single
chart that can be scrolled horizontally and zoomed in or out. It is designed
to be relatively efficient when there are billions of points.

This was motivated by needing to see the magnetic tape data that the "readtape"
program analyzes, and I wasn't able to find a program that would work well for
such large datasets. The Saleae logic analyzer software does a pretty good job,
but it requires the original very large .logicdata files to be preserved.

The program can read a CSV (comma separated value) file with the first column
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

markers:    Place a marker on the plot by clicking the marker number, moving the mouse
                into plot window, then clicking again to place it where you want.
            Move a placed marker by clicking the circled marker number at the top of the line.
            Make a marker the delta time reference by doubleclicking the marker's time.
            Scroll to center a marker in the plot window by doubleclicking the marker's number.
            Scroll to the start or end of the plot by doubleclicking the L or R marker's name.

options/goto:  
            Center the plot on a specified time, and put time marker 9 there.

File/Save.tbin or File/Save.csv:  
           The data between markers 1 and 2 is saved into a new file of the specified format.
           If saving .tbin and the data came from a .tbin file, that header is used.

This is unabashedly a windows-only program for a little-endian 64-bit CPU with lots of virtual memory.

Len Shustek
July 2022