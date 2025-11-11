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
#define IV_SIZE 16
#define KEY_SIZE 32   // AES-256
#define PBKDF2_ITER 20000
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

    bool ok = false;
    EVP_CIPHER_CTX *ctx = NULL;
    unsigned char inbuf[BUF_SIZE];
    unsigned char outbuf[BUF_SIZE + EVP_MAX_BLOCK_LENGTH];
    int outlen;

    // Open files
    fin = fopen(in_path, "rb");
    if (!fin) { perror("fopen input"); goto cleanup; }
    fout = fopen(out_path, "wb");
    if (!fout) { perror("fopen output"); goto cleanup; }

    // Generate salt and IV
    if (RAND_bytes(salt, SALT_SIZE) != 1) { fprintf(stderr, "RAND_bytes(salt) failed\n"); goto cleanup; }
    if (RAND_bytes(iv, IV_SIZE) != 1) { fprintf(stderr, "RAND_bytes(iv) failed\n"); goto cleanup; }

    // Derive key using PBKDF2 (HMAC-SHA256)
    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          salt, SALT_SIZE,
                          PBKDF2_ITER,
                          EVP_sha256(),
                          KEY_SIZE, key) != 1) {
        fprintf(stderr, "PBKDF2 failed\n"); goto cleanup;
    }

    // Write salt and IV to output file (header)
    if (!write_all(fout, salt, SALT_SIZE)) { perror("write salt"); goto cleanup; }
    if (!write_all(fout, iv, IV_SIZE)) { perror("write iv"); goto cleanup; }

    // Setup encryption
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { fprintf(stderr, "EVP_CIPHER_CTX_new failed\n"); goto cleanup; }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) { fprintf(stderr, "EncryptInit failed\n"); goto cleanup; }

    // Process file
    size_t r;
    while ((r = fread(inbuf, 1, BUF_SIZE, fin)) > 0) {
        if (EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, (int)r) != 1) { fprintf(stderr, "EncryptUpdate failed\n"); goto cleanup; }
        if (!write_all(fout, outbuf, outlen)) { perror("write ciphertext"); goto cleanup; }
    }
    if (ferror(fin)) { perror("fread"); goto cleanup; }

    if (EVP_EncryptFinal_ex(ctx, outbuf, &outlen) != 1) { fprintf(stderr, "EncryptFinal failed\n"); goto cleanup; }
    if (!write_all(fout, outbuf, outlen)) { perror("write final"); goto cleanup; }

    ok = true;

cleanup:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (fin) fclose(fin);
    if (fout) fclose(fout);
    // clear sensitive memory
    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(iv, sizeof(iv));
    OPENSSL_cleanse(salt, sizeof(salt));
    return ok;
}

bool decrypt_file(const char *in_path, const char *out_path, const char *password) {
    FILE *fin = NULL, *fout = NULL;
    unsigned char salt[SALT_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char key[KEY_SIZE];

    bool ok = false;
    EVP_CIPHER_CTX *ctx = NULL;
    unsigned char inbuf[BUF_SIZE + EVP_MAX_BLOCK_LENGTH];
    unsigned char outbuf[BUF_SIZE];
    int outlen, inlen;

    fin = fopen(in_path, "rb");
    if (!fin) { perror("fopen input"); goto cleanup; }
    fout = fopen(out_path, "wb");
    if (!fout) { perror("fopen output"); goto cleanup; }

    // Read salt and IV from file
    if (fread(salt, 1, SALT_SIZE, fin) != SALT_SIZE) { fprintf(stderr, "Failed to read salt\n"); goto cleanup; }
    if (fread(iv, 1, IV_SIZE, fin) != IV_SIZE) { fprintf(stderr, "Failed to read iv\n"); goto cleanup; }

    // Derive key
    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          salt, SALT_SIZE,
                          PBKDF2_ITER,
                          EVP_sha256(),
                          KEY_SIZE, key) != 1) {
        fprintf(stderr, "PBKDF2 failed\n"); goto cleanup;
    }

    // Setup decryption
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { fprintf(stderr, "EVP_CIPHER_CTX_new failed\n"); goto cleanup; }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) { fprintf(stderr, "DecryptInit failed\n"); goto cleanup; }

    // Read and decrypt
    while ((inlen = (int)fread(inbuf, 1, BUF_SIZE, fin)) > 0) {
        if (EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen) != 1) { fprintf(stderr, "DecryptUpdate failed\n"); goto cleanup; }
        if (outlen > 0) {
            if (fwrite(outbuf, 1, outlen, fout) != (size_t)outlen) { perror("fwrite"); goto cleanup; }
        }
    }
    if (ferror(fin)) { perror("fread"); goto cleanup; }

    if (EVP_DecryptFinal_ex(ctx, outbuf, &outlen) != 1) {
        fprintf(stderr, "DecryptFinal failed — possibly wrong password or corrupted file\n");
        goto cleanup;
    }
    if (outlen > 0) {
        if (fwrite(outbuf, 1, outlen, fout) != (size_t)outlen) { perror("fwrite final"); goto cleanup; }
    }

    ok = true;

cleanup:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (fin) fclose(fin);
    if (fout) fclose(fout);
    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(iv, sizeof(iv));
    OPENSSL_cleanse(salt, sizeof(salt));
    return ok;
}

bool sha256_file_hex(const char *path, char *out_hex) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); return false; }

    unsigned char buf[BUF_SIZE];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha_ctx;
    SHA256_Init(&sha_ctx);

    size_t r;
    while ((r = fread(buf, 1, BUF_SIZE, f)) > 0) {
        SHA256_Update(&sha_ctx, buf, r);
    }
    if (ferror(f)) { perror("fread"); fclose(f); return false; }
    SHA256_Final(hash, &sha_ctx);
    fclose(f);

    // convert to hex
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(out_hex + (i * 2), "%02x", hash[i]);
    }
    out_hex[SHA256_DIGEST_LENGTH * 2] = '\0';
    return true;
}
