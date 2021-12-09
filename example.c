#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}

static const char *parse_number(const char *num)
{
	double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

	if (*num=='-') sign=-1,num++;	/* Has sign? */
	if (*num=='0') num++;			/* is zero */
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
	if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
	if (*num=='e' || *num=='E')		/* Exponent? */
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
	
    printf("%lf ,%d\n", n, (int)n);
	return num;
}

int main(int argc, char const *argv[])
{
    //const char *p = skip("  hh tt xx");
    //const char *p = parse_number("1.7334");

    char *end = NULL;
    double num = strtod("1.57hhh", (char **)&end);
    if (end == NULL) {
        printf("end is null\n");
    } else if (*end == '\0') {
        printf("end = '\\0'");
    } else {
        printf("end:%s\n", end);
    }

    printf("%lf\n", num);

    //printf("%s\n", p);
    return 0;
}
