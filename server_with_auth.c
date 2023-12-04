#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#define PORT 12345
#define MAX_CLIENTS 10

typedef struct {
    int client_socket;
    char command[256];
} ClientData;

typedef struct {
    char nickname[256];
} AuthData;

unsigned long long factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

int authenticate(char* nickname) {
    FILE* file = fopen("nickname.txt", "r");
    if (!file) {
        perror("Error opening nickname file");
        return 0;  // Authentication failed
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove newline character
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, nickname) == 0) {
            fclose(file);
            return 1;  // Authentication successful
        }
    }

    fclose(file);
    return 0;  // Authentication failed
}

void handle_command(ClientData* client_data, AuthData* auth_data) {
    char response[256];
    memset(response, 0, sizeof(response));

    if (strncmp(client_data->command, "auth", 4) == 0) {
        char nickname[256];
        if (sscanf(client_data->command + 4, "%s", nickname) == 1) {
            if (authenticate(nickname)) {
                strncpy(auth_data->nickname, nickname, sizeof(auth_data->nickname));
                snprintf(response, sizeof(response), "Authentication successful\n");
            } else {
                snprintf(response, sizeof(response), "Authentication failed\n");
            }
        } else {
            snprintf(response, sizeof(response), "Invalid argument for auth\n");
        }
    } else if (strncmp(client_data->command, "factorial", 9) == 0) {
        if (strlen(auth_data->nickname) > 0) {
            int n;
            if (sscanf(client_data->command + 9, "%d", &n) == 1) {
                snprintf(response, sizeof(response), "Factorial: %llu\n", factorial(n));
            } else {
                snprintf(response, sizeof(response), "Invalid argument for factorial\n");
            }
        } else {
            snprintf(response, sizeof(response), "Authentication required for factorial\n");
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

    fd_set read_fds;
    fd_set master_fds;
    int max_fd = server_socket;

    FD_ZERO(&master_fds);
    FD_SET(server_socket, &master_fds);

    printf("Server listening on port %d...\n", PORT);

    AuthData auth_data;
    memset(&auth_data, 0, sizeof(auth_data));

    while (1) {
        read_fds = master_fds;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Error in select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= max_fd; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    // Новое подключение
                    int client_socket = accept(server_socket, NULL, NULL);
                    if (client_socket != -1) {
                        FD_SET(client_socket, &master_fds);
                        if (client_socket > max_fd) {
                            max_fd = client_socket;
                        }
                    }
                } else {
                    // Данные от клиента
                    char buffer[256];
                    ssize_t bytes_received = recv(i, buffer, sizeof(buffer), 0);

                    if (bytes_received > 0) {
                        // Обработка команды
                        ClientData* client_data = (ClientData*)malloc(sizeof(ClientData));
                        client_data->client_socket = i;
                        strncpy(client_data->command, buffer, sizeof(client_data->command));

                        handle_command(client_data, &auth_data);
                    } else if (bytes_received == 0) {
                        // Сокет был закрыт клиентом
                        close(i);
                        FD_CLR(i, &master_fds);
                        if (i == max_fd) {
                            while (!FD_ISSET(max_fd, &master_fds)) {
                                max_fd--;
                            }
                        }
                    }
                }
            }
        }
    }

    // Закрытие серверного сокета
    close(server_socket);

    return 0;
}
