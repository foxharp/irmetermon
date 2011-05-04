/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 */

void irmeter_init(void);
void led_handle(void);
void led_flash(void);
void show_adc(void);
void show_pulse(char full);
void tracker(void);


extern unsigned int adc_fastdump;
extern char use_median;
extern char up_only;
extern char step_size;
