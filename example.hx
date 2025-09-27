/*int argc, string[] argv*/

//fun print(int num);


fun main() -> int
{

	/*
	OK = 0
	ERROR CODES
	fibonacci_series	= 1
	add					= 2
	sub					= 3
	mul					= 4
	div					= 5
	calculations		= 6
	if equal			= 21
	*/
	
	
	// if else if test
	int ERROR = 0;


	if (2 < 1)
	{
		ERROR = 4;
	}
	else if (2 == 1)
	{
		ERROR = 10;
	}
	else if (2 && 0)
	{
		ERROR = 123;
	}

	

	
	
	if (fibonacci_series(0, 1, 79) != 14472334024676221)
	{
		ERROR = 1;
	}
	
	

	
	if (add(1235, add(122, 7534)) != 8891)
		ERROR = 2;

	if (sub(888, 666) >= 223)
		ERROR = 3;

	if (mul(12, 14) != 168)
		ERROR = 4;

	

	if (div(100, 9) < 10 || div(100, 9) > 12)
		ERROR = 5;
	
	

	int a = 3;
	int b = 4;
	int c = 5;
	{
	if ( a + 14472334024676 < 5)
		ERROR = 6;

	}

	if ( 3-4 == 0-2)
		ERROR = 21;

	


	int i = 0;

	while(i < 10)
	{
		print(i);
		i = i + 1;

	}
	
	




	ERROR = -3 * 4 + 2 - 15 + 34/(2+2);

	//ERROR = -3 + 4;



	print(ERROR);
	return ERROR;

	

	
}



fun fibonacci_series(int a, int b, int n) -> int
{
	if (n == 1) return 0;
	int c = a + b;
	print(c);
	if (n < 3)
	{
		return c;
	}
	 
	return fibonacci_series(b, c, n - 1);
}

fun add(int a, int b) -> int
{
	return a + b;
}

fun sub(int a, int b) -> int
{
	return a - b;
}

fun mul(int a, int b) -> int
{
	return a * b;
}


fun div(int a, int b) -> int
{
	return a / b;
}


