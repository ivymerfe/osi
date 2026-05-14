#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#define SOCKET_PATH "/tmp/govno.sock"
#define BUFFER_SIZE 256
#define NUM_CLIENTS 64
#define REQUESTS_PER_CLIENT 5 

void run_client(int id) {
    int fd;
    struct sockaddr_un addr;
    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        fprintf(stderr, "[Клиент %d] Ошибка подключения\n", id);
        exit(EXIT_FAILURE);
    }

    // ЦИКЛ ОТПРАВКИ ЗАПРОСОВ
    for (int j = 1; j <= REQUESTS_PER_CLIENT; j++) {
        snprintf(send_buf, sizeof(send_buf), "Клиент %d, запрос %d", id, j);
        
        if (write(fd, send_buf, strlen(send_buf)) < 0) break;

        int n = read(fd, recv_buf, sizeof(recv_buf));
        if (n > 0) {
            printf("[Клиент %d] Получено: %.*s\n", id, n, recv_buf);
        }

        // Небольшая пауза, чтобы сервер успевал "переварить" запросы 
        // и мы видели параллельность в консоли
        usleep(100000); // 100 миллисекунд
    }

    close(fd);
    exit(0);
}

int main() {
    printf("Тестирование: %d клиентов по %d запросов...\n\n", NUM_CLIENTS, REQUESTS_PER_CLIENT);

    for (int i = 1; i <= NUM_CLIENTS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            run_client(i);
        } else if (pid < 0) {
            perror("fork error");
        }
    }

    for (int i = 0; i < NUM_CLIENTS; i++) {
        wait(NULL);
    }

    printf("\nВсе запросы обработаны.\n");
    return 0;
}