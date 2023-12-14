#pragma once

typedef unsigned char BYTE;

#define DETAIL_PRINT

#define CHECK_RETURN_0(value) if(value==NULL){return 0;}
#define CHECK_MSG_RETURN_0(value,msg) if(value==NULL){puts(msg); return 0;}
#define FREE_CLOSE_FILE_RETURN_0(fp, value) {free(value); fclose(fp); return 0;}
#define CLOSE_RETURN_0(fp) {fclose(fp); return 0;}


