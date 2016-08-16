/*
timer.c
*/

#include <types.h>
#include <x86.h>
#include <mutex.h>


mutex timer_lock = { .lock = 0 };
uint32_t ticks = 0;
char* timer_buf = 0;


extern uint32_t K_SCHED_ACTIVE;
extern uint32_t K_SCHED_TIME;



void timer(regs_t *r) {
	ticks++;
	thread_add_ticks();
	if (!(ticks%1)) {
		itoa(ticks, timer_buf, 10);
		vga_kputs(timer_buf, 150, 0);
	}

	if (ticks % K_SCHED_TIME == 0)
		return k_schedule(r);
	return r;
}

void timer_init() {
	ticks = 0;
	timer_buf = malloc(8);
	memset(timer_buf, 0, 8);
	irq_install_handler(0, timer);
}

uint32_t get_ticks(){
	return ticks;
}

void wait(int n) {
	spin_lock(&timer_lock);
	int wait_for = ticks + n;
	while (wait_for > ticks) {
		preempt();	
	}
	spin_unlock(&timer_lock);
	return;
}

