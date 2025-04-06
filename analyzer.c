#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_c.c"  // json_c.c 파일에 정의된 JSON 파서 사용

/*
  [함수 설명]
  count_if_nodes_val: json_value 타입의 AST 노드를 재귀적으로 순회하며,
  객체(object)나 배열(array) 내부에서 키가 "_nodetype"이고 값이 "If"인 노드의 개수를 센다.
*/
int count_if_nodes_val(json_value val) {
    int count = 0;
    // JSON 객체인 경우: 내부의 모든 key-value 쌍을 검사
    if(val.type == JSON_OBJECT) {
        json_object *obj = (json_object *)(val.value);
        // json_object의 유효 원소 수는 last_index+1
        for (int i = 0; i <= obj->last_index; i++) {
            // 만약 key가 "_nodetype"이고, 해당 값이 문자열 "If"라면 count 증가
            if(obj->keys[i] != NULL && strcmp(obj->keys[i], "_nodetype") == 0) {
                if(obj->values[i].type == JSON_STRING &&
                   strcmp(json_to_string(obj->values[i]), "If") == 0) {
                    count++;
                }
            }
            // 재귀적으로 각 값에 대해서도 탐색
            count += count_if_nodes_val(obj->values[i]);
        }
    }
    // JSON 배열인 경우: 각 요소에 대해 재귀 탐색
    else if(val.type == JSON_ARRAY) {
        json_array *arr = (json_array *)(val.value);
        for (int i = 0; i <= arr->last_index; i++) {
            count += count_if_nodes_val(arr->values[i]);
        }
    }
    // 기본 타입(문자열, 숫자 등)은 if문 노드가 없으므로 무시
    return count;
}

/*
  [함수 설명]
  get_type_string_val: AST의 타입 관련 노드(예, IdentifierType, TypeDecl, PtrDecl 등)를 재귀적으로 따라가서
  최종적인 타입 문자열을 outBuf에 구성한다.
  
  사용 예:
    - IdentifierType 노드의 "names" 배열에 ["int"]가 있으면 "int"를 반환.
    - PtrDecl 노드라면 내부 타입을 구한 뒤 " *"를 추가.
*/
void get_type_string_val(json_value typeVal, char *outBuf) {
    if(typeVal.type != JSON_OBJECT) return;

    // _nodetype 값를 가져옴
    char *nodetype = json_get_string(typeVal, "_nodetype");
    if(nodetype == NULL) return;

    if(strcmp(nodetype, "IdentifierType") == 0) {
        // IdentifierType: "names" 배열에 실제 타입 명칭이 있음.
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
        // TypeDecl: 내부의 "type" 필드를 재귀적으로 따라감
        json_value innerType = json_get(typeVal, "type");
        get_type_string_val(innerType, outBuf);
    } else if(strcmp(nodetype, "PtrDecl") == 0) {
        // PtrDecl: 내부 타입을 구한 뒤 포인터 표시 " *"를 덧붙임.
        json_value innerType = json_get(typeVal, "type");
        get_type_string_val(innerType, outBuf);
        strcat(outBuf, " *");
    } else if(strcmp(nodetype, "ArrayDecl") == 0) {
        // ArrayDecl: 내부 타입과 배열 크기를 표현.
        json_value innerType = json_get(typeVal, "type");
        json_value dimNode = json_get(typeVal, "dim");
        get_type_string_val(innerType, outBuf);
        strcat(outBuf, " [");
        if(dimNode.type == JSON_STRING) {
            strcat(outBuf, json_to_string(dimNode));
        }
        strcat(outBuf, "]");
    }
    // 기타 nodetype (예, FuncDecl 등)은 필요 시 추가 구현
}

int main(void) {
    // 1. ast.json 파일 읽기
    FILE *fp = fopen("ast.json", "r");
    if(fp == NULL) {
        fprintf(stderr, "파일 열기 실패: ast.json\n");
        return 1;
    }
    // 파일 크기를 구한 후 전체 내용을 메모리에 로드
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

    // 2. json_c.c를 이용해 JSON 파싱 (ast.json의 내용을 json_value 타입으로 생성)
    json_value root = json_create(jsonData);
    if(root.type == JSON_UNDEFINED) {
        fprintf(stderr, "JSON 파싱 실패\n");
        free(jsonData);
        return 1;
    }
    // 최상위 객체에서 "ext" 배열 추출 (전체 외부 선언들)
    json_value extArray = json_get(root, "ext");
    if(extArray.type != JSON_ARRAY) {
        fprintf(stderr, "\"ext\" 배열을 찾을 수 없음\n");
        free(jsonData);
        return 1;
    }

    // 3. 함수 정의(FuncDef) 노드들을 찾아 개수와 상세 정보 출력
    int funcCount = 0;
    int extLen = json_len(extArray);
    // 우선 전체 ext 배열을 순회하며 FuncDef의 개수를 센다.
    for(int i = 0; i < extLen; i++) {
        json_value item = json_get(extArray, i);
        char *nodeType = json_get_string(item, "_nodetype");
        if(nodeType != NULL && strcmp(nodeType, "FuncDef") == 0) {
            funcCount++;
        }
    }
    printf("전체 함수 개수: %d\n\n", funcCount);

    // 각 함수의 상세 정보 출력
    for(int i = 0; i < extLen; i++) {
        json_value item = json_get(extArray, i);
        char *nodeType = json_get_string(item, "_nodetype");
        if(nodeType == NULL || strcmp(nodeType, "FuncDef") != 0)
            continue; // 함수 정의가 아니면 건너뛰기

        // 함수 이름 및 리턴 타입은 decl 필드 안에 있음.
        json_value decl = json_get(item, "decl");
        char *funcName = json_get_string(decl, "name");
        json_value funcType = json_get(decl, "type");
        // funcType 내부의 "type" 필드가 리턴 타입 정보임.
        json_value returnTypeNode = json_get(funcType, "type");
        char returnTypeBuf[100] = "";
        get_type_string_val(returnTypeNode, returnTypeBuf);
        if(strlen(returnTypeBuf) == 0)
            strcpy(returnTypeBuf, "(unknown)");

        // 파라미터 정보: funcType의 "args" 필드가 ParamList 객체로 있음.
        json_value args = json_get(funcType, "args");
        char paramsStr[256] = "";
        if(args.type == JSON_NULL) {
            strcpy(paramsStr, "void");
        } else {
            // args 내부의 "params" 배열
            json_value paramList = json_get(args, "params");
            if(paramList.type != JSON_ARRAY) {
                strcpy(paramsStr, "void");
            } else {
                int paramCount = json_len(paramList);
                for(int j = 0; j < paramCount; j++) {
                    json_value paramItem = json_get(paramList, j);
                    char paramTypeBuf[100] = "";
                    // 각 파라미터의 타입은 "type" 필드
                    json_value paramTypeNode = json_get(paramItem, "type");
                    get_type_string_val(paramTypeNode, paramTypeBuf);
                    if(strlen(paramTypeBuf) == 0)
                        strcpy(paramTypeBuf, "unknown");
                    // 파라미터 이름: "name" 필드
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

        // 함수 본문(body) 내 if 문 개수 카운트
        json_value body = json_get(item, "body");
        int ifCount = count_if_nodes_val(body);

        // 정보 출력
        printf("함수 이름: %s\n", funcName ? funcName : "(noname)");
        printf("  리턴 타입: %s\n", returnTypeBuf);
        printf("  파라미터: (%s)\n", paramsStr);
        printf("  if 문 개수: %d\n\n", ifCount);
    }

    // 4. 메모리 해제
    // json_c.c에서 동적 할당된 메모리 해제 함수(json_free 등)를 사용하면 좋겠으나,
    // 여기서는 간단히 jsonData만 해제 (실제 과제에서는 적절한 메모리 해제를 고려할 것)
    free(jsonData);
    return 0;
}
