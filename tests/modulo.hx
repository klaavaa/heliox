extern fun printf() void;

fun main() i32
{
    i32 i = 0; 
    while (i < 20)
    {
        if (i % 3 == 0)
        {
            printf("%d\n", i);
        }
        i += 1;
    }

    return 0;
}
