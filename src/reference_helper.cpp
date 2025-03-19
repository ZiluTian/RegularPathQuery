// references.cpp
#include <stdio.h>
#include <iostream>

struct S {
   short i;
};

int main() {
   S  s;   // Declare the object.
   S& SRef = s;   // Declare and initialize the reference.
   s.i = 3;

   printf("%d\n", s.i);
   printf("%d\n", SRef.i);

   SRef.i = 4;
   printf("%d\n", s.i);
   printf("%d\n", SRef.i);
}