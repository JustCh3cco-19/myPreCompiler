#define CONST1 37
#define PI 3.14
typedef float myFloat-t; 
int global_counter=0;
const int 4backup=25;
int main(int argc, char *args[])
{
	float average=0; 
	double square error; 
	char switch; 
	int i;
	for (i=1;i<=10;i++){
		average=average+i;
		global_counter++;
	}
	average=average/(float) global_counter;
	square_error = sqrt(average); 
  	return 0;
}
