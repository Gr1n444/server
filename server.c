#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>

#define MAX_EVENTS 64
#define PORT 12345

// Структура для хранения данных о клиенте
typedef struct {
    int client_socket;
    char command[256];
} ClientData;

// Функция для вычисления факториала
unsigned long long factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

// Функция для вычисления экспоненты
double exponent(double x) {
    return exp(x);
}

// Функция для обработки команды и отправки ответа
void handle_command(ClientData* client_data) {
    char response[256];
    memset(response, 0, sizeof(response));

    if (strncmp(client_data->command, "factorial", 9) == 0) {
        int n;
        if (sscanf(client_data->command + 9, "%d", &n) == 1) {
            snprintf(response, sizeof(response), "Factorial: %llu\n", factorial(n));
        } else {
            snprintf(response, sizeof(response), "Invalid argument for factorial\n");
        }
    } else if (strncmp(client_data->command, "exponent", 8) == 0) {
        double x;
        if (sscanf(client_data->command + 8, "%lf", &x) == 1) {
            snprintf(response, sizeof(response), "Exponent: %lf\n", exponent(x));
        } else {
            snprintf(response, sizeof(response), "Invalid argument for exponent\n");
        }
    } else {
        snprintf(response, sizeof(response), "Unknown command\n");
    }

    // Отправка ответа клиенту
    send(client_data->client_socket, response, strlen(response), 0);

    // Закрытие сокета и освобождение ресурсов
    close(client_data->client_socket);
    free(client_data);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("Error listening");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Создание неблокирующего сокета
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    // Создание и настройка объекта kevent
    int kq = kqueue();
    struct kevent server_event;
    EV_SET(&server_event, server_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);

    // Регистрация серверного сокета в очереди событий
    if (kevent(kq, &server_event, 1, NULL, 0, NULL) == -1) {
        perror("Error registering server socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        struct kevent events[MAX_EVENTS];
        int event_count = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);

        for (int i = 0; i < event_count; ++i) {
            int socket_fd = events[i].ident;

            if (socket_fd == server_socket) {
                // Принятие нового подключения
                int client_socket = accept(server_socket, NULL, NULL);
                if (client_socket != -1) {
                    // Регистрация нового клиентского сокета в очереди событий
                    struct kevent client_event;
                    EV_SET(&client_event, client_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    kevent(kq, &client_event, 1, NULL, 0, NULL);
                }
            } else {
                // Чтение данных от клиента
                char buffer[256];
                ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);

                if (bytes_received > 0) {
                    // Обработка команды в фоновом режиме
                    ClientData* client_data = (ClientData*)malloc(sizeof(ClientData));
                    client_data->client_socket = socket_fd;
                    strncpy(client_data->command, buffer, sizeof(client_data->command));

                    handle_command(client_data);
                } else {
                    // Закрытие сокета при ошибке или завершении клиента
                    close(socket_fd);
                }
            }
        }
    }

    // Закрытие серверного сокета
    close(server_socket);

    return 0;
}
