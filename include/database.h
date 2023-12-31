#ifndef DATABASE_H
#define DATABASE_H

enum database_values {
    OK,
    NOT_FOUND,
    ERROR
};

struct credential {
    char* username;
    char* password;
};

extern char* db_file;

/* creates the tables if they arent already created: credentials and session ids */
int database_initialize(char* database_file);

/* credentials */

int database_add_credential(char* username, char* password);
int database_delete_credential(char* username);
/* `result` can be NULL just to check whether credential exists or not */
/* char arrays in `result` are dynamically allocated. */
int database_get_credential(char* username, struct credential* result);

/* session ids */

int database_add_session_id(char* username, char* session_id);
int database_delete_session_id(char* session_id);
/* `result` can be NULL just to check whether session id exists or not */
int database_get_session_id(char* username, char* result);

int database_deinitialize();

#endif