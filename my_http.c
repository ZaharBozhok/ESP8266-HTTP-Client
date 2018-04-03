#include <stdio.h>
#include <sys/types.h>
#include "my_http.h"

static void url_parse(char *uri, char **host,size_t *host_size, char **path, size_t *path_size)
{
	*host = uri;
	char *tmp = strstr(uri, "//");
	if (tmp)
		*host = tmp + 2;
	*path = strchr(*host, '/');
	if (*path)
	{
		*host_size = *path - *host;
		tmp = strchr(*path, ':');
		if (tmp)
			*tmp = 0;
		*path_size = strlen(*path);
	}
	else
	{
		*host_size = strlen(*host);
		*path_size = 1;
	}
	
}
static char * create_request(char **request, char *uri,const char *params,const char* params_size)
{
	if (0 == uri || 0 == strlen(uri))
		return 0;
	char *host = 0;
	char *path = 0;
	size_t host_size = 0;
	size_t path_size = 0;
	url_parse(uri, &host,&host_size, &path,&path_size);
	printf("host_size=%d,path_size=%d\n",host_size,path_size);
	size_t req_size = STR_SIZE(HTTP_HOST ENDL ENDL) + path_size + host_size + 1;
	char *it = 0;
	if (0 == params)//GET
	{
		req_size += STR_SIZE(GET);
		*request = (char*)malloc(req_size);
		it = *request;

		memcpy(it, GET, STR_SIZE(GET));
		it += STR_SIZE(GET);

	}
	else //POST
	{
		req_size += STR_SIZE(POST CONTENT ENDL ENDL) +strlen(params_size) + strlen(params);
		*request = (char*)malloc(req_size);
		it = *request;

		memcpy(it, POST, STR_SIZE(POST));
		it += STR_SIZE(POST);
	}

	if (path_size == 1)
		memcpy(it, "/", 1);
	else
		memcpy(it, path, path_size);
	it += path_size;

	memcpy(it, HTTP_HOST, STR_SIZE(HTTP_HOST));
	it += STR_SIZE(HTTP_HOST);

	memcpy(it, host, host_size);
	it += host_size;

	if (params)
	{
		memcpy(it, CONTENT, STR_SIZE(CONTENT));
		it += STR_SIZE(CONTENT);

		memcpy(it, params_size, strlen(params_size));
		it += strlen(params_size);

		memcpy(it, ENDL ENDL, STR_SIZE(ENDL ENDL));
		it += STR_SIZE(ENDL ENDL);

		size_t size = atoi(params_size);
		memcpy(it, params,size);
		it += size;
	}

	memcpy(it, ENDL ENDL, STR_SIZE(ENDL ENDL));
	it += STR_SIZE(ENDL ENDL);

	*it = 0;
	if(path)
		*path = 0;
	return host;
}
void free_request(char **request)
{
	if(*request)
	{
		free(*request);
		*request = 0;
	}
}

void ito_str(size_t num, char *dst)
{
	size_t tmp = num;
	size_t size = 1;
	while(tmp/=10)
		++size;
	while(size)
	{
		dst[--size] = num % 10 + '0';
		num/=10;
	}
}

char * init_request(char **request,char *uri,const char *params)
{
	char *ret = 0;
	char *size = 0;
	if (params)
	{
		size = (char *)malloc(5);
		bzero(size,5);
		ito_str(strlen(params),size);
		printf("size=%s",size);
	}
	ret =  create_request(request, uri, params, size);
	if(size)
		free(size);
	return ret;
}
