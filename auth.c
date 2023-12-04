#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mhash.h>

#define TOKEN_LENGTH 40

// Генерация случайного токена
char* generate_token() {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* token = (char*)malloc((TOKEN_LENGTH + 1) * sizeof(char));

    for (int i = 0; i < TOKEN_LENGTH; ++i) {
        int index = rand() % (sizeof(charset) - 1);
        token[i] = charset[index];
    }

    token[TOKEN_LENGTH] = '\0';
    return token;
}

// Функция аутентификации
int authenticate(const char* login, const char* password) {
    FILE* database_file = fopen("database.txt", "r");
    FILE* tokens_file = fopen("tokens.txt", "a");

    if (!database_file || !tokens_file) {
        perror("Error opening files");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), database_file) != NULL) {
        char stored_login[256];
        char stored_hash[16];  // Размер хэша MD4

        // Remove newline character
        line[strcspn(line, "\n")] = '\0';

        if (sscanf(line, "%s %s", stored_login, stored_hash) == 2 &&
            strcmp(login, stored_login) == 0) {
            // Найдено совпадение логина, теперь проверим хэш пароля
            MHASH td;
            td = mhash_init(MHASH_MD4);

            mhash(td, password, strlen(password));

            unsigned char input_hash[16];
            mhash_deinit(td, input_hash);

            char input_hash_str[32];  // Размер для хранения хэша в виде строки
            for (int i = 0; i < 16; i++) {
                sprintf(&input_hash_str[i * 2], "%02x", input_hash[i]);
            }

            if (strcmp(input_hash_str, stored_hash) == 0) {
                // Пароль совпадает, генерируем токен
                char* token = generate_token();
                fprintf(tokens_file, "%s %s\n", login, token);
                fclose(database_file);
                fclose(tokens_file);
                return 1;
            }
        }
    }

    fclose(database_file);
    fclose(tokens_file);
    return 0;  // Аутентификация неудачна
}

int main() {
    const char* login = "example_user";
    const char* password = "example_password";

    if (authenticate(login, password)) {
        printf("Authentication successful\n");
    } else {
        printf("Authentication failed\n");
    }

    return 0;
}
