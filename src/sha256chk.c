 #include <btide/crypt/sha256.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHA256_BFLEN (1024)

int main(int argc, char** argv) {


  FILE* file = NULL;
  struct sha256_compute_data cdata = { 0 };
  char buf[SHA256_BFLEN];
  size_t nbytes = 0;
  uint8_t hashout[SHA256_INT_SZ];
  char final_hash[65] = { 0 };
  if(argc < 2) {
    puts("No file provided");
    return 1;
  }

  file = fopen(argv[1], "r");

  if(file == NULL) {
    puts("Unable to open file");
    fclose(file);
    return 1;
  }

  sha256_compute_data_init(&cdata);


  while((nbytes = fread(buf, 1, SHA256_BFLEN, file))
      == SHA256_BFLEN) {
    sha256_update(&cdata, &buf, SHA256_BFLEN);
  }

  sha256_update(&cdata, buf, nbytes);
  sha256_finalize(&cdata, hashout);
  sha256_output_hex(&cdata, final_hash);

  puts(final_hash);

}