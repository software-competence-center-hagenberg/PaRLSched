/*
 * ThreadSuspendControl.h
 *
 *  Created on: May 2, 2016
 *      Author: chasparis
 * Description: This library collects functions with respect to 'suspending' a thread. Currently is not being used by the scheduler.
 * 				This library can be found under boinc.berkley.edu
*/

#ifndef SRC_THREADSUSPENDCONTROL_H_
#define SRC_THREADSUSPENDCONTROL_H_

#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>      /* printf, scanf, puts */
#include <stdlib.h>     /* realloc, free, exit, NULL */
#include <unistd.h>

volatile int sentinel = 0;
pthread_once_t once = PTHREAD_ONCE_INIT;
pthread_t* array = NULL;
pthread_t null_pthread = {0};

pthread_mutex_t the_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int bottom = 0;
int inited = 0;


/**
 * Handle SIGUSR1 in the target thread, to suspend it until
 * receiving SIGUSR2 (resume).
 *
 * @param sig
 */
void suspend_signal_handler (int sig)
{
    sigset_t signal_set;
    /*
     * Block all signals except SIGUSR2 while suspended.
    */
    sigfillset(&signal_set);
    sigdelset(&signal_set, SIGUSR2);
    sentinel = 1;

    /**
     * temporarily replaces the signal mask with the mask "signal_set" and then
     * suspends the process until delivery of a signal whose action is to invoke
     * a signal handler or to terminate a process.
     * */
    sigsuspend(&signal_set);

    /*
     * Once I'm here, I've been resumed, and the resume signal
     * handler has been run to completion.
    */
    return;
}


/**
 * Handle SIGUSR2 in the target thread, to resume it. Note that
 * the signal handler does nothing. It exists only because we need
 * to cause sigsuspend() to return.
 *
 * @param sig
 */
void resume_signal_handler (int sig)
{
    return;
}


/**
 * Dynamically initialize the "suspend package" when first used
 * (called by pthread_once).
 */
void suspend_init_routine (void)
{

    int status;
    struct sigaction sigusr1, sigusr2;

    /*
     * Allocate the suspended threads array. This array is used
     * to guarentee idempotency
    */
    bottom = 10;
    array = (pthread_t*) calloc (bottom, sizeof (pthread_t));

    /*
     * Install the signal handlers for suspend/resume.
    */
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = suspend_signal_handler;
    sigemptyset (&sigusr1.sa_mask);
    sigusr2.sa_flags = 0;
    sigusr2.sa_handler = resume_signal_handler;
    sigusr2.sa_mask = sigusr1.sa_mask;
    status = sigaction (SIGUSR1, &sigusr1, NULL);
    if (status == -1)
        //errno_abort ("Installing suspend handler");
    	std::cout << "Installing suspend handler" << std::endl;
    status = sigaction (SIGUSR2, &sigusr2, NULL);
    if (status == -1)
        //errno_abort ("Installing resume handler");
    	std::cout << "Installing resume handler" << std::endl;

    inited = 1;

    return;
}


/**
 * Suspend a thread by sending it a signal (SIGUSR1), which will
 * block the thread until another signal (SIGUSR2) arrives.
 *
 * Multiple calls to thd_suspend for a single thread have no
 * additional effect on the thread -- a single thd_continue
 * call will cause it to resume execution.
 *
 * @param target_thread
 * @return
 */
int thd_suspend (pthread_t target_thread)
{
    int status;
    int i = 0;

    /*
     * The first call to thd_suspend will initialize the
     * package.
    */
    status = pthread_once(&once, suspend_init_routine);
    if (status != 0)
        return status;

    /*
     * Serialize access to suspend, makes life easier
    */
    status = pthread_mutex_lock (&mut);
    if (status != 0)
        return status;

    /*
     * Threads that are suspended are added to the target_array;
     * a request to suspend a thread already listed in the array
     * is ignored. Sending a second SIGUSR1 would cause the
     * thread to re-suspend itself as soon as it is resumed.
    */
    while (i < bottom)
        if (array[i++] == target_thread)
        {
            status = pthread_mutex_unlock(&mut);
            return status;
        }

    /*
     * Find the location in the array that we'll use. If we run off
     * the end, realloc the array for more space.
    */
    i = 0;
    while (array[i] != 0)
        i++;

    if (i == bottom)
    {
        array = (pthread_t*) realloc (array, (++bottom * sizeof (pthread_t)));
        if (array == NULL) {
            pthread_mutex_unlock(&mut);
            // return errno;
            return 0;
        }
        array[bottom] = null_pthread;   /* Clear new entry */
    }

    /*
     * Clear the sentinel and signal the thread to suspend.
    */
    sentinel = 0;
    status = pthread_kill (target_thread, SIGUSR1);
    if (status != 0) {
        pthread_mutex_unlock(&mut);
        return status;
    }

    /*
     * Wait for the sentinel to change.
     */
    while (sentinel == 0)
        sched_yield ();

    array[i] = target_thread;
    status = pthread_mutex_unlock (&mut);
    return status;

}


/**
 * Resume a suspended thread by sending it SIGUSR2 to break
 * it out of the sigsuspend() in which it's waiting. If the
 * target thread isn't suspended, return with success.
 *
 * @param target_thread
 * @return
 */
int thd_continue (pthread_t target_thread)
{
    int status;
    int i = 0;

    /*
     * Serialize access to suspend, makes life easier
     */
    status = pthread_mutex_lock (&mut);
    if (status != 0)
        return status;

    /*
     * If we haven't been initialized, then the thread must be "resumed"
     * it couldn't have been suspended!
     */
    if (!inited) {
        status = pthread_mutex_unlock (&mut);
        return status;
    }

    /*
     * Make sure the thread is in the suspend array. If not, it
     * hasn't been suspended (or it has already been resumed) and
     * we can just carry on.
     */
    while (array[i] != target_thread && i < bottom)
        i++;

    if (i >= bottom) {
        pthread_mutex_unlock(&mut);
        return 0;
    }

    /*
     * Signal the thread to continue, and remove the thread from
     * the suspended array.
     */
    status = pthread_kill (target_thread, SIGUSR2);
    if (status != 0) {
        pthread_mutex_unlock(&mut);
        return status;
    }

    array[i] = 0;               /* Clear array element */
    status = pthread_mutex_unlock (&mut);
    return status;

}


#endif /* SRC_THREADSUSPENDCONTROL_H_ */
