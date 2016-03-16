#ifndef _TIMEOUT_H_
#define _TIMEOUT_H_

typedef struct _timeout_t timeout_t;
typedef void (* timeout_handler)(timeout_t *to, void *arg);

struct _timeout_t
{
    list_t item;
    
    timer_t *timer;
    
    mutex_t *mutex;
    void *signal;
    
    timeout_handler handler;
    void *handler_arg;
    
    unsigned long interval;
    unsigned long cur_time;
    
    int autoreload;
};

void timeout_init (timeout_t *to, timer_t *timer, mutex_t *mutex);
void timeout_set_signal (timeout_t *to, void *signal);
void timeout_set_value (timeout_t *to, unsigned long interval_msec);
void timeout_set_value_us (timeout_t *to, unsigned long interval_usec);
void timeout_set_autoreload (timeout_t *to, int autoreload);
void timeout_set_handler (timeout_t *to, timeout_handler handler, void *arg);
void timeout_start (timeout_t *to);
void timeout_stop (timeout_t *to);


#endif
