#include "../include/database.h"
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>

char* db_file = NULL;

int database_initialize(char* database_file) {
    db_file = strdup(database_file);
    sqlite3* db = NULL;

    int status = sqlite3_open(db_file, &db);
    if (status != SQLITE_OK)
        return ERROR;

    /* create the tables */
    /* credentials table */
    status = sqlite3_exec(db, "create table if not exists credentials(username varchar, password text)", NULL, NULL, NULL);
    if (status != SQLITE_OK)
        return ERROR;

    /* session id table */
    status = sqlite3_exec(db, "create table if not exists session_ids(username varchar, session_id text)", NULL, NULL, NULL);
    if (status != SQLITE_OK)
        return ERROR;

    sqlite3_close(db);
    return OK;
}

int database_add_credential(char* username, char* password) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    int status = sqlite3_open(db_file, &db);
    if (status != SQLITE_OK)
        return ERROR;

    char* sql = "insert into credentials(username, password) values(?, ?);";

    status = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (status != SQLITE_OK) {
        sqlite3_close(db);
        return ERROR;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (status != SQLITE_DONE)
        return ERROR;

    return OK;
}

int database_delete_credential(char* username) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    int status = sqlite3_open(db_file, &db);
    if (status != SQLITE_OK)
        return ERROR;

    char* sql = "delete from credentials where username=?;";

    status = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (status != SQLITE_OK) {
        sqlite3_close(db);
        return ERROR;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    status = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (status != SQLITE_DONE || changes == 0)
        return ERROR;

    return OK;
}

int database_get_credential(char* username, struct credential* result) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    int status = sqlite3_open(db_file, &db);
    if (status != SQLITE_OK)
        return ERROR;

    char* sql = "select * from credentials where username=?;";

    status = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (status != SQLITE_OK) {
        sqlite3_close(db);
        return ERROR;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    status = sqlite3_step(stmt);
    if (status != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return NOT_FOUND;
    }

    if (result != NULL) {
        result->username = strdup(sqlite3_column_text(stmt, 0));
        result->password = strdup(sqlite3_column_text(stmt, 1));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return OK;
}

int database_add_session_id(char* username, char* session_id) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    int status = sqlite3_open(db_file, &db);
    if (status != SQLITE_OK)
        return ERROR;

    char* sql = "insert into session_ids(username, session_id) values(?, ?);";

    status = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (status != SQLITE_OK) {
        sqlite3_close(db);
        return ERROR;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, session_id, -1, SQLITE_STATIC);

    status = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (status != SQLITE_DONE)
        return ERROR;

    return OK;
}

int database_delete_session_id(char* session_id) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    int status = sqlite3_open(db_file, &db);
    if (status != SQLITE_OK)
        return ERROR;

    char* sql = "delete from session_ids where session_id=?;";

    status = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (status != SQLITE_OK) {
        sqlite3_close(db);
        return ERROR;
    }

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);

    status = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (status != SQLITE_DONE || changes == 0)
        return ERROR;

    return OK;
}

int database_get_session_id(char* username, char* result) {
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    int status = sqlite3_open(db_file, &db);
    if (status != SQLITE_OK)
        return ERROR;

    char* sql = "select session_id from session_ids where username=?;";

    status = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (status != SQLITE_OK) {
        sqlite3_close(db);
        return ERROR;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    status = sqlite3_step(stmt);
    if (status != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return NOT_FOUND;
    }

    if (result != NULL)
        result = strdup(sqlite3_column_text(stmt, 0));

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return OK;
}

int database_deinitialize() {
    free(db_file);
    return 0;
}