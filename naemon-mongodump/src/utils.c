#include <libmongoc-1.0/mongoc.h>
#include <time.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "utils.h"

extern mongoc_client_t	*client;

int
to_mongo(struct metric_t *m)
{
	bson_oid_t	 	 oid;
	bson_t			*doc;
	bson_error_t		 error;
	mongoc_collection_t	*collection;

	collection = mongoc_client_get_collection(client, "naemon", "performance");
	doc = bson_new();
	bson_oid_init(&oid, NULL);
	BSON_APPEND_OID(doc, "_id", &oid);
	BSON_APPEND_UTF8(doc, "host_name", m->host_name);
	BSON_APPEND_UTF8(doc, "service_description", m->service_description);
	BSON_APPEND_UTF8(doc, "name", m->name);
	BSON_APPEND_UTF8(doc, "unit", m->unit);
	BSON_APPEND_DOUBLE(doc, "warning", m->warning);
	BSON_APPEND_DOUBLE(doc, "critical", m->critical);
	BSON_APPEND_DOUBLE(doc, "value", m->value);
	BSON_APPEND_INT64(doc, "timestamp", time(NULL));
	if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
		FILE *fp = fopen("/tmp/teste", "a");
		fprintf(fp, "error: %s\n", error.message);
		fclose(fp);
	}
	bson_destroy (doc);
	mongoc_collection_destroy(collection);

	return 0;
}

int
metric_to_json(struct metric_t *m, char **dst)
{
	json_t	*root = NULL;
	char	*ret = NULL;

	root = json_object();
	json_object_set_new(root, "host_name", json_string(m->host_name));
	json_object_set_new(root, "service_description", json_string(m->service_description));
	json_object_set_new(root, "name", json_string(m->name));
	json_object_set_new(root, "unit", json_string(m->unit));
	json_object_set_new(root, "warning", json_real(m->warning));
	json_object_set_new(root, "critical", json_real(m->critical));
	json_object_set_new(root, "value", json_real(m->value));
	ret = json_dumps(root, 0);

	FILE *fp = fopen("/tmp/teste", "a");
	fprintf(fp, ">%s<\n", ret);
	fclose(fp);
	json_decref(root);

	/*
	*dst = calloc(strlen(ret)+1, sizeof(char));
	strcpy(*dst, ret);
	*/
	return 0;
}

struct metric_t *
parse_perfdata(char *perfdata)
{
	struct metric_t		*previous_metric = NULL;
	struct metric_t		*metric = NULL;
	regmatch_t		 pmatch[MAXMETRICS];
	regex_t			 regex;
	regex_t			 metric_regex;
	char			 *metric_string = NULL;
	char			 *aux_string = NULL;
	int			 ret;
	int			 i;
	int			 length;

	// regex to split by metric
	// OK: \PhysicalDisk(0 C:)\Disk Transfers/sec: 0|'\PhysicalDisk(0 C:)\Disk Transfers/sec'=0;20;25;
	ret = regcomp(&regex," *([^=]+=[^ ]+)", REG_EXTENDED); 
	if (ret != 0) {
		printf("Unable to compile regular expression\n");
		return NULL;
	}

	// a regex to extract our metric values 
	ret = regcomp(
	    &metric_regex,
	    "([^=]+)=([0-9.]+)([^;]*);*([^;]*);*([^;]*);*([^;]*);*([^;]*)",
	    REG_EXTENDED
	);

	if (ret != 0) {
		printf("Unable to compile internal regular expression\n");
		return NULL;
	}

	// for every metric, we gonna extract our data
	while(regexec(&regex, perfdata, MAXMETRICS, pmatch, 0) == 0 ) {

		length = pmatch[1].rm_eo - pmatch[1].rm_so + 1;

		// create a string to store our metric data
		metric_string = (char *)calloc(length,sizeof(char));

		// copy metric data to string
		strncpy(metric_string, perfdata + pmatch[1].rm_so,
		    pmatch[1].rm_eo - pmatch[1].rm_so);

		// advance perfdata to the next metric
		perfdata += pmatch[1].rm_eo;

		//printf("%s\n",metric_string);

		// apply the regex to one metric data
		if (regexec(&metric_regex, metric_string, MAXMETRICS, pmatch, 0) == 0) {

			metric = malloc(sizeof(struct metric_t));
			metric->next = NULL;

			// iterate through our matches
			for(i=1; i<=METRICFIELDS; i++) {

				// end of matches
				if (pmatch[i].rm_so < 0)
					break;

				length = pmatch[i].rm_eo - pmatch[i].rm_so + 1;

				aux_string = calloc(length,sizeof(char));
				strncpy(aux_string, 
				    metric_string + pmatch[i].rm_so,
				    pmatch[i].rm_eo - pmatch[i].rm_so);

				switch(i) {

				case METRICNAME:
					metric->name = strdup(aux_string);
					break;
					
				case METRICVALUE:
					metric->value = atof(aux_string);
					break;

				case METRICUNIT:
					metric->unit = strdup(aux_string);
					break;

				case METRICWARNING:
					metric->warning = atof(aux_string);
					break;

				case METRICCRITICAL:
					metric->critical = atof(aux_string);
					break;

				case METRICMIN:
					metric->min = atof(aux_string);
					break;

				case METRICMAX:
					metric->max = atof(aux_string);
					break;

				}


				free(aux_string);


			}

			if (previous_metric)
				metric->next = previous_metric;

			previous_metric = metric;
		}

		free(metric_string);
	}

	regfree(&metric_regex);
	regfree(&regex);
	return metric;
}
