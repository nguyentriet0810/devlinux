#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Biến cờ toàn cục báo hiệu vòng lặp tiếp tục chạy
volatile sig_atomic_t keep_running = 1;

// Hàm xử lý tín hiệu
void sigterm_handler(int signum)
{
    if (signum == SIGTERM)
    {
        keep_running = 0; // Đổi cờ để thoát vòng lặp an toàn
    }
}

int main()
{
    // Đăng ký bắt tín hiệu SIGTERM
    struct sigaction action;
    action.sa_handler = sigterm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGTERM, &action, NULL);

    // Tắt bộ đệm cho stdout để log ghi thẳng vào systemd journal
    setbuf(stdout, NULL);

    printf("Monitor service started.\n");

    // Vòng lặp vô hạn, in ra log mỗi 1 giây
    while (keep_running)
    {
        printf("Monitor is running...\n");
        sleep(1);
    }

    // In dòng thông báo khi bị ngắt bởi systemctl stop
    printf("Service shutting down...\n");
    return 0;
}
