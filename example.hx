extern fun printf(u8* str) void;
extern fun malloc(u32 size) void*;

fun main(i32 argc, u8** argv) i32
{
    1;
    i64 a = 14; 
    i64 b = 3;
    return add(a, sub(mul(div(a, b), b), b));

}
//fun asd(i64 a,i64 b,i64 c,i64 d,i64 e,i64 f,i64 g,i64 h,i64 i) i64
//{
//    return 0;
//}
fun add(i32 a, i32 b) i64
{
    printf("add: %d + %d = %d    ", a, b, a+b);
    return a + b;
}
fun sub(i32 a, i32 b) i64
{
    printf("sub: %d - %d = %d    ", a, b, a-b);
    return a - b;
}
fun mul(i64 a, i64 b) i64
{
    printf("mul: %d * %d = %d    ", a, b, a*b);
    return a * b;    
}
fun div(i64 a, i64 b) i64
{
    printf("div: %d / %d = %d    ", a, b, a/b);
    return a / b;
}

