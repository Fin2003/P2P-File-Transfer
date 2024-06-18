#ifndef BTYDE_CRYPT_SHA256
#define BTYDE_CRYPT_SHA256

#include <stdint.h>

#define SHA256_CHUNK_SZ (64)
#define SHA256_INT_SZ (8)
#define SHA256_DFTLEN (1024)

//Original: https://github.com/LekKit/sha256/blob/master/sha256.h
struct sha256_compute_data {
	uint64_t data_size;
	uint32_t hcomps[SHA256_INT_SZ];
	uint8_t last_chunk[SHA256_CHUNK_SZ];
	uint8_t chunk_size;
};

void sha256_calculate_chunk(struct sha256_compute_data* data,
		uint8_t chunk[SHA256_CHUNK_SZ]);

void sha256_compute_data_init(struct sha256_compute_data* data);

void sha256_update(struct sha256_compute_data* data,
		void* bytes, uint32_t size); 

void sha256_finalize(struct sha256_compute_data* data, 
		uint8_t hash[SHA256_INT_SZ]);


void sha256_output_hex(struct sha256_compute_data* data, 
		char hexbuf[SHA256_CHUNK_SZ]);

#endif

