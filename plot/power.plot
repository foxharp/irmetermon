#
set terminal png size 1150, 300
set output "`echo $PLOT_OUTPUT`"
set noborder
set xdata time
#set format x "%m/%d\n%I:%M %p"
#set format x "%I:%M %p\n%A"
set format x "%a %m/%d\n%l:%M %p"
set grid ytics 
set tics scale 2, 1
set mxtics `echo $PLOT_MINOR_XTICS`
set xtics `echo $PLOT_XTIC_SECONDS` nomirror  out
set ytics border mirror 500
set xlabel "Date/time"
set timefmt "%m/%d/%y %H:%M"
set xrange [ * : * ] noreverse nowriteback
set yrange [ 0 : `echo $PLOT_MAX_wH` ] noreverse nowriteback
set locale "C"
set title "`echo $PLOT_TITLE`"
plot "`echo $PLOT_DATAFILE`" \
    using 1:($3 * 60. / `echo $PLOT_wH_MINUTES`.) \
    title "Watts" `echo $PLOT_WITHSTYLE`
