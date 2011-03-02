#include <math.h>
#include <stdio.h>
static double FindExpCoeff(double b) {
  double a, bg;
  int Try;
  if (fabs(b-1)<=0.001) return 0;
  if (b<2) a=(b-1)/2; else a=b;
  bg = a/(1-exp(-a));
  /* The limit on Try is just to be sure there is no infinite loop. */
  for (Try=0; fabs(bg-b)>0.001 || Try<100; Try++) {
    a = a + (b-bg);
    bg = a/(1-exp(-a));
  }
  return a;
}

double Curve(double a,double x) {
  if (fabs(a)<0.001) return x; // limit for a->0
  return (1-exp(-1*a*x))/(1-exp(-1*a));
}

main() {

double exp;
double x;
double a;

for (exp=0.25;exp<4;exp+=0.25) {
  printf("%f : %f\n",exp,FindExpCoeff(exp));
}
// Header row
printf("(x,exp);");
for (exp=0.25;exp<4;exp+=0.25) {
  printf("%4.2f;",exp);
}
printf("\n");

// Value table
for (x=0.0;x<=1.0;x+=0.01) {
  printf("%f;",x);
  for (exp=0.25;exp<4;exp+=0.25) {
    printf("%f;",Curve(FindExpCoeff(exp),x));
  }
  printf("\n");
}

/*
for (exp=0.99;exp<1.01;exp+=0.001) {
  printf("exp : %f - a : %f\n",exp,FindExpCoeff(exp));
}
*/

}

