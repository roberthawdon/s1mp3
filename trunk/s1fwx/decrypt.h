#ifndef _DECRYPT_H_
#define _DECRYPT_H_

bool decrypt(void *dst_ptr, void *src_ptr, uint32 size, void *key, uint32 keylen);
void extract_key(char *key, uint8 *enc_file, size_t enc_file_size, uint8 *file_out = NULL);

#endif