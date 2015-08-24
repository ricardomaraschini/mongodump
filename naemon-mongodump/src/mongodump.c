#include <libmongoc-1.0/mongoc.h>
#include <naemon/naemon.h>
#include <unistd.h>
#include "utils.h"

NEB_API_VERSION(CURRENT_NEB_API_VERSION); 
void		*ghandle;
mongoc_client_t	*client;

int
broker_check(int event_type, void *data)
{
	nebstruct_service_check_data	*broker_data = NULL;
	struct metric_t			*metrics = NULL;
	struct metric_t			*cursor = NULL;

	broker_data = (nebstruct_service_check_data *)data;
	if (broker_data->perf_data == NULL)
		return OK;
	
	if (broker_data->type != NEBTYPE_SERVICECHECK_PROCESSED)
		return OK;

	/*
	FILE *fp = fopen("/tmp/teste", "a");
	fprintf(fp, "<%d>[%s]\n", broker_data->type, broker_data->perf_data);
	fclose(fp);
	*/

	metrics = parse_perfdata(broker_data->perf_data);

	for(cursor=metrics; cursor; cursor=cursor->next) {
		asprintf(&cursor->host_name, "%s", broker_data->host_name);
		asprintf(&cursor->service_description, "%s", broker_data->service_description);
		to_mongo(cursor);
		//metric_to_json(cursor, &jsonstr);
		//fprintf(fp, "%s\n", jsonstr);
	}

	FILE *fp = fopen("/tmp/teste", "a");
	fprintf(fp, ".\n");
	fclose(fp);

	return OK;
}

void
register_callbacks()
{
	neb_register_callback(
	    NEBCALLBACK_SERVICE_CHECK_DATA,
	    ghandle,
	    0,
	    broker_check
	);
}

void
deregister_callbacks()
{
	neb_deregister_callback(
	    NEBCALLBACK_SERVICE_CHECK_DATA,
	    broker_check
	);
}

int
nebmodule_init(int flags, char *args, void *handle)
{
	ghandle = handle;
	neb_set_module_info(ghandle, NEBMODULE_MODINFO_TITLE, "mongodump");
	neb_set_module_info(ghandle, NEBMODULE_MODINFO_AUTHOR, "rmars");
	neb_set_module_info(ghandle, NEBMODULE_MODINFO_TITLE, "copyright (c) 2015");
	neb_set_module_info(ghandle, NEBMODULE_MODINFO_VERSION, "1");
	neb_set_module_info(ghandle, NEBMODULE_MODINFO_LICENSE, "gpl");
	neb_set_module_info(ghandle, NEBMODULE_MODINFO_DESC, "dumps perfdata to mongodb.");
	register_callbacks();

	mongoc_init();
	client = mongoc_client_new("mongodb://192.168.10.160:27017/");

	return OK;
}

int
nebmodule_deinit(int flags, int reason)
{
	deregister_callbacks();
	mongoc_client_destroy(client);
	return OK;
}
