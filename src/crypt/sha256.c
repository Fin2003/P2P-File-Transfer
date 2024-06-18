
#include <crypt/sha256.h>
#include <string.h>
#include <stdio.h>

#define SHA256K 64
#define rotate_r(val, bits) (val >> bits | val << (32 - bits))

//Constant List from: https://en.wikipedia.org/wiki/SHA-2#Pseudocode
static const uint32_t k[SHA256K] = {
    0x428a2f98, 0x71374491, 
    0xb5c0fbcf, 0xe9b5dba5, 
    0x3956c25b, 0x59f111f1, 
    0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 
    0x243185be, 0x550c7dc3, 
    0x72be5d74, 0x80deb1fe, 
    0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 
    0x0fc19dc6, 0x240ca1cc, 
    0x2de92c6f, 0x4a7484aa, 
    0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 
    0xb00327c8, 0xbf597fc7, 
    0xc6e00bf3, 0xd5a79147, 
    0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 
    0x4d2c6dfc, 0x53380d13, 
    0x650a7354, 0x766a0abb, 
    0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 
    0xc24b8b70, 0xc76c51a3, 
    0xd192e819, 0xd6990624, 
    0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 
    0x2748774c, 0x34b0bcb5, 
    0x391c0cb3, 0x4ed8aa4a, 
    0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 
    0x84c87814, 0x8cc70208, 
    0x90befffa, 0xa4506ceb, 
    0xbef9a3f7, 0xc67178f2
};

//Initialisation From: https://en.wikipedia.org/wiki/SHA-2#Pseudocode
//and https://github.com/LekKit/sha256/blob/master/sha256.c
void sha256_compute_data_init(struct sha256_compute_data* data) {
	data->hcomps[0] = 0x6a09e667;
	data->hcomps[1] = 0xbb67ae85;
	data->hcomps[2] = 0x3c6ef372;
	data->hcomps[3] = 0xa54ff53a;
	data->hcomps[4] = 0x510e527f;
	data->hcomps[5] = 0x9b05688c;
	data->hcomps[6] = 0x1f83d9ab;
	data->hcomps[7] = 0x5be0cd19;

	data->data_size = 0;
	data->chunk_size = 0;

}

//Derived from: https://en.wikipedia.org/wiki/SHA-2#Pseudocode
//And https://github.com/LekKit/sha256/blob/master/sha256.c
void sha256_calculate_chunk(struct sha256_compute_data *data, 
		uint8_t chunk[SHA256_CHUNK_SZ]) {
	uint32_t w[SHA256_CHUNK_SZ];
	uint32_t tv[SHA256_INT_SZ];

    //
	for(uint32_t i = 0; i < 16; i++) {
		w[i] = (uint32_t) chunk[0] << 24 
			| (uint32_t) chunk[1] << 16 
			| (uint32_t) chunk[2] << 8 
			| (uint32_t) chunk[3];

		chunk += 4;
	}

    //
	for(uint32_t i = 16; i < 64; i++) {
		
		uint32_t s0 = rotate_r(w[i-15], 7) 
			    ^ rotate_r(w[i-15], 18) 
			    ^ (w[i-15] >> 3);
		
		uint32_t s1 = rotate_r(w[i-2], 17) 
			    ^ rotate_r(w[i-2], 19) 
			    ^ (w[i-2] >> 10);

		w[i] = w[i-16] + s0 + w[i-7] + s1;
	}

	for(uint32_t i = 0; i < SHA256_INT_SZ; i++) {
		tv[i] = data->hcomps[i];
	}

	for(uint32_t i = 0; i < SHA256_CHUNK_SZ; i++) {
		uint32_t S1 = rotate_r(tv[4], 6) 
			    ^ rotate_r(tv[4], 11) 
			    ^ rotate_r(tv[4], 25);

		uint32_t ch = (tv[4] & tv[5]) 
			    ^ (~tv[4] & tv[6]);
		
		uint32_t temp1 = tv[7] + S1 + ch + k[i] + w[i];
		
		uint32_t S0 = rotate_r(tv[0], 2) 
			    ^ rotate_r(tv[0], 13) 
			    ^ rotate_r(tv[0], 22);
		
		uint32_t maj = (tv[0] & tv[1]) 
			     ^ (tv[0] & tv[2]) 
			     ^ (tv[1] & tv[2]);
		
		uint32_t temp2 = S0 + maj;
		
		tv[7] = tv[6];
		tv[6] = tv[5];
		tv[5] = tv[4];
		tv[4] = tv[3] + temp1;
		tv[3] = tv[2];
		tv[2] = tv[1];
		tv[1] = tv[0];
		tv[0] = temp1 + temp2;
	}

	for(uint32_t i = 0; i < SHA256_INT_SZ; i++) {
		data->hcomps[i] += tv[i];
	}
}

//Derived from: https://en.wikipedia.org/wiki/SHA-2#Pseudocode
//And https://github.com/LekKit/sha256/blob/master/sha256.c
void sha256_update(struct sha256_compute_data *data, 
		void *bytes, uint32_t size) {
	
	uint8_t* ptr = (uint8_t*) bytes;
	data->data_size += size;
	
	if (size + data->chunk_size >= 64) {
		uint8_t tmp_chunk[64];
		memcpy(tmp_chunk, 
				data->last_chunk, 
				data->chunk_size);
		memcpy(tmp_chunk + data->chunk_size, 
				ptr, 
				64 - data->chunk_size);
		ptr += (64 - data->chunk_size);
		size -= (64 - data->chunk_size);
		data->chunk_size = 0;
		sha256_calculate_chunk(data, tmp_chunk);
	}

	while (size >= 64) {
		sha256_calculate_chunk(data, ptr);
		ptr += 64;
		size -= 64; 
	}

	memcpy(data->last_chunk + data->chunk_size, ptr, size);
	data->chunk_size += size;
}

//Derived from: https://en.wikipedia.org/wiki/SHA-2#Pseudocode
//And https://github.com/LekKit/sha256/blob/master/sha256.c
void sha256_finalize(struct sha256_compute_data *data, 
		uint8_t hash[SHA256_INT_SZ]) {
	
	

	data->last_chunk[data->chunk_size] = 0x80;
	data->chunk_size++;

	memset(data->last_chunk + 
			data->chunk_size, 
			0, 
			64 - data->chunk_size);

	if (data->chunk_size > 56) {
		sha256_calculate_chunk(data, data->last_chunk);
		memset(data->last_chunk, 0, 64);
	}

	/* Add total size as big-endian int64 x8 */
	uint64_t size = data->data_size * 8;
	
	for (int32_t i = 8; i > 0; --i) {
		data->last_chunk[55+i] = size & 255;
		size >>= 8;
	}

	sha256_calculate_chunk(data, data->last_chunk);
}

//Original: https://github.com/LekKit/sha256/blob/master/sha256.c
void sha256_output(struct sha256_compute_data* data, 
		uint8_t* hash) {
	for (uint32_t i = 0; i < 8; i++) {
		hash[i*4] = (data->hcomps[i] >> 24) & 255;
		hash[i*4 + 1] = (data->hcomps[i] >> 16) & 255;
		hash[i*4 + 2] = (data->hcomps[i] >> 8) & 255;
		hash[i*4 + 3] = data->hcomps[i] & 255;
	}
}

//Original: https://github.com/LekKit/sha256/blob/master/sha256.c
static void bin_to_hex(const void* data, uint32_t len, char* out) {
    
	static const char* const lut = "0123456789abcdef";

	for (uint32_t i = 0; i < len; ++i){
		uint8_t c = ((const uint8_t*)data)[i];
		out[i*2] = lut[c >> 4];
		out[i*2 + 1] = lut[c & 15];
	}
}

//Original: https://github.com/LekKit/sha256/blob/master/sha256.c
void sha256_output_hex(struct sha256_compute_data* data, 
		char hexbuf[SHA256_CHUNK_SZ]) {
	uint8_t hash[32] = { 0 };
	sha256_output(data, hash);
	bin_to_hex(hash, 32, hexbuf);
}