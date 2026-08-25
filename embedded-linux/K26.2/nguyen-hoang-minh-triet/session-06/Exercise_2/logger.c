#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define LOG_ERR "<3>"
#define LOG_WARNING "<4>"
#define LOG_INFO "<6>"

#define MAX_OF_CYCLE 15
#define CYCLE_INTERVAL 2

int main(void)
{
        int cycle = 0;

        setbuf(stdout, NULL);
        setbuf(stderr, NULL);

        srand((unsigned int)time(NULL));

        while (1)
        {
                cycle++;

                fprintf(stderr,
                        LOG_INFO
                        "Service running normally, cycle %d\n",
                        cycle);

                fprintf(stderr,
                        LOG_WARNING
                        "Memory usage high: %d%%\n",
                        80 + rand() % 15);

                fprintf(stderr,
                        LOG_ERR
                        "Failed to connect to database, retry %d\n",
                        cycle);

                sleep(CYCLE_INTERVAL);

                if (cycle >= MAX_OF_CYCLE)
                {
                        /*
                         * Intentionally abort the process after approximately 30 seconds
                         * to simulate a service crash for systemd/journalctl analysis.
                         */
                        fprintf(stderr,
                                LOG_ERR
                                "Maximum runtime reached, aborting service\n");

                        abort();
                }
        }

        return 0;
}
