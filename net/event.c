
#include "config.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include "misc.h"
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>


#include "evsys/tree.h"
#include "evsys/queue.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else 
#include "evsys/_time.h"
#endif


#ifndef WIN32
#include <unistd.h>
#endif

#include <errno.h>
#include <string.h>
#include <assert.h>

#include "event.h"
#include "event-internal.h"
#include "glog.h"

#ifdef HAVE_SELECT
extern const struct eventop selectops;
#endif
#ifdef HAVE_POLL
extern const struct eventop pollops;
#endif
#ifdef HAVE_RTSIG
extern const struct eventop rtsigops;
#endif
#ifdef HAVE_EPOLL
extern const struct eventop epollops;
#endif
#ifdef HAVE_WORKING_KQUEUE
extern const struct eventop kqops;
#endif
#ifdef HAVE_DEVPOLL
extern const struct eventop devpollops;
#endif
#ifdef WIN32
extern const struct eventop win32ops;
#endif

/* In order of preference */
const struct eventop *eventops[] = {
#ifdef HAVE_WORKING_KQUEUE
	&kqops,
#endif
#ifdef HAVE_EPOLL
	&epollops,
#endif
#ifdef HAVE_DEVPOLL
	&devpollops,
#endif
#ifdef HAVE_RTSIG
	&rtsigops,
#endif
//#ifdef HAVE_POLL
//	&pollops,
//#endif
//#ifdef HAVE_SELECT
//	&selectops,
//#endif
#ifdef WIN32
	&win32ops,
#endif
	NULL
};

/* Global state */
struct event_list signalqueue;

struct event_base *current_base = NULL;

/* Handle signals - This is a deprecated interface */
int (*event_sigcb)(void);	/* Signal callback when gotsig is set */
volatile int event_gotsig;	/* Set in signal handler */

/* Prototypes */
static void	event_queue_insert(struct event_base *, struct event *, int);
static void	event_queue_remove(struct event_base *, struct event *, int);
static int	event_haveevents(struct event_base *);

static void	event_process_active(struct event_base *);

static int	timeout_next(struct event_base *, struct timeval *);
static void	timeout_process(struct event_base *);
static void	timeout_correct(struct event_base *, struct timeval *);

static int
compare(struct event *a, struct event *b)
{
	if (timercmp(&a->ev_timeout, &b->ev_timeout, <))
		return (-1);
	else if (timercmp(&a->ev_timeout, &b->ev_timeout, >))
		return (1);

	if (a < b)
		return (-1);
	else if (a > b)
		return (1);

	return (0);
}

RB_PROTOTYPE(event_tree, event, ev_timeout_node, compare);

RB_GENERATE(event_tree, event, ev_timeout_node, compare);


void * event_init(void)
{
	int i;

	if(current_base)
		event_error("current_base no null.");

	if ((current_base = calloc(1, sizeof(struct event_base))) == NULL)
		event_error("calloc failed.");

	event_sigcb = NULL;
	event_gotsig = 0;
	gettimeofday(&current_base->event_tv, NULL);
	
	RB_INIT(&current_base->timetree);
	TAILQ_INIT(&current_base->eventqueue);
	TAILQ_INIT(&signalqueue);
	
	current_base->evbase = NULL;
	for (i = 0; eventops[i] && !current_base->evbase; i++) 
	{
		current_base->evsel = eventops[i];

		current_base->evbase = current_base->evsel->init();
	}

	if (current_base->evbase == NULL)
		event_error("%s: no event mechanism available", __func__);

	//if (getenv("EVENT_SHOW_METHOD")) 
		event_info("libevent using: %s", current_base->evsel->name);

	/* allocate a single active event queue */
	event_base_priority_init(current_base, 1);

	return (current_base);
}

void event_destroy()
{
	if(current_base!=NULL) 
	{
		free(current_base);
		current_base = NULL;
	}
}

int event_priority_init(int npriorities)
{
  	return event_base_priority_init(current_base, npriorities);
}

int event_base_priority_init(struct event_base *base, int npriorities)
{
	int i;

	if (base->event_count_active)
		return (-1);

	if (base->nactivequeues && npriorities != base->nactivequeues) 
	{
		for (i = 0; i < base->nactivequeues; ++i) 
		{
			free(base->activequeues[i]);
		}
		free(base->activequeues);
	}

	/* Allocate our priority queues */
	base->nactivequeues = npriorities;
	base->activequeues = (struct event_list **)calloc(base->nactivequeues,
	    npriorities * sizeof(struct event_list *));
	if (base->activequeues == NULL)
		event_error("%s: calloc", __func__);

	for (i = 0; i < base->nactivequeues; ++i) 
	{
		base->activequeues[i] = malloc(sizeof(struct event_list));
		if (base->activequeues[i] == NULL)
			event_error("%s: malloc", __func__);
		TAILQ_INIT(base->activequeues[i]);
	}

	return (0);
}

int event_haveevents(struct event_base *base)
{
	return (base->event_count > 0);
}

/*
 * Active events are stored in priority queues.  Lower priorities are always
 * process before higher priorities.  Low priority events can starve high
 * priority ones.
 */

static void event_process_active(struct event_base *base)
{
	struct event *ev;
	struct event_list *activeq = NULL;
	int i;
	short ncalls;

	if (!base->event_count_active)
		return;

	for (i = 0; i < base->nactivequeues; ++i) 
	{
		if (TAILQ_FIRST(base->activequeues[i]) != NULL) 
		{
			activeq = base->activequeues[i];
			break;
		}
	}

	for (ev = TAILQ_FIRST(activeq); ev; ev = TAILQ_FIRST(activeq)) 
	{
		event_queue_remove(base, ev, EVLIST_ACTIVE);
		
		/* Allows deletes to work */
		ncalls = ev->ev_ncalls;
		ev->ev_pncalls = &ncalls;
		while (ncalls) 
		{
			ncalls--;
			ev->ev_ncalls = ncalls;
			(*ev->ev_callback)((int)ev->ev_fd, ev->ev_res, ev->ev_arg);
		}
	}
}

/*
 * Wait continously for events.  We exit only if no events are left.
 */

int event_dispatch(void)
{
	return (event_loop(0));
}

int event_base_dispatch(struct event_base *event_base)
{
  return (event_base_loop(event_base, 0));
}

static void event_loopexit_cb(int fd, short what, void *arg)
{
	struct event_base *base = arg;
	base->event_gotterm = 1;
}

/* not thread safe */
int event_loopexit(struct timeval *tv)
{
	return (event_once(-1, EV_TIMEOUT, event_loopexit_cb, current_base, tv));
}

int event_base_loopexit(struct event_base *event_base, struct timeval *tv)
{
	return (event_once(-1, EV_TIMEOUT, event_loopexit_cb, event_base, tv));
}

/* not thread safe */
int event_loop(int flags)
{
	return event_base_loop(current_base, flags);
}

int event_base_loop(struct event_base *base, int flags)
{
	const struct eventop *evsel = base->evsel;
	void *evbase = base->evbase;
	struct timeval tv;
	int res, done;

	done = 0;
	while (!done) 
	{
		/* Calculate the initial events that we are waiting for */
		if (evsel->recalc(base, evbase, 0) == -1)
			return (-1);

		/* Terminate the loop if we have been asked to */
		if (base->event_gotterm) 
		{
			base->event_gotterm = 0;
			break;
		}

		/* You cannot use this interface for multi-threaded apps */
		while (event_gotsig) 
		{
			event_gotsig = 0;
			if (event_sigcb) 
			{
				res = (*event_sigcb)();
				if (res == -1) 
				{
					errno = EINTR;
					return (-1);
				}
			}
		}

		/* Check if time is running backwards */
		gettimeofday(&tv, NULL);
		if (timercmp(&tv, &base->event_tv, <)) 
		{
			struct timeval off;
			event_debug("%s: time is running backwards, corrected", __func__);

			timersub(&base->event_tv, &tv, &off);
			timeout_correct(base, &off);
		}
		base->event_tv = tv;

		if (!base->event_count_active && !(flags & EVLOOP_NONBLOCK))
			timeout_next(base, &tv);
		else
			timerclear(&tv);
		
		/* If we have no events, we just exit */
		if (!event_haveevents(base)) 
		{
			event_debug("%s: no events registered.", __func__);
			return (1);
		}

		res = evsel->dispatch(base, evbase, &tv);

		if (res == -1)
			return (-1);

		timeout_process(base);

		if (base->event_count_active) 
		{
			event_process_active(base);
			if (!base->event_count_active && (flags & EVLOOP_ONCE))
				done = 1;
		}
		else if (flags & EVLOOP_NONBLOCK)
			done = 1;
	}

	event_debug("%s: asked to terminate loop.", __func__);
	return (0);
}

/* Sets up an event for processing once */

struct event_once 
{
	struct event ev;

	void (*cb)(int, short, void *);
	void *arg;
};

/* One-time callback, it deletes itself */

static void event_once_cb(int fd, short events, void *arg)
{
	struct event_once *eonce = arg;

	(*eonce->cb)(fd, events, eonce->arg);
	free(eonce);
}

/* Schedules an event once */
int event_once(int fd, short events, void (*callback)(int, short, void *), void *arg, struct timeval *tv)
{
	struct event_once *eonce;
	struct timeval etv;

	/* We cannot support signals that just fire once */
	if (events & EV_SIGNAL)
		return (-1);

	if ((eonce = calloc(1, sizeof(struct event_once))) == NULL)
		return (-1);

	eonce->cb = callback;
	eonce->arg = arg;

	if (events == EV_TIMEOUT) 
	{
		if (tv == NULL) 
		{
			timerclear(&etv);
			tv = &etv;
		}

		evtimer_set(&eonce->ev, event_once_cb, eonce);
	} 
	else if (events & (EV_READ|EV_WRITE)) 
	{
		events &= EV_READ|EV_WRITE;

		event_set(&eonce->ev, fd, events, event_once_cb, eonce);
	} 
	else 
	{
		/* Bad event combination */
		free(eonce);
		return (-1);
	}

	event_add(&eonce->ev, tv);

	return (0);
}

void event_set(struct event *ev, int fd, short events, void (*callback)(int, short, void *), void *arg)
{
	/* Take the current base - caller needs to set the real base later */
	ev->ev_base = current_base;

	ev->ev_callback = callback;
	ev->ev_arg = arg;
	ev->ev_fd = fd;
	ev->ev_events = events;
	ev->ev_flags = EVLIST_INIT;
	ev->ev_ncalls = 0;
	ev->ev_pncalls = NULL;

	/* by default, we put new events into the middle priority */
	ev->ev_pri = current_base->nactivequeues/2;
}

int event_base_set(struct event_base *base, struct event *ev)
{
	/* Only innocent events may be assigned to a different base */
	if (ev->ev_flags != EVLIST_INIT)
		return (-1);

	ev->ev_base = base;
	ev->ev_pri = base->nactivequeues/2;

	return (0);
}

/*
 * Set's the priority of an event - if an event is already scheduled
 * changing the priority is going to fail.
 */

int event_priority_set(struct event *ev, int pri)
{
	if (ev->ev_flags & EVLIST_ACTIVE)
		return (-1);
	if (pri < 0 || pri >= ev->ev_base->nactivequeues)
		return (-1);

	ev->ev_pri = pri;

	return (0);
}

/*
 * Checks if a specific event is pending or scheduled.
 */
int event_pending(struct event *ev, short event, struct timeval *tv)
{
	int flags = 0;

	if (ev->ev_flags & EVLIST_INSERTED)
		flags |= (ev->ev_events & (EV_READ|EV_WRITE));
	if (ev->ev_flags & EVLIST_ACTIVE)
		flags |= ev->ev_res;
	if (ev->ev_flags & EVLIST_TIMEOUT)
		flags |= EV_TIMEOUT;
	if (ev->ev_flags & EVLIST_SIGNAL)
		flags |= EV_SIGNAL;

	event &= (EV_TIMEOUT|EV_READ|EV_WRITE|EV_SIGNAL);

	/* See if there is a timeout that we should report */
	if (tv != NULL && (flags & event & EV_TIMEOUT))
		*tv = ev->ev_timeout;

	return (flags & event);
}

int event_add(struct event *ev, struct timeval *tv)
{
	struct event_base *base = ev->ev_base;
	const struct eventop *evsel = base->evsel;
	void *evbase = base->evbase;

	event_debug(
		 "event_add: event: %p, %s%s%scall %p",
		 ev,
		 ev->ev_events & EV_READ ? "EV_READ " : " ",
		 ev->ev_events & EV_WRITE ? "EV_WRITE " : " ",
		 tv ? "EV_TIMEOUT " : " ",
		 ev->ev_callback);

	assert(!(ev->ev_flags & ~EVLIST_ALL));

	if (tv != NULL) 
	{
		struct timeval now;

		if (ev->ev_flags & EVLIST_TIMEOUT)
			event_queue_remove(base, ev, EVLIST_TIMEOUT);

		/* Check if it is active due to a timeout.  Rescheduling
		 * this timeout before the callback can be executed
		 * removes it from the active list. */
		if ((ev->ev_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)) 
		{
			/* See if we are just active executing this
			 * event in a loop
			 */
			if (ev->ev_ncalls && ev->ev_pncalls) 
			{
				/* Abort loop */
				*ev->ev_pncalls = 0;
			}
			
			event_queue_remove(base, ev, EVLIST_ACTIVE);
		}

		gettimeofday(&now, NULL);
		timeradd(&now, tv, &ev->ev_timeout);

		event_debug(
			 "event_add: timeout in %ld seconds, call %p",
			 tv->tv_sec, ev->ev_callback);

		event_queue_insert(base, ev, EVLIST_TIMEOUT);
	}

	if ((ev->ev_events & (EV_READ|EV_WRITE)) && !(ev->ev_flags & (EVLIST_INSERTED|EVLIST_ACTIVE))) 
	{
		event_queue_insert(base, ev, EVLIST_INSERTED);

		return (evsel->add(evbase, ev));
	} 
	else if ((ev->ev_events & EV_SIGNAL) && !(ev->ev_flags & EVLIST_SIGNAL)) 
	{
		event_queue_insert(base, ev, EVLIST_SIGNAL);

		return (evsel->add(evbase, ev));
	}

	return (0);
}

int event_del(struct event *ev)
{
	struct event_base *base;
	const struct eventop *evsel;
	void *evbase;

	event_debug("event_del: %p, callback %p",
		ev, ev->ev_callback);

	/* An event without a base has not been added */
	if (ev->ev_base == NULL)
		return (-1);

	base = ev->ev_base;
	evsel = base->evsel;
	evbase = base->evbase;

	event_error("assert begin.");
	assert(!(ev->ev_flags & ~EVLIST_ALL));
	event_error("assert done %d %d", ev->ev_flags, ~EVLIST_ALL);

	/* See if we are just active executing this event in a loop */
	if (ev->ev_ncalls && ev->ev_pncalls) 
	{
		/* Abort loop */
		*ev->ev_pncalls = 0;
	}

	if (ev->ev_flags & EVLIST_TIMEOUT)
		event_queue_remove(base, ev, EVLIST_TIMEOUT);

	if (ev->ev_flags & EVLIST_ACTIVE)
		event_queue_remove(base, ev, EVLIST_ACTIVE);

	if (ev->ev_flags & EVLIST_INSERTED) 
	{
		event_queue_remove(base, ev, EVLIST_INSERTED);
		return (evsel->del(evbase, ev));
	}
	else if (ev->ev_flags & EVLIST_SIGNAL) 
	{
		event_queue_remove(base, ev, EVLIST_SIGNAL);
		return (evsel->del(evbase, ev));
	}

	event_error("done...");

	return (0);
}

void event_active(struct event *ev, int res, short ncalls)
{
	/* We get different kinds of events, add them together */
	if (ev->ev_flags & EVLIST_ACTIVE) 
	{
		ev->ev_res |= res;
		return;
	}

	ev->ev_res = res;
	ev->ev_ncalls = ncalls;
	ev->ev_pncalls = NULL;
	event_queue_insert(ev->ev_base, ev, EVLIST_ACTIVE);
}

int timeout_next(struct event_base *base, struct timeval *tv)
{
	struct timeval dflt = TIMEOUT_DEFAULT;

	struct timeval now;
	struct event *ev;

	if ((ev = RB_MIN(event_tree, &base->timetree)) == NULL) 
	{
		*tv = dflt;
		return (0);
	}

	if (gettimeofday(&now, NULL) == -1)
		return (-1);

	if (timercmp(&ev->ev_timeout, &now, <=)) 
	{
		timerclear(tv);
		return (0);
	}

	timersub(&ev->ev_timeout, &now, tv);

	assert(tv->tv_sec >= 0);
	assert(tv->tv_usec >= 0);

	event_debug("timeout_next: in %ld seconds", tv->tv_sec);

	return (0);
}

static void timeout_correct(struct event_base *base, struct timeval *off)
{
	struct event *ev;

	/*
	 * We can modify the key element of the node without destroying
	 * the key, beause we apply it to all in the right order.
	 */
	RB_FOREACH(ev, event_tree, &base->timetree)
		timersub(&ev->ev_timeout, off, &ev->ev_timeout);
}

void timeout_process(struct event_base *base)
{
	struct timeval now;
	struct event *ev, *next;

	gettimeofday(&now, NULL);

	for (ev = RB_MIN(event_tree, &base->timetree); ev; ev = next) 
	{
		if (timercmp(&ev->ev_timeout, &now, >))
			break;
		next = RB_NEXT(event_tree, &base->timetree, ev);

		event_queue_remove(base, ev, EVLIST_TIMEOUT);

		/* delete this event from the I/O queues */
		event_del(ev);

		event_debug("timeout_process: call %p",
			 ev->ev_callback);
		event_active(ev, EV_TIMEOUT, 1);
	}
}

void event_queue_remove(struct event_base *base, struct event *ev, int queue)
{
	int docount = 1;

	if (!(ev->ev_flags & queue))
		event_error( "%s: %p(fd %d) not on queue %x", __func__,
			   ev, ev->ev_fd, queue);

	if (ev->ev_flags & EVLIST_INTERNAL)
		docount = 0;

	if (docount)
		base->event_count--;

	ev->ev_flags &= ~queue;
	switch (queue) 
	{
		case EVLIST_ACTIVE:
			if (docount)
				base->event_count_active--;
			TAILQ_REMOVE(base->activequeues[ev->ev_pri],
				ev, ev_active_next);
			break;

		case EVLIST_SIGNAL:
			TAILQ_REMOVE(&signalqueue, ev, ev_signal_next);
			break;

		case EVLIST_TIMEOUT:
			RB_REMOVE(event_tree, &base->timetree, ev);
			break;

		case EVLIST_INSERTED:
			TAILQ_REMOVE(&base->eventqueue, ev, ev_next);
			break;
		default:
			event_error( "%s: unknown queue %x", __func__, queue);
	}
}

void event_queue_insert(struct event_base *base, struct event *ev, int queue)
{
	int docount = 1;

	if (ev->ev_flags & queue) 
	{
		/* Double insertion is possible for active events */
		if (queue & EVLIST_ACTIVE)
			return;

		event_error("%s: %p(fd %d) already on queue %x", __func__,
			   ev, ev->ev_fd, queue);
	}

	if (ev->ev_flags & EVLIST_INTERNAL)
		docount = 0;

	if (docount)
		base->event_count++;

	ev->ev_flags |= queue;
	switch (queue) 
	{
		case EVLIST_ACTIVE:
			if (docount)
				base->event_count_active++;
			TAILQ_INSERT_TAIL(base->activequeues[ev->ev_pri], ev,ev_active_next);
			break;

		case EVLIST_SIGNAL:
			TAILQ_INSERT_TAIL(&signalqueue, ev, ev_signal_next);
			break;

		case EVLIST_TIMEOUT: {
			struct event *tmp = RB_INSERT(event_tree, &base->timetree, ev);
			assert(tmp == NULL);
			break;
		}

		case EVLIST_INSERTED:
			TAILQ_INSERT_TAIL(&base->eventqueue, ev, ev_next);
			break;
		default:
			event_error("%s: unknown queue %x", __func__, queue);
	}
}

/* Functions for debugging */

const char * event_get_version(void)
{
	return (VERSION);
}

/* 
 * No thread-safe interface needed - the information should be the same
 * for all threads.
 */

const char *event_get_method(void)
{
	return (current_base->evsel->name);
}
