extern fun printf() void;

fun exec(i32 x) i32
{
    printf("%d: exec\n", x);
    return 1;
}

fun main() i32
{
    i32 x = 0;
    i32 a1 = 0 && exec(x); 
    x += 1;
    i32 a2 = 1 && exec(x); 
    x += 1;
    i32 b1 = 0 || exec(x);
    x += 1;
    i32 b2 = 1 || exec(x);
    return 0;
}
