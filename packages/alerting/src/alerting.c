/**********************************************************
 * Copyright ,2016, unionsmartit.com
 * FileName:alerting.c
 * Author: Chad   Date:2016-6-20
 * Description: send email to user to report the attack.
 * 
 * History:
 *     <author>        <time>        <desc>
 *
 **********************************************************/
     
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "debug.h"
#include "email_sql.h"
#include <json-c/json.h>
#include <es_action.h>

#define NAIL_BIN "/opt/psi_nids/bin/nail"
#define DATABASE "surveyor"
#define SERVER_NAME "localhost"
#define USER_NAME "root"
#define USER_PWD "13246"
#define MAX_RECEIVER 16
#define MAX_EMAIL_ADDRESS_LEN 64
#define INTERVAL_MINUTES 5
#define NAILRC_CONFIG "/etc/psi_nids/nailrc"
#define LAST_SEND_TIME 0
#define CURRENT_SEND_TIME 1
#define DAEMON_MODE

struct _email {
    char receiver[MAX_RECEIVER][MAX_EMAIL_ADDRESS_LEN];
    char subject[512];
    char sender[64];
    char mail_server[128];
    char sender_name[128];
    char sender_pwd[64];
    char *content;
};

struct _alerting_info {
    uint32_t attacked_times;
    uint32_t successful_times;
};


/*Global values*/
static uint64_t cur_id; /*current id of portrait table(named id)*/
static uint64_t last_id; /*last id of portrait table(named id)*/
static uint64_t alerts_cur_id; /*current id of portrait table(named id)*/
static uint64_t alerts_last_id; /*last id of portrait table(named id)*/
static uint8_t quit = 0; /*exit flag*/
static uint32_t all_attack_cnt;
static uint32_t all_succeed_attack_cnt;
static uint32_t alerts_all_attack_cnt;
static uint32_t cur_email_id = 1;
static char last_send_mail_time[24] = "";
static char cur_send_mail_time[24] = "";

struct _email email;
struct _alerting_info alerting_info;
struct _alerting_info alerts_alerting_info;
#define PORTRAIT_EMAIL 0x01
#define ALERTS_EMAIL 0x10

static void UpdateTime(int type);


void handle_signal(int signo)
{
    quit = 1;
}

void handle_sigchld(int signo)
{
    while( waitpid(-1,NULL,WNOHANG) > 0 ){
    }
}

#define EVENT_TYPE_LEN 100
#define MAX_EVENT_TYPE 100
char event_type_eng[MAX_EVENT_TYPE][EVENT_TYPE_LEN];
char event_type_chn[MAX_EVENT_TYPE][EVENT_TYPE_LEN];
int total_event_type = 0;
void loadEventType()
{
    char size[64] = {0};
    snprintf(size,sizeof(size),"{\"size\":%d}",MAX_EVENT_TYPE);
    json_object *event_obj = es_search("utils","attack_type",size);
    if(event_obj){
        json_object *hits = es_fetch_hits(event_obj);
        if(hits){
            int total = es_fetch_hits_len(event_obj);
            int i = 0;
            json_object *elem_obj;
            json_object *eng_obj;
            json_object *chn_obj;
            while(i<total){
                elem_obj = json_object_array_get_idx(hits,i);
                eng_obj = es_fetch_node(elem_obj,"_source.event_type_eng");
                chn_obj = es_fetch_node(elem_obj,"_source.event_type_chn");
                if(eng_obj)
                    snprintf(event_type_eng[i],EVENT_TYPE_LEN,"%s",json_object_get_string(eng_obj));
                else
                    strcpy(event_type_eng[i],"unknown-attack-type");
                if(chn_obj)
                    snprintf(event_type_chn[i],EVENT_TYPE_LEN,"%s",json_object_get_string(chn_obj));
                else
                    strcpy(event_type_chn[i],"unknown-attack-type");
                #ifdef DEBUG
                printf("%d-%d:%s===>%s\n",total,i,event_type_eng[i],event_type_chn[i]);
                #endif
                i++;
                total_event_type++;
            }
        }
        free_json_object(event_obj);
    }
}

/*
Function        : Initialize
Description    : setup signal,connect to database and initialize the cur_id.
Arguments     : NULL.
Returns         : 0->success.
*/
int Initialize()
{
    signal(SIGHUP,handle_signal);
    signal(SIGINT,handle_signal);
    signal(SIGTERM,handle_signal);
    //signal(SIGCHLD,handle_sigchld);
    
    return init_curl();
}

int put_email_body(json_object *obj,FILE *fp)
{
    json_object *hits = es_fetch_hits(obj);
    json_object *node;
    json_object *tmp_obj;
    int i = 0;
    int hits_len = es_fetch_hits_len(obj);
    
    while(i<hits_len){
        node = json_object_array_get_idx(hits,i);
        
        tmp_obj = es_fetch_node(node,"_source.remote_ip");
        if(tmp_obj){
            fprintf(fp,"    攻击源IP地址为：%s，",json_object_get_string(tmp_obj));
        }
        else{
            fprintf(fp,"    攻击源IP地址未知，");
        }
        
        tmp_obj = es_fetch_node(node,"_source.property_ip");
        if(tmp_obj){
            fprintf(fp,"被攻击资产IP地址为：%s，",json_object_get_string(tmp_obj));
            printf("pip:%s===\n",json_object_get_string(tmp_obj));
        }
        else{
            fprintf(fp,"被攻击资产IP地址未知，");
        }
        
        tmp_obj = es_fetch_node(node,"_source.event_type");
        if(tmp_obj){
            int idx = 0;
            int found = 0;
            
            while(idx<total_event_type){
                if(strcasecmp(event_type_eng[idx],json_object_get_string(tmp_obj)) == 0){
                    found = 1;
                    break;
                }
                idx++;
            }
            
            if(found)
                fprintf(fp,"攻击类型为：%s",event_type_chn[idx]);
            else
                fprintf(fp,"攻击类型为：%s",json_object_get_string(tmp_obj));
        }
        else{
            fprintf(fp,"攻击类型未知");
        }
        fprintf(fp,"；\n");
        i++;
    }
    return hits_len;
}

/*
Function        : FetchAttackEvent
Description    : check wheter the network has attacked by hacker.
Arguments     : NULL.
Returns         : the count of successful attack.
*/
int FetchAttackEvent()
{
    /*prepare email*/
    int err = 0;
    memset(&email,0,sizeof(struct _email));
    
    /*prepare email header->sender*/
    json_object *setting_obj = es_get("utils","email_alerts","server_setting");
    if(setting_obj){
        json_object *tmp_obj = es_fetch_node(setting_obj,"_source.mail_server");
        if(tmp_obj)
            strncpy(email.mail_server,json_object_get_string(tmp_obj),sizeof(email.mail_server)-1);
        else
            err++;
        
        tmp_obj = es_fetch_node(setting_obj,"_source.mail_subject");
        if(tmp_obj)
            strncpy(email.subject,json_object_get_string(tmp_obj),sizeof(email.subject)-1);
        else
            err++;
        
        tmp_obj = es_fetch_node(setting_obj,"_source.sender_addr");
        if(tmp_obj)
            strncpy(email.sender,json_object_get_string(tmp_obj),sizeof(email.sender)-1);
        else
            err++;
        
        tmp_obj = es_fetch_node(setting_obj,"_source.sender_account");
        if(tmp_obj){
            strncpy(email.sender_name,json_object_get_string(tmp_obj),sizeof(email.sender_name)-1);
        }
        
        tmp_obj = es_fetch_node(setting_obj,"_source.sender_password");
        if(tmp_obj){
            strncpy(email.sender_pwd,json_object_get_string(tmp_obj),sizeof(email.sender_pwd)-1);
        }
    }
    else
        err++;
    
    free_json_object(setting_obj);
    if(err){
        #ifdef DEBUG
        printf("server setting error...\n");
        #endif
        return 0;
    }
    /*prepare email header->receiver*/
    int receiver_cnt = 0;
    const char *get_receiver = "{\"size\":100,\"query\":{\"term\":{\"doc_type\":\"receiver\"}}}";
    json_object *receiver_obj = es_search("utils","email_alerts",get_receiver);
    int total_receivers = es_fetch_hits_total(receiver_obj);
    
    if(total_receivers > 0){
        int hits_len = es_fetch_hits_len(receiver_obj);
        json_object *hits = es_fetch_hits(receiver_obj);
        int i = 0;
        json_object *node;
        json_object *tmp_obj;
        while(i<hits_len && receiver_cnt < MAX_RECEIVER){
            node = json_object_array_get_idx(hits,i);
            tmp_obj = es_fetch_node(node,"_source.receiver_addr");
            if(tmp_obj){
                strncpy(email.receiver[receiver_cnt],json_object_get_string(tmp_obj),sizeof(email.receiver[receiver_cnt])-1);
                receiver_cnt++;
            }
            i++;
        }
    }
    
    free_json_object(receiver_obj);

    if(receiver_cnt<=0){
        #ifdef DEBUG
        printf("not found any valid receivers...\n");
        #endif
        return 0;
    }
    
    /*get total succeed attacked events*/
    UpdateTime(CURRENT_SEND_TIME);
    json_object *query_obj = json_object_new_object();
    if(!query_obj){
        #ifdef DEBUG
        printf("faild to new query object...\n");
        #endif
        return 0;
    }
    char es_type[16] = {0};
    char timestamp_range[128] = {0};
    
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if(tm_now->tm_mday == 1 && tm_now->tm_hour== 0 && tm_now->tm_min < 10){/*the first 10 minutes of month will check 2 types*/
        if(tm_now->tm_mon == 9)/*oct*/
            snprintf(es_type,sizeof(es_type),"%d0%d,%d%d",tm_now->tm_year+1900,tm_now->tm_mon,tm_now->tm_year+1900,tm_now->tm_mon+1);
        else if(tm_now->tm_mon < 9)
            snprintf(es_type,sizeof(es_type),"%d0%d,%d0%d",tm_now->tm_year+1900,tm_now->tm_mon,tm_now->tm_year+1900,tm_now->tm_mon+1);
        else
            snprintf(es_type,sizeof(es_type),"%d%d,%d%d",tm_now->tm_year+1900,tm_now->tm_mon,tm_now->tm_year+1900,tm_now->tm_mon+1);
    }
    else{
        if(tm_now->tm_mon >= 9)
            snprintf(es_type,sizeof(es_type),"%d%d",tm_now->tm_year+1900,tm_now->tm_mon+1);
        else
            snprintf(es_type,sizeof(es_type),"%d0%d",tm_now->tm_year+1900,tm_now->tm_mon+1);
    }
    snprintf(timestamp_range,sizeof(timestamp_range),"{\"gte\":\"%s\",\"lt\":\"%s\"}",last_send_mail_time,cur_send_mail_time);
    es_set_node(query_obj,"size",json_type_int,"100");
    es_set_node(query_obj,"_source[0]",json_type_array,"[\"property_ip\",\"remote_ip\",\"event_type\"]");
    es_set_node(query_obj,"query.bool.filter[0].term.status",json_type_boolean,"1");
    es_set_node(query_obj,"query.bool.filter[1].range.timestamp",json_type_object,timestamp_range);
    #ifdef DEBUG
    printf("succeed attack:es_type is %s,query obj is %s\n",es_type,json_object_to_json_string(query_obj));
    #endif
    json_object *succeed_obj = es_scroll("events",es_type,"3m",json_object_to_json_string(query_obj));
    int64_t succeed_attacked = es_fetch_hits_total(succeed_obj);
    free_json_object(query_obj);
    if(succeed_attacked <= 0){
        #ifdef DEBUG
        printf("not found any succeed attack event...\n");
        #endif
        free_json_object(succeed_obj);
        return 0;
    }
    
    /*get total attacked and max timestamp*/
    int64_t total_attacked = 0;
    int64_t max_timestamp = 0;
    snprintf(timestamp_range,sizeof(timestamp_range),"{\"field\":\"timestamp\",\"ranges\":{\"from\":\"%s\",\"to\":\"%s\"}}",last_send_mail_time,cur_send_mail_time);
    query_obj = json_object_new_object();
    es_set_node(query_obj,"size",json_type_int,"0");
    es_set_node(query_obj,"aggs.range_arr.range",json_type_object,timestamp_range);
    es_set_node(query_obj,"aggs.range_arr.aggs.max_t.max.field",json_type_string,"timestamp");
    #ifdef DEBUG
    printf("total attack:query obj is %s\n",json_object_to_json_string(query_obj));
    #endif
    json_object *total_obj = es_search("events",es_type,json_object_to_json_string(query_obj));
    json_object *tmp_obj;
    free_json_object(query_obj);
    tmp_obj = es_fetch_node(total_obj,"aggregations.range_arr.buckets[0].doc_count");
    if(tmp_obj){
        total_attacked = json_object_get_int64(tmp_obj);
    }
    #if 0
    tmp_obj = es_fetch_node(total_obj,"aggregations.range_arr.buckets[0].max_t.value");
    if(tmp_obj){
        max_timestamp = json_object_get_int64(tmp_obj);
    }
    #endif
    free_json_object(total_obj);
    
    /*prepare email body(content as file)*/
    FILE *fp = fopen("/var/run/alert_email","w");
    if(!fp){
        log_message(L_ERR,"%s:failed to open alert_email file:%s\n",__func__,strerror(errno));
        #ifdef DEBUG
        printf("%s:failed to open alert_email file:%s\n",__func__,strerror(errno));
        #endif
        free_json_object(succeed_obj);
        return -1;
    }
    fprintf(fp,"尊敬的用户：\n");
    fprintf(fp,"    您的网络正在遭受黑客攻击，从%s到%s这一段时间内，您的网络已受到%ld次攻击，其中有%ld次攻击成功，请注意防范！\n\n",last_send_mail_time,cur_send_mail_time,total_attacked,succeed_attacked);

    char scroll_id[256] = {0};
    int doc_count = put_email_body(succeed_obj,fp);
    json_object *scroll_id_obj = es_get_scroll_id(succeed_obj);
    if(scroll_id_obj){
        strncpy(scroll_id,json_object_get_string(scroll_id_obj),sizeof(scroll_id));
    }
    free_json_object(succeed_obj);

    if(succeed_attacked > doc_count){
        while(doc_count>0){
            if(strlen(scroll_id) > 0){
                succeed_obj = es_scroll_next("3m",scroll_id);
                doc_count = put_email_body(succeed_obj,fp);
                scroll_id_obj = es_get_scroll_id(succeed_obj);
                if(scroll_id_obj){
                    strncpy(scroll_id,json_object_get_string(scroll_id_obj),sizeof(scroll_id));
                }
                free_json_object(succeed_obj);
            }
            else
                break;
        }
    }
    fclose(fp);
    
    email.content = "/var/run/alert_email";

    /*done,email is ok*/
    #ifdef DEBUG
    printf("email is ready to send...\n");
    #endif
    return succeed_attacked;
        
}


/*
Function        : SendEmail
Description    : call nail to send email.
Arguments     : NULL.
Returns         : 0->success,others->fail .
*/
int SendEmail(const char *receiver_address)
{
    char receiver[MAX_EMAIL_ADDRESS_LEN*MAX_RECEIVER+MAX_RECEIVER] = "";
    char cmd[sizeof(receiver) + 512] = "";
    int i = 0;
    int exit_code;

    if(receiver_address != NULL){
        log_message(L_DEBUG,"receiver is %s\n",receiver_address);
        strncpy(receiver,receiver_address,sizeof(receiver)-1);
    }
    else{
        /*use space charactor split the address*/
        while(strlen(email.receiver[i])){
            snprintf(receiver+strlen(receiver),MAX_EMAIL_ADDRESS_LEN,"%s ",email.receiver[i]);
            i++;
        }
    }
    
    snprintf(cmd,sizeof(cmd),"env LC_ALL=zh_CN.UTF-8 %s -r %s -s \"%s\" %s < %s",NAIL_BIN,email.sender,email.subject,receiver,email.content);
    //snprintf(cmd,sizeof(cmd),"env LC_ALL=zh_CN.UTF-8 %s -r %s -s \"勘探者攻击告警邮件\" %s < %s",NAIL_BIN,email.sender,receiver,email.content);
    log_message(L_INFO,"%s\n",cmd);

	exit_code = system(cmd);
    /*check exit code*/
	if ( WIFEXITED(exit_code) )
		return WEXITSTATUS(exit_code);
    
    return -1;
}

/*
Function        : UpdateNailConfig
Description    : update config file.
Arguments     : NULL.
Returns         : 0 on success and 1 on fail .
*/
int UpdateNailConfig()
{
    log_message(L_INFO,"modify nailrc config...\n");
    FILE *fp = fopen(NAILRC_CONFIG,"w");
    if(fp){
        fprintf(fp,"set from=%s\n",email.sender);
        fprintf(fp,"set smtp=%s\n",email.mail_server);
        if(strlen(email.sender_pwd) > 0){
            //if smtp server need to authenticate the client,set username and password
            fprintf(fp,"set smtp-auth-user=%s\n",email.sender_name);
            fprintf(fp,"set smtp-auth-password=%s\n",email.sender_pwd);
            fprintf(fp,"set smtp-auth=login\n");
        }
        fclose(fp);
        return 0;
    }
    else{
        log_message(L_ERR,"failed to open config file\n");
        return 1;
    }
}

/*
Function        : LogFailedEmail
Description    : log the event that failed to send email.
Arguments     : NULL.
Returns         : NULL .
*/
void LogFailedEmail()
{
    FILE *fp = fopen("/var/log/system_log","a+");
    if(fp)
    {
        time_t now_time; 
        time (&now_time); 
        fprintf(fp,"failed to send email...the time is %s",ctime(&now_time));
        fclose(fp);
    }
}

static void UpdateTime(int type)
{
    time_t t;
    struct tm *tm;
    
    if(type == LAST_SEND_TIME){//last send email time
        #ifdef DEBUG
        strncpy(last_send_mail_time,"2017-01-01 00:00:00",sizeof(last_send_mail_time));
        #else
        strncpy(last_send_mail_time,cur_send_mail_time,sizeof(last_send_mail_time));
        #endif
    }
    else{ //current send email time
        t=time(NULL);
        tm=localtime(&t);
        strftime(cur_send_mail_time,sizeof(cur_send_mail_time),"%F %T",tm);
    }
}

/*main entry*/
int main(int argc,char *argv[])
{
    /*step1:initialize*/
    /*step2:check wheter the user has configured the email,if not,keep cheking,else go no next*/
    /*step3:select data from portrait table*/
    /*step4:check the result of step3,if no attack,back to step2,else go to next*/
    /*step5:summarize the email content,fetch the email address by user name from user table*/
    /*step6:send email to user*/
    /*step7:check the result of step6,if failed to send email,try again, if still fail,log this action*/
    /*step8:end of this alerting and back to step2 for the next round*/
    
    int ret;

    //set nailrc config file
    if((argc == 4 || argc == 6) && strcmp(argv[1],"set_config") == 0){
        //handle set config command
        log_message(L_INFO,"modify nailrc config...\n");
        FILE *fp = fopen(NAILRC_CONFIG,"w");
        if(fp){
            fprintf(fp,"set from=%s\n",argv[2]);
            fprintf(fp,"set smtp=%s\n",argv[3]);
            if(argc == 6){
                //if smtp server need to authenticate the client,set username and password
                fprintf(fp,"set smtp-auth-user=%s\n",argv[4]);
                fprintf(fp,"set smtp-auth-password=%s\n",argv[5]);
                fprintf(fp,"set smtp-auth=login\n");
            }
            fclose(fp);
        }
        else{
            log_message(L_ERR,"failed to open config file\n");
        }
        return 0;
    }
    //initialize the signal,database
    Initialize();
    loadEventType();
    UpdateTime(CURRENT_SEND_TIME);
    UpdateTime(LAST_SEND_TIME);
    #ifdef DAEMON_MODE
    close(0);
    close(1);
    close(2);
    open("/var/run/alert_email", O_CREAT | O_RDWR);  /* stdin, fd 0 */
    open("/var/log/alerting_log", O_CREAT | O_RDWR);  /* stdout, fd 1 */
    dup(1);
    #endif

    /*call hearbeat module,send hello to mgmt every 30 seconds*/
    while(1){
check_attack:
        if(quit)
            break;
        #ifdef DEBUG
        sleep(10);
        #else
        sleep(INTERVAL_MINUTES*60);
        #endif
        
        /*fetch attack events*/
        ret = FetchAttackEvent();
        
        /*no attack? step back*/
        if( ret <= 0)
            goto check_attack;
        
        /*update nailrc*/
        ret = UpdateNailConfig();
        if( ret )
            goto check_attack;
        
        /*ok,everything is good,send email*/
        ret = SendEmail(NULL);
        
        if( ret ){
            /*failed to send email,sleep 5 seconds and try again.*/
            log_message(L_INFO,"failed to send email:%d-%s\n",ret,strerror(errno));
            sleep(5);
            ret = SendEmail(NULL);
            
            if( ret ){
                /*still fail,log this event*/
                log_message(L_INFO,"try again,still failed to send email\n");
                LogFailedEmail();
            }
        }
        
        if(ret == 0){
            //succeed to send email
            UpdateTime(LAST_SEND_TIME);
        }
    }
    
    out:
    log_message(L_INFO,"alerting exit now...\n");
    destroy_curl();
  
    return 0;
}
