extern fun printf(u8* str) void;
extern fun malloc(u32 size) void*;
fun main(i32 argc, u8** argv) i32
{
    i64 a = 1;
    a = 2;
    i64 b = a + a;
    i64 c = a + b;
    i64 d = c + a;
    i64 add = add(a,b,c,d,5,6,7,8);
    i64 mul = b(b, add);

    return add(add, mul);
}

fun add(i64 a, i64 b, i64 c, i64 d, i64 e, i64 f, i64 g, i64 h) i64
{
    return a + b + c + d + e + f + g + h;
}

fun b(i64 a, i64 b) i64
{
    return a * b;
}