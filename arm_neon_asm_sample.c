#include <arm_neon.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
/*
1、指令分类
    通用指令集(Normal instructions)：普通指令可以对任何向量类型进行操作，并产生与操作数向量相同大小且通常与类型相同的结果向量。即向量与结果长度相同
    长整数指令集(Long instructions)：长指令对 双字向量操作数 进行运算并产生 四字向量 结果。结果元素通常是操作数宽度的两倍，并且类型相同。即输入向量的长度为输出向量的一半（长指令使用附加在指令后的L来指定）。
    宽整数指令集(Wide instructions)：宽指令对 双字向量操作数 和 四字向量操作数 进行操作，产生 四字向量结果 。结果元素和第一个操作数(四字向量操作数)的宽度是第二个操作数元素(双字向量操作数)的宽度的两倍。 即双字向量和四字向量计算后输出四字向量。宽指令在指令后附加了W。
    窄整数指令集(Narrow instructions):窄指令对四字向量操作数进行运算，并产生双字向量结果。结果元素通常是操作数元素宽度的一半。即四字向量和四字向量计算后输出双字向量。窄指令通过在指令后附加N来指定。
2、指令格式
3、neon寄存器组
    neon指令集有专用的寄存器组，第一种是Q0-Q15一共16个，每个Q寄存器为128bit，第二种是D0-D31，一共32个，每个D寄存器为64bit，第三种是S0-S31，一共32个，每个S寄存器为32bit
    neon寄存器有对照关系，可以理解为他们共同使用一块存储数据的区域，但该区域被分别映射为D寄存器，Q寄存器和3寄存器，下面的图是寄存器之间的对应关系
    NEON 在查看寄存器时将每个寄存器视为均包含一个向量， 而该向量又包含 1、2、 4、 8 或 16 个大小和类型均相同的元素。 也可以将各元素当作标量 加以访问。
    即每个neon寄存器都将可以看成是一个向量，比如D0中有8个元素，每个元素是8bit，那么D0寄存器看成是有8个元素的向量
4、编译
    在编译含有neon指令集的代码时，是指定使用的fpu和cpu，通常情况下我们使用arm-gcc需要加-ftree-vectorize -mfpu=neon -mcpu=your_chip_arch 来使能编译器使用neon指令集
5、neon指令释放方法
    打开编译器的向量化选项
    使用neon库，如Ne10
    使用neon指令的内嵌汇编
    使用neon intrinsics，该库提供了一系列的操作接口

NEON简介及基本架构：http://zyddora.github.io/2016/02/28/neon_1/
ARM平台NEON指令的编译和优化：https://blog.csdn.net/heli200482128/article/details/79303286
neon intrinsics速查地址：https://developer.arm.com/architectures/instruction-sets/simd-isas/neon/intrinsics


编译器自动进行向量优化的条件：
• 短而简单的循环
• 不使用break跳出循环.
• 循环次数为2的幂次.
• 循环次数为编译器支持的范围.
• 循环内部调试函数时，该函数需要是内联属性
• 使用数组索引而不是指针.
• 间接寻址无法向量化.
• 使用strict关键字告诉编译器指针不引用内存的重叠区域。


用这个例子集合图画来说明neon指令集的格式
V{<mod>}<op>{<shape>}{<cond>}{.<dt>}{<dest>}, src1, src2
<mod>:即模式，neon指令集有几种计算模式：饱和计算、
    Q: 饱和计算，各个数据类型的饱和范围请查看表.
    H: 该指令将结果减半。 它实际上通过向右移一个位来完成此操作，例如VHADD，VHSUB。
    D: 使用指令使结果加倍
    R: 对截断的指令结果进行舍入，比如某一个指令的操作数为整形，但指令结果带有小数，则需要对小数进行舍入
<op> - 执行的操作，比如加法ADD, 减法SUB, 乘法MUL).
<shape> - Shape，用于指定长指令(L), 宽指令(W), 窄指令(N)
<cond> - 条件码, neon条件码与普通的arm汇编条件码使用的是同一套条件码，但其含义不同，详情查看表格。
<.dt> - 操作的数据类型，比如U8，U16等
<dest> - 目的寄存器.
<src1> - 源寄存器1.
<src2> - 源寄存器2.



*/

int add_int_c(int* dst, int* src1, int* src2, int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        dst[i] = src1[i] + src2[i];
    }
}
 
int add_float_neon1(int* dst, int* src1, int* src2, int count)
{
    if(count % 4 != 0)
    {
        printf("can not calculate\n");        
        return -1;
    }
    int i;
    for (i = 0; i < count; i += 4)
    {
        int32x4_t in1, in2, out;
        in1 = vld1q_s32(src1);
        src1 += 4;
        in2 = vld1q_s32(src2);
        src2 += 4;
        out = vaddq_s32(in1, in2);
        vst1q_s32(dst, out);
        dst += 4;
    }
}
void add_float_neon2(int* dst, int* src1, int* src2, int count)
{
    
    __asm__ __volatile__
    (
        "1:"
        "vld1.u32 {q0}, [%1]!;"
        "vld1.u32 {q1}, [%2]!;"
        "vadd.u32 q2, q0, q1;"//这里需要修改为 vadd.u32
        "subs %3, %3, #4;"
        "vst1.u32 {q2}, [%0]!;"
        "bgt 1b;"
        :"+r"(dst)
        :"r"(src1), "r"(src2), "r"(count)
        :"memory", "q0", "q1", "q2"
    );
    
}

#define LOOP_NUM 10000
int neon_test_sample(void)
{    clock_t start = 0;
    clock_t end = 0;
    int i = 0;
    double total_time = 0;
    int src1[1000*4] = {99};
    int src2[1000*4] = {100};
    int dst[1000*4] = {0};
    for(i = 0; i < 4*1000; i++)
    {
        src1[i] = 99;
        src2[i] = 100;
        dst[i] = 0;
    }


    for(i = 0; i < 4*1000; i++)
    {
        dst[i] = 0;
    }
    i = LOOP_NUM;
    start = clock();
    while(i--)
        add_int_c(dst, src1, src2, 1000*4);
    end = clock();
    total_time = (double)(end - start) / CLOCKS_PER_SEC;
    for(i = 0; i < 4*1000; i++)
    {
        if(199 != dst[i])
        {
            printf("calculate error, i = %d\n", i);
            return -1;
        }
    }
    printf("use normal C code, total time = %f\n", total_time);


    for(i = 0; i < 4*1000; i++)
    {
        dst[i] = 0;
    }
    i = LOOP_NUM;
    start = clock();
    while(i--)
        add_float_neon1(dst, src1, src2, 1000*4);
    end = clock();
    total_time = (double)(end - start) / CLOCKS_PER_SEC;
    for(i = 0; i < 4*1000; i++)
    {
        if(199 != dst[i])
        {
            printf("calculate error, i = %d\n", i);
            return -1;
        }
    }
    printf("use neon intrinsics, total time = %f\n", total_time);


    for(i = 0; i < 4*1000; i++)
    {
        dst[i] = 0;
    }
    i = LOOP_NUM;
    start = clock();
    while(i--)
        add_float_neon2(dst, src1, src2, 1000*4);
    end = clock();
    total_time = (double)(end - start) / CLOCKS_PER_SEC;
    for(i = 0; i < 4*1000; i++)
    {
        if(199 != dst[i])
        {
            printf("calculate error, i = %d, dst[%d] = %d\n", i, i, dst[i]);
            return -1;
        }
    }
    printf("use neon asm, total time = %f\n", total_time);
}

void neon_vmov(int var_r0, int var_r1)//between arm register and neon register
{
    /* 
        neon在处理过程中可以使用标量，但只有指定的寄存器可以存放标量
        指令语法通过在双字向量中使用索引来引用标量，从而使 Dm[x] 表示 Dm 中的第 x 个元素。
        16 位标量限定为寄存器 D0-D7，其中 x 位于范围 0-3 内
        32 位标量限定为寄存器 D0-D15，其中 x 为 0 或 1。
    */

   int top_32 = 0;
   int bottom_32 = 0;
   int vector_32 = 0x12345678;
   char scalar_from_vector = 0;

   __asm__ 
   (
       "vmov D0, r0, r1;"
       "vmov %[top_32], %[bottom_32], D0;"
       "mov r8, #0;"
       "vmov D1, %[vector_32], r8;"
       "vmov.u8 %[scalar_from_vector], D1[0];"
       :[top_32]"=r"(top_32), [bottom_32]"=r"(bottom_32), [scalar_from_vector]"=r"(scalar_from_vector)
       :[vector_32]"r"(vector_32)
       :"memory", "r0", "r1", "r8"
   );
   printf("top_32 = %d, bottom_32 = %d, scalar = %#x\n", top_32, bottom_32, scalar_from_vector);
   
}

void neon_vbit(void)//当src_0中的bit为1时，将src_1对应的bit送到dst中的bit
{
    int src_0 = 0x11111111;
    int src_1 = 0x12345678;
    int dst   = 0;
    __asm__
    (
        "vmov.32 D0[0], %[src_0];"
        "vmov.32 D1[0], %[src_1];"
        "vbit.u32 D2, D1, D0;"
        "vmov.u32 %[dst], D2[0];"
        :[dst]"=r"(dst)
        :[src_0]"r"(src_0), [src_1]"r"(src_1)
        :"memory"
    );
    printf("dst = %#x\n", dst);
}

void neon_vacge(void)//这里带了a，指的是abs绝对值，所以f32可以是负数。比较src_0和src_1的大小，并设置dst为0xffffffff或0
{
    int src_0 = 0x11112222;
    int src_1 = 0x22221111;
    int dst   = 0;
    __asm__
    (
        "vmov.32 D0[0], %[src_0];"
        "vmov.32 D1[0], %[src_1];"
        "vacgt.f32 D2, D1, D0;"//注意这里为f32，即表明一次比较的是3将2位作为一个元素
        "vmov.i32 %[dst], D2[0];"
        :[dst]"=r"(dst)
        :[src_0]"r"(src_0), [src_1]"r"(src_1)
        :"memory"
    );
    printf("dst = %#x\n", dst);
}

void neon_vcompare(void)
{
    int src_0 = 0x11112222;
    int src_1 = 0x22221111;
    int dst   = 0;
    __asm__
    (
        "vmov.32 D0[0], %[src_0];"
        "vmov.32 D1[0], %[src_1];"
        "vcle.u8 D2, D1, D0;"//注意这里为f32，即表明一次比较的是3将2位作为一个元素,cvle可以替换为VCEQ、 VCGE、 VCGT、 VCLE等
        "vmov.u32 %[dst], D2[0];"
        :[dst]"=r"(dst)
        :[src_0]"r"(src_0), [src_1]"r"(src_1)
        :"memory"
    );
    printf("dst = %#x\n", dst);
}

void neon_vcvt(void)
{
    int src_0 = 20;
    float dst   = 0;
    __asm__
    (
        "vmov.u32 D0[0], %[src_0];"
        "vcvt.f32.u32 d1, d0;"//注意，后面是寄存器是源，前面的寄存器是目的
        "vmov.f32 %[dst], D1[0];"
        :[dst]"=r"(dst)
        :[src_0]"r"(src_0)
        :"memory"
    );
    printf("dst = %#f\n", dst);
}

void neon_vdup(void)
{
    int src_0 = 0x12345678;
    int src_1 = 0x87654321;
    int dst_0 = 0;
    int dst_1 = 0;
    __asm__
    (
        "vmov.u32 D0, %[src_0], %[src_1];"
        "vdup.u32 d1, d0[0];"//注意，该指令进复制标量，标量可以是neon寄存器中的一个值或者一个rn
        "vdup.u32 d2, d0[1];"
        "vmov.u32 %[dst_0], D1[0];"
        "vmov.u32 %[dst_1], D2[0];"
        :[dst_0]"=r"(dst_0), [dst_1]"=r"(dst_1)
        :[src_0]"r"(src_0), [src_1]"r"(src_1)
        :"memory"
    );
    printf("dst_0 = %#x, dst_1 = %#x\n", dst_0, dst_1);
}

void neon_vmvn(void)//求取反移动指令
{
    int dst = 0;
    __asm__
    (
        "vmvn.u32 D0, #0x81818181;"
        "vmov.u32 %[dst], D0[0];"
        :[dst]"=r"(dst)
        :
        :"memory"
    );
    printf("dst = %#x\n", dst);  //0x7e7e7e7e 
}

void neon_vrev(void)//反转指令
{
    int src = 0x12345678;
    int dst = 0;
    __asm__
    (
        "vmov.u32 D0[0], %[src];"
        "vrev32.u8 d1, d0;"
        "vmov.u32 %[dst], D1[0];"
        :[dst]"=r"(dst)
        :[src]"r"(src)
        :"memory"
    );
    printf("dst = %#x\n", dst);  //0x78563412 
}

void neon_vswp(void)//交换指令
{
    int src[4] = {1, 2, 3, 4};
    int dst[4] = {4, 3, 2, 1};
    int* src_p = src;
    int* dst_p = dst;
    int i = 0;
    for(i = 0; i < 4; i++)
    {
        printf("src[%d] = %#d\n", i, src[i]);
    }
    for(i = 0; i < 4; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }
    __asm__
    (
        "vld1.u32 {q0}, [%2]!;"
        "vld1.u32 {q1}, [%3]!;"
        "vswp.u32 q1, q0;"
        "vst1.u32 {q0}, [%0]!;"
        "vst1.u32 {q1}, [%1]!;"
        :"+r"(src_p), "+r"(dst_p)
        :"r"(src), "r"(dst)
        :"memory"
    );
    for(i = 0; i < 4; i++)
    {
        printf("src[%d] = %#d\n", i, src[i]);
    }
    for(i = 0; i < 4; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }
}

void neon_vtbl(void)//索引查表
{
    unsigned char src[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    unsigned char index[8] = {0, 0, 1, 1, 2, 2, 3, 4};
    unsigned char dst[8] = {0}; 
    unsigned char* p_src = src;
    unsigned char* p_dst = dst;
    unsigned char* p_index = index;
    __asm__
    (
        "vld1.u8 {d0}, [%1]!;"//src
        "vld1.u8 {d11}, [%2]!;"//index
        "vtbl.8 d10, {d0}, d11;"
        "vst1.u8 {d10}, [%0]!;"
        :"+r"(p_dst)//注意这里需要使用 +r 而不是 =r
        :"r"(p_src), "r"(p_index)
        :"memory"
    );
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }           
}

void neon_vtrn(void)//转置向量, 将 2 个寄存器看成 2*2 的矩阵
{
    unsigned char src[8] = {1, 2, 4, 8, 16, 32, 64, 128};
    unsigned char dst[8] = {128, 64, 32, 16, 8, 4, 2, 1}; 
    unsigned char* p_src = src;
    unsigned char* p_dst = dst;
    __asm__
    (
        "vld1.u8 {d0}, [%2]!;"//src
        "vld1.u8 {d2}, [%3]!;"//src
        "vtrn.8 d0, d2;"
        "vst1.u8 {d0}, [%0]!;"
        "vst1.u8 {d2}, [%1]!;"
        :"+r"(p_src), "+r"(p_dst)
        :"r"(src), "r"(dst)
        :"memory"
    );
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }  
    for(i = 0; i < 8; i++)
    {
        printf("src[%d] = %#d\n", i, src[i]);
    }          
}

void neon_vuzp(void)//交叉存取建议直接使用vld和vst命令完成
{
    unsigned char src[24] = {0};
    unsigned char dst_1[8] = {0}; 
    unsigned char dst_2[8] = {0}; 
    unsigned char dst_3[8] = {0}; 
    unsigned char* p_dst_1 = dst_1; 
    unsigned char* p_dst_2 = dst_2; 
    unsigned char* p_dst_3 = dst_3; 
    int i = 0;
    for(i = 0; i < 24; i++)
    {
        src[i] = (i / 3) + 1;
        printf("src[%d] = %#d\n", i, src[i]);
    }
    __asm__
    (
        "vld3.u8 {d0, d1, d2}, [%3]!;"//r4记得加[], 以表示这里地址递增，且需要将 dn寄存器填满 才不会出错
        "vst1.u8 {d0}, [%0]!;"
        "vst1.u8 {d1}, [%1]!;"
        "vst1.u8 {d2}, [%2]!;"
        :"+r"(p_dst_1), "+r"(p_dst_2), "+r"(p_dst_3)
        :"r"(src)
        :"memory"
    );
    
    for(i = 0; i < 8; i++)
    {
        printf("dst_1[%d] = %#d\n", i, dst_1[i]);
    }  
    for(i = 0; i < 8; i++)
    {
        printf("dst_2[%d] = %#d\n", i, dst_2[i]);
    }  
    for(i = 0; i < 8; i++)
    {
        printf("dst_3[%d] = %#d\n", i, dst_3[i]);
    }            
}

void neon_vzip(void)
{
    unsigned char src[24] = {0};
    unsigned char dst[24] = {0};
    unsigned char* p_dst = dst; 
    int i = 0;
    for(i = 0; i < 24; i++)
    {
        src[i] = (i % 8) + 1;
        printf("src[%d] = %#d\n", i, src[i]);
    }  
    __asm__
    (
        "vld1.u8 {d0, d1, d2}, [%1]!;"//r4记得加[], 以表示这里地址递增，且需要将 dn寄存器填满 才不会出错
        "vst3.u8 {d0, d1, d2}, [%0]!;"
        :"+r"(p_dst)
        :"r"(src)
        :"memory"
    );   
    for(i = 0; i < 24; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }             
}
void neon_vshl(void)//VQSHL、 VQSHLU 和 VSHLL同理，只是各自的特性不同
{
    unsigned char src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    unsigned char dst[8] = {0};
    unsigned char* p_dst = dst;
    
    __asm__
    (
        "vld1.u8 {d0}, [%1]!;"
        "vshl.u8 d1, d0, #1;"
        "vst1.u8 d1, [%0]!;"
        :"+r"(p_dst)
        :"r"(src)
        :"memory"        
    );
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }        
}
void neon_vqrshl(void)
{
    unsigned char src[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    unsigned char offset[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    unsigned char dst[8] = {0};
    unsigned char* p_src = src;
    unsigned char* p_dst = dst;
    unsigned char* p_offset = offset;
    asm
    (
        "vld1.u8 {d0}, [%1]!;"
        "vld1.u8 {d1}, [%2]!;"
        "vqrshl.u8 d2, d0, d1;"//这里的q是用于饱和操作，使得溢出时不会被归零
        "vst1.u8 {d2}, [%0]!;"
        :"+r"(p_dst)
        :"r"(src), "r"(offset)
        :"memory"
    );
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    } 
}

void neon_vrshrn(void)//第一个r表示舍入计算，n表示可以进行窄型运算，vrsra同理，只是能将结果累加到目标寄存器，vqrshrun增加支持饱和 和 无符号。右移仅支持立即数位移
{
    unsigned short src[8] = {0};
    unsigned short offset[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    unsigned char dst[8] = {0};
    unsigned short* p_src = src;
    unsigned short* p_offset = offset;
    unsigned char* p_dst = dst;
   
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src[i] = 3;//将输入初始化为3
    }

    asm
    (
        "vld1.u16 {q0}, [%1]!;"
        /*
            舍入的意思应该是如果右移后将小数点后向上取整，如果不带 r 则向下取整
            例如这里将 3 右移一位，则结果为1.5，如果带r就向上取整变为2 ，不带则向下取整变为1
        */
        "vshrn.u16 d2, q0, #1;"//这里的q是用于饱和操作，使得溢出时不会被归零
        "vst1.u8 {d2}, [%0]!;"
        :"+r"(p_dst)
        :"r"(src), "r"(offset)
        :"memory"
    );
    
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    } 
}

void neon_vaba(void)//支持 L 型操作，将双字数据装载到四字寄存器上, vabd同理，只是不支持累加
{
    unsigned char src_1[8] = {0};
    unsigned char src_2[8] = {0};
    unsigned char tmp[8] = {0};
    unsigned char dst[8] = {0};
    unsigned char dif[8] = {0};
    unsigned char* p_src_1 = src_1;
    unsigned char* p_src_2 = src_2;
    unsigned char* p_tmp = tmp;  
    unsigned char* p_dst = dst;  
    unsigned char* p_dif = dif;  
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src_1[i] = i + 1;
    } 
    for(i = 0; i < 8; i++)
    {
        src_2[i] = (i + 1) * 2;
    }  
    for(i = 0; i < 8; i++)
    {
        tmp[i] = (i + 1 ) * 2;
        printf("tmp[%d] = %#d\n", i, tmp[i]);
    }     
    asm
    (
        "vld1.u8 {d0}, [%3]!;"
        "vld1.u8 {d1}, [%4]!;"
        "vaba.u8 d3, d0, d1;"
        "vst1.u8 {d3}, [%2]!;"
        "vld1.u8 {d2}, [%1]!;"
        "vaba.u8 d2, d0, d1;"
        "vst1.u8 {d2}, [%0]!;"
        :"+r"(p_dst), "+r"(p_tmp), "+r"(p_dif)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("dif[%d] = %#d\n", i, dif[i]);
    }      
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }    

}

void neon_vabs_vneg(void)//支持饱和操作
{
    char src[8] = {0};
    unsigned char abs_dst[8] = {0};
    unsigned char neg_dst[8] = {0};
    unsigned char* p_src = src;
    unsigned char* p_abs_dst = abs_dst;  
    unsigned char* p_neg_dst = neg_dst;  
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src[i] = -(i+1);
    }   
    asm
    (
        "vld1.u8 {d0}, [%2]!;"
        "vabs.s8 d2, d0;"
        "vneg.s8 d3, d0;"
        "vst1.u8 {d2}, [%0]!;"
        "vst1.s8 {d3}, [%1]!;"
        :"+r"(p_abs_dst), "+r"(p_neg_dst)
        :"r"(p_src)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("abs_dst[%d] = %#d\n", i, abs_dst[i]);
    }     
    for(i = 0; i < 8; i++)
    {
        printf("neg_dst[%d] = %#d\n", i, neg_dst[i]);
    }          
}

void neon_vadd(void)//支持 L 和 W 操作， 支持饱和操作, sub同理
{
    char src_1[8] = {0};
    char src_2[8] = {0};
    unsigned char add_dst[8] = {0};
    unsigned char q_add_dst[8] = {0};
    unsigned short add_dst_short[8] = {0};
    char* p_src_1 = src_1;
    char* p_src_2 = src_2;
    unsigned char* p_add_dst = add_dst;  
    unsigned char* p_q_add_dst = q_add_dst;  
    unsigned short* p_add_dst_short = add_dst_short; 
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src_1[i] = 1 << i;
        
    }     
    for(i = 0; i < 8; i++)
    {
        src_2[i] = 200;
    }     
    asm
    (
        "vld1.u8 d0, [%1]!;"
        "vld1.u8 d1, [%2]!;"
        "vadd.u8 d2, d0, d1;"
        "vst1.u8 d2, [%0]!;"
        :"+r"(p_add_dst)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("add_dst[%d] = %#d\n", i, add_dst[i]);
    }   

    asm
    (
        "vld1.u8 d0, [%1]!;"
        "vld1.u8 d1, [%2]!;"
        "vqadd.u8 d2, d0, d1;"//带饱和
        "vst1.u8 d2, [%0]!;"
        :"+r"(p_q_add_dst)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("q_add_dst[%d] = %#d\n", i, q_add_dst[i]);
    } 

    asm
    (
        "vld1.u8 d0, [%1]!;"
        "vld1.u8 d1, [%2]!;"
        "vaddl.u8 q0, d0, d1;"//长整型相加
        "vst1.u16 {q0}, [%0]!;"//使用 q 寄存器时需要加花括号, 为了适应 l长整型 类型的加法，需要将使用short型数组，从汇编看，相加后是将每一个数据从8bit变为16bit再存入q寄存器，其实就是将 q寄存器当成 2 个d寄存器来使用, 比如q0汇编为{d0, d1}
        : "+r"(p_add_dst_short)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("add_dst_short[%d] = %#d\n", i, add_dst_short[i]);
    }         
}
void neon_vaddhn(void)//h为选择高部分，n代表窄型运算, sub同理
{
    unsigned short src_1[8] = {0};
    unsigned short src_2[8] = {0};
    unsigned char dst[8] = {0};
    unsigned short* p_src_1 = src_1;
    unsigned short* p_src_2 = src_2;    
    unsigned char* p_dst = dst;  
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src_1[i] = (i + 1) << 8;
        printf("src_1[%d] = %#d\n", i,(i + 1));
        
    } 
    for(i = 0; i < 8; i++)
    {
        src_2[i] = (i * 2) << 8;
        printf("src_2[%d] = %#d\n", i,  (i * 2));
        
    }    
    asm
    (
        "vld1.u16 {q0}, [%1]!;"
        "vld1.u16 {q1}, [%2]!;"
        "vaddhn.u16 d0, q0, q1;"//高部分相加
        "vst1.u8 d0, [%0]!;"
        :"+r"(p_dst)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }    
}

void neon_vhadd(void)//向量相加后每个元素想右移动一位，可带 r 加入舍入操作， sub同理
{
    unsigned char src_1[8] = {0};
    unsigned char src_2[8] = {0};
    unsigned char dst[8] = {0};
    unsigned char* p_src_1 = src_1;
    unsigned char* p_src_2 = src_2;    
    unsigned char* p_dst = dst;  
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src_1[i] = (i + 1) ;
        src_2[i] = src_1[i];
        printf("src[%d] = %#d\n", i, src_1[i]);
    } 
    asm
    (
        "vld1.u8 d0, [%1]!;"
        "vld1.u8 d1, [%2]!;"
        "vhadd.u8 d2, d1, d0;"
        "vst1.u8 d2, [%0]!;"
        :"+r"(p_dst)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }        
}

void neon_vpaddl(void)//p 是指向量内部的相邻元素进行运算
{
    unsigned char src_1[8] = {0};
    unsigned char src_2[8] = {0};
    unsigned char dst[8] = {0};
    unsigned short dst_short[4] = {0};
    unsigned char* p_src_1 = src_1;
    unsigned char* p_src_2 = src_2;    
    unsigned char* p_dst = dst;  
    unsigned short* p_dst_short = dst_short;  
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src_1[i] = (i + 1) ;
        src_2[i] = src_1[i];
        printf("src[%d] = %#d\n", i, src_1[i]);
    } 
    asm
    (
        "vld1.u8 d0, [%2]!;"
        "vld1.u8 d1, [%3]!;"
        "vpadd.u8 d2, d1, d0;"
        "vst1.u8 d2, [%0]!;"
        :"+r"(p_dst), "+r"(p_dst_short)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }  

    asm
    (
        "vld1.u8 d0, [%2]!;"
        "vld1.u8 d1, [%3]!;"
        "vpaddl.s8 d2, d1;"//l 长整型运算仅支持单个寄存器
        "vst1.u16 d2, [%1]!;"
        :"+r"(p_dst), "+r"(p_dst_short)
        :"r"(p_src_1), "r"(p_src_2)
        :"memory"
    );

    for(i = 0; i < 4; i++)
    {
        printf("dst_short[%d] = %#d\n", i, dst_short[i]);
    }       
}

void neon_vmax(void)
{
    unsigned char src_1[8] = {0};
    unsigned char src_2[8] = {0};
    unsigned char dst[8] = {0};
    unsigned char* p_src_1 = src_1;
    unsigned char* p_src_2 = src_2;    
    unsigned char* p_dst = dst;  
    int i = 0;

    /* vmax */
    for(i = 0; i < 8; i++)
    {
        src_1[i] = (i % 2) + 1;
        printf("src_1[%d] = %#d\n", i, src_1[i]);
    }
    for(i = 0; i < 8; i++)
    {

        if(i % 2 == 0)
            src_2[i] = 2;
        else
            src_2[i] = 1;
        printf("src_2[%d] = %#d\n", i, src_2[i]);
    }    
    // asm
    // (
    //     "vld1.u8 d0, [%1]!;"
    //     "vld1.u8 d1, [%2]!;"
    //     "vmax.u8 d2, d1, d0;"
    //     "vst1.u8 d2, [%0]!;"
    //     :"+r"(p_dst)
    //     :"r"(p_src_1), "r"(p_src_2)
    // );
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }   

    /* vpmax */   
    for(i = 0; i < 8; i++)
    {
        src_1[i] = (i % 2) + 1;
        printf("src_1[%d] = %#d\n", i, src_1[i]);
    }
    for(i = 0; i < 8; i++)
    {

        if(i % 2 == 0)
            src_2[i] = 3;
        else
            src_2[i] = 2;
        printf("src_2[%d] = %#d\n", i, src_2[i]);
    }   
    asm
    (
        "vld1.u8 d3, [%1]!;"
        "vld1.u8 d4, [%2]!;"
        "vpmax.u8 d5, d4, d3;"//d4的比较结果存于d5的低位，而d3的比较结果存于d5的高位
        "vst1.u8 d5, [%0]!;"
        :"+r"(p_dst)
        :"r"(p_src_1), "r"(p_src_2)
    );  
    for(i = 0; i < 8; i++)
    {
        printf("dst[%d] = %#d\n", i, dst[i]);
    }        
}
void neon_vcxx(void)//前导计位指令,即根据最高bit判断当前元素中各个bit的统计情况, 比如以vcnt为例, 元素值为0xfe, 则最高bit为1, 后面紧跟着6个1, 那么此时元素的统计情况为6个
{
    unsigned char src_1[8] = {0};
    unsigned char src_2[8] = {0};
    unsigned char dst_1[8] = {0};
    unsigned char dst_2[8] = {0};
    unsigned char dst_3[8] = {0};
    unsigned char* p_src_1 = src_1;
    unsigned char* p_src_2 = src_2;
    unsigned char* p_dst_1 = dst_1;  
    unsigned char* p_dst_2 = dst_2;
    unsigned char* p_dst_3 = dst_3;  
    int i = 0;   
    for(i = 0; i < 8; i++)
    {
        if(0 == i)
            src_1[i] = 0xff;
        else
            src_1[i] = src_1[i - 1] & ~(1 << (i - 1));
        printf("src_1[%d] = %#x\n", i, src_1[i]);
    }    
    for(i = 0; i < 8; i++)
    {
        if(0 == i)
            src_2[i] = 0;
        else
            src_2[i] = src_2[i - 1] | (1 << (i - 1));
        
        printf("src_2[%d] = %#x\n", i, src_2[i]);
    }
    asm
    (
        "vld1.u8 d0, [%3]!;"
        "vld1.u8 d1, [%4]!;"
        "vcnt.i8 d2, d0;"
        "vclz.i8 d3, d1;"//从最高位开始对连续bit进行计数，如果bit为0, 则计数加1，计数时最高位参与计算
        "vcls.s8 d4, d0;"//cls是对从最高位开始的连续的多个bit进行计数, 如果某一个bit和最高位相同，则计数加1, 计数时不算最高位, 比如0b10110101为0, 0b11110101为3，
        "vst1.u8 d2, [%0]!;"
        "vst1.u8 d3, [%1]!;"
        "vst1.u8 d4, [%2]!;"
        :"+r"(p_dst_1), "+r"(p_dst_2), "+r"(p_dst_3)
        :"r"(p_src_1), "r"(p_src_2)
    );  
    for(i = 0; i < 8; i++)
    {
        printf("dst_1[%d] = %#d\n", i, dst_1[i]);
    } 
    for(i = 0; i < 8; i++)
    {
        printf("dst_2[%d] = %#d\n", i, dst_2[i]);
    }  
    for(i = 0; i < 8; i++)
    {
        printf("dst_3[%d] = %#d\n", i, dst_3[i]);
    }                    
}

void neon_vrecpe(void)//求倒数
{
    float src[4] = {1.0, 2.0, 4.0, 8.0};
    float dst[4] = {0};
    float* p_src = src;
    float* p_dst = dst;
    int i = 4;
    asm
    (
        "vld1.f32 {q0}, [%1]!;"
        "vrecpe.f32 q1, q0;"
        "vst1.f32 {q1}, [%0]!;"
        :"+r"(p_dst)
        :"r"(p_src)
    );   
    for(i = 0; i < 4; i++)
    {
        printf("dst[%d] = %#f\n", i, dst[i]);
    }                    

}

void neon_vrsqrte(void)//求平方根倒数
{
    float src[4] = {32.0, 64.0, 128.0, 256.0};
    float dst[4] = {0};
    float* p_src = src;
    float* p_dst = dst;
    int i = 4;
    asm
    (
        "vld1.f32 {q0}, [%1]!;"
        "vrsqrte.f32 q1, q0;"
        "vst1.f32 {q1}, [%0]!;"
        :"+r"(p_dst)
        :"r"(p_src)
    );   
    for(i = 0; i < 4; i++)
    {
        printf("dst[%d] = %#f\n", i, dst[i]);
    }                    

}

void neon_vrecps(void)//无法编译通过
{
    float src_1[4] = {32, 64, 128, 256};
    float src_2[4] = {2, 2, 2, 2};
    float dst[4] = {0};
    float* p_src_1 = src_1;
    float* p_src_2 = src_2;
    float* p_dst = dst;
    int i = 0;
    asm
    (
        "vld1.f32 {q0}, [%1]!;"
        "vld1.f32 {q1}, [%2]!;"
        // "vrecps.f32 {q2}, q1, q0;"
        "vst1.f32 {q2}, [%0]!;"
        :"+r"(p_dst)
        :"r"(p_src_1), "r"(p_src_2)
    );   
    for(i = 0; i < 4; i++)
    {
        printf("dst[%d] = %#f\n", i, dst[i]);
    }                    

}

void neon_vxxmullx(void)//向量或标量乘法(带累加或累减)，这里累加以作为示例，累减同理，支持长整型运算，支持运算结果加倍操作(带D的乘法指令)，
{
    char src_1[8] = {0};
    char src_2[8] = {0};
    short src_3[4] = {0};
    short src_4[4] = {0};   
    short src_5[4] = {0};
    short src_6[4] = {0};  

    char dst_1[8] = {0};
    short dst_2[8] = {0};
    int dst_3[4] = {0};
    int dst_4[4] = {0};

    char* p_src_1 = src_1;
    char* p_src_2 = src_2;
    short* p_src_3 = src_3;
    short* p_src_4 = src_4;
    short* p_src_5 = src_5;
    short* p_src_6 = src_6;

    char* p_dst_1 = dst_1;
    short* p_dst_2 = dst_2;
    int* p_dst_3 = dst_3;
    int* p_dst_4 = dst_4;

    int i = 0;
    for(i = 0; i < 8; i++)
    {
        src_1[i] = (i + 1) * 2;
        printf("src_1[%d] = %#d\n", i, src_1[i]);
    }  
    for(i = 0; i < 8; i++)
    {
        src_2[i] = 16;
        printf("src_2[%d] = %#d\n", i, src_2[i]);
    }    
    for(i = 0; i < 4; i++)
    {
        src_3[i] = (i + 1) * 2;
        printf("src_3[%d] = %#d\n", i, src_3[i]);
    }  
    for(i = 0; i < 4; i++)
    {
        src_4[i] = 16;
        printf("src_4[%d] = %#d\n", i, src_4[i]);
    }      
    for(i = 0; i < 4; i++)
    {
        src_5[i] = 181;
        printf("src_5[%d] = %#d\n", i, src_5[i]);
    }  
    for(i = 0; i < 4; i++)
    {
        src_6[i] = 182;
        printf("src_6[%d] = %#d\n", i, src_6[i]);
    }       

    asm __volatile__
    (
        "vld1.s8 d0, [%4]!;"
        "vld1.s8 d1, [%5]!;"

        "vmov.s8 d2, #100;"
        "vmla.s8 d2, d1, d0;"//普通向量

        "vmov.s16 q3, #100;"
        "vmlal.s8 q3, d1, d0;"//

        "vld1.s16 d0, [%6]!;"
        "vld1.s16 d1, [%7]!;"

        "vmov.s32 q4, #100;"//注意，由使用长整型进行16bit，结果是32bit，所以这里需要将q4分成4个32bit的元素
        "vmlal.u16 q4, d1, d0[0];"//长整型标量, 仅支持16bit以上的运算

        "vld1.s16 d0, [%8]!;"
        "vld1.s16 d1, [%9]!;"
        "vmov.s32 q5, #0xffffffff;"
        "vqdmlal.s16 q5, d1, d0;"//长整型向量加倍饱和, 仅支持16bit以上的运算，但饱和并没有生效

        "vst1.u8 d2, [%0]!;"
        "vst1.u16 {q3}, [%1]!;"
        "vst1.u32 {q4}, [%2]!;"
        "vst1.s32 {q5}, [%3]!;"
        :"+r"(p_dst_1), "+r"(p_dst_2), "+r"(p_dst_3), "+r"(p_dst_4)
        :"r"(p_src_1), "r"(p_src_2), "r"(p_src_3), "r"(p_src_4), "r"(p_src_5), "r"(p_src_6)
        :"memory"
    );   

    for(i = 0; i < 8; i++)
    {
        printf("mla dst_1[%d] = %#d\n", i, dst_1[i]);
    }     
    for(i = 0; i < 8; i++)
    {
        printf("mlal dst_2[%d] = %#d\n", i, dst_2[i]);
    }   
    for(i = 0; i < 4; i++)
    {
        printf("mlal scalar dst_3[%d] = %#d\n", i, dst_3[i]);
    }   
    for(i = 0; i < 4; i++)
    {
        printf("qdmlal dst_4[%d] = %#x\n", i, dst_4[i]);
    }                              

}
/*arm-linux-gnueabihf-gcc -ftree-vectorize -mfpu=neon -mcpu=cortex-a7 arm_neon_asm_sample.c -o arm_neon_asm_sample*/
int main()
{
    neon_vrev();
}