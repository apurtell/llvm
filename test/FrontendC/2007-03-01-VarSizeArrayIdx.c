// RUN: %llvmgcc %s -O3 -S -o - | grep mul
// PR1233

float foo(int w, float A[][w], int g, int h) {
  return A[g][0];
}

