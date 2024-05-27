#include <cstddef>

static const char blit_kernel[] = {
0x23, 0x69, 0x66, 0x64, 0x65, 0x66, 0x20, 0x5f, 0x5f, 0x43, 0x4c, 0x49, 0x4f, 0x4e, 0x5f, 0x49, 0x44, 0x45, 0x5f, 0x5f, 
0x0a, 0x23, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x20, 0x3c, 0x6c, 0x69, 0x62, 0x67, 0x70, 0x75, 0x2f, 0x6f, 0x70, 
0x65, 0x6e, 0x63, 0x6c, 0x2f, 0x63, 0x6c, 0x2f, 0x63, 0x6c, 0x69, 0x6f, 0x6e, 0x5f, 0x64, 0x65, 0x66, 0x69, 0x6e, 0x65, 
0x73, 0x2e, 0x63, 0x6c, 0x3e, 0x0a, 0x23, 0x65, 0x6e, 0x64, 0x69, 0x66, 0x0a, 0x0a, 0x23, 0x6c, 0x69, 0x6e, 0x65, 0x20, 
0x36, 0x0a, 0x0a, 0x2f, 0x2f, 0x6d, 0x5f, 0x72, 0x6f, 0x77, 0x53, 0x74, 0x72, 0x69, 0x64, 0x65, 0x20, 0x3d, 0x20, 0x28, 
0x6d, 0x5f, 0x77, 0x69, 0x64, 0x74, 0x68, 0x20, 0x2b, 0x20, 0x36, 0x33, 0x29, 0x20, 0x3e, 0x3e, 0x20, 0x36, 0x3b, 0x0a, 
0x0a, 0x62, 0x6f, 0x6f, 0x6c, 0x20, 0x63, 0x61, 0x6e, 0x42, 0x6c, 0x69, 0x74, 0x28, 0x5f, 0x5f, 0x67, 0x6c, 0x6f, 0x62, 
0x61, 0x6c, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x6c, 0x6f, 0x6e, 0x67, 0x20, 0x2a, 0x6f, 0x6e, 
0x65, 0x2c, 0x0a, 0x09, 0x09, 0x09, 0x20, 0x5f, 0x5f, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x20, 0x75, 0x6e, 0x73, 0x69, 
0x67, 0x6e, 0x65, 0x64, 0x20, 0x6c, 0x6f, 0x6e, 0x67, 0x20, 0x2a, 0x6f, 0x74, 0x68, 0x65, 0x72, 0x2c, 0x0a, 0x09, 0x09, 
0x09, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x77, 0x2c, 0x20, 0x75, 0x6e, 
0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x68, 0x2c, 0x0a, 0x09, 0x09, 0x09, 0x20, 0x75, 0x6e, 
0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x63, 0x77, 0x2c, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 
0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x63, 0x68, 0x2c, 0x0a, 0x09, 0x09, 0x09, 0x20, 0x75, 0x6e, 0x73, 0x69, 
0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x6f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x58, 0x2c, 0x20, 0x75, 0x6e, 
0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x6f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x59, 0x29, 0x20, 
0x7b, 0x0a, 0x09, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x72, 0x6f, 0x77, 0x53, 
0x74, 0x72, 0x69, 0x64, 0x65, 0x20, 0x3d, 0x20, 0x28, 0x77, 0x20, 0x2b, 0x20, 0x36, 0x33, 0x29, 0x20, 0x3e, 0x3e, 0x20, 
0x36, 0x3b, 0x0a, 0x09, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x6f, 0x74, 0x68, 
0x65, 0x72, 0x52, 0x6f, 0x77, 0x53, 0x74, 0x72, 0x69, 0x64, 0x65, 0x20, 0x3d, 0x20, 0x28, 0x63, 0x77, 0x20, 0x2b, 0x20, 
0x36, 0x33, 0x29, 0x20, 0x3e, 0x3e, 0x20, 0x36, 0x3b, 0x0a, 0x09, 0x66, 0x6f, 0x72, 0x20, 0x28, 0x75, 0x69, 0x6e, 0x74, 
0x33, 0x32, 0x5f, 0x74, 0x20, 0x79, 0x20, 0x3d, 0x20, 0x30, 0x3b, 0x20, 0x79, 0x20, 0x3c, 0x20, 0x63, 0x68, 0x3b, 0x20, 
0x79, 0x2b, 0x2b, 0x29, 0x20, 0x7b, 0x0a, 0x09, 0x09, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 
0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x74, 0x68, 0x69, 0x73, 0x59, 0x20, 0x3d, 0x20, 0x79, 0x20, 0x2b, 0x20, 
0x6f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x59, 0x3b, 0x0a, 0x09, 0x09, 0x69, 0x66, 0x20, 0x28, 0x74, 0x68, 0x69, 0x73, 0x59, 
0x20, 0x3e, 0x3d, 0x20, 0x68, 0x29, 0x0a, 0x09, 0x09, 0x09, 0x63, 0x6f, 0x6e, 0x74, 0x69, 0x6e, 0x75, 0x65, 0x3b, 0x0a, 
0x09, 0x09, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x78, 0x20, 0x3d, 0x20, 0x30, 
0x3b, 0x0a, 0x09, 0x09, 0x66, 0x6f, 0x72, 0x20, 0x28, 0x3b, 0x3b, 0x29, 0x20, 0x7b, 0x0a, 0x09, 0x09, 0x09, 0x63, 0x6f, 
0x6e, 0x73, 0x74, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x74, 0x68, 0x69, 
0x73, 0x58, 0x20, 0x3d, 0x20, 0x78, 0x20, 0x2b, 0x20, 0x6f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x58, 0x3b, 0x0a, 0x09, 0x09, 
0x09, 0x69, 0x66, 0x20, 0x28, 0x74, 0x68, 0x69, 0x73, 0x58, 0x20, 0x3e, 0x3d, 0x20, 0x77, 0x29, 0x0a, 0x09, 0x09, 0x09, 
0x09, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x3b, 0x0a, 0x09, 0x09, 0x09, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x75, 0x6e, 0x73, 
0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x74, 0x68, 0x69, 0x73, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 
0x68, 0x69, 0x66, 0x74, 0x20, 0x3d, 0x20, 0x74, 0x68, 0x69, 0x73, 0x58, 0x20, 0x25, 0x20, 0x36, 0x34, 0x3b, 0x0a, 0x09, 
0x09, 0x09, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x6c, 0x6f, 0x6e, 
0x67, 0x20, 0x74, 0x68, 0x69, 0x73, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x20, 0x3d, 0x20, 0x6f, 0x6e, 0x65, 0x5b, 0x28, 0x74, 
0x68, 0x69, 0x73, 0x58, 0x20, 0x3e, 0x3e, 0x20, 0x36, 0x29, 0x20, 0x2b, 0x20, 0x74, 0x68, 0x69, 0x73, 0x59, 0x20, 0x2a, 
0x20, 0x72, 0x6f, 0x77, 0x53, 0x74, 0x72, 0x69, 0x64, 0x65, 0x5d, 0x20, 0x3e, 0x3e, 0x20, 0x74, 0x68, 0x69, 0x73, 0x42, 
0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x68, 0x69, 0x66, 0x74, 0x3b, 0x0a, 0x09, 0x09, 0x09, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 
0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x68, 
0x69, 0x66, 0x74, 0x20, 0x3d, 0x20, 0x78, 0x20, 0x25, 0x20, 0x36, 0x34, 0x3b, 0x0a, 0x09, 0x09, 0x09, 0x63, 0x6f, 0x6e, 
0x73, 0x74, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x6c, 0x6f, 0x6e, 0x67, 0x20, 0x62, 0x6c, 0x6f, 
0x63, 0x6b, 0x20, 0x3d, 0x20, 0x6f, 0x74, 0x68, 0x65, 0x72, 0x5b, 0x28, 0x78, 0x20, 0x3e, 0x3e, 0x20, 0x36, 0x29, 0x20, 
0x2b, 0x20, 0x79, 0x20, 0x2a, 0x20, 0x6f, 0x74, 0x68, 0x65, 0x72, 0x52, 0x6f, 0x77, 0x53, 0x74, 0x72, 0x69, 0x64, 0x65, 
0x5d, 0x20, 0x3e, 0x3e, 0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x68, 0x69, 0x66, 0x74, 0x3b, 0x0a, 0x09, 0x09, 0x09, 
0x69, 0x66, 0x20, 0x28, 0x28, 0x74, 0x68, 0x69, 0x73, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x20, 0x26, 0x20, 0x62, 0x6c, 0x6f, 
0x63, 0x6b, 0x29, 0x20, 0x21, 0x3d, 0x20, 0x30, 0x29, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 
0x20, 0x66, 0x61, 0x6c, 0x73, 0x65, 0x3b, 0x0a, 0x09, 0x09, 0x09, 0x78, 0x20, 0x2b, 0x3d, 0x20, 0x36, 0x34, 0x20, 0x2d, 
0x20, 0x6d, 0x61, 0x78, 0x28, 0x74, 0x68, 0x69, 0x73, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x68, 0x69, 0x66, 0x74, 0x2c, 
0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x68, 0x69, 0x66, 0x74, 0x29, 0x3b, 0x0a, 0x09, 0x09, 0x09, 0x69, 0x66, 0x20, 
0x28, 0x78, 0x20, 0x3e, 0x3d, 0x20, 0x63, 0x77, 0x29, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x3b, 
0x0a, 0x09, 0x09, 0x7d, 0x0a, 0x09, 0x7d, 0x0a, 0x09, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x74, 0x72, 0x75, 0x65, 
0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x5f, 0x5f, 0x6b, 0x65, 0x72, 0x6e, 0x65, 0x6c, 0x20, 0x76, 0x6f, 0x69, 0x64, 0x20, 0x62, 
0x6c, 0x69, 0x74, 0x28, 0x5f, 0x5f, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 
0x64, 0x20, 0x6c, 0x6f, 0x6e, 0x67, 0x20, 0x2a, 0x61, 0x74, 0x6c, 0x61, 0x73, 0x2c, 0x20, 0x5f, 0x5f, 0x67, 0x6c, 0x6f, 
0x62, 0x61, 0x6c, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x6c, 0x6f, 0x6e, 0x67, 0x20, 0x2a, 0x63, 
0x68, 0x61, 0x72, 0x74, 0x2c, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x20, 0x20, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x69, 
0x6e, 0x74, 0x20, 0x77, 0x2c, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x68, 0x2c, 0x0a, 0x09, 
0x09, 0x09, 0x09, 0x20, 0x20, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x63, 0x77, 0x2c, 0x20, 
0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x63, 0x68, 0x2c, 0x0a, 0x09, 0x09, 0x09, 0x09, 0x20, 0x20, 
0x20, 0x5f, 0x5f, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x63, 
0x68, 0x61, 0x72, 0x20, 0x2a, 0x72, 0x65, 0x73, 0x75, 0x6c, 0x74, 0x73, 0x29, 0x20, 0x7b, 0x0a, 0x09, 0x63, 0x6f, 0x6e, 
0x73, 0x74, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x78, 0x20, 0x3d, 0x20, 
0x67, 0x65, 0x74, 0x5f, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x5f, 0x69, 0x64, 0x28, 0x30, 0x29, 0x3b, 0x0a, 0x09, 0x63, 
0x6f, 0x6e, 0x73, 0x74, 0x20, 0x75, 0x6e, 0x73, 0x69, 0x67, 0x6e, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x74, 0x20, 0x79, 0x20, 
0x3d, 0x20, 0x67, 0x65, 0x74, 0x5f, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x5f, 0x69, 0x64, 0x28, 0x31, 0x29, 0x3b, 0x0a, 
0x0a, 0x09, 0x69, 0x66, 0x20, 0x28, 0x78, 0x20, 0x2b, 0x20, 0x63, 0x77, 0x20, 0x3e, 0x20, 0x77, 0x20, 0x7c, 0x7c, 0x20, 
0x79, 0x20, 0x2b, 0x20, 0x63, 0x68, 0x20, 0x3e, 0x20, 0x68, 0x29, 0x0a, 0x09, 0x09, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 
0x3b, 0x0a, 0x0a, 0x09, 0x69, 0x66, 0x20, 0x28, 0x63, 0x61, 0x6e, 0x42, 0x6c, 0x69, 0x74, 0x28, 0x61, 0x74, 0x6c, 0x61, 
0x73, 0x2c, 0x20, 0x63, 0x68, 0x61, 0x72, 0x74, 0x2c, 0x20, 0x77, 0x2c, 0x20, 0x68, 0x2c, 0x20, 0x63, 0x77, 0x2c, 0x20, 
0x63, 0x68, 0x2c, 0x20, 0x78, 0x2c, 0x20, 0x79, 0x29, 0x29, 0x20, 0x7b, 0x0a, 0x09, 0x09, 0x72, 0x65, 0x73, 0x75, 0x6c, 
0x74, 0x73, 0x5b, 0x79, 0x20, 0x2a, 0x20, 0x28, 0x77, 0x20, 0x2b, 0x20, 0x31, 0x29, 0x20, 0x2b, 0x20, 0x78, 0x5d, 0x20, 
0x3d, 0x20, 0x31, 0x3b, 0x0a, 0x09, 0x7d, 0x0a, 0x7d, 
};

size_t blit_kernel_length = sizeof(blit_kernel) / sizeof(char);
