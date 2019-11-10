#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include "arm_neon.h"

int32_t neon_intrinsics_matrixMul_float4x4(const float* matrix_left, float* matrix_right, float* matrix_result)
{
    if(NULL == matrix_left)
    {
        return -1;
    }   
    if(NULL == matrix_right)
    {
        return -1;
    }    
    if(NULL == matrix_result)
    {
        return -1;
    } 

    int offset = 4;
    int result_addr_offset = 0;
    int i = 0;
    int j = 0;
    float32x4_t matrix_right_row[4];
    float32x4_t matrix_left_row;
    float32x4_t result;

    for(i = 0; i < 4; i++)
    {
        matrix_right_row[i] = vld1q_f32(matrix_right + offset * i);
    }    

    for(j = 0; j < 4; j++)
    {
        matrix_left_row = vld1q_f32(matrix_left + j * offset);
        result = vdupq_n_f32(0);
        for(i = 0; i < 4; i++)
        {
            switch(i)
            {
                case 0:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_low_f32(matrix_left_row), 0);
                    break;
                case 1:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_low_f32(matrix_left_row), 1);
                    break;
                case 2:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_high_f32(matrix_left_row), 0);
                    break;
                case 3:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_high_f32(matrix_left_row), 1);
                    break;
            }
        }

        vst1q_f32(matrix_result, result);
        matrix_result = matrix_result + offset;
    }  
    return 0;
}

int32_t neon_intrinsics_matrixMul_float3x3(const float* matrix_left, float* matrix_right, float* matrix_result)
{
    if(NULL == matrix_left)
    {
        return -1;
    }   
    if(NULL == matrix_right)
    {
        return -1;
    }    
    if(NULL == matrix_result)
    {
        return -1;
    } 

    int offset = 3;
    int left_addr_offset = 0;
    int result_addr_offset = 0;
    int i = 0;
    int j = 0;
    float right_tmp[4] = {0};
    float left_tmp[4] = {0};
    float result_tmp[4] = {0};
    float32x4_t matrix_right_row[4];
    float32x4_t matrix_left_row;
    float32x4_t result;

    for(i = 0; i < 3; i++)
    {
        bzero(right_tmp, sizeof(float) * 4);
        memcpy(right_tmp, matrix_right + i * offset, sizeof(float) * offset);
        matrix_right_row[i] = vld1q_f32(right_tmp);
    }    

    for(j = 0; j < 3; j++)
    {
        bzero(left_tmp, sizeof(float) * 4);
        memcpy(left_tmp, matrix_left + j * offset, sizeof(float) * offset);
        matrix_left_row = vld1q_f32(left_tmp);
        result = vdupq_n_f32(0);
        for(i = 0; i < 4; i++)
        {
            switch(i)
            {
                case 0:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_low_f32(matrix_left_row), 0);
                    break;
                case 1:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_low_f32(matrix_left_row), 1);
                    break;
                case 2:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_high_f32(matrix_left_row), 0);
                    break;
                case 3:
                    result = vmlaq_lane_f32(result, matrix_right_row[i], vget_high_f32(matrix_left_row), 1);
                    break;
            }
        }
        bzero(result_tmp, sizeof(float) * 4);
        vst1q_f32(result_tmp, result);
        memcpy(matrix_result, result_tmp, sizeof(float) * offset);
        matrix_result = matrix_result + offset;
    }  
    return 0;
}

int neon_intrinsics_rgb888Tobgr888(uint8_t* image_src, uint8_t* image_dst, uint32_t pixel_num)
{

    uint8_t *src = image_src;
    uint8_t *dst = image_dst;
    int count = pixel_num;
    uint8_t bit_mask = 0xff;
    uint8x16x3_t vsrc;
    uint8x16x3_t vdst;
    uint8x16_t add_tmp;
    uint8x16_t tmp;
    uint8x16_t mask;

    
    /* 注意: 仅支持像素个数8对齐的图像 */
    mask = vdupq_n_u8(bit_mask);
    add_tmp = vdupq_n_u8(0);
    while (count >= 8) 
    {
        vsrc = vld3q_u8(src);//装载源数据
        tmp = vdupq_n_u8(0);

        //vswp无内建函数 使用其他方法实现
        tmp = vaddq_u8(vsrc.val[0], add_tmp);

        vsrc.val[0] = vbicq_u8(vsrc.val[0], mask);
        vsrc.val[0] = vaddq_u8(vsrc.val[2], add_tmp);

        vsrc.val[2] = vbicq_u8(vsrc.val[2], mask);
        vsrc.val[2] = vaddq_u8(tmp, add_tmp);

        // /* 循环 */
        vst3q_u8(dst, vsrc);
        dst += 8*3;
        src += 8*3;
        count -= 8;
    }

    return 0;
}

int neon_intrinsics_rgb565Torgb888(uint16_t* image_src, uint8_t* image_dst, uint32_t pixel_num)
{
    uint16_t *src = image_src;
    uint8_t *dst = image_dst;
    int count = pixel_num;
    uint16x8_t vsrc;
    uint8x8x3_t vdst;

    /* 注意: 仅支持像素个数8对齐的图像 */
    while (count >= 8) 
    {
        vsrc = vld1q_u16(src);//装载源数据
        /*  
            注意: rgb565转rgb88因为通道的bit位宽不同，所以需要使用低位进行补偿, 具体请自行查阅
            1. 使用vreinterpretq_u8_u16将源向量中的16bit元素转为8bit元素
            2. 使用vshrq_n将8bit元素作移 5 位, 这样即可将位于高位的红色数据从5bit转为8bi
            3. 将vreinterpretq_u16_u8 将8bit数据转换为16bit, 此时高8bit为红色通道数据
            4. 然后在使用vshrn_n_u16向右移动8个bit, 此时低8bit为红色数据, 并因为窄型数据操作的原因, 所以可以将16bit数据移动并转换为8bit
        */
        vdst.val[0] = vshrn_n_u16(vreinterpretq_u16_u8(vshrq_n_u8(vreinterpretq_u8_u16(vsrc), 3)), 5);
        /* 使用支持窄型数据操作的右移指令vshrn_n_u16, 将绿色通道的数据移动的低8位, 此时需要清空非绿色通道的数据并且进行补偿, 所以需要使用vshl_n_u8向左移动2bit, 移动时因为是窄型操作, 所以数据从16bit变为8bit, 从而提取出绿色通道数据 */
        vdst.val[1] = vshl_n_u8(vshrn_n_u16(vsrc, 5), 2);
        /* 先使用vshlq_n_u16将数据向左移动2bit, 然后使用窄型移动指令, 所以数据从16bit变为8bit, 从而提取出蓝色通道数据 */
        vdst.val[2] = vmovn_u16(vshlq_n_u16(vsrc, 3));
        /* 循环 */
        vst3_u8(dst, vdst);
        dst+= 8*3;
        src += 8;
        count -= 8;
    }
}

#define PIC_W	512
#define PIC_H	256
int neon_color_convert(void)
{
        int i = 0;
    /* 创建红色图像 */
    uint16_t* rgb565_data = (uint16_t*)malloc(PIC_W*PIC_H*2);
    for(i = 0; i < PIC_W*PIC_H; i++)
    {
        rgb565_data[i] = 0xf800;
    }

    /* 转换图像 */
    uint8_t* rgb888_data = (uint8_t*)malloc(PIC_W * PIC_H * 3);
    neon_intrinsics_rgb565Torgb888(rgb565_data, rgb888_data, PIC_W*PIC_H);

    uint8_t* bgr888_data = (uint8_t*)malloc(PIC_W * PIC_H * 3);
    neon_intrinsics_rgb888Tobgr888(rgb888_data, bgr888_data, PIC_W*PIC_H);

    for(i = 0; i < 3; i++)
    {
        printf("rgb888_data[%d] = %#x\n", i, rgb888_data[i]);
        
    }
    for(i = 0; i < 3; i++)
    {
        printf("bgr888_data[%d] = %#x\n", i, bgr888_data[i]);
    }

    FILE* pic = NULL;
    int ret = 0;
    pic = fopen("/mnt/test_bgr888", "w+");
    if(NULL == pic)
    {
        printf("open file error\n");
        goto ERROR_EXIT;
    }

    ret = fwrite(bgr888_data, 1, PIC_W*PIC_H*3, pic);
    if(PIC_W*PIC_H*3 != ret)
    {
        printf("write file error\n");
        goto ERROR_EXIT;
    }
    free(rgb888_data);
    free(bgr888_data);
    return 0;

ERROR_EXIT:
    free(rgb888_data);
    free(bgr888_data);
    return -1;
}

int matrix_mul_4x4(void)
{    float left[16] = {0};
    float right[16] = {0};
    float result[16] = {0};
    int i = 0;
    for(i = 0; i < 16; i++)
    {
        left[i] = i * 2.0;
        printf("left[%d] = %f\n", i, left[i]);
    }
    for(i = 0; i < 16; i++)
    {
        right[i] = i * 1.0;
        printf("right[%d] = %f\n", i, right[i]);
    }    
    neon_intrinsics_matrixMul_float4x4(left, right, result);
    for(i = 0; i < 16; i++)
    {
        printf("result[%d] = %f\n", i, result[i]);
    } 
    return 0;
}

int matrix_mul_3x3(void)
{    
    float left[9] = {0};
    float right[9] = {0};
    float result[9] = {0};
    int i = 0;
    for(i = 0; i < 9; i++)
    {
        left[i] = i * 2.0;
        printf("left[%d] = %f\n", i, left[i]);
    }
    for(i = 0; i < 9; i++)
    {
        right[i] = i * 1.0;
        printf("right[%d] = %f\n", i, right[i]);
    }    
    neon_intrinsics_matrixMul_float3x3(left, right, result);
    for(i = 0; i < 9; i++)
    {
        printf("result[%d] = %f\n", i, result[i]);
    } 
    return 0;
}

int main(void)
{
    matrix_mul_3x3();  
}