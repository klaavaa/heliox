extern fun printf(u8* str) void;

fun main(i64 argc, u8** argv) i32
{
    i64 i = 0;
    while (i < argc)
    {
        printf("%d: %s\n", i, *(argv+8*i));
        i = i + 1;
    }

    return 0;
}

