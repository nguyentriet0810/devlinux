#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t keep_running = 1;

void sigterm_handler(int signum)
{
    if (signum == SIGTERM)
    {
        keep_running = 0;
    }
}

int main()
{
    struct sigaction action;
    action.sa_handler = sigterm_handler;

    sigemptyset(&action.sa_mask);
    if (sigemptyset(&action.sa_mask) == -1)
    {
        perror("sigemptyset");
        return EXIT_FAILURE;
    }

    action.sa_flags = 0;

    sigaction(SIGTERM, &action, NULL);
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // turnoff buffer for stdout write log into systemd journal
    setbuf(stdout, NULL);

    printf("Monitor service started.\n");

    // infinity loop, retry every 1s
    while (keep_running)
    {
        printf("Monitor is running...\n");
        sleep(1);
    }

    // write notify when systemctl stop
    printf("Service shutting down...\n");
    return 0;
}
