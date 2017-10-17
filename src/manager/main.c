#include <stdio.h>

#include <funprint.h>

int
main (void)
{
  puts ("Hello World!");
  puts ("This is " PACKAGE_STRING ".");
  print_func(3);
  return 0;
}