#ifndef CLOUDSAVER_NETWORK_H
#define CLOUDSAVER_NETWORK_H

#include "common.h"

bool net_init(void);
void net_exit(void);
bool net_connect(const ServerConfig *cfg);
void net_disconnect(void);
bool net_is_connected(void);

bool net_upload(const char *local_path, const char *remote_path);
bool net_download(const char *remote_path, const char *local_path);
int  net_list_dir(const char *remote_path, char ***out_names);
bool net_mkdir(const char *remote_path);
bool net_exists(const char *remote_path);
bool net_delete(const char *remote_path);

const char *net_last_error(void);
void net_free_names(char **names, int count);

#endif /* CLOUDSAVER_NETWORK_H */
