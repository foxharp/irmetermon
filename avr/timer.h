/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 */

typedef unsigned long time_t;


void init_timer(void);
time_t get_ms_timer(void);
unsigned char check_timer(time_t time0, int duration);
