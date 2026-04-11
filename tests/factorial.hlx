extern fun printf(u8* str) void;

fun factorial(i32 x) i32
{
    if (x <= 1) 
    {
        return 1;
    }
    return x * factorial(x - 1);
}

fun main(i32 argc, u8** argv) i32
{
    i32 x = 12;
    printf("%d", factorial(x));
    return 0;
}

