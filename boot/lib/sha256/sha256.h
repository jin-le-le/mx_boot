/**
 * @file sha256.h
 * @brief SHA-256 hash (NIST FIPS 180-4)
 *
 * Used to hash the firmware before ECDSA signature verification.
 * HMAC is not included (the project now uses asymmetric ECDSA signatures).
 */

#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_DIGEST_SIZE  32

/**
 * @brief Compute the SHA-256 hash (one-shot; internally performs init/update/final)
 * @param data Input data
 * @param len  Data length
 * @param out  Output buffer (at least 32 bytes)
 */
void sha256_compute(const uint8_t *data, size_t len, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* SHA256_H */
