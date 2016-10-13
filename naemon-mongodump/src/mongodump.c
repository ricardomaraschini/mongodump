#include <libmongoc-1.0/mongoc.h>
#include <naemon/naemon.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include "utils.h"

NEB_API_VERSION(CURRENT_NEB_API_VERSION); 
void		*ghandle;
mongoc_client_t	*client;
pid_t		 mongoin_pid;

int
mongo_broker_check(int event_type, void *data)
{
	nebstruct_service_check_data	*broker_data = NULL;
	struct metric_t			*metrics = NULL;
	struct metric_t			*cursor = NULL;

	broker_data = (nebstruct_service_check_data *)data;
	if (broker_data->perf_data == NULL)
		return OK;
	
	if (broker_data->type != NEBTYPE_SERVICECHECK_PROCESSED)
		return OK;

	metrics = parse_perfdata(broker_data->perf_data);
	for(cursor=metrics; cursor; cursor=cursor->next) {
		asprintf(&cursor->host_name, "%s", broker_data->host_name);
		asprintf(&cursor->service_description, "%s", broker_data->service_description);
		to_mongo(cursor);
	}

	return OK;
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

	if (args == NULL)
		return OK;

	neb_register_callback(
	    NEBCALLBACK_SERVICE_CHECK_DATA,
	    ghandle,
	    0,
	    mongo_broker_check
	);

	mongoc_init();
	client = mongoc_client_new(args);

	mongoin_pid = fork();
	if (mongoin_pid != 0)
		return OK;

	while(1) {
		sleep(1);
	}
	exit(0);
}

int
nebmodule_deinit(int flags, int reason)
{
	int	ret;

	neb_deregister_callback(
	    NEBCALLBACK_SERVICE_CHECK_DATA,
	    mongo_broker_check
	);
	mongoc_client_destroy(client);

	kill(mongoin_pid, SIGKILL);
	waitpid(mongoin_pid, &ret, 0);

	return OK;
}
