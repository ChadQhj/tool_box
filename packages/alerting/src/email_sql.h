#ifndef __SURVEYOR_SQL_H__
#define __SURVEYOR_SQL_H__

#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <stdint.h>


typedef struct _database_info 
{
    char *server;
    char *database;
    char *user;
    char *password;
    MYSQL *mysql;
} database_info;

int MysqlInit(char *server,char *database,char *user,char *password);

int MysqlConnect();

int MysqlClose();

int CleanMysqlData();

int MysqlExecuteQuery(char *sql);

int MysqlSelectAsUInt(char *sql, uint64_t *result);

int MysqlSelectAsUInt2(char *sql, unsigned long *result,unsigned long *result2);

int MysqlSelectAsRow(char *sql, MYSQL_RES **result);

int MysqlInsert(char *sql, unsigned int *row_id);



#endif

