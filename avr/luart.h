/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 */
uint8_t sgetchar(void);
int kbhit(void);
void sputchar(char c);
void sputstring(const char *s);
void sputstring_p(const prog_char * s);
void luart_run(void);
void luart_init(void);