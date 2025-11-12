#ifndef REMOTE_H
#define REMOTE_H

#include <stdbool.h>

// Start TLS server on specified port
// Returns true on success, false on error
bool start_tls_server(int port);

// Connect to TLS server and run CLI session
// Returns true on success, false on error
bool connect_tls_client(const char *hostname, int port);

#endif // REMOTE_H


