#include <stdio.h>
#include <stdlib.h>

// my_realloc: 새로운 메모리를 할당하고 old 배열의 내용을 복사
char* my_realloc(char *old, int oldlen, int newlen) {
    char *new = (char *)malloc(newlen);
    int i = 0;
    while (i <= oldlen - 1) {
        new[i] = old[i];
        i = i + 1;
    }
    return new;
}

// save_int: 정수 n의 각 바이트를 버퍼 p에 저장 (리틀엔디언)
void save_int(char *p, int n) {
    p[0] = n;
    p[1] = n >> 8;
    p[2] = n >> 16;
    p[3] = n >> 24;
}

// load_int: 버퍼 p의 4바이트를 결합하여 정수로 복원
int load_int(char *p) {
    return (p[0] & 255)
         + ((p[1] & 255) << 8)
         + ((p[2] & 255) << 16)
         + ((p[3] & 255) << 24);
}

int main() {
    // my_realloc 테스트
    char *orig = (char*)malloc(5);
    orig[0] = 'A'; orig[1] = 'B'; orig[2] = 'C'; orig[3] = 'D'; orig[4] = '\0';
    char *resized = my_realloc(orig, 5, 10);
    printf("my_realloc result: %s\n", resized);

    // save_int & load_int 테스트
    int num = 305419896; // 예: 0x12345678
    char buffer[4];
    save_int(buffer, num);
    printf("Saved bytes: %d %d %d %d\n",
           buffer[0] & 0xFF, buffer[1] & 0xFF, buffer[2] & 0xFF, buffer[3] & 0xFF);
    int recovered = load_int(buffer);
    printf("Recovered int: %d\n", recovered);

    free(orig);
    free(resized);
    return 0;
}
