#include <stdio.h>
#include "mathslib.h"

int main(){
	int number = 6;
	int number_plus_two = addTwo(number);
	printf("number is now: %d\n", number_plus_two);
	int number_sq = square(number);
	printf("number squared is: %d\n", number_sq);
	return 0;
}