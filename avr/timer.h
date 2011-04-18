
typedef unsigned long time_t;


void init_timer(void);
time_t get_ms_timer(void);
unsigned char check_timer(time_t time0, int duration);
