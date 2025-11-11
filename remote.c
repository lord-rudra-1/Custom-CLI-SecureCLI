#include "remote.h"
#include "auth.h"
#include "logger.h"
#include "commands.h"

extern int user_count;
extern User users[];
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

static SSL_CTX *server_ctx = NULL;
static SSL_CTX *client_ctx = NULL;

// Initialize OpenSSL
static void init_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

// Cleanup OpenSSL
static void cleanup_openssl() {
    EVP_cleanup();
}

// Create self-signed certificate (for demo purposes)
// In production, use proper certificates
static int create_certificate(const char *cert_file, const char *key_file) {
    FILE *f = fopen(cert_file, "r");
    if (f) {
        fclose(f);
        return 1; // Certificate already exists
    }

    // Generate a self-signed certificate using OpenSSL command
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "openssl req -x509 -newkey rsa:4096 -keyout %s -out %s -days 365 -nodes -subj \"/CN=securecli\" 2>/dev/null",
        key_file, cert_file);
    
    return system(cmd) == 0;
}

// Setup server SSL context
static SSL_CTX *create_server_context() {
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // Create certificate if it doesn't exist
    if (!create_certificate("server.crt", "server.key")) {
        fprintf(stderr, "Failed to create certificate\n");
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Load certificate and key
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

// Setup client SSL context
static SSL_CTX *create_client_context() {
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // For demo, we'll accept self-signed certificates
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
    
    return ctx;
}

// Handle client connection in server
static void handle_client(SSL *ssl, int client_fd) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    // Send welcome message
    const char *welcome = "🔒 SecureSysCLI Remote Session\n";
    SSL_write(ssl, welcome, strlen(welcome));

    // Load users for authentication
    load_users();
    
    // Authentication loop
    bool authenticated = false;
    char username[50], password[50], hash[65];
    
    while (!authenticated) {
        // Request username
        const char *prompt_user = "Username: ";
        SSL_write(ssl, prompt_user, strlen(prompt_user));
        
        int len = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (len <= 0) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_fd);
            return;
        }
        buffer[len] = '\0';
        // Remove newline
        if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        strncpy(username, buffer, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';

        // Request password
        const char *prompt_pass = "Password: ";
        SSL_write(ssl, prompt_pass, strlen(prompt_pass));
        
        len = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (len <= 0) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_fd);
            return;
        }
        buffer[len] = '\0';
        if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        strncpy(password, buffer, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';

        // Hash password and verify
        unsigned char hash_bytes[32];
        SHA256((unsigned char *)password, strlen(password), hash_bytes);
        for (int i = 0; i < 32; i++) {
            sprintf(hash + (i * 2), "%02x", hash_bytes[i]);
        }
        hash[64] = '\0';

        // Check credentials
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, username) == 0 &&
                strcmp(users[i].password_hash, hash) == 0) {
                current_user = &users[i];
                authenticated = true;
                snprintf(response, sizeof(response), "✅ Login successful. Welcome %s (%s)\n", 
                        username, current_user->role);
                SSL_write(ssl, response, strlen(response));
                break;
            }
        }

        if (!authenticated) {
            const char *error = "❌ Invalid credentials. Try again.\n";
            SSL_write(ssl, error, strlen(error));
        }
    }

    // Main command loop
    const char *prompt = "SecureSysCLI@remote:~$ ";
    
    while (1) {
        SSL_write(ssl, prompt, strlen(prompt));
        
        int len = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (len <= 0) {
            break; // Client disconnected
        }
        buffer[len] = '\0';
        
        // Remove newline
        if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        if (buffer[len - 1] == '\r') buffer[len - 1] = '\0';

        // Handle exit
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            const char *msg = "Exiting SecureSysCLI...\n";
            SSL_write(ssl, msg, strlen(msg));
            break;
        }

        // Log command
        log_command(buffer);

        // Execute command (simplified - capture output)
        // For full implementation, we'd need to redirect stdout/stderr to SSL
        // This is a simplified version that sends responses back
        FILE *fp = popen(buffer, "r");
        if (fp) {
            char cmd_output[BUFFER_SIZE] = {0};
            size_t total = 0;
            while (fgets(response, sizeof(response), fp) && total < sizeof(cmd_output) - 1) {
                size_t resp_len = strlen(response);
                if (total + resp_len < sizeof(cmd_output) - 1) {
                    strcat(cmd_output, response);
                    total += resp_len;
                }
            }
            pclose(fp);
            SSL_write(ssl, cmd_output, strlen(cmd_output));
        } else {
            const char *error = "Command execution failed\n";
            SSL_write(ssl, error, strlen(error));
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_fd);
}

// Start TLS server
bool start_tls_server(int port) {
    init_openssl();
    
    server_ctx = create_server_context();
    if (!server_ctx) {
        cleanup_openssl();
        return false;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        SSL_CTX_free(server_ctx);
        cleanup_openssl();
        return false;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        SSL_CTX_free(server_ctx);
        cleanup_openssl();
        return false;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        SSL_CTX_free(server_ctx);
        cleanup_openssl();
        return false;
    }

    printf("🔒 TLS Server listening on port %d\n", port);
    printf("Certificate: server.crt, Key: server.key\n");

    // Handle SIGCHLD to reap zombie processes
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Fork to handle client
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(server_fd);
            
            SSL *ssl = SSL_new(server_ctx);
            SSL_set_fd(ssl, client_fd);
            
            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                close(client_fd);
                exit(1);
            }

            handle_client(ssl, client_fd);
            exit(0);
        } else if (pid > 0) {
            // Parent process
            close(client_fd);
        } else {
            perror("fork");
            close(client_fd);
        }
    }

    close(server_fd);
    SSL_CTX_free(server_ctx);
    cleanup_openssl();
    return true;
}

// Connect to TLS server (client)
bool connect_tls_client(const char *hostname, int port) {
    init_openssl();
    
    client_ctx = create_client_context();
    if (!client_ctx) {
        cleanup_openssl();
        return false;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        SSL_CTX_free(client_ctx);
        cleanup_openssl();
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, hostname, &addr.sin_addr) <= 0) {
        // Try hostname resolution
        struct hostent *he = gethostbyname(hostname);
        if (!he) {
            fprintf(stderr, "Failed to resolve hostname\n");
            close(sock);
            SSL_CTX_free(client_ctx);
            cleanup_openssl();
            return false;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        SSL_CTX_free(client_ctx);
        cleanup_openssl();
        return false;
    }

    SSL *ssl = SSL_new(client_ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(client_ctx);
        cleanup_openssl();
        return false;
    }

    printf("🔒 Connected to SecureSysCLI server\n");

    // Simple interactive loop
    // Note: For production, use non-blocking I/O or threads for better UX
    char buffer[BUFFER_SIZE];
    int len;
    
    // Set socket to non-blocking for better interaction
    // For simplicity, we'll use a basic approach
    
    while (1) {
        // Try to read from server (non-blocking check)
        len = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (len > 0) {
            buffer[len] = '\0';
            printf("%s", buffer);
            fflush(stdout);
            
            // If we see a prompt, it's time to read from stdin
            if (strstr(buffer, "SecureSysCLI@remote:~$") || 
                strstr(buffer, "Username:") || 
                strstr(buffer, "Password:")) {
                // Read from stdin
                if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                    break; // EOF
                }
                SSL_write(ssl, buffer, strlen(buffer));
            }
        } else if (len < 0) {
            int err = SSL_get_error(ssl, len);
            if (err == SSL_ERROR_WANT_READ) {
                // Would block - check stdin instead
                fd_set readfds;
                struct timeval tv;
                FD_ZERO(&readfds);
                FD_SET(STDIN_FILENO, &readfds);
                tv.tv_sec = 0;
                tv.tv_usec = 100000; // 100ms
                
                if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0) {
                    if (FD_ISSET(STDIN_FILENO, &readfds)) {
                        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                            break;
                        }
                        SSL_write(ssl, buffer, strlen(buffer));
                    }
                }
            } else {
                break; // Error or connection closed
            }
        } else {
            break; // Connection closed
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(client_ctx);
    cleanup_openssl();
    
    return true;
}

