#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの型を表す値
enum {
    TK_NUM = 256,   // 整数トークン
    TK_EOF,         // 入力の終わりを表すトークン
};

// トークンの型
typedef struct {
    int ty;         // token type
    int val;        // If ty is TK_NUM, it represents the number
    char *input;    // token strings(for error)
} Token;

// Input program
char *user_input;

// トークナイズした結果のトークン列はこの配列に保存する
// 100個以上のトークンはこないものとする
Token tokens[100];

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー個所を報告するための関数
void error_at(char *loc, char *msg) {
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ %s\n", msg);
    exit(1);
}

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void tokenize() {
    char *p = user_input;

    int i = 0;
    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            tokens[i].ty = *p;
            tokens[i].input = p;
            i++;
            p++;
            continue;
        }

        if (isdigit(*p)) {
            tokens[i].ty = TK_NUM;
            tokens[i].input = p;
            tokens[i].val = strtol(p, &p, 10);
            i++;
            continue;
        }

        error_at(p, "トークナイズできません。");
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}

enum {
    ND_NUM = 256,       // 整数のノードの型
};

typedef struct Node {
    int ty;             // 演算子かND_NUM
    struct Node *lhs;   // 左辺
    struct Node *rhs;   // 右辺
    int val;            // tyがND_NUMの場合のみ使う
} Node;

int pos = 0;            // 次に読み込むトークン列の位置

Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

int consume(int ty) {
    if (tokens[pos].ty != ty) {
        return 0;
    }
    pos++;
    return 1;
}

Node *expr();
Node *mul();
Node *term();

Node *expr() {
    Node *node = mul();

    while (1) {
        if (consume('+')) {
            node = new_node('+', node, mul());
        } else if (consume('-')) {
            node = new_node('-', node, mul());
        } else {
            return node;
        }
    }
}


Node *mul() {
    Node *node = term();

    while (1) {
        if (consume('*')) {
            node = new_node('*', node, term());
        } else if (consume('/')) {
            node = new_node('/', node, term());
        } else {
            return node;
        }
    }
}

Node *term() {
    // 次のトークンが'('なら、"(" expr ")" のはず
    if (consume('(')) {
        Node *node = expr();
        if (!consume(')')) {
            error_at(tokens[pos].input, "開き括弧に対応する閉じ括弧がありません。");
        }
        return node;
    }

    // そうでなければ数値のはず
    if (tokens[pos].ty == TK_NUM) {
        return new_node_num(tokens[pos++].val);
    }

    error_at(tokens[pos].input, "数値でも開き括弧でもないトークンです。");
}


int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    // トークナイズする
    user_input = argv[1];
    tokenize();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 式の最初は和でなければならないので、それをチェックして
    // 最初のmov命令を出力
    if (tokens[0].ty != TK_NUM) {
        error_at(tokens[0].input, "数ではありません");
    }
    printf("  mov rax, %d\n", tokens[0].val);

    // `+ <数>`あるいは`- <数>`というトークンの並びを消費しつつ
    // アセンブリを出力
    int i = 1;
    while (tokens[i].ty != TK_EOF) {
        if (tokens[i].ty == '+') {
            i++;
            if (tokens[i].ty != TK_NUM) {
                error_at(tokens[i].input, "数ではありません");
            }
            printf("  add rax, %d\n", tokens[i].val);
            i++;
            continue;
        }

        if (tokens[i].ty == '-') {
            i++;
            if (tokens[i].ty != TK_NUM) {
                error_at(tokens[i].input, "数ではありません");
            }
            printf("  sub rax, %d\n", tokens[i].val);
            i++;
            continue;
        }

        error_at(tokens[i].input, "予期しないトークンです");
    }

    printf("  ret\n");

    return 0;
}
