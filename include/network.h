/**
 * 3DS CloudSaver - Network Module
 * SFTP/SMB connectivity for cloud synchronisation.
 */

#ifndef CLOUDSAVER_NETWORK_H
#define CLOUDSAVER_NETWORK_H

#include "common.h"

/** Initialise the networking subsystem (SOC). */
bool net_init(void);

/** Shut down the networking subsystem. */
void net_exit(void);

/** Attempt to connect to the configured server. */
bool net_connect(const ServerConfig *cfg);

/** Disconnect from the server. */
void net_disconnect(void);

/** Check if currently connected. */
bool net_is_connected(void);

/** Upload a file to the remote server.
 *  remote_path is relative to the configured base path. */
bool net_upload(const char *local_path, const char *remote_path);

/** Download a file from the remote server. */
bool net_download(const char *remote_path, const char *local_path);

/** List files in a remote directory.
 *  Returns the number of entries, or -1 on error.
 *  Caller must free the returned names. */
int net_list_dir(const char *remote_path, char ***out_names);

/** Create a remote directory (recursive). */
bool net_mkdir(const char *remote_path);

/** Check if a remote file/dir exists. */
bool net_exists(const char *remote_path);

/** Delete a remote file. */
bool net_delete(const char *remote_path);

/** Get the last network error message. */
const char *net_last_error(void);

/** Free a name list returned by net_list_dir. */
void net_free_names(char **names, int count);

#endif /* CLOUDSAVER_NETWORK_H */
