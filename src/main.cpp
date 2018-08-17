#include <cstdio>

int main(void) {
  int x;
  scanf("%d", &x);
  printf(R"(.intel_syntax noprefix
.global main
main:
    mov eax, %d
    ret
)", x);
  return 0;
}
