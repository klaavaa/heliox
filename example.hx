extern fun printf(u8* str) void;
extern fun malloc(u32 size) void*;

fun main(i32 argc, u8** argv) i32
{
    i64 a = 14; 
    i64 b = 3;
    return add(a, mul(b, sub(a, div(a, b))));
}

fun add(i64 a, i64 b) i64
{
    return a + b;
}
fun sub(i64 a, i64 b) i64
{
    return a - b;
}
fun mul(i64 a, i64 b) i64
{
    return a * b;    
}
fun div(i64 a, i64 b) i64
{
    return a / b;
}

