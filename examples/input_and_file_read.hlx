extern fun printf(u8* str) void;
extern fun malloc(u32 size) void*;
extern fun fopen(u8* path, u8* mode) u32*;
extern fun fgets(u8* buf, u32 a, u32* b) u32*;
extern fun fclose(u32* f) void;
extern fun srand(u32 seed) void;
extern fun rand() i64;
extern fun time(u32* n) u64;
extern fun scanf(u8* format) void;

fun main(i64 argc, u8** argv) i32
{
    srand(time(0));
    i64* input = malloc(8);
    printf("If you guess the number right from 1 to 3,\nI will print this file!\n");
    i64 random = rand() % 3 + 1;
    while (*input != random)
    {
        random = rand() % 3 + 1;
        scanf("%d", input);
        printf("entered: %d\n", *input);
        printf("random: %d\n", random);
    }
    if (*input == random)
    { 
        u32* fptr = fopen("example.hx", "r");
        u8* buf = malloc(8);
        while (fgets(buf, 8, fptr) != 0)
        {
            printf("%s", buf);
        }
        fclose(fptr);
    }
    return 0;
}

