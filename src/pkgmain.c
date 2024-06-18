 
#include <chk/pkgchk.h>
#include <crypt/sha256.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define SHA256_HEX_LEN (64)



int arg_select(int argc, char** argv, int* asel, char* harg) {


	char* cursor = argv[2];
	*asel = 0;
	if(argc < 3) {
		puts("bpkg or flag not provided");
		exit(1);
	}

	if(strcmp(cursor, "-all_hashes") == 0) {
		*asel = 1;
	}
	if(strcmp(cursor, "-chunk_check") == 0) {
		*asel = 2;
	}
	if(strcmp(cursor, "-min_hashes") == 0) {
		*asel = 3;
	}
	if(strcmp(cursor, "-hashes_of") == 0) {
		if(argc < 4) {
			puts("filename not provided");
			exit(1);
		}
		*asel = 4;
		strncpy(harg, argv[3], SHA256_HEX_LEN);
	}
	if(strcmp(cursor, "-file_check") == 0) {
		*asel = 5;
	}
	return *asel;
}


void bpkg_print_hashes(struct bpkg_query* qry) {
	for(int i = 0; i < qry->len; i++) {
		printf("%.64s\n", qry->hashes[i]);
	}

}

int main(int argc, char** argv) {

	int argselect = 0;
	char hash[SHA256_HEX_LEN];


	if(arg_select(argc, argv, &argselect, hash)) {
		struct bpkg_query qry = { 0 };
		struct bpkg_obj* obj = bpkg_load(argv[1]);

		if(!obj) {
			puts("Unable to load pkg and tree");
			exit(1);
		}

		if(argselect == 1) {
			qry = bpkg_get_all_hashes(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		} else if(argselect == 2) {

			qry = bpkg_get_completed_chunks(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		} else if(argselect == 3) {

			qry = bpkg_get_min_completed_hashes(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		} else if(argselect == 4) {

			qry = bpkg_get_all_chunk_hashes_from_hash(obj,
					hash);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		} else if(argselect == 5) {

			qry = bpkg_file_check(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		} else {
			puts("Argument is invalid");
			return 1;
		}
		bpkg_obj_destroy(obj);

	}

	return 0;
}