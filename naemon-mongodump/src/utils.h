#ifndef __MONGODUMPUTILS
#define __MONGODUMPUTILS

#define MAXMETRICS	10
#define METRICFIELDS	8
#define METRICNAME	1 
#define METRICVALUE	2
#define METRICUNIT	3
#define METRICWARNING	4 
#define METRICCRITICAL	5
#define METRICMIN	6
#define METRICMAX	7 

struct metric_t {
	struct metric_t	*next;
	float		 value;
	float		 warning;
	float		 critical;
	float		 min;
	float		 max;
	char		*name;
	char		*unit;
	char		*host_name;
	char		*service_description;
};

struct metric_t *parse_perfdata(char *);
int to_mongo(struct metric_t *);
int metric_to_json(struct metric_t *, char **);

#endif
