#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdbool.h>

// Encrypt the input file and write to output file.
// Password is used to derive a 256-bit key using PBKDF2.
// Output format: [16 bytes salt][16 bytes iv][ciphertext...]
bool encrypt_file(const char *in_path, const char *out_path, const char *password);

// Decrypt the input file (format as above) and write plaintext to out_path.
// Returns true on success.
bool decrypt_file(const char *in_path, const char *out_path, const char *password);

// Compute SHA-256 checksum of a file, output hex string into `out_hex` (must be at least 65 bytes).
// Returns true on success.
bool sha256_file_hex(const char *path, char *out_hex);

#endif // CRYPTO_H
