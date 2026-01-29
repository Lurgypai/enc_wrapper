#pragma once
#include "enc_grain.h"

// open
// close
// add object
// read object
// write object

// open file
void enc_store_contig_open();
// close file
void enc_store_contig_close();

// add object to file
void enc_store_contig_add_object(char* tag);

// get object (grain) meta from file
enc_grain enc_store_contig_get_grain(char* tag);
// set object (grain) meta
void enc_store_contig_set_grain(char* tag, enc_grain);

// read object
void enc_store_contig_read_object(char* tag, void* dest);
// write obejct
void enc_store_contig_write_object(char* tag, void* data);

// replicate vol connector
