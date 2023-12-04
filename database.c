#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

sqlite3* db; // Глобальная переменная для хранения соединения с базой данных

// Функция для обработки ошибок SQLite
void handle_sqlite_error(int rc, const char* operation) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error during %s: %s\n", operation, sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

// 1. Создать базу данных
void create_database() {
    int rc = sqlite3_open("mydatabase.db", &db);
    handle_sqlite_error(rc, "opening database");

    // Закрываем базу данных, так как нам нужно только создать файл базы данных
    sqlite3_close(db);
}

// 2. Добавить поля логин и пароль в таблицу "Пользователи"
void create_users_table() {
    int rc = sqlite3_open("mydatabase.db", &db);
    handle_sqlite_error(rc, "opening database");

    const char* create_users_table_sql = "CREATE TABLE IF NOT EXISTS Users (id INTEGER PRIMARY KEY, login TEXT, password TEXT);";
    
    rc = sqlite3_exec(db, create_users_table_sql, 0, 0, 0);
    handle_sqlite_error(rc, "creating Users table");

    sqlite3_close(db);
}

// 3. Поиск по логину в таблице "Пользователи"
int find_user_by_login(const char* login) {
    int rc = sqlite3_open("mydatabase.db", &db);
    handle_sqlite_error(rc, "opening database");

    sqlite3_stmt* stmt;
    const char* find_user_sql = "SELECT id FROM Users WHERE login = ?;";
    rc = sqlite3_prepare_v2(db, find_user_sql, -1, &stmt, 0);
    handle_sqlite_error(rc, "preparing find_user_by_login statement");

    rc = sqlite3_bind_text(stmt, 1, login, -1, SQLITE_STATIC);
    handle_sqlite_error(rc, "binding login parameter");

    int user_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return user_id;
}

// 4. Добавить запись в таблицу "Сообщения"
void add_message(const char* login, const char* login_to, const char* title, const char* data) {
    int rc = sqlite3_open("mydatabase.db", &db);
    handle_sqlite_error(rc, "opening database");

    const char* add_message_sql = "INSERT INTO Messages (login, login_to, time, title, data) VALUES (?, ?, CURRENT_TIMESTAMP, ?, ?);";

    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, add_message_sql, -1, &stmt, 0);
    handle_sqlite_error(rc, "preparing add_message statement");

    rc = sqlite3_bind_text(stmt, 1, login, -1, SQLITE_STATIC);
    handle_sqlite_error(rc, "binding login parameter");

    rc = sqlite3_bind_text(stmt, 2, login_to, -1, SQLITE_STATIC);
    handle_sqlite_error(rc, "binding login_to parameter");

    rc = sqlite3_bind_text(stmt, 3, title, -1, SQLITE_STATIC);
    handle_sqlite_error(rc, "binding title parameter");

    rc = sqlite3_bind_text(stmt, 4, data, -1, SQLITE_STATIC);
    handle_sqlite_error(rc, "binding data parameter");

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error during add_message: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// 5. Количество записей с определенным значением в "login_to" в таблице "Сообщения"
int count_messages_by_login_to(const char* login_to) {
    int rc = sqlite3_open("mydatabase.db", &db);
    handle_sqlite_error(rc, "opening database");

    sqlite3_stmt* stmt;
    const char* count_messages_sql = "SELECT COUNT(*) FROM Messages WHERE login_to = ?;";

    rc = sqlite3_prepare_v2(db, count_messages_sql, -1, &stmt, 0);
    handle_sqlite_error(rc, "preparing count_messages_by_login_to statement");

    rc = sqlite3_bind_text(stmt, 1, login_to, -1, SQLITE_STATIC);
    handle_sqlite_error(rc, "binding login_to parameter");

    int message_count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        message_count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return message_count;
}

// 6. Получить определенную строку в таблице "Сообщения" по индексу
void get_message_by_index(int index, char* result) {
    int rc = sqlite3_open("mydatabase.db", &db);
    handle_sqlite_error(rc, "opening database");

    sqlite3_stmt* stmt;
    const char* get_message_sql = "SELECT * FROM Messages LIMIT 1 OFFSET ?;";

    rc = sqlite3_prepare_v2(db, get_message_sql, -1, &stmt, 0);
    handle_sqlite_error(rc, "preparing get_message_by_index statement");

    rc = sqlite3_bind_int(stmt, 1, index);
    handle_sqlite_error(rc, "binding index parameter");

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // Пример получения данных из результата запроса
        int message_id = sqlite3_column_int(stmt, 0);
        const char* login = (const char*)sqlite3_column_text(stmt, 1);
        const char* login_to = (const char*)sqlite3_column_text(stmt, 2);
        const char* time = (const char*)sqlite3_column_text(stmt, 3);
        const char* title = (const char*)sqlite3_column_text(stmt, 4);
        const char* data = (const char*)sqlite3_column_text(stmt, 5);

        // Пример вывода данных в строку
        snprintf(result, 256, "ID: %d, Login: %s, Login_to: %s, Time: %s, Title: %s, Data: %s",
                 message_id, login, login_to, time, title, data);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// 7. Удалить определенную строку в таблице "Сообщения" по индексу
void delete_message_by_index(int index) {
    int rc = sqlite3_open("mydatabase.db", &db);
    handle_sqlite_error(rc, "opening database");

    const char* delete_message_sql = "DELETE FROM Messages WHERE id IN (SELECT id FROM Messages LIMIT 1 OFFSET ?);";

    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, delete_message_sql, -1, &stmt, 0);
    handle_sqlite_error(rc, "preparing delete_message_by_index statement");

    rc = sqlite3_bind_int(stmt, 1, index);
    handle_sqlite_error(rc, "binding index parameter");

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error during delete_message_by_index: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int main() {
    // Вызываем нужные функции для работы с базой данных
    create_database();
    create_users_table();

    // Пример использования функций
    add_message("user1", "user2", "Hello", "How are you?");
    int count = count_messages_by_login_to("user2");
    printf("Number of messages for user2: %d\n", count);

    char result[256];
    get_message_by_index(0, result);
    printf("Message at index 0: %s\n", result);

    delete_message_by_index(0);

    return 0;
}
