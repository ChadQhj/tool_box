#ifndef __STATISTIC_SQL_H__
#define __STATISTIC_SQL_H__

#include <mysql.h>
#include <errmsg.h>
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

int MysqlExecuteQuery(MYSQL *mysql,char *sql);

int MysqlSelectAsUInt(char *sql, unsigned int *result);

int MysqlSelectAsULong2(char *sql, uint64_t *result,uint64_t *result2);

int MysqlInsert(char *sql, unsigned int *row_id);



#endif

