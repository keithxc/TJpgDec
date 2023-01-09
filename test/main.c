#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include "src/tjpgd.h"
#include "src/tjpgdcnf.h"

#define OUT_W   640
#define OUT_H   360
#define CLOLR_DEPTH 4 // BGRA8888
unsigned char output_buf[OUT_W * OUT_H * CLOLR_DEPTH] = {0};

// jpeg

/* 用户定义设备标识 */
typedef struct {
    void *fp;      /* 用于输入函数的文件指针 */
    unsigned char *fbuf;    /* 用于输出函数的帧缓冲区的指针 */
    unsigned int wfbuf;    /* 帧缓冲区的图像宽度[像素] */
} IODEV;


/*------------------------------*/
/*      用户定义input funciton  */
/*------------------------------*/
static int input_type = 0;// 0:FILE* 1:buffer address
unsigned char inputbuf[640 * 480] = {0};
static unsigned int inputbuf_offset = 0;
static unsigned int input_buf_len = 0;

size_t in_func(JDEC* jd, uint8_t* buff, size_t nbyte)
{
    IODEV *dev = (IODEV*)jd->device;   /* Device identifier for the session (5th argument of jd_prepare function) */
    if (buff) {
        /* 从输入流读取 nbyte 个字节 */
//		printf(">>> [ %s ] line = %d nbyte = %d\n", __FUNCTION__, __LINE__, nbyte);
		if (input_type == 0)
        	return (unsigned int)fread(buff, 1, nbyte, (FILE *)dev->fp);
		else if (input_type == 1)
		{
			if (input_buf_len - inputbuf_offset >= nbyte)
			{
				memcpy(buff, (unsigned char*)(dev->fp+inputbuf_offset), nbyte);
				inputbuf_offset += nbyte;
//				printf(">>> [ %s ] line = %d nbyte = %d\n", __FUNCTION__, __LINE__, nbyte);
				return nbyte;
			}
			else if(input_buf_len - inputbuf_offset < nbyte && inputbuf_offset != input_buf_len)
			{
				memcpy(buff, (unsigned char*)dev->fp, input_buf_len - inputbuf_offset);
				size_t ret = input_buf_len - inputbuf_offset;
				inputbuf_offset = input_buf_len;
//				printf(">>> [ %s ] line = %d nbyte = %d\n", __FUNCTION__, __LINE__, input_buf_len - inputbuf_offset);
				return ret;
			}
		}
    } else {
        /* 从输入流移除一字节 */
		if (input_type == 0)
	        return fseek(dev->fp, nbyte, SEEK_CUR) ? 0 : nbyte;
		else if (input_type == 1)
		{
			printf(">>> [ %s ] line = %d nbyte = %d\n", __FUNCTION__, __LINE__, nbyte);
			inputbuf_offset += nbyte;
			if (inputbuf_offset < input_buf_len)
				return nbyte;
			else if (inputbuf_offset >= input_buf_len)
				return 0;
		}
    }
}


/*------------------------------*/
/*      用户定义output funciton */
/*------------------------------*/
int out_func (JDEC* jd, void* bitmap, JRECT* rect)
{
    IODEV *dev = (IODEV*)jd->device;
    unsigned char *src, *dst;
    unsigned int y, bws, bwd;


    /* 输出进度 */
    if (rect->left == 0) {
        ;//printf("\r%lu%%", (rect->top << jd->scale) * 100UL / jd->height);
    }
	// printf(">>> [ %s ] line = %d \n", __FUNCTION__, __LINE__);
	/* 拷贝解码的RGB矩形范围到帧缓冲区(BGRA8888) */
    src = (unsigned char*)bitmap;
    dst = dev->fbuf + CLOLR_DEPTH * (rect->top * dev->wfbuf + rect->left);
    bws = 3 * (rect->right - rect->left + 1);
    bwd = CLOLR_DEPTH * dev->wfbuf;
	unsigned char * s, * d;
    for (y = rect->top; y <= rect->bottom; y++) {
//        memcpy(dst, src, bws);   /* 拷贝一行 */
		s=src;
		d=dst;
		for(s; s<src+bws; s+=3,d+=4) {
			*(d) = *(s+2);
			*(d+1) = *(s+1);
			*(d+2) = *(s);
			*(d+3) = 0XFF;
		}
        src += bws; dst += bwd;  /* 定位下一行 */
    }

    return 1;    /* 继续解码 */
}
int my_dec_jpeg (void * path_or_buf)
{
	static bool _init_flag = false;
	printf(">>> [ %s ] line = %d path = %s\n", __FUNCTION__, __LINE__, path_or_buf);
    static void *work;       /* 指向解码工作区域 */
    static JDEC jdec;        /* 解码对象 */
    static JRESULT res;      /* TJpgDec API的返回值 */
    static IODEV devid;      /* 用户定义设备标识 */

	if (input_type == 0) {
	    /* 打开一个JPEG文件 */
	    if (path_or_buf == NULL) return -1;
	    devid.fp = fopen(path_or_buf, "rb");
	    if (!devid.fp) return -1;
	} else if (input_type == 1) {
		devid.fp = path_or_buf;
	}
	
	if (_init_flag == false) {
	    /* 分配一个用于TJpgDec的工作区域 */
	    work = malloc(3092);
		_init_flag = true;
	}
	memset(work, 0x00, 3092);
    /* 准备解码 */
	
    res = jd_prepare(&jdec, in_func, work, 3092, &devid);
    if (res == JDR_OK) {
       	printf("Image dimensions: %u by %u. %u bytes used.\n", jdec.width, jdec.height, 3092 - jdec.sz_pool);
		devid.fbuf = output_buf;
        devid.wfbuf = jdec.width;
		printf(">>> [ %s ] line = %d \n", __FUNCTION__, __LINE__);
        res = jd_decomp(&jdec, out_func,0);   /* 开始1/1缩放解码 */
        if (res == JDR_OK) {
           printf("OK-------------------\n");
        } else {
           printf("Failed to decompress: rc=%d\n", res);
        }
    } else {
        printf("Failed to prepare: rc=%d\n", res);
    }

    free(work);             /* 释放工作区域 */
	work = NULL;

	if (input_type == 0)
    	fclose(devid.fp);       /* 关闭JPEG文件 */

    return res;
}



int main(int argc, char * argv[])
{
    printf("It's a jpeg dec demo.\n");
	input_type = 0;
	my_dec_jpeg("./a.jpg");
	printf(">>> [ %s ] line = %d \n", __FUNCTION__, __LINE__);
    return 0;
}