
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <json-c/json.h>
#include <json-c/json_inttypes.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <json-c/json_util.h>
#include "es_action.h"

#define ES_SERVER_ADDR "localhost"
#define ES_SERVER_PORT 6201
#define FREE_ES_RESULT 1
#define KEEP_ES_RESULT 0

#define ES_ACTION_GET 0
#define ES_ACTION_PUT 1
#define ES_ACTION_POST 2
#define ES_ACTION_POST_BULK 3
#define ES_ACTION_DELETE 4
#define ES_ACTION_DELETE_BY_QUERY 5
#define ES_ACTION_UPDATE 6
#define ES_ACTION_UPDATE_BY_QUERY 7
#define ES_ACTION_SEARCH 8
#define ES_DESTROY 255

struct WriteData {
  const char *readptr;
  size_t sizeleft;
};

static CURLcode perform_curl_request(CURL *m_curl,int action_type,const char *url,const char *data,char **resp);

int64_t es_fetch_node_len(json_object *top_obj,char *node);

/*destrory the static curl handler*/
static inline void es_destroy()
{
    perform_curl_request(NULL,ES_DESTROY,NULL,NULL,NULL);
}

/*initialize curl,call it in the main thread,remember to call destroy_curl
return value:
0:succeed;
1:failed
*/
int init_curl()
{
    CURLcode res;

    res = curl_global_init(CURL_GLOBAL_ALL);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n",curl_easy_strerror(res));
        return 1;
    }
    return 0;
}

/*cleanup curl*/
void destroy_curl()
{
    es_destroy();
    curl_global_cleanup();
}

/*initialize curl context,remember to call destroy_curl_handle if you do not want to use it
return value:
not NULL:succeed;
NULL:failed
*/
CURL * init_curl_handle()
{
    return curl_easy_init();
}

/*cleanup the curl context*/
void destroy_curl_handle(CURL *p_curl)
{
    if(p_curl)
        curl_easy_cleanup(p_curl);
}

/*
timeout:seconds,0 means no timeout,
ignore_sig: 0:generate a signal if timeout,1,not genertate signal if timeout
verbose:0: no error details,1:put error details
return value:
0:succeed;
1:failed.
*/
int set_curl_common_options(CURL *p_curl,int timeout,short ignore_sig,short verbose,short keep_alive)
{
    if(!p_curl)
        return 1;
    
	curl_easy_setopt(p_curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(p_curl, CURLOPT_CONNECTTIMEOUT, timeout);
    if(ignore_sig)
        curl_easy_setopt(p_curl, CURLOPT_NOSIGNAL,1L);
    #ifdef DEBUG
    curl_easy_setopt(p_curl, CURLOPT_VERBOSE, 1L);
    #else
    if(verbose)
        curl_easy_setopt(p_curl, CURLOPT_VERBOSE, 1L);
    #endif

    if(keep_alive){
        /*make the server/client cleanup the dead connection,if no response,retry 9 times by default*/
        /* enable TCP keep-alive for this transfer */
        curl_easy_setopt(p_curl, CURLOPT_TCP_KEEPALIVE, 1L);
        /* keep-alive idle time to 120 seconds */
        curl_easy_setopt(p_curl, CURLOPT_TCP_KEEPIDLE, 120L);
        /* interval time between keep-alive probes: 60 seconds */
        curl_easy_setopt(p_curl, CURLOPT_TCP_KEEPINTVL, 60L);
    }
    return 0;
}

#if 0
int is_curl_handle_active(CURL *p_curl)
{
  curl_socket_t sockfd;
  CURLcode res;
  if(!p_curl){
    printf("curl is null\n");
    return 0;
  }
  /*not support this option(CURLINFO_ACTIVESOCKET)???*/
  res = curl_easy_getinfo(p_curl, CURLINFO_ACTIVESOCKET, &sockfd);
  if(res != CURLE_OK){
    fprintf(stderr,"the connection was lost...(%s)\n",curl_easy_strerror(res));
    return 0;
  }
  else{
    fprintf(stderr,"the socket is actived...\n");
    return 1;
  }
}
#endif

/*free search or get result*/
static inline void es_free_result(char *res)
{
    if(res){
        free(res);
        res = NULL;
    }
}

/*save the response data*/
static size_t write_response_data(void *ptr, size_t size, size_t nmemb, void **user_p) 
{
    /*in curl.h CURL_MAX_WRITE_SIZE = 16k,
    so this function might be called many times,we could extend the CURL_MAX_WRITE_SIZE,
    but,realloc might be a safer way.
    */
    if(*user_p == NULL){
        (*user_p) = (char *)calloc(nmemb,size+1);
        if(*user_p){
            memmove(*user_p,ptr,size*nmemb);
            *((char *)*user_p+size*nmemb) = 0;
        }
        else
            return 0;
    }
    else{
        int len = strlen(*user_p);
        (*user_p) = (char *)realloc(*user_p,nmemb*size+len+1);
        if(*user_p){
            memmove(*user_p+len,ptr,size*nmemb);
            *((char *)*user_p+len+size*nmemb) = 0;
        }
        else
            return 0;
    }
    #ifdef DEBUG
        printf("response:%s\n",(char *)*user_p);
    #endif
    /*if the return value is not size*nmemb,curl_easy_perform will return error:Failed writing received data to disk/application*/
    return size*nmemb;
}

/*put user's data*/
static size_t read_put_data(void *dest, size_t size, size_t nmemb, void *userp)
{
  struct WriteData *wt = (struct WriteData *)userp;
  size_t buffer_size = size*nmemb;

  if(wt->sizeleft) {
    /* copy as much as possible from the source to the destination */
    size_t copy_this_much = wt->sizeleft;
    if(copy_this_much > buffer_size)
      copy_this_much = buffer_size;
    
    memcpy(dest, wt->readptr, copy_this_much);

    wt->readptr += copy_this_much;
    wt->sizeleft -= copy_this_much;
    return copy_this_much; /* copied this many bytes */
  }

  return 0; /* no more data left to deliver */
}

/*return value:
0:success
<0:failed
>0 ERROR*/
static int check_error(json_object *response,int free_response)
{
    int err = 0;
    if(response){
        json_object *error_obj = es_fetch_node(response,"error");
        if(error_obj){
            err = 500;
            json_object *status_obj = es_fetch_node(response,"status");
            if(status_obj)
                err = json_object_get_int(status_obj);
            #ifdef DEBUG_ERR
            json_object *type_obj = es_fetch_node(response,"error.caused_by.type");
            json_object *reason_obj = es_fetch_node(response,"error.caused_by.reason");
            if(type_obj && reason_obj){
                fprintf(stderr,"error:caused by type:%s,caused by reason:%s\n",json_object_get_string(type_obj),json_object_get_string(reason_obj));
            }
            else{
                fprintf(stderr,"error:%s\n",json_object_get_string(error_obj));
            }
            #endif
        }

        if(err == 0){
            json_object *succeed_obj = es_fetch_node(response,"_shards.successful");
            if(succeed_obj){
                if(json_object_get_int(succeed_obj) <= 0)
                    err = -1;
            }
        }
        
        if(free_response){
            free_json_object(response);
        }
    }
    return err;
}


/*remember to free this json_object if the return value is not null
return value:
not NULL:error
NULL:success
*/
static json_object *check_bulk_errors(json_object *response,int free_response)
{
    json_object *error_idx = NULL;
    if(response){
        json_object *error_obj = es_fetch_node(response,"errors");
        if(error_obj){
            if(json_object_get_boolean(error_obj)){
                json_object *items_obj = es_fetch_node(response,"items");
                int items_len = es_fetch_node_len(response,"items");
                int idx = 0;
                int err = 0;
                int failed_cnt = 0;
                json_object *node;
                error_idx = json_object_new_array();
                
                while(idx<items_len){
                    node = json_object_array_get_idx(items_obj,idx);
                    err = check_error(es_fetch_node(node,"index"),KEEP_ES_RESULT); /*the node name might by update,delete,not index*/
                    if(err != 0){
                        failed_cnt++;
                        json_object_array_add(error_idx,json_object_new_int(idx));
                    }
                    idx++;
                }
                
                if(failed_cnt == 0){
                    free_json_object(error_idx);
                }
            }
        }
        
        if(free_response){
            free_json_object(response);
        }
    }
    
    return error_idx;
}


static CURLcode curl_request(CURL *p_curl,int action_type,const char *url,const char *data,char **resp)
{
    if(!p_curl)
        return -1;
    
    struct WriteData wd = {NULL,0};
    if(data){
        wd.readptr = data;
        wd.sizeleft = strlen(data);
    }
    
    (*resp) = NULL; /*it is necessary*/
    
    curl_easy_setopt(p_curl, CURLOPT_URL, url);
    
    switch(action_type){
        case ES_ACTION_SEARCH:
        case ES_ACTION_POST:
        case ES_ACTION_POST_BULK:
        case ES_ACTION_DELETE_BY_QUERY:
        case ES_ACTION_UPDATE:
        case ES_ACTION_UPDATE_BY_QUERY:
    	    curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "POST");
        	curl_easy_setopt(p_curl, CURLOPT_UPLOAD, 1L);
        	curl_easy_setopt(p_curl, CURLOPT_INFILESIZE_LARGE, wd.sizeleft);
            curl_easy_setopt(p_curl, CURLOPT_POSTFIELDSIZE, (long)wd.sizeleft);
            break;
        case ES_ACTION_DELETE:
    	    curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        	curl_easy_setopt(p_curl, CURLOPT_UPLOAD, 0L);
            curl_easy_setopt(p_curl, CURLOPT_POSTFIELDSIZE, 0L);
        	curl_easy_setopt(p_curl, CURLOPT_INFILESIZE_LARGE, 0L);
            break;
        case ES_ACTION_PUT:
	        curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(p_curl, CURLOPT_POSTFIELDSIZE, 0L);
        	curl_easy_setopt(p_curl, CURLOPT_UPLOAD, 1L);
        	curl_easy_setopt(p_curl, CURLOPT_INFILESIZE_LARGE, wd.sizeleft);
            break;
        case ES_ACTION_GET:
    	    curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "GET");
        	curl_easy_setopt(p_curl, CURLOPT_UPLOAD, 0L);
            curl_easy_setopt(p_curl, CURLOPT_POSTFIELDSIZE, 0L);
        	curl_easy_setopt(p_curl, CURLOPT_INFILESIZE_LARGE, 0L);
            break;
        default:
            break;
    }
    
    /*set request callback func*/
    if(data){
    	curl_easy_setopt(p_curl, CURLOPT_READDATA, &wd);
    	curl_easy_setopt(p_curl, CURLOPT_READFUNCTION, read_put_data);
    }
    /*set response callback func*/
	curl_easy_setopt(p_curl, CURLOPT_WRITEDATA, resp);
	curl_easy_setopt(p_curl, CURLOPT_WRITEFUNCTION, write_response_data);

    CURLcode res = curl_easy_perform(p_curl);
    
    if(res != CURLE_OK){
        #ifdef DEBUG_ERR
        fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
        #else
        syslog(LOG_ERR,"es request error(%d): %s\n",res,curl_easy_strerror(res));
        #endif
    }

    return res;
}

static CURLcode perform_curl_request(CURL *m_curl,int action_type,const char *url,const char *data,char **resp)
{
    static CURL *p_curl = NULL;

    if(m_curl)
        return curl_request(m_curl,action_type,url,data,resp);

    if(action_type == ES_DESTROY){
        destroy_curl_handle(p_curl);
        return 0;
    }
    else{
        if(!p_curl){
            p_curl = init_curl_handle();
            #ifdef DEBUG
            if(set_curl_common_options(p_curl,30,1,1,1))
            #else
            if(set_curl_common_options(p_curl,30,1,0,1))
            #endif
                return -1;
        }
        return curl_request(p_curl,action_type,url,data,resp);
    }
    
}

static json_object *es_do(CURL *curl,int action_type,const char *url,const char *data)
{
    #ifdef CHECK_INPUT_JSON
    if(data){
        json_object *root_object = json_tokener_parse(data);
        if(!root_object){
            fprintf(stderr,"json parse error...\n");
            return NULL;
        }
        json_object_put(root_object);
    }
    #endif
    
    char *response = NULL;
    json_object *resp_obj = NULL;
    CURLcode res = perform_curl_request(curl,action_type,url,data,&response);

    if(res == CURLE_OK){
        resp_obj = json_string_to_json_object(response);
        #if 0
        if(resp_obj){
            json_object *err_obj = es_fetch_node(resp_obj,"error");
            if(err_obj){
                #ifdef DEBUG
                json_object *reason_obj = es_fetch_node(resp_obj,"reason");
                if(reason_obj){
                    fprintf(stderr,"%s\n",json_object_get_string(reason_obj));
                }
                #endif
                free_json_object(resp_obj);
                es_free_result(response);
                return NULL;
            }
        }
        #endif
    }
    es_free_result(response);
    return resp_obj;
}

/*insert a document,user specify the document id,if the document id is existed,it will update it,the created of result is set to false
and the _version will be added
return value:
0:succeed;
<0:failed
>0:error
*/
int es_put(const char *index,const char *type,const char *id,const char *data)
{
    return es_mul_put(NULL,index,type,id,data);
}
/*
return value:
0:succeed;
<0:failed
>0:error
*/
int es_mul_put(CURL *curl,const char *index,const char *type,const char *id,const char *data)
{
    if(!index || !type || !id || !data){
        fprintf(stderr,"index document failed:parameter error");
        return -1;
    }

    char url[256] = {0};
    snprintf(url,sizeof(url),"http://%s:%d/%s/%s/%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,type,id);
    json_object *resp = es_do(curl,ES_ACTION_PUT,url,data);
    return check_error(resp,FREE_ES_RESULT);
}

/*insert a document,es will generated a document id automatically,and the op_type will be set to create too
just equals to :post /index/type/id/_create
return value:
0:succeed;
<0:failed
>0:error
*/
int es_post(const char *index,const char *type,const char *data)
{
    return es_mul_post(NULL,index,type,data);
}

int es_mul_post(CURL *curl,const char *index,const char *type,const char *data)
{
    if(!index || !type || !data)
        return -1;
    
    char url[256] = {0};
    snprintf(url,sizeof(url),"http://%s:%d/%s/%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,type);
    json_object *resp = es_do(curl,ES_ACTION_POST,url,data);
    return check_error(resp,FREE_ES_RESULT);
}

/*
return value:
not NULL:error
NULL:success
*/
json_object *es_post_bulk(const char *index,const char *type,const char *data)
{
    return es_mul_post_bulk(NULL,index,type,data);
}

json_object *es_mul_post_bulk(CURL *curl,const char *index,const char *type,const char *data)
{
    if(!index || !type || !data)
        return NULL;
    
    char url[256] = {0};
    snprintf(url,sizeof(url),"http://%s:%d/%s/%s/_bulk",ES_SERVER_ADDR,ES_SERVER_PORT,index,type);
    json_object *resp = es_do(curl,ES_ACTION_POST_BULK,url,data);
    return check_bulk_errors(resp,FREE_ES_RESULT);
}

/*udpate a document
return value:
0:succeed;
<0:failed
>0:error
*/
int es_update(const char *index,const char *type,const char *id,const char *data)
{
    return es_mul_update(NULL,index,type,id,data);
}

int es_mul_update(CURL *curl,const char *index,const char *type,const char *id,const char *data)
{
    if(!index || !type || !id || !data)
        return -1;
    
    char url[256] = {0};
    snprintf(url,sizeof(url),"http://%s:%d/%s/%s/%s/_update",ES_SERVER_ADDR,ES_SERVER_PORT,index,type,id);
    json_object *resp = es_do(curl,ES_ACTION_UPDATE,url,data);
    return check_error(resp,FREE_ES_RESULT);
}

/*udpate a document by query
return value:
not NULL:error
NULL:success
*/
json_object *es_update_by_query(const char *index,const char *type,const char *data,char *options)
{
    return es_mul_update_by_query(NULL,index,type,data,options);
}

json_object *es_mul_update_by_query(CURL *curl,const char *index,const char *type,const char *data,char *options)
{
    if(!index)
        return NULL;

    uint64_t failed = -1;
    char url[256] = {0};
    json_object *resp_obj;
    char *opt= options;
    if(!opt)
        opt = "";
    
    if(type)
        snprintf(url,sizeof(url),"http://%s:%d/%s/%s/_update_by_query%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,type,opt);
    else
        snprintf(url,sizeof(url),"http://%s:%d/%s/_update_by_query%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,opt);
    if(data)
        resp_obj = es_do(curl,ES_ACTION_UPDATE_BY_QUERY,url,data);
    else
        resp_obj = es_do(curl,ES_ACTION_UPDATE_BY_QUERY,url,"");

    #if 0
    if(resp_obj){
        json_object *total_obj = es_fetch_node(resp_obj,"total");
        json_object *updated_obj = es_fetch_node(resp_obj,"updated");
        json_object *failures_obj = es_fetch_node(resp_obj,"failures");
        /*use failures? */
        if(failures_obj){
            failed = json_object_array_length(failures_obj);
        }
        else if(total_obj && updated_obj){
            int64_t total = json_object_get_int64(total_obj);
            int64_t updated = json_object_get_int64(updated_obj);
            failed = total-updated; 
        }
        free_json_object(resp_obj);
    }
    #endif
    return check_bulk_errors(resp_obj,FREE_ES_RESULT);
}

/*delete a document,can not delete the whole type,but we can delete the whole index
return value:
0:succeed;
<0:failed;
>0:error.
*/
int es_delete(const char *index,const char *type,const char *id)
{
    return es_mul_delete(NULL,index,type,id);
}
int es_mul_delete(CURL *curl,const char *index,const char *type,const char *id)
{
    if(!index)
        return -1;
    
    char url[256] = {0};
    if(type && id)
        snprintf(url,sizeof(url),"http://%s:%d/%s/%s/%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,type,id);
    else if(!type && !id)
        snprintf(url,sizeof(url),"http://%s:%d/%s",ES_SERVER_ADDR,ES_SERVER_PORT,index);
    else
        return -2;
    json_object *resp_obj = es_do(curl,ES_ACTION_DELETE,url,NULL);
    json_object *found_obj = es_fetch_node(resp_obj,"found");
    if(found_obj && json_object_get_boolean(found_obj)){
        return check_error(resp_obj,FREE_ES_RESULT);
    }
    else{
        free_json_object(resp_obj);
        return -3;
    }
}
/*delete documents by query
return value:
not NULL:error
NULL:success
*/
json_object *es_delete_by_query(const char *index,const char *type,const char *data,char *options)
{
    return es_mul_delete_by_query(NULL,index,type,data,options);
}

json_object *es_mul_delete_by_query(CURL *curl,const char *index,const char *type,const char *data,char *options)
{
    if(!index)
        return NULL;

    int64_t failed = -1;
    char url[256] = {0};
    json_object *resp_obj;
    char *opt = options;
    if(!opt)
        opt = "";
    
    if(type)
        snprintf(url,sizeof(url),"http://%s:%d/%s/%s/_delete_by_query%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,type,opt);
    else
        snprintf(url,sizeof(url),"http://%s:%d/%s/_delete_by_query%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,opt);

    if(data)
        resp_obj = es_do(curl,ES_ACTION_DELETE_BY_QUERY,url,data);
    else
        resp_obj = es_do(curl,ES_ACTION_DELETE_BY_QUERY,url,"");
    #if 0
    if(resp_obj){
        json_object *total_obj = es_fetch_node(resp_obj,"total");
        json_object *deleted_obj = es_fetch_node(resp_obj,"deleted");
        json_object *failures_obj = es_fetch_node(resp_obj,"failures");
        /*use failures? */
        if(failures_obj){
            failed = json_object_array_length(failures_obj);
        }
        else if(total_obj && deleted_obj){
            int64_t total = json_object_get_int64(total_obj);
            int64_t deleted = json_object_get_int64(deleted_obj);
            failed = total-deleted; 
        }
        free_json_object(resp_obj);
    }
    
    return failed;
    #endif
    return check_bulk_errors(resp_obj,FREE_ES_RESULT);
}

/*fetch a document by http get mothod,remember to free the result(call es_free_result function)
return value:
not NULL:succeed,the pointer to document data;
NULL:failed
*/
json_object *es_get(const char *index,const char *type,const char *id)
{
    return es_mul_get(NULL,index,type,id);
}

json_object *es_mul_get(CURL *curl,const char *index,const char *type,const char *id)
{
    if(!index || !id)
        return NULL;
    
    char url[256] = {0};
    if(type)
        snprintf(url,sizeof(url),"http://%s:%d/%s/%s/%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,type,id);
    else
        snprintf(url,sizeof(url),"http://%s:%d/%s/_all/%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,id);
    json_object *resp_obj = es_do(curl,ES_ACTION_GET,url,NULL);
    json_object *found_obj = es_fetch_node(resp_obj,"found");
    if(found_obj && json_object_get_boolean(found_obj)){
        return resp_obj;
    }
    else{
        check_error(resp_obj,FREE_ES_RESULT);
        return NULL;
    }
}


/*search domuments,remember to free the result(call es_free_result function)
return value:
not NULL:succeed,the pointer to search result data;
NULL:failed
*/
json_object * es_search(const char *index,const char *type,const char *data)
{
    return es_mul_search(NULL,index,type,data);
}

json_object * es_mul_search(CURL *curl,const char *index,const char *type,const char *data)
{
    char url[512] = {0};
    
    if(index && type)
        snprintf(url,sizeof(url),"http://%s:%d/%s/%s/_search",ES_SERVER_ADDR,ES_SERVER_PORT,index,type);
    else if(!index && !type)
        snprintf(url,sizeof(url),"http://%s:%d/_search",ES_SERVER_ADDR,ES_SERVER_PORT);
    else if(!index && type)
        snprintf(url,sizeof(url),"http://%s:%d/*/%s/_search",ES_SERVER_ADDR,ES_SERVER_PORT,type);
    else if(index && !type)
        snprintf(url,sizeof(url),"http://%s:%d/%s/_search",ES_SERVER_ADDR,ES_SERVER_PORT,index);
    else
        return NULL;

    json_object *resp_obj = NULL;
    if(data)
        resp_obj = es_do(curl,ES_ACTION_SEARCH,url,data);
    else
        resp_obj = es_do(curl,ES_ACTION_SEARCH,url,"");

    if(check_error(resp_obj,KEEP_ES_RESULT)){
        free_json_object(resp_obj);
        return NULL;
    }
    else{
        return resp_obj;
    }
}

/*
return value:
not NULL:succeed,the pointer to scroll result data;
NULL:failed
*/
json_object * es_scroll(const char *index,const char *type,const char *scroll_timeout,const char *data)
{
    return es_mul_scroll(NULL,index,type,scroll_timeout,data);
}

/*search domuments,remember to free the result(call es_free_result function)
The scroll_timeout parameter tells Elasticsearch to keep the search context open, such as 1m,means 1 minute;
return value:
not NULL:succeed,the pointer to scroll result data;
NULL:failed
*/
json_object * es_mul_scroll(CURL *curl,const char *index,const char *type,const char *scroll_timeout,const char *data)
{
    if(!index || !data){
        fprintf(stderr,"delete document failed:parameter error");
        return NULL;
    }
    char url[256] = {0};
    if(type)
        snprintf(url,sizeof(url),"http://%s:%d/%s/%s/_search?scroll=%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,type,scroll_timeout);
    else
        snprintf(url,sizeof(url),"http://%s:%d/%s/_search?scroll=%s",ES_SERVER_ADDR,ES_SERVER_PORT,index,scroll_timeout);

    json_object *resp_obj = es_do(curl,ES_ACTION_SEARCH,url,data);
    
    if(check_error(resp_obj,KEEP_ES_RESULT)){
        free_json_object(resp_obj);
        return NULL;
    }
    else{
        return resp_obj;
    }
}

/*
return value:
not NULL:succeed,the pointer to next scroll result data;
NULL:failed
*/
json_object * es_scroll_next(const char *scroll_timeout,const char *scroll_id)
{
    return es_mul_scroll_next(NULL,scroll_timeout,scroll_id);
}

/*search domuments,remember to free the result(call es_free_result function)*/
/*
The scroll_timeout parameter tells Elasticsearch to keep the search context open for another time;
The initial search request and each subsequent scroll request returns a new _scroll_id,only the most recent _scroll_id should be used
return value:
not NULL:succeed,the pointer to next scroll result data;
NULL:failed
*/
json_object * es_mul_scroll_next(CURL *curl,const char *scroll_timeout,const char *scroll_id)
{
    char url[256] = {0};
    snprintf(url,sizeof(url),"http://%s:%d/_search/scroll",ES_SERVER_ADDR,ES_SERVER_PORT);
    json_object *scroll_obj;
    if(scroll_id){
        scroll_obj = json_object_new_object();
        if(!scroll_obj)
            return NULL;
	    json_object_object_add(scroll_obj, "scroll_id", json_object_new_string(scroll_id));
    }
    else
        return NULL;
    if(scroll_timeout){
	    json_object_object_add(scroll_obj, "scroll", json_object_new_string(scroll_timeout));
    }
    json_object *resp_obj =  es_do(curl,ES_ACTION_SEARCH,url,json_object_to_json_string(scroll_obj));
    json_object_put(scroll_obj);
    
    if(check_error(resp_obj,KEEP_ES_RESULT)){
        free_json_object(resp_obj);
        return NULL;
    }
    else{
        return resp_obj;
    }
}

/*
return value:
0:succeed;
1:failed
*/
int es_scroll_delete(const char *scroll_id)
{
    return es_mul_scroll_delete(NULL,scroll_id);
}

/*
Search context are automatically removed when the scroll timeout has been exceeded. 
However keeping scrolls open has a cost, so scrolls should be explicitly cleared as soon as the scroll is not being used anymore
return value:
0:succeed;
1:failed
*/
int es_mul_scroll_delete(CURL *curl,const char *scroll_id)
{
    int failure = 1;
    char url[256] = {0};
    snprintf(url,sizeof(url),"http://%s:%d/_search/scroll",ES_SERVER_ADDR,ES_SERVER_PORT);
    json_object *scroll_obj;
    if(scroll_id){
        scroll_obj = json_object_new_object();
        if(!scroll_obj)
            return failure;
	    json_object_object_add(scroll_obj, "scroll_id", json_object_new_string(scroll_id));
    }
    else
        return failure;
    json_object *root_obj =  es_do(curl,ES_ACTION_DELETE,url,json_object_to_json_string(scroll_obj));
    json_object_put(scroll_obj);
    if(root_obj){
        json_object *suc_obj = es_fetch_node(root_obj,"succeeded");
        if(suc_obj){
            if(json_object_get_boolean(suc_obj)){
                failure = 0;
            }
        }
        free_json_object(root_obj);
    }
    return failure;
}

/*
return value:
not NULL:the pointer to scroll_id object;
NULL:failed
*/
json_object *es_get_scroll_id(json_object *root_obj)
{
    return es_fetch_node(root_obj,"_scroll_id");
}

int64_t es_count(const char *index,const char *type,const char *data)
{
    return es_mul_count(NULL,index,type,data);
}

int64_t es_mul_count(CURL *curl,const char *index,const char *type,const char *data)
{
    int64_t c = 0;
    char url[256] = {0};
    snprintf(url,sizeof(url),"http://%s:%d/%s/%s/_count",ES_SERVER_ADDR,ES_SERVER_PORT,index,type);
    json_object *root_obj = es_do(curl,ES_ACTION_POST,url,data);
    json_object *count_obj = es_fetch_node(root_obj,"count");
    if(count_obj){
        c = json_object_get_int64(count_obj);
    }
    free_json_object(root_obj);
    return c;
}

/*remember to call  free_json_object function
return value:
not NULL:the pointer to json object;
NULL:failed
*/
json_object *json_string_to_json_object(const char *str)
{
    if(!str)
        return NULL;

    //json_object *root_object = json_tokener_parse(str);
    
    enum json_tokener_error jerr_ignored;
    struct json_object* root_object;
    root_object = json_tokener_parse_verbose(str, &jerr_ignored);
    if(root_object){
        return root_object;
    }
    else{
        printf("%s:the input string is not json format,check it\n",__func__);
        printf("%s:the string is %s,error is %d\n",__func__,str,jerr_ignored);
        return NULL;
    }
}

void free_json_object(json_object *obj)
{
    if(obj){
        json_object_put(obj);
        obj = NULL;
    }
}

/*fetch the node of json object by specified the node name
return value:
not NULL:the pointer to the node object;
NULL:failed
*/
json_object *es_fetch_node(json_object *obj,char *node)
{
    if(!obj || !node)
        return NULL;
    
    char dest_str[16*64] = {0};
    char node_elem[16][64];
    int idx = 0;
    int node_depth = 0;
    int err = 0;
    int arr_idx = -1;
    json_object *top_obj = obj;
    json_object *node_obj;
    json_type t;
    strncpy(dest_str,node,sizeof(dest_str)-1);
    char *parray = NULL;
    char *p = strtok(dest_str,".");
    
    while(p != NULL && idx < 16){
        strcpy(node_elem[idx++],p);
        p = strtok(NULL,".");
        node_depth++;
    }
    
    idx = 0;
    while(idx < node_depth){
        if((parray = strchr(node_elem[idx],'[')) != NULL){
            arr_idx = -1;
            if(sscanf(node_elem[idx],"%*[^[][%d%%[^]]",&arr_idx) == 1){/*get array index*/
                *parray = 0;
            }
        }
        
        if(!json_object_object_get_ex(top_obj, node_elem[idx],&node_obj)){
            err = 1;
            //fprintf(stderr,"failed to get object %s\n",node_elem[idx]);
            break;
        }
        
        t = json_object_get_type(node_obj);
        switch(t){
            case json_type_object:/*4*/
                break;
            case json_type_array:/*5*/
                {
                    int arr_len = json_object_array_length(node_obj);
                    if(arr_idx < 0)/*array,but not specified the index,it might be the leaf node,break out*/
                        break;
                    if(arr_idx >= arr_len){
                        err = 1;
                        fprintf(stderr,"the index(%d) over the array length(%d)\n",arr_idx,arr_len);
                        break;
                    }
                    node_obj = json_object_array_get_idx(node_obj,arr_idx);
                    if(!node_obj){
                        err = 1;
                        //fprintf(stderr,"failed to get object %s[%d]\n",node_elem[idx],arr_idx);
                    }
                }
                break;
            case json_type_string:/*6*/
            case json_type_int:/*3*/
            case json_type_boolean:/*1*/
            case json_type_double:/*2*/
                return node_obj;
            case json_type_null:/*0*/
                return NULL;
        }
        
        if(err)
            break;
        
        idx++;
        top_obj = node_obj;
    }
    
    return err ? NULL : node_obj;
}

/*fetch total searched domuments*/
int64_t es_fetch_hits_total(json_object *top_obj)
{
    json_object *total = es_fetch_node(top_obj,"hits.total");
    if(!total)
        return 0;
    else
        return json_object_get_int64(total);
}

/*fetch searched domuments(hits object)
return value:
not NULL:the pointer to the hits object;
NULL:failed
*/
json_object *es_fetch_hits(json_object *top_obj)
{
    return es_fetch_node(top_obj,"hits.hits");
}

/*fetch searched domuments length*/
int64_t es_fetch_hits_len(json_object *top_obj)
{
    json_object *hits = es_fetch_node(top_obj,"hits.hits");
    if(hits){
        if(json_object_get_type(hits) == json_type_array){
            return json_object_array_length(hits);
        }
    }
    return 0;
}

int64_t es_fetch_node_len(json_object *top_obj,char *node_name)
{
    json_object *node_obj = es_fetch_node(top_obj,node_name);
    if(node_obj){
        if(json_object_get_type(node_obj) == json_type_array){
            return json_object_array_length(node_obj);
        }
    }
    return 0;
}

/*fetch searched domuments source data
return value:
not NULL:the pointer to the hits source object;
NULL:failed
*/
json_object *es_fetch_hits_source(json_object *top_obj,int index)
{
    json_object *hits = es_fetch_node(top_obj,"hits.hits");
    if(hits){
        if(json_object_get_type(hits) == json_type_array){
            int arr_len = json_object_array_length(hits);
            if(index < arr_len){
                json_object *node = json_object_array_get_idx(hits,index);
                return es_fetch_node(node,"_source");
            }
        }
    }
    return NULL;
}

json_object *es_fetch_array_elem(json_object *arr_obj,int elem_idx)
{
    if(json_object_get_type(arr_obj) == json_type_array){
        if(json_object_array_length(arr_obj) <= elem_idx)
            return NULL;
        else
            return json_object_array_get_idx(arr_obj,elem_idx);
    }
    else
        return NULL;
}

int es_delete_node(json_object *obj,char *node)
{
    json_object *node_obj = es_fetch_node(obj,node);
    if(node_obj){
        json_object_put(node_obj);
        return 0;
    }
    else{
        fprintf(stderr,"not found node(%s) to delete\n",node);
        return 1;
    }
}

/*
node_type:the node(leaf node) type
*/
int es_set_node(json_object *obj,char *node,json_type node_type,void *node_value)
{
    if(!obj || !node || !node_value)
        return -1;

    char dest_str[16*64] = {0};
    char node_elem[16][64];
    int idx = 0;
    int err = 0;
    int node_depth = 0;
    int arr_idx = -1;
    json_object *top_obj = obj;
    json_object *node_obj = NULL;
    strncpy(dest_str,node,sizeof(dest_str)-1);
    char *parray = NULL;
    char *safe_ptr = NULL;
    char *p = strtok_r(dest_str,".",&safe_ptr);
    
    while(p != NULL && idx < 16){
        strcpy(node_elem[idx++],p);
        p = strtok_r(NULL,".",&safe_ptr);
        node_depth++;
    }
    
    idx = 0;
    json_type cur_node_type = json_type_null;
    json_type jt = json_type_null;
    int leaf_node; /*is leaf node? 0:no,1:yes*/
    int arr_node;/*is array node?0:no,1:yes*/
    char arr_node_name[128] = {0};
    
    while(idx < node_depth){
        if(!top_obj){
            fprintf(stderr,"error========top object is null?\n");
            err = 1;
            break;
        }
        leaf_node = 0;
        arr_node = 0;
        arr_idx = -1;
        /*if the node name contains charactor '[',we treat it as array, so ,get the array index*/
        if((parray = strchr(node_elem[idx],'[')) != NULL){
            if(sscanf(node_elem[idx],"%*[^[][%d%%[^]]",&arr_idx) == 1){/*get array index*/
                strncpy(arr_node_name,node_elem[idx],sizeof(arr_node_name)-1);
                *parray = 0;
                arr_node = 1;
            }
            else{
                fprintf(stderr,"failed to get array index,check the node...\n");
                err = 2;
                break;
            }
        }
        if(idx + 1 == node_depth){/*the last node,leaf node*/
            leaf_node = 1;
            if(arr_node)
                cur_node_type = json_type_array;
            else
                cur_node_type = node_type;
        }
        else{
            if(arr_node)
                cur_node_type = json_type_array;
            else
                cur_node_type = json_type_object;
        }
        
        node_obj = es_fetch_node(top_obj,node_elem[idx]);/*the node might by a array*/
        if(node_obj){
            if(arr_node){/*this node is a array node*/
                /*got node,now get element==========got array node*/
                int arr_len = json_object_array_length(node_obj);
                json_type arr_elem_type = json_type_null;
                if(arr_len > 0){
                    arr_elem_type = json_object_get_type(json_object_array_get_idx(node_obj,0));
                }
                json_object *elem_obj = json_object_array_get_idx(node_obj,arr_idx);
                if(!elem_obj){/*not found this element*/
                    if(arr_idx > arr_len){/*check it,we can not create a element without order,so we can handle array by array index*/
                        fprintf(stderr,"the index(%d) over the array length(%d)...\n",arr_idx,arr_len);
                        err = 3;
                        break;
                    }
                    if(leaf_node){
                        json_type node_value_type = 0;
                        json_object *node_value_obj = json_string_to_json_object((const char *)node_value);
                        if(node_value_obj){
                            json_type node_value_type = json_object_get_type(node_value_obj);
                            if(node_value_type != arr_elem_type){
                                fprintf(stderr,"array element type is wrong\n");
                                err = 10;
                            }
                            else{
                                json_object_array_add(node_obj,node_value_obj);
                            }
                        }
                        else{
                            err = 11;
                        }
                        break;
                    }
                    else{
                        if(arr_elem_type != json_type_object){
                            fprintf(stderr,"array element type is wrong\n");
                            err = 11;
                            break;
                        }
                        elem_obj = json_object_new_object();
                        json_object_array_add(node_obj,elem_obj);
                        node_obj = elem_obj;
                    }
                }
                else{
                    fprintf(stderr,"the array element is existed,please delete it first if you want to update it...\n");
                    err = 4;
                    break;
                }
            }
            else if(leaf_node){/*normal node and this is leaf node but not array,call put function*/
                fprintf(stderr,"the leaf node(%s) is existed,please delete it first if you want to update it...\n",node_elem[idx]);
                err = 4;
                break;
            }
            /*node existed,keep move*/
        }
        else{/*not found the node,new one,and then put it to the object*/
            if(!node_value && (cur_node_type != json_type_object && cur_node_type != json_type_array)){
                err = 2;
                break;
            }

            switch(cur_node_type){
                case json_type_object:/*4*/
                    if(leaf_node)
                        node_obj = json_string_to_json_object((const char *)node_value);
                    else
                        node_obj = json_object_new_object();
                    break;
                case json_type_array:/*5*/
                    node_obj = json_object_new_array();
                    if(leaf_node){
                        node_obj = json_string_to_json_object((const char *)node_value);
                        if(!node_obj || json_object_get_type(node_obj) != json_type_array){
                            fprintf(stderr,"value is not a array,but the node need a array...\n");
                            node_obj = NULL;/*make sure it could break out*/
                        }
                    }
                    else{
                        json_object *elem_obj = json_object_new_object();
                        json_object_array_add(node_obj,elem_obj);
                    }
                    break;
                case json_type_string:/*6*/
                    node_obj = json_object_new_string((char *)node_value);
                    break;
                case json_type_int:/*3*/
                    node_obj = json_object_new_int(atoi((char *)node_value));
                    break;
                case json_type_int64:/*3*/
                    node_obj = json_object_new_int64(atoll((char *)node_value));
                    break;
                case json_type_boolean:/*1*/
                    node_obj = json_object_new_boolean(atoi((char *)node_value));
                    break;
                case json_type_double:/*2*/
                    node_obj = json_object_new_double(atof((char *)node_value));
                    break;
                case json_type_null:/*0*/
                    return -2;
            }
            
            if(!node_obj){
                fprintf(stderr,"failed to new object,it's too bad...\n");
                err = 5;
                break;
            }
            if((jt = json_object_get_type(top_obj)) != json_type_object){
                fprintf(stderr,"json_object_object_add:the json type is not object,it's type is %d.\n",jt);
                err = 5;
                break;
            }
            json_object_object_add(top_obj,node_elem[idx],node_obj);
            if(json_object_get_type(node_obj) == json_type_array){
                if(arr_idx != 0){
                    printf("first index must be 0,but the array idx is %d=====\n",arr_idx);
                    err = 6;
                    break;
                }
                node_obj = json_object_array_get_idx(node_obj,arr_idx);
            }
        }
        
        idx++;
        top_obj = node_obj;
    }
    return err;
}



