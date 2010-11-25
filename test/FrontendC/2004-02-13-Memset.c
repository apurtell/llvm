// RUN: %llvmgcc -xc %s -S -o - | grep llvm.memset | count 3

void *memset(void*, int, long);
void bzero(void*, long);

void test(int* X, char *Y) {
  memset(X, 4, 1000);
  bzero(Y, 100);
}
