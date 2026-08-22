#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "crc32.h"

#define BUFFER_SIZE 4096

int main()
{
    FILE *file;
    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;
    uint32_t checksum = 0;
    uint32_t total_bytes = 0;
    
    // 打开二进制文件
    file = fopen("stm32f407.bin", "rb");
    if (file == NULL) {
        fprintf(stderr, "错误：无法打开文件 stm32f407.bin\n");
        return 1;
    }
    
    printf("正在计算 stm32f407.bin 的CRC32校验和...\n");
    
    // 分块读取文件并计算CRC32
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // 对于多块数据的CRC32计算，需要将之前的结果作为初始值
        // 但是这里的crc32函数每次都从头开始计算，所以我们需要读取整个文件
        // 重新设计算法
        total_bytes += bytes_read;
    }
    
    // 关闭文件
    fclose(file);
    
    // 重新打开文件，一次性读取所有数据
    file = fopen("stm32f407.bin", "rb");
    if (file == NULL) {
        fprintf(stderr, "错误：无法重新打开文件 stm32f407.bin\n");
        return 1;
    }
    
    // 分配内存存储整个文件
    unsigned char *file_data = (unsigned char *)malloc(total_bytes);
    if (file_data == NULL) {
        fprintf(stderr, "错误：内存分配失败\n");
        fclose(file);
        return 1;
    }
    
    // 读取整个文件
    size_t read_bytes = fread(file_data, 1, total_bytes, file);
    if (read_bytes != total_bytes) {
        fprintf(stderr, "错误：文件读取不完整\n");
        free(file_data);
        fclose(file);
        return 1;
    }
    
    // 计算CRC32校验和
    checksum = crc32(file_data, total_bytes);
    
    // 输出结果
    printf("文件大小: %u 字节\n", total_bytes);
    printf("CRC32校验和: 0x%08X\n", checksum);
    
    // 清理资源
    free(file_data);
    fclose(file);
    
    return 0;
}
