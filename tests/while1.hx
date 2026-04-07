extern fun printf(u8* str) void;

fun main() i32
{
    i32 i = 0; 
    while (i < 10)
    {
        printf("%d\n", i);
        i = i + 1; 
    }
    return i;
}

