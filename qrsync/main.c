//
//  main.c
//  qrsync
//
//  Created by Danilo Altheman on 23/01/13.
//  Copyright (c) 2013 Danilo Altheman. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include "librsync.h"

enum kSignatureStatus {
    kSignatureOK,
    kSignatureFailed
};

int sign_file(const char *filename, const char *signature_filename) {
    FILE *base_file, *signature_file;
    if((base_file = fopen(filename, "rb")) == NULL) {
        perror("File to Sign");
        return kSignatureFailed;
    }
    if((signature_file = fopen(signature_filename, "wb")) == NULL) {
        perror("Signature File");
        return kSignatureFailed;
    }
    rs_stats_t stat;
    rs_result result = rs_sig_file(base_file, signature_file, RS_DEFAULT_BLOCK_LEN, RS_DEFAULT_STRONG_LEN, &stat);
    int sig_status;
    if (result != RS_DONE) {
        printf("Signing file error: %s\n", rs_strerror(result));
        sig_status = kSignatureFailed;
    }
    else {
        sig_status = kSignatureOK;
    }
    rs_log_stats(&stat);
    fclose(base_file);
    fclose(signature_file);
    return sig_status;
}

int gen_delta(const char *filename, const char *signature_filename, const char *delta_filename) {
    FILE *base_file, *signature_file, *delta_file;
    rs_stats_t stat;
    rs_signature_t *signature;
    int sig_status;
    
    if ((base_file = fopen(filename, "rb")) == NULL) {
        perror("File to Diff");
        sig_status = kSignatureFailed;
    }
    
    if ((signature_file = fopen(signature_filename, "rb")) == NULL) {
        perror("Signature File");
        sig_status = kSignatureFailed;
    }
    
    if ((delta_file = fopen(delta_filename, "wb")) == NULL) {
        perror("Delta File");
        sig_status = kSignatureFailed;
    }
    
    rs_result result = rs_loadsig_file(signature_file, &signature, &stat);
    if (result != RS_DONE) {
        printf("Loading signature file error: %s\n", rs_strerror(result));
        rs_log_stats(&stat);
    }
    else {
        result = rs_build_hash_table(signature);
        if(result != RS_DONE) {
            printf("Hash table Error: %s\n", rs_strerror(result));
        }
        else {
            result = rs_delta_file(signature, base_file, delta_file, &stat);
            if(result != RS_DONE) {
                printf("Delta file error: %s\n", rs_strerror(result));
                rs_log_stats(&stat);
            }
        }
    }
    
    if (signature) rs_free_sumset(signature);
    fclose(base_file);
    fclose(signature_file);
    fclose(delta_file);
    rs_log_stats(&stat);
    return sig_status;
}

int apply_patch(const char *filename, const char *delta_filename) {
     FILE *base_file, *delta_file, *new_file;
    rs_result result;
    rs_stats_t stat;
    
    if((base_file = fopen(filename, "rb")) == NULL) {
        perror("File to Sign");
        return kSignatureFailed;
    }
    if((delta_file = fopen(delta_filename, "rb")) == NULL) {
        perror("File to Sign");
        return kSignatureFailed;
    }
    
    if((new_file = fopen("newfile.txt", "wb")) == NULL) {
        perror("File to Sign");
        return kSignatureFailed;
    }
    
    result = rs_patch_file(base_file, delta_file, new_file, &stat);
    fclose(base_file);
    fclose(delta_file);
    fclose(new_file);
    
    if (result != RS_DONE) {
        printf("Error patching file: %s\n", rs_strerror(result));
        rs_log_stats(&stat);
        return kSignatureFailed;
    }
    rs_log_stats(&stat);
    return kSignatureOK;
}

int main(int argc, const char * argv[]) {
    if (argc > 1) {
        sign_file(argv[1], "signature");
        gen_delta(argv[1], "signature", "delta");
        if(apply_patch(argv[1], "delta") == kSignatureOK) {
            unlink("delta");
            unlink("signature");
        }
        printf("Version: %s\n", rs_librsync_version);
    }
    else {
        printf("Usage: %s file_to_patch\n", argv[0]);
    }
    return 0;
}

