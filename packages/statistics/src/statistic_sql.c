/**********************************************************
 * Copyright ,2016, unionsmartit.com
 * FileName:statistic_sql.c
 * Author: Chad   Date:2016-5-24
 * Description: encapsulate mysql APIs.
 * 
 * History:
 *     <author>        <time>        <desc>
 *
 **********************************************************/

#include "statistic_sql.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*Global values*/
static database_info *mysql_data;

/*
Function        : MysqlInit
Description    : Fill the global database_info structure.
Arguments     : server->the host that run mysql server;
                      database->database name;
                      user->user name of database;
                      password->password of database.
Returns         : 0 ->success,1 ->fail.
*/
int MysqlInit(char *server,char *database,char *user,char *password)
{
    if(!server || !database || !user || !password)
        return 1;
    
    mysql_data = (database_info *)calloc(1,sizeof(database_info));
    if(!mysql_data)
        return 1;
    
    mysql_data->server = (char *)calloc(1,strlen(server)+1);
    mysql_data->database = (char *)calloc(1,strlen(database)+1);
    mysql_data->user = (char *)calloc(1,strlen(user)+1);
    mysql_data->password = (char *)calloc(1,strlen(password)+1);
    
    if(mysql_data->server && mysql_data->database && mysql_data->user && mysql_data->password)
    {
        strcpy(mysql_data->server,server);
        strcpy(mysql_data->database,database);
        strcpy(mysql_data->user,user);
        strcpy(mysql_data->password,password);
        return 0;
    }
    else
    {
        CleanMysqlData();
        return 1;
    }
}

/*
Function        : MysqlConnect
Description    : Connetct to the database specified by the MysqlInit func.
Arguments     : NULL
Returns         : 0 ->success,1 ->fail.
*/
int MysqlConnect()
{
    mysql_data->mysql = mysql_init(NULL);
    if(!mysql_real_connect(mysql_data->mysql, mysql_data->server, mysql_data->user, 
                mysql_data->password, mysql_data->database, 0, NULL, 0))
    {
        log_message(L_ERR,"Failed to connect to database %s:%s@%s/%s: %s\n",
                mysql_data->user, mysql_data->password, mysql_data->server, 
                mysql_data->database, mysql_error(mysql_data->mysql));
        return 1;
    }
    return 0;
}

/*
Function        : MysqlClose
Description    : Close database.
Arguments     : NULL
Returns         : 0 ->success.
*/
int MysqlClose()
{
    mysql_close(mysql_data->mysql);
    return 0;
}

int CleanMysqlData()
{
    free(mysql_data->password);
    free(mysql_data->user);
    free(mysql_data->database);
    free(mysql_data->server);
    free(mysql_data);
}

/*
Function        : MysqlExecuteQuery
Description    : Execute mysql sentence.
Arguments     : mysql->pointer to the connected database;
                      sql->mysql sentence.
Returns         : 0 ->success,others->fail.
*/
int MysqlExecuteQuery(MYSQL *mysql,char *sql)
{
    int mysqlErrno;
    int result;
    int tryTimes = 0;
    
    while((result = mysql_query(mysql, sql) != 0))
    {
        mysqlErrno = mysql_errno(mysql);
        if(mysqlErrno < CR_MIN_ERROR)
        {
            log_message(L_ERR,"MySQL ERROR(%i): %s.  Aborting Query\n",
                    mysql_errno(mysql), mysql_error(mysql));
            return result;
        }
        if((mysqlErrno == CR_SERVER_LOST) 
                || (mysqlErrno == CR_SERVER_GONE_ERROR))
        {
            log_message(L_NOTICE,"Lost connection to MySQL server.  Reconnecting\n");
            while(mysql_ping(mysql) != 0 && tryTimes < 10)
            {
                tryTimes++;
                sleep(15);
            }
            if(tryTimes >= 10){
                log_message(L_NOTICE,"Reconnected to MySQL server.\n");
                return result;
            }
            else{
                log_message(L_NOTICE,"Reconnected to MySQL server.\n");
            }
        }
        else
        {
            log_message(L_ERR,"MySQL Error(%i): %s\n", mysqlErrno, mysql_error(mysql));
        }
    }
    return result;
}


/*
Function        : MysqlSelectAsUInt
Description    : Execute mysql select sentence and return a execute result.
Arguments     : sql->mysql sentence;
                      result->address to store the select result.
Returns         : 0 ->success,others->fail.
*/
int MysqlSelectAsUInt(char *sql, unsigned int *result)
{
    int rval = 0;
    MYSQL_RES *mysql_res;
    MYSQL_ROW tuple;
    MYSQL *mysql = mysql_data->mysql;
    
    if(MysqlExecuteQuery(mysql, sql) != 0)
    {
        /* XXX: should really just return up the chain */
        log_message(L_ERR,"Error (%s) executing query: %s\n", mysql_error(mysql), sql);
        return -1;
    }

    mysql_res = mysql_store_result(mysql);
    if((tuple = mysql_fetch_row(mysql_res)))
    {
        if(tuple[0] == NULL)
            *result = 0;
        else
            *result = atoi(tuple[0]);
        rval = 1;
    }
    mysql_free_result(mysql_res);
    return rval;
}
/*
Function        : MysqlSelectAsULong2
Description    : Execute mysql select sentence and return a execute result,if need to get the whole row,
                      please use other function or evaluate the mysql_row before free the result.
Arguments     : sql->mysql sentence;
                      result->address to store the select result of first colum,
                      result2->address to store the select result of second colum.
Returns         : 0 ->success,others->fail.
*/
int MysqlSelectAsULong2(char *sql, uint64_t *result,uint64_t *result2)
{
    int rval = 0;
    MYSQL_RES *mysql_res;
    MYSQL_ROW tuple;
    MYSQL *mysql = mysql_data->mysql;
    
    if(MysqlExecuteQuery(mysql, sql) != 0)
    {
        /* XXX: should really just return up the chain */
        log_message(L_ERR,"Error (%s) executing query: %s\n", mysql_error(mysql), sql);
        return -1;
    }

    mysql_res = mysql_store_result(mysql);
    if((tuple = mysql_fetch_row(mysql_res)))
    {
        if(tuple[0] == NULL)
            *result = 0;
        else
            *result = strtoull(tuple[0],NULL,0);
        if(tuple[1] == NULL)
            *result2 = 0;
        else
            *result2 = strtoull(tuple[1],NULL,0);
        rval = 1;
    }
    mysql_free_result(mysql_res);
    return rval;
}


/*
Function        : MysqlInsert
Description    : Execute mysql insert sentence.
Arguments     : sql->mysql sentence;
                      row_id->address to store the row id of insert.
Returns         : 0 ->success,others->fail.
*/
int MysqlInsert(char *sql, unsigned int *row_id)
{
    MYSQL *mysql = mysql_data->mysql;
    
    if(MysqlExecuteQuery(mysql, sql) != 0)
    {
        log_message(L_ERR,"Error (%s) executing query: %s\n", mysql_error(mysql), sql);
        return -1;
    }

    if(row_id != NULL)
        *row_id = mysql_insert_id(mysql);
    return 0;
}

