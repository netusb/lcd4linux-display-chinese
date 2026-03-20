/* Compatibility header - jpeg_mem_dest is provided by libjpeg-turbo */
#ifndef JPEG_MEM_DEST_H
#define JPEG_MEM_DEST_H

/* Include jpeglib to get jpeg_mem_dest from libjpeg-turbo */
#include <jpeglib.h>

/* Make sure we don't redefine if already available */
#ifndef JERR_OUT_OF_MEMORY
#define JERR_OUT_OF_MEMORY  JMSG_LENGTH_MAX
#endif

#endif /* JPEG_MEM_DEST_H */
