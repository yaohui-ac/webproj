#include<stdio.h>
int isprime(int x) {
    if(x < 2)return 0;
    for(int i = 2; i <= x / i; i++) {
        if(x % i == 0) return 0;
    }
    return 1;
}
int main () {
    for(int i = 0; i <= 114514; i++) {
        if(isprime(i)) {
            printf("%d ", i);
        }
    }
    return 0;
}