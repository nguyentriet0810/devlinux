#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>

#define SLEEP_TIME 1

volatile sig_atomic_t keep_running = 1;

void sigterm_handler(int signum)
{
    if ((signum == SIGINT) || (signum == SIGTERM))
    {
        keep_running = 0;
    }
}

int main()
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));

    action.sa_handler = sigterm_handler;
    action.sa_flags = 0;

    if (sigemptyset(&action.sa_mask) == -1)
    {
        perror("sigemptyset");
        return EXIT_FAILURE;
    }

    // register handler if user pess ctrl + c
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction SIGINT");
        return EXIT_FAILURE;
    }

    // register handler if systemd stop process
    if (sigaction(SIGTERM, &action, NULL) == -1)
    {
        perror("sigaction SIGTERM");
        return EXIT_FAILURE;
    }

    // turnoff buffer for stdout write log into systemd journal
    setbuf(stdout, NULL);

    printf("Monitor service started.\n");

    // infinity loop, retry every 1s
    while (keep_running)
    {
        printf("Monitor is running...\n");
        sleep(SLEEP_TIME);
    }

    // write notify when systemctl stop
    printf("Service shutting down...\n");
    return EXIT_SUCCESS;
}
