
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_c.c"  // 제공된 json_c.c 파일의 API를 활용하여 JSON 파싱

// 재귀적으로 AST의 json_value 노드를 탐색하여, 키가 "_nodetype"이고 값이 "If"인 노드 개수를 센다.
// 만약 JSON 객체나 배열이면 내부 요소들을 모두 순회하여 if 문이 몇 개 등장하는지 합산한다.
int count_if_nodes_val(json_value val) {
    int count = 0;
    // JSON 객체인 경우
    if(val.type == JSON_OBJECT) {
        json_object *obj = (json_object *)(val.value);
        // 객체의 각 key-value 쌍을 순회
        for (int i = 0; i <= obj->last_index; i++) {
            // 현재 key가 "_nodetype"이면, 그 값이 "If"인지 확인
            if(obj->keys[i] != NULL && strcmp(obj->keys[i], "_nodetype") == 0) {
                if(obj->values[i].type == JSON_STRING &&
                   strcmp(json_to_string(obj->values[i]), "If") == 0) {
                    count++;  // if 문 발견 시 카운트 증가
                }
            }
            // 해당 값에 대해 재귀적으로 탐색하여 하위 if 문도 카운트
            count += count_if_nodes_val(obj->values[i]);
        }
    }
    // JSON 배열인 경우: 배열의 각 요소를 재귀적으로 탐색
    else if(val.type == JSON_ARRAY) {
        json_array *arr = (json_array *)(val.value);
        for (int i = 0; i <= arr->last_index; i++) {
            count += count_if_nodes_val(arr->values[i]);
        }
    }
    // 기본 타입(문자열, 숫자 등)은 if 문이 없으므로 그냥 0 반환
    return count;
}

// 재귀적으로 AST의 타입 노드(IdentifierType, TypeDecl, PtrDecl, ArrayDecl 등)를 따라가며
// 최종적인 타입 문자열(예: "char *")을 outBuf에 구성한다.
void get_type_string_val(json_value typeVal, char *outBuf) {
    // 만약 typeVal이 객체가 아니면 처리하지 않음
    if(typeVal.type != JSON_OBJECT) return;
    
    // "_nodetype" 값을 추출 (예: "IdentifierType", "TypeDecl", "PtrDecl" 등)
    char *nodetype = json_get_string(typeVal, "_nodetype");
    if(nodetype == NULL) return;

    if(strcmp(nodetype, "IdentifierType") == 0) {
        // IdentifierType: 기본 타입명이 "names" 배열에 들어 있음 (예: ["int"], ["unsigned", "int"])
        json_value namesArray = json_get(typeVal, "names");
        if(namesArray.type == JSON_ARRAY) {
            int len = json_len(namesArray);
            for (int i = 0; i < len; i++) {
                json_value nameItem = json_get(namesArray, i);
                if(nameItem.type == JSON_STRING) {
                    strcat(outBuf, json_to_string(nameItem));
                    if(i < len - 1) strcat(outBuf, " ");
                }
            }
        }
    } else if(strcmp(nodetype, "TypeDecl") == 0) {
        // TypeDecl: 내부의 "type" 필드를 재귀적으로 탐색하여 실제 타입 정보를 얻음
        json_value innerType = json_get(typeVal, "type");
        get_type_string_val(innerType, outBuf);
    } else if(strcmp(nodetype, "PtrDecl") == 0) {
        // PtrDecl: 포인터 선언이므로 내부 타입을 구한 뒤, 결과 문자열에 " *"를 추가한다.
        json_value innerType = json_get(typeVal, "type");
        get_type_string_val(innerType, outBuf);
        strcat(outBuf, " *");
    } else if(strcmp(nodetype, "ArrayDecl") == 0) {
        // ArrayDecl: 배열 타입인 경우, 내부 타입과 함께 배열 크기를 표현
        json_value innerType = json_get(typeVal, "type");
        json_value dimNode = json_get(typeVal, "dim");
        get_type_string_val(innerType, outBuf);
        strcat(outBuf, " [");
        if(dimNode.type == JSON_STRING) {
            strcat(outBuf, json_to_string(dimNode));
        }
        strcat(outBuf, "]");
    }
    // 기타 nodetype은 필요한 경우 추가 구현 가능
}

int main(void) {
    // 1. ast.json 파일 읽기 및 메모리 로드
    FILE *fp = fopen("ast.json", "r");
    if(fp == NULL) {
        fprintf(stderr, "파일 열기 실패: ast.json\n");
        return 1;
    }
    // 파일 크기 측정 후 전체 내용을 jsonData에 저장
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *jsonData = (char *)malloc(fileSize + 1);
    if(jsonData == NULL) {
        fprintf(stderr, "메모리 할당 실패\n");
        fclose(fp);
        return 1;
    }
    fread(jsonData, 1, fileSize, fp);
    jsonData[fileSize] = '\0';  // 문자열 종료
    fclose(fp);

    // 2. json_c.c를 이용하여 JSON 파싱: ast.json의 내용을 json_value 구조체 형태로 변환
    json_value root = json_create(jsonData);
    if(root.type == JSON_UNDEFINED) {
        fprintf(stderr, "JSON 파싱 실패\n");
        free(jsonData);
        return 1;
    }
    // 최상위 객체에서 "ext" 배열 추출 (외부 선언들이 저장된 배열)
    json_value extArray = json_get(root, "ext");
    if(extArray.type != JSON_ARRAY) {
        fprintf(stderr, "\"ext\" 배열을 찾을 수 없음\n");
        free(jsonData);
        return 1;
    }

    // 3. 함수 정의(FuncDef) 노드 탐색 및 전체 함수 개수 출력
    int funcCount = 0;
    int extLen = json_len(extArray);
    for(int i = 0; i < extLen; i++) {
        json_value item = json_get(extArray, i);
        char *nodeType = json_get_string(item, "_nodetype");
        if(nodeType != NULL && strcmp(nodeType, "FuncDef") == 0) {
            funcCount++;
        }
    }
    printf("전체 함수 개수: %d\n\n", funcCount);

    // 4. 각 함수별로 상세 정보(함수 이름, 리턴 타입, 파라미터, if문 개수) 추출 및 출력
    for(int i = 0; i < extLen; i++) {
        json_value item = json_get(extArray, i);
        char *nodeType = json_get_string(item, "_nodetype");
        if(nodeType == NULL || strcmp(nodeType, "FuncDef") != 0)
            continue;  // 함수 정의가 아니면 건너뜀

        // (1) 함수 이름과 리턴 타입 추출
        json_value decl = json_get(item, "decl");
        char *funcName = json_get_string(decl, "name");
        json_value funcType = json_get(decl, "type");
        // funcType 내부의 "type" 필드가 리턴 타입 정보임
        json_value returnTypeNode = json_get(funcType, "type");
        char returnTypeBuf[100] = "";
        get_type_string_val(returnTypeNode, returnTypeBuf);
        if(strlen(returnTypeBuf) == 0)
            strcpy(returnTypeBuf, "(unknown)");

        // (2) 파라미터 정보 추출: args -> params 배열에서 각 파라미터의 타입과 이름을 얻음
        json_value args = json_get(funcType, "args");
        char paramsStr[256] = "";
        if(args.type == JSON_NULL) {
            strcpy(paramsStr, "void");
        } else {
            json_value paramList = json_get(args, "params");
            if(paramList.type != JSON_ARRAY) {
                strcpy(paramsStr, "void");
            } else {
                int paramCount = json_len(paramList);
                for(int j = 0; j < paramCount; j++) {
                    json_value paramItem = json_get(paramList, j);
                    char paramTypeBuf[100] = "";
                    json_value paramTypeNode = json_get(paramItem, "type");
                    get_type_string_val(paramTypeNode, paramTypeBuf);
                    if(strlen(paramTypeBuf) == 0)
                        strcpy(paramTypeBuf, "unknown");
                    char *pname = json_get_string(paramItem, "name");
                    char oneParam[128] = "";
                    if(pname != NULL && strlen(pname) > 0)
                        sprintf(oneParam, "%s %s", paramTypeBuf, pname);
                    else
                        sprintf(oneParam, "%s", paramTypeBuf);
                    if(j > 0) strcat(paramsStr, ", ");
                    strcat(paramsStr, oneParam);
                }
            }
        }

        // (3) 함수 본문(body)에서 if 문 개수 카운트
        json_value body = json_get(item, "body");
        int ifCount = count_if_nodes_val(body);

        // (4) 추출한 정보를 보기 좋게 출력
        printf("함수 이름: %s\n", funcName ? funcName : "(noname)");
        printf("  리턴 타입: %s\n", returnTypeBuf);
        printf("  파라미터: (%s)\n", paramsStr);
        printf("  if 문 개수: %d\n\n", ifCount);
    }

    // 5. 동적 할당된 메모리 해제 (여기서는 jsonData만 해제)
    free(jsonData);
    return 0;
}
