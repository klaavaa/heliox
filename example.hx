extern fun printf(u8* str) void;
extern fun malloc(u32 size) void*;
fun main(i32 argc, u8** argv) i32
{
    u8* str = "Hello, World!\n";  
    
    i8 a = 4 + str;
    i16 b = 10;
    printf(str);

    a = 12;
    a = a - b;
    return add(1, 2, 3, 4+2*a, 5, 6, 7, 8) - 15; 
    return 0;
}

fun add(i32 a, i32 b, i32 c, i64 d, i8 e, i16 f, i32 g, i32 h) i32
{
    return a + b + c + d + e + f + g + h;
}
