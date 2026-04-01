extern fun printf(u8* str) void;
extern fun malloc(u32 size) void*;
fun main(i32 argc, u8** argv) i32
{
    u8* str = "Hello, World!\n";  
    i64 a = 1 + 2; 
    i64 b = str * a;

    return a;
}