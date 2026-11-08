#ifndef BLADES_TLS_CONFIG_H
#define BLADES_TLS_CONFIG_H
#include "mbedtls/mbedtls_config.h"
/* Portable scalar C on Xenon; use the kernel CSPRNG, never filesystem entropy. */
#undef MBEDTLS_HAVE_ASM
#undef MBEDTLS_AESNI_C
#undef MBEDTLS_AESCE_C
#undef MBEDTLS_PADLOCK_C
#undef MBEDTLS_ENTROPY_NV_SEED
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#undef MBEDTLS_NET_C
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_SSL_PROTO_TLS1_3
#define MBEDTLS_PLATFORM_MS_TIME_ALT
#endif
