#include <stdio.h>
#include <stdlib.h>

// 1. my_realloc 함수 복원
// 원형: char* my_realloc(char *old, int oldlen, int newlen)
// 기능: newlen 바이트 크기의 새로운 메모리 할당 후 old 메모리 내용 복사, new 포인터 반환
char* my_realloc(char *old, int oldlen, int newlen) {
    // AST 분석: 함수 반환타입 char*, 파라미터 (char* old, int oldlen, int newlen)
    // 함수 내부:
    //   - 선언: char *new = malloc(newlen);
    //   - 선언: int i = 0;
    //   - while 루프: while (i <= oldlen - 1) { new[i] = old[i]; i = i + 1; }
    //   - return new;
    char *new = (char *)malloc(newlen);  // malloc 결과를 char*로 받음 (C에서는 캐스팅 없어도 되지만 명시)
    int i = 0;
    while (i <= oldlen - 1) {
        new[i] = old[i];
        i = i + 1;
    }
    return new;
}

// 2. save_int 함수 복원
// 원형: void save_int(char *p, int n)
// 기능: 정수 n의 각 바이트를 순차적으로 버퍼 p에 저장 (리틀엔디언 형태)
void save_int(char *p, int n) {
    // AST 분석: 반환 void, 파라미터 (char *p, int n)
    // 함수 내부:
    //   p[0] = n;
    //   p[1] = n >> 8;
    //   p[2] = n >> 16;
    //   p[3] = n >> 24;
    // 각 할당문은 AST에서 Assignment 노드와 BinaryOp (>> 연산)으로 표현되어 있었음.
    p[0] = n;           // 하위 8비트 저장 (0xFF & n 와 동일한 효과)
    p[1] = n >> 8;      // 다음 8비트 저장
    p[2] = n >> 16;     // ...
    p[3] = n >> 24;
}

// 3. load_int 함수 복원
// 원형: int load_int(char *p)
// 기능: 버퍼 p로부터 4바이트를 읽어 정수값으로 복원하여 반환
int load_int(char *p) {
    // AST 분석: 반환 int, 파라미터 (char *p)
    // 함수 내부: return (p[0] & 255) + ((p[1] & 255) << 8) + ((p[2] & 255) << 16) + ((p[3] & 255) << 24);
    // AST의 BinaryOp 노드들이 중첩되어 위와 같은 수식을 형성하고 있었음.
    return (p[0] & 255)
         + ((p[1] & 255) << 8)
         + ((p[2] & 255) << 16)
         + ((p[3] & 255) << 24);
}

// 메인함수 (테스트 용도)
// 복원한 함수들을 간단히 테스트해볼 수 있습니다.
int main() {
    // my_realloc 테스트
    char *orig = (char*)malloc(5);
    orig[0] = 'A'; orig[1] = 'B'; orig[2] = 'C'; orig[3] = 'D'; orig[4] = '\0';
    char *resized = my_realloc(orig, 5, 10);  // 크기 5 -> 10 확장
    // (주의: 실제 realloc은 orig 메모리를 해제하지만, 여기 구현에서는 해제하지 않으므로 메모리 누수에 유의)

    printf("my_realloc result: %s\n", resized); // "ABCD"가 그대로 들어있어야 함

    // save_int & load_int 테스트
    int num = 305419896; // 임의의 값 (0x12345678)
    char buffer[4];
    save_int(buffer, num);
    // buffer 내용 확인 (각 바이트 값 출력)
    printf("Saved bytes: %d %d %d %d\n",
            buffer[0] & 0xFF, buffer[1] & 0xFF, buffer[2] & 0xFF, buffer[3] & 0xFF);
    int recovered = load_int(buffer);
    printf("Recovered int: %d\n", recovered);

    free(orig);
    free(resized);
    return 0;
}
