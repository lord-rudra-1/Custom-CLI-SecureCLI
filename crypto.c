#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

#define SALT_SIZE 16
#define IV_SIZE 12   // GCM uses 12-byte IV (96 bits)
#define KEY_SIZE 32   // AES-256
#define TAG_SIZE 16   // GCM authentication tag
#define PBKDF2_ITER 100000  // Increased from 20k to 100k for better security
#define BUF_SIZE 4096

// --- Helper: write buffer to file, check errors ---
static bool write_all(FILE *f, const unsigned char *buf, size_t len) {
    size_t w = fwrite(buf, 1, len, f);
    return w == len;
}

bool encrypt_file(const char *in_path, const char *out_path, const char *password) {
    FILE *fin = NULL, *fout = NULL;
    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char key[KEY_SIZE];
    unsigned char tag[TAG_SIZE];

    bool ok = false;
    EVP_CIPHER_CTX *ctx = NULL;
    unsigned char inbuf[BUF_SIZE];
    unsigned char outbuf[BUF_SIZE + EVP_MAX_BLOCK_LENGTH];
    int outlen;

    // Open files
    fin = fopen(in_path, "rb");
    if (!fin) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }
    fout = fopen(out_path, "wb");
    if (!fout) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }

    // Generate salt and IV
    if (RAND_bytes(salt, SALT_SIZE) != 1) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }
    if (RAND_bytes(iv, IV_SIZE) != 1) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }

    // Derive key using PBKDF2 (HMAC-SHA256) with 100k iterations
    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          salt, SALT_SIZE,
                          PBKDF2_ITER,
                          EVP_sha256(),
                          KEY_SIZE, key) != 1) {
        fprintf(stderr, "Encryption failed\n"); goto cleanup;
    }

    // Write salt and IV to output file (header)
    if (!write_all(fout, salt, SALT_SIZE)) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }
    if (!write_all(fout, iv, IV_SIZE)) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }

    // Setup encryption with AES-256-GCM
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1) { 
        fprintf(stderr, "Encryption failed\n"); goto cleanup; 
    }

    // Process file
    size_t r;
    while ((r = fread(inbuf, 1, BUF_SIZE, fin)) > 0) {
        if (EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, (int)r) != 1) { 
            fprintf(stderr, "Encryption failed\n"); goto cleanup; 
        }
        if (!write_all(fout, outbuf, outlen)) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }
    }
    if (ferror(fin)) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, outbuf, &outlen) != 1) { 
        fprintf(stderr, "Encryption failed\n"); goto cleanup; 
    }
    if (outlen > 0) {
        if (!write_all(fout, outbuf, outlen)) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }
    }

    // Get authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag) != 1) {
        fprintf(stderr, "Encryption failed\n"); goto cleanup;
    }

    // Write tag at the end
    if (!write_all(fout, tag, TAG_SIZE)) { fprintf(stderr, "Encryption failed\n"); goto cleanup; }

    ok = true;

cleanup:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (fin) fclose(fin);
    if (fout) fclose(fout);
    // clear sensitive memory
    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(iv, sizeof(iv));
    OPENSSL_cleanse(salt, sizeof(salt));
    OPENSSL_cleanse(tag, sizeof(tag));
    return ok;
}

bool decrypt_file(const char *in_path, const char *out_path, const char *password) {
    FILE *fin = NULL, *fout = NULL;
    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char key[KEY_SIZE];
    unsigned char tag[TAG_SIZE];

    bool ok = false;
    EVP_CIPHER_CTX *ctx = NULL;
    unsigned char inbuf[BUF_SIZE + EVP_MAX_BLOCK_LENGTH];
    unsigned char outbuf[BUF_SIZE];
    int outlen, inlen;
    long file_size, ciphertext_size;

    fin = fopen(in_path, "rb");
    if (!fin) { fprintf(stderr, "Decryption failed\n"); goto cleanup; }
    fout = fopen(out_path, "wb");
    if (!fout) { fprintf(stderr, "Decryption failed\n"); goto cleanup; }

    // Get file size
    fseek(fin, 0, SEEK_END);
    file_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    // Check minimum file size (salt + iv + tag)
    if (file_size < SALT_SIZE + IV_SIZE + TAG_SIZE) {
        // Generic error message - don't leak details
        fprintf(stderr, "Decryption failed\n");
        goto cleanup;
    }

    // Read salt and IV from file
    if (fread(salt, 1, SALT_SIZE, fin) != SALT_SIZE) { 
        fprintf(stderr, "Decryption failed\n"); goto cleanup; 
    }
    if (fread(iv, 1, IV_SIZE, fin) != IV_SIZE) { 
        fprintf(stderr, "Decryption failed\n"); goto cleanup; 
    }

    // Calculate ciphertext size (file - salt - iv - tag)
    ciphertext_size = file_size - SALT_SIZE - IV_SIZE - TAG_SIZE;

    // Derive key using PBKDF2 with 100k iterations
    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          salt, SALT_SIZE,
                          PBKDF2_ITER,
                          EVP_sha256(),
                          KEY_SIZE, key) != 1) {
        fprintf(stderr, "Decryption failed\n"); goto cleanup;
    }

    // Setup decryption with AES-256-GCM
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { fprintf(stderr, "Decryption failed\n"); goto cleanup; }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1) { 
        fprintf(stderr, "Decryption failed\n"); goto cleanup; 
    }

    // Read and decrypt ciphertext (excluding tag)
    long bytes_read = 0;
    while (bytes_read < ciphertext_size) {
        inlen = (int)fread(inbuf, 1, 
                          (ciphertext_size - bytes_read < BUF_SIZE) ? 
                          (size_t)(ciphertext_size - bytes_read) : BUF_SIZE, 
                          fin);
        if (inlen <= 0) break;
        
        if (EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen) != 1) { 
            fprintf(stderr, "Decryption failed\n"); goto cleanup; 
        }
        if (outlen > 0) {
            if (fwrite(outbuf, 1, outlen, fout) != (size_t)outlen) { 
                fprintf(stderr, "Decryption failed\n"); goto cleanup; 
            }
        }
        bytes_read += inlen;
    }
    if (ferror(fin)) { 
        fprintf(stderr, "Decryption failed\n"); goto cleanup; 
    }

    // Read authentication tag from end of file
    if (fread(tag, 1, TAG_SIZE, fin) != TAG_SIZE) {
        fprintf(stderr, "Decryption failed\n"); goto cleanup;
    }

    // Set expected tag for verification
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag) != 1) {
        fprintf(stderr, "Decryption failed\n"); goto cleanup;
    }

    // Finalize and verify tag
    if (EVP_DecryptFinal_ex(ctx, outbuf, &outlen) != 1) {
        // Generic error - don't leak whether it's wrong password or corrupted file
        fprintf(stderr, "Decryption failed\n");
        goto cleanup;
    }
    if (outlen > 0) {
        if (fwrite(outbuf, 1, outlen, fout) != (size_t)outlen) { 
            fprintf(stderr, "Decryption failed\n"); goto cleanup; 
        }
    }

    ok = true;

cleanup:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (fin) fclose(fin);
    if (fout) fclose(fout);
    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(iv, sizeof(iv));
    OPENSSL_cleanse(salt, sizeof(salt));
    OPENSSL_cleanse(tag, sizeof(tag));
    return ok;
}

bool sha256_file_hex(const char *path, char *out_hex) {
    FILE *f = fopen(path, "rb");
    if (!f) { return false; }

    unsigned char buf[BUF_SIZE];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha_ctx;
    SHA256_Init(&sha_ctx);

    size_t r;
    while ((r = fread(buf, 1, BUF_SIZE, f)) > 0) {
        SHA256_Update(&sha_ctx, buf, r);
    }
    if (ferror(f)) { fclose(f); return false; }
    SHA256_Final(hash, &sha_ctx);
    fclose(f);

    // convert to hex
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(out_hex + (i * 2), "%02x", hash[i]);
    }
    out_hex[SHA256_DIGEST_LENGTH * 2] = '\0';
    return true;
}
