extern fun printf(u8* str) void;
extern fun malloc(u32 size) void*;
fun main(i32 argc, u8** argv) i32
{
    u8* str = "Hello, World!\n";  
    i64 a = 1 + 2; 
    i64 b = str * a;
    i64 c = a + str;
    i64 d = a+b+c;
    i64 e = str + c;
    i64 f = a + b+c;
    i64 g = f + a; 
    
    return add(a,b,f,2,e,4,5,6);
}

fun add(i64 a, i64 b, i64 c, i64 d, i64 e, i64 f, i64 g, i64 h) i64
{
    return a + b + c + d + e + f + g + h;
}