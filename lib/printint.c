#include <stdio.h>

void putint(int a) { printf("%d\n", a); }
void putch(char a) { printf("%c\n", a); }
void putlong(long a) { printf("%ld\n", a); }
void putarray(int n, int a[]) {
  printf("%d\n", n);
  for (int i = 0; i < n; i++)
    printf("%d ", a[i]);
  putchar('\n');
}
