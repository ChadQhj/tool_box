#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "es_action.h"
#include <json-c/json.h>


#define INDEX "test_index"
#define TYPE "test_type"

int max = 500001;

const char *post_bulk = "{\"index\":{}}\n\
{\
\"pip\":122333,\
\"aip\":32124,\
\"sport\":8034,\
\"dport\":1312,\
\"proto\":\"tcp\",\
\"app_proto\":\"https\",\
\"signature\":\"sql&where 1 = 11?\",\
\"type\":\"linux-os-vul-1\",\
\"event_level\":1,\
\"timestamp\":1313210,\
\"status\":0\
}\n\
{\"index\":{}}\n\
{\
\"pip\":223344333,\
\"aip\":32312,\
\"sport\":80,\
\"dport\":132,\
\"proto\":\"tcp\",\
\"app_proto\":\"http\",\
\"signature\":\"sql&where 1 = 1?\",\
\"type\":\"linux-os-vul\",\
\"event_level\":4,\
\"timestamp\":131321,\
\"status\":0\
}\n";

const char *doc_data1 = "{\
\"property_ip\":\"192.168.2.107\",\
\"dst_ip\":\"183.126.64.44\",\
\"property_port\":221,\
\"dst_port\":12323,\
\"protocol\":\"tcp\",\
\"dst_country\":\"china\",\
\"dst_city\":\"bj\",\
\"event_type\":\"suspicious domain\",\
\"event_detail\":\"domain belongs to blacklist\",\
\"event_data\":\"www.something.com/\",\
\"suspicious_content\":\"eeeeeeeeeeeeeeeeee\",\
\"event_cnt\":2,\
\"algorithm\":\"static check\",\
\"event_level\":1,\
\"timestamp\":1510248912\
}\n";

const char *doc_data = "{\
\"pip\":1134,\
\"aip\":12246,\
\"sport\":8088,\
\"dport\":8013,\
\"proto\":\"tcp\",\
\"app_proto\":\"https\",\
\"signature\":\"sql&where 1 = 3?\",\
\"type\":\"linux-o-os-vul133122\",\
\"event_level\":1,\
\"timestamp\":131232100,\
\"status\":0\
}\n";

char *search=  "{\
\"query\":{\
    \"bool\":{\
        \"filter\":{\
            \"term\":{\
                \"event_level\":1\
            }\
        }\
    }\
}\
}";
    
int test_mul_thread(int *max)
{
    int i = 0;
    CURL *p_curl = NULL;
    
    if(!p_curl){
        p_curl = init_curl_handle();
        if(set_curl_common_options(p_curl,10,1,0,0))
            return -1;
    }
    
    while(i<*max){
      if(es_mul_post(p_curl,INDEX,TYPE,doc_data))
        break;
      i++;
    }
    printf("thread %d post %d docs\n",pthread_self(),i);
    
    destroy_curl_handle(p_curl);
}

void test_put()
{
	json_object *my_object = json_object_new_object();
	json_object_object_add(my_object, "pip", json_object_new_int(121233333));
	json_object_object_add(my_object, "aip", json_object_new_int(3212));
	json_object_object_add(my_object, "sport", json_object_new_int(80));
	json_object_object_add(my_object, "dport", json_object_new_int(32112));
	json_object_object_add(my_object, "proto", json_object_new_string("bar"));
	json_object_object_add(my_object, "type", json_object_new_string("sql-inject111"));
	json_object_object_add(my_object, "level", json_object_new_int(5));
	json_object_object_add(my_object, "timestamp", json_object_new_int(13254524));
	json_object_object_add(my_object, "status", json_object_new_int(0));
    const char *putd = json_object_to_json_string(my_object);
    if(!es_put(INDEX,TYPE,"doc1",doc_data))
        printf("put doc1 ok\n");
    else
        printf("put doc1 error\n");
}
void test_json()
{

    	int now = 100;
	json_object *my_object = json_object_new_object();
	json_object *pkt_arr_object = json_object_new_array();
	json_object *pkt_object = json_object_new_object();
	json_object_object_add(my_object, "timestamp", json_object_new_int(now*1000));
	json_object_object_add(my_object, "property_ip", json_object_new_string("111"));
	json_object_object_add(my_object, "property_port", json_object_new_int(1));
	json_object_object_add(my_object, "remote_ip", json_object_new_string("222"));
	json_object_object_add(my_object, "remote_port", json_object_new_int(2));
	json_object_object_add(my_object, "protocol", json_object_new_string("tcp"));
	json_object_object_add(my_object, "app_proto", json_object_new_string("http"));
	json_object_object_add(my_object, "remote_country", json_object_new_string("cn"));
	json_object_object_add(my_object, "remote_city", json_object_new_string("cd"));
	json_object_object_add(my_object, "event_type", json_object_new_string("ssss"));
	json_object_object_add(my_object, "level", json_object_new_int(1));
	json_object_object_add(my_object, "rule_id", json_object_new_int(0));
	json_object_object_add(my_object, "signature", json_object_new_string("igotyou"));
	json_object_object_add(my_object, "status", json_object_new_boolean(0));
	json_object_object_add(my_object, "handled", json_object_new_boolean(0));
	json_object_object_add(my_object, "advisement", json_object_new_string("scan the property"));
	json_object_object_add(my_object, "source", json_object_new_string("digger"));
	json_object_object_add(pkt_object, "timestamp", json_object_new_int(now*1000));
	json_object_object_add(pkt_object, "pkt", json_object_new_string("wwwwwwwwwwwwwwwwwwwwwwwwww"));
	json_object_array_add(pkt_arr_object, pkt_object);
	json_object_object_add(my_object, "pkt_data", pkt_arr_object);
    	const char *put_data = json_object_to_json_string(my_object);
	if(put_data)
		printf("json string is:%s\n",put_data);
	else
		printf("json string is not right\n");
}

void test_delete()
{
    if(!es_delete(INDEX,TYPE,"doc1"))
        printf("delete doc1 ok\n");
    else
        printf("delete doc1 error\n");
}

void test_single_thread_post(int max)
{
    int i = 0;
    printf("test single thread post===\n");
    
    while(i<max){
        if(es_post(INDEX,TYPE,doc_data))
            break;
        i++;
    }
    
    printf("indexd %d one by one\n",i);
}

void test_single_thread_post_bulk(int max)
{
    int i=0,j=0;
    char put_data[1024*100];
    
    while(i<max){
        if(j>=100){
            if(es_post_bulk(INDEX,TYPE,put_data)){
                i -= j;
                break;
            }
            j= 0;
            memset(put_data,0,sizeof(put_data));
        }
        else{
            strcat(put_data,"{\"index\":{}}\n");
            strcat(put_data,doc_data);
        }
        i++;
        j++;
    }
    
    if(j>0){
        es_post_bulk(INDEX,TYPE,put_data);
    }
    printf("indexd %d bulk\n",i);
}

void test_mul_thread_post(int max)
{
    #define TNUM 4
    pthread_t tid[TNUM];
    int error;
    int k;
    
    for(k = 0; k< TNUM; k++) {
        error = pthread_create(&tid[k],NULL,test_mul_thread,&max);
        if(0 != error)
          fprintf(stderr, "Couldn't run thread number %d, errno %d\n", k, error);
        else
          fprintf(stderr, "Thread %d, start\n", k);
    }
    
    for(k = 0; k< TNUM; k++) {
        error = pthread_join(tid[k], NULL);
        fprintf(stderr, "Thread %d terminated\n", k);
    }
}

void test_search()
{
    json_object * resp = es_search(INDEX,NULL,search);
    if(resp){
        json_object *node = es_fetch_node(resp,"hits.hits[0]._source.event_data");
        if(node){
            printf("got node...\n");
            printf("event_data is %s\n",json_object_get_string(node));
        }
        int source_len = es_fetch_hits_len(resp);
        int i = 0;
        while(i<source_len){
            node = es_fetch_hits_source(resp,i++);
            if(node){
                printf("source data is %s\n",json_object_to_json_string(node));
            }
        }
        free_json_object(resp);
    }
}

void test_node_action()
{
    int err = 0;
    json_object *root_obj = json_object_new_object();
    err += es_set_node(root_obj,"size",json_type_int,"5");
    err += es_set_node(root_obj,"aggs.pips.must[0].terms.field",json_type_string,"pip");
    err += es_set_node(root_obj,"aggs.pips.must[0].terms.text",json_type_string,"aip");
    err += es_set_node(root_obj,"aggs.pips.must[1].terms.size",json_type_int,"5");
    err += es_set_node(root_obj,"aggs.pips.must[2].terms.order.max_time",json_type_string,"desc");

    if(err)
        printf("==================err=======================\n");
    else
        fprintf(stderr,"==========end object:%s\n",json_object_to_json_string(root_obj));

    free_json_object(root_obj);
    
    //return;
    err = 0;
     root_obj = json_object_new_object();
    err += es_set_node(root_obj,"size",json_type_int,"5");
    err += es_set_node(root_obj,"aggs.pips.must.terms.field",json_type_string,"pip");
    err += es_set_node(root_obj,"aggs.pips.terms.text",json_type_string,"aip");
    err += es_set_node(root_obj,"aggs.pips.terms.size",json_type_int,"5");
    err += es_set_node(root_obj,"aggs.pips.test",json_type_object,"{\"order\":\"desc\",\"111\":123}");
    
    if(err)
        printf("==================err=======================\n");
    else
        fprintf(stderr,"==========end object:%s\n",json_object_to_json_string(root_obj));
    
    free_json_object(root_obj);
}

void print_usage()
{
    printf("usage:\n");
    printf("./example 1             //test put\n");
    printf("./example 2 [count]     //test single thread post,count is 1000 by default\n");
    printf("./example 3 [count]     //test single thread post bulk,count is 1000 by default\n");
    printf("./example 4 [count]     //test mul-thread post,count is 1000 by default\n");
    printf("./example 5             //test search\n");
    printf("./example 6             //test delete\n");
    printf("./example 7             //test json object\n");
    printf("./example 8             //test set node and delete node\n");
}
int main(int argc,char *argv[])
{

    int action = 0;
    int max = 1000;
    
    if(init_curl())
        return -1;
    
    switch(argc){
        case 3:
            max = atoll(argv[2]);
        case 2:
            action = atoi(argv[1]);
            break;
        default:
            print_usage();
            return 0;
    }
    
    switch(action){
        case 1:
            test_put();
            break;
        case 2:
            test_single_thread_post(max);
            break;
        case 3:
            test_single_thread_post_bulk(max);
            break;
        case 4:
            test_mul_thread_post(max);
            break;
        case 5:
            test_search();
            break;
        case 6:
            test_delete();
            break;
	    case 7:
    		test_json();
    		break;
	    case 8:
    		test_node_action();
    		break;
        default:
            print_usage();
            break;
    }
    destroy_curl();
    return 0;
}

