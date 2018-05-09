
#ifndef _ES_ACTION_H_
#define _ES_ACTION_H_

#include <curl/curl.h>
#include <json-c/json.h>

#define json_type_int64 10
/*NOTE:
all of those APIs if contains any parameters below:
curl:  the curl handler, fetch it by call init_curl_handle function;
index: the es index name;
type: the es type name under index;
id: the es document id under type;
data: the json format of query data.
*/
#ifdef __cplusplus
extern "C" {
#endif

/*initialize the curl global values for es c-api,es c-api use curl to call the es REST API,call it at the main thread context once*/
int init_curl();
/*destrory curl global values,call it when you're not want to use es c-api at the end of main thread context*/
void destroy_curl();
/*initialize the fd to handle the curl connection,at multi-thread context,each thread should initialize a fd if the thread want to use es c-api,
but at single-thread context,there is no need to call it,because the es c-api(es_xxx) will handle this*/
CURL * init_curl_handle();
/*destroy the fd resource*/
void destroy_curl_handle(CURL *curl);
/*set curl options*/
int set_curl_common_options(CURL *curl,int timeout,short ignore_sig,short verbose,short keep_alive);



/*es _search api, like select*/
json_object *es_search(const char *index,const char *type,const char *data);
/*get document by doc id*/
json_object *es_get(const char *index,const char *type,const char *id);
/*es _delete_by_query api, like delete from xxx where*/
json_object *es_delete_by_query(const char *index,const char *type,const char *data,char *options);
/*delete document by doc id*/
int es_delete(const char *index,const char *type,const char *id);
/*es _update_by_query api, like update xxx where*/
json_object *es_update_by_query(const char *index,const char *type,const char *data,char *options);
/*es _update api, update document by doc id*/
int es_update(const char *index,const char *type,const char *id,const char *data);
/*post method,post a batch of documents*/
json_object *es_post_bulk(const char *index,const char *type,const char *data);
/*post method ,post a document*/
int es_post(const char *index,const char *type,const char *data);
/*put method,put a document by specified the doc id*/
int es_put(const char *index,const char *type,const char *id,const char *data);



/*the function of those APIs below is same as these functions which without mul,the only diffrecence is those APIs used in mul-thread context*/
json_object * es_mul_search(CURL *curl,const char *index,const char *type,const char *data);
json_object *es_mul_get(CURL *curl,const char *index,const char *type,const char *id);
json_object *es_mul_delete_by_query(CURL *curl,const char *index,const char *type,const char *data,char *options);
int es_mul_delete(CURL *curl,const char *index,const char *type,const char *id);
json_object *es_mul_update_by_query(CURL *curl,const char *index,const char *type,const char *data,char *options);
int es_mul_update(CURL *curl,const char *index,const char *type,const char *id,const char *data);
json_object *es_mul_post_bulk(CURL *curl,const char *index,const char *type,const char *data);
int es_mul_post(CURL *curl,const char *index,const char *type,const char *data);
int es_mul_put(CURL *curl,const char *index,const char *type,const char *id,const char *data);



/*es scroll api,it can be used to retrieve large numbers of results (or even all results) from a single search request*/
json_object * es_scroll(const char *index,const char *type,const char *scroll_timeout,const char *data);
/*get the next results*/
json_object * es_scroll_next(const char *scroll_timeout,const char *scroll_id);
/*delete the scroll context*/
int es_scroll_delete(const char *scroll_id);
/*get scroll id from scroll result*/
json_object *es_get_scroll_id(json_object *root_obj);
/*es _count api,like select count(*) from xxx*/
int64_t es_count(const char *index,const char *type,const char *data);



/*the function of those APIs below is same as these functions which without mul,the only diffrecence is those APIs used in mul-thread context*/
json_object * es_mul_scroll(CURL *curl,const char *index,const char *type,const char *scroll_timeout,const char *data);
json_object * es_mul_scroll_next(CURL *curl,const char *scroll_timeout,const char *scroll_id);
int es_mul_scroll_delete(CURL *curl,const char *scroll_id);
int64_t es_mul_count(CURL *curl,const char *index,const char *type,const char *data);



/*json string transfer to json object,input a json string,output a json object,if the str is not a json string,return NULL*/
json_object *json_string_to_json_object(const char *str);
/*call it after you call the json_string_to_json_object function*/
void free_json_object(json_object *obj);



/*fetch the hits.hits object from the result object*/
json_object *es_fetch_hits(json_object *top_obj);
/*fetch the hits.hits._source object from the result object*/
json_object *es_fetch_hits_source(json_object *top_obj,int index);
/*fetch the length of hits.hits array object from the result object*/
int64_t es_fetch_hits_len(json_object *top_obj);
/*fetch the total count of hits(hits.total) from the result object*/
int64_t es_fetch_hits_total(json_object *top_obj);
/*fetch the node object from the result object,the node name such as:hits.total, hits.hits._source.property_ip,hits.hits[0].timestamp*/
json_object *es_fetch_node(json_object *object,char *node);
int es_set_node(json_object *obj,char *node,json_type node_type,void *node_value);
int es_delete_node(json_object *obj,char *node);
json_object *es_fetch_array_elem(json_object *arr_obj,int elem_idx);

#ifdef __cplusplus
}
#endif

#endif

