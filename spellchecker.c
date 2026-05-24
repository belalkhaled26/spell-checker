#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
typedef struct Node {
    char word[50];
    struct Node *left, *right;
    int height;
} Node;
static int avl_max(int a, int b) { return a > b ? a : b; }
static int avl_height(Node *n) { return n ? n->height : 0; }
static Node *newNode(const char *w) {
    Node *n = (Node *)malloc(sizeof(Node));
    strncpy(n->word, w, 49);
    n->word[49] = '\0';
    n->left = n->right = NULL;
    n->height = 1;
    return n;
}
static int getsize(Node *n) {
    return n ? 1 + getsize(n->left) + getsize(n->right) : 0;
}
static Node *rightRotate(Node *y) {
    Node *x = y->left, *T2 = x->right;
    x->right = y; y->left = T2;
    y->height = avl_max(avl_height(y->left), avl_height(y->right)) + 1;
    x->height = avl_max(avl_height(x->left), avl_height(x->right)) + 1;
    return x;
}
static Node *leftRotate(Node *x) {
    Node *y = x->right, *T2 = y->left;
    y->left = x; x->right = T2;
    x->height = avl_max(avl_height(x->left), avl_height(x->right)) + 1;
    y->height = avl_max(avl_height(y->left), avl_height(y->right)) + 1;
    return y;
}
static int getbalance(Node *n) {
    return n ? avl_height(n->left) - avl_height(n->right) : 0;
}
static Node *insert(Node *node, const char *word) {
    if (!node) return newNode(word);
    int cmp = strcasecmp(node->word, word);
    if (cmp > 0) node->left = insert(node->left, word);
    else if (cmp < 0) node->right = insert(node->right, word);
    else return node;
    node->height = 1 + avl_max(avl_height(node->left), avl_height(node->right));
    int bal = getbalance(node);
    if (bal > 1 && strcasecmp(word, node->left->word) < 0) return rightRotate(node);
    if (bal < -1 && strcasecmp(word, node->right->word) > 0) return leftRotate(node);
    if (bal > 1 && strcasecmp(word, node->left->word) > 0) {
        node->left = leftRotate(node->left); return rightRotate(node);
    }
    if (bal < -1 && strcasecmp(word, node->right->word) < 0) {
        node->right = rightRotate(node->right); return leftRotate(node);
    }
    return node;
}
static Node *avl_search(Node *root, const char *word, Node *prev) {
    if (!root) return prev;
    int cmp = strcasecmp(word, root->word);
    if (cmp == 0) return root;
    prev = root;
    return cmp < 0 ? avl_search(root->left, word, prev) : avl_search(root->right, word, prev);
}
static Node *predecessor(Node *root, const char *word) {
    Node *pred = NULL;
    while (root) {
        if (strcasecmp(word, root->word) > 0) { pred = root; root = root->right; }
        else root = root->left;
    }
    return pred;
}
static Node *successor(Node *root, const char *word) {
    Node *succ = NULL;
    while (root) {
        if (strcasecmp(word, root->word) < 0) { succ = root; root = root->left; }
        else root = root->right;
    }
    return succ;
}
static Node *loadDictionary(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;
    Node *root = NULL;
    char w[50];
    while (fscanf(f, "%49s", w) != EOF)
        root = insert(root, w);
    fclose(f);
    return root;
}
#define MAX_WORDS 256
typedef struct {
    char word[50];
    int correct;
    char pred[50];
    char succ[50];
} WordResult;
static WordResult gResults[MAX_WORDS];
static int gResultCount = 0;
static void checkWord(Node *root, const char *word) {
    if (gResultCount >= MAX_WORDS) return;
    WordResult *r = &gResults[gResultCount++];
    strncpy(r->word, word, 49); r->word[49] = '\0';
    r->pred[0] = r->succ[0] = '\0';
    Node *found = avl_search(root, word, NULL);
    if (found && strcasecmp(found->word, word) == 0) {
        r->correct = 1;
        return;
    }
    r->correct = 0;
    const char *base = (found && found->word[0]) ? found->word : word;
    Node *p = predecessor(root, base);
    Node *s = successor(root, base);
    if (p) strncpy(r->pred, p->word, 49);
    if (s) strncpy(r->succ, s->word, 49);
}
static void checkSentence(Node *root, const char *sentence) {
    gResultCount = 0;
    char buf[512];
    strncpy(buf, sentence, 511); buf[511] = '\0';
    char *tok = strtok(buf, " \t\n\r");
    while (tok) {
        checkWord(root, tok);
        tok = strtok(NULL, " \t\n\r");
    }
}
#define COL_BG      ((Color){13,  15,  22,  255})
#define COL_PANEL   ((Color){22,  26,  38,  255})
#define COL_ACCENT  ((Color){82,  162, 255, 255})
#define COL_GREEN   ((Color){60,  210, 140, 255})
#define COL_RED     ((Color){255, 90,  90,  255})
#define COL_YELLOW  ((Color){255, 200, 80,  255})
#define COL_WHITE   ((Color){225, 232, 248, 255})
#define COL_MUTED   ((Color){105, 118, 150, 255})
#define COL_BORDER  ((Color){44,  52,  76,  255})
#define COL_INPUTBG ((Color){18,  22,  36,  255})
typedef enum { STATE_INPUT = 0, STATE_RESULTS } AppState;
static void FillRR(Rectangle r, float rnd, Color c) {
    DrawRectangleRounded(r, rnd, 8, c);
}
static void LineRR(Rectangle r, float rnd, float thick, Color c) {
    DrawRectangleRoundedLinesEx(r, rnd, 8, thick, c);
}
static int Button(Rectangle r, const char *label, Color bgNorm, Color bgHov, Color textCol, int fontSize) {
    Vector2 mp = GetMousePosition();
    int hov = CheckCollisionPointRec(mp, r);
    FillRR(r, 0.22f, hov ? bgHov : bgNorm);
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)(r.x + (r.width - tw) / 2), (int)(r.y + (r.height - fontSize) / 2), fontSize, textCol);
    return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}
int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(860, 650, "Spell Checker");
    SetTargetFPS(60);
    Node *dictRoot = loadDictionary("Dictionary.txt");
    int dictSize = getsize(dictRoot);
    int dictH = avl_height(dictRoot);
    int dictOK = (dictRoot != NULL);
    char inputBuf[512] = {0};
    int inputLen = 0;
    float scrollY = 0.0f;
    float maxScroll = 0.0f;
    AppState state = STATE_INPUT;
    while (!WindowShouldClose()) {
        int W = GetScreenWidth();
        int H = GetScreenHeight();
        if (state == STATE_INPUT) {
            int ch = GetCharPressed();
            while (ch > 0) {
                if (ch >= 32 && inputLen < 510) {
                    inputBuf[inputLen++] = (char)ch;
                    inputBuf[inputLen] = '\0';
                }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && inputLen > 0)
                inputBuf[--inputLen] = '\0';
            if (IsKeyPressed(KEY_ENTER) && inputLen > 0 && dictOK) {
                checkSentence(dictRoot, inputBuf);
                scrollY = 0;
                state = STATE_RESULTS;
            }
        }
        if (state == STATE_RESULTS) {
            float wheel = GetMouseWheelMove();
            scrollY -= wheel * 40.0f;
            if (scrollY < 0) scrollY = 0;
            if (scrollY > maxScroll) scrollY = maxScroll;
        }
        BeginDrawing();
        ClearBackground(COL_BG);
        Rectangle hdr = {0, 0, (float)W, 58};
        FillRR(hdr, 0, COL_PANEL);
        DrawLine(0, 58, W, 58, COL_BORDER);
        DrawText("SPELL", 20, 15, 26, COL_ACCENT);
        DrawText("CHECK", 20 + MeasureText("SPELL", 26) + 6, 15, 26, COL_WHITE);
        if (dictOK) {
            char s[100];
            snprintf(s, sizeof(s), "%d words  height %d", dictSize, dictH);
            DrawText(s, W - MeasureText(s, 13) - 18, 22, 13, COL_MUTED);
        } else {
            const char *err = "Dictionary.txt not found";
            DrawText(err, W - MeasureText(err, 13) - 18, 22, 13, COL_RED);
        }
        int MARGIN = 36;
        int CW = W - MARGIN * 2;
        if (state == STATE_INPUT) {
            int cy = 88;
            DrawText("Enter a sentence:", MARGIN, cy, 18, COL_WHITE);
            cy += 36;
            Rectangle inputRect = {(float)MARGIN, (float)cy, (float)CW, 52};
            FillRR(inputRect, 0.10f, COL_INPUTBG);
            LineRR(inputRect, 0.10f, 2.0f, COL_ACCENT);
            int showCursor = ((int)(GetTime() * 2) & 1);
            char disp[512];
            strncpy(disp, inputBuf, 511); disp[511] = '\0';
            int avail = CW - 24;
            while (MeasureText(disp, 19) > avail && disp[0])
                memmove(disp, disp + 1, strlen(disp));
            DrawText(disp, MARGIN + 12, cy + 16, 19, COL_WHITE);
            if (showCursor) {
                int cx2 = MARGIN + 12 + MeasureText(disp, 19);
                DrawLine(cx2 + 1, cy + 10, cx2 + 1, cy + 42, COL_ACCENT);
            }
            if (inputLen == 0)
                DrawText("Type your sentence here...", MARGIN + 14, cy + 16, 18, COL_MUTED);
            cy += 62;
            Color btnBg  = dictOK ? (Color){30, 70, 150, 255} : (Color){50, 50, 60, 255};
            Color btnHov = dictOK ? COL_ACCENT : (Color){50, 50, 60, 255};
            Rectangle btnCheck = {(float)MARGIN, (float)cy, 170, 44};
            if (Button(btnCheck, "CHECK", btnBg, btnHov, COL_WHITE, 17) && dictOK) {
                checkSentence(dictRoot, inputBuf);
                scrollY = 0;
                state = STATE_RESULTS;
            }
            DrawText("or press Enter", MARGIN + 185, cy + 14, 14, COL_MUTED);
            cy += 62;
            Rectangle info = {(float)MARGIN, (float)cy, (float)CW, 72};
            FillRR(info, 0.08f, COL_PANEL);
            DrawText("Each word is looked up in the AVL tree dictionary.", MARGIN + 16, cy + 12, 14, COL_MUTED);
            DrawText("Misspelled words show the nearest predecessor and successor from the tree.", MARGIN + 16, cy + 32, 14, COL_MUTED);
            DrawText("The dictionary is loaded once at startup.", MARGIN + 16, cy + 52, 13, (Color){70, 80, 100, 255});
        } else if (state == STATE_RESULTS) {
            int nCorr = 0, nWrong = 0;
            for (int i = 0; i < gResultCount; i++)
                if (gResults[i].correct) nCorr++; else nWrong++;
            Rectangle sumBar = {(float)MARGIN, 68, (float)CW, 40};
            FillRR(sumBar, 0.12f, COL_PANEL);
            char s[120];
            snprintf(s, sizeof(s), "%d word%s  %d correct  %d incorrect",
                gResultCount, gResultCount == 1 ? "" : "s", nCorr, nWrong);
            DrawText(s, MARGIN + 14, 80, 15, COL_MUTED);
            int BTN_H = 44, BTN_W = 180;
            int BTN_Y = H - 58;
            Rectangle btnAgain = {(float)(W / 2 - BTN_W - 10), (float)BTN_Y, (float)BTN_W, (float)BTN_H};
            Rectangle btnExit  = {(float)(W / 2 + 10), (float)BTN_Y, (float)BTN_W, (float)BTN_H};
            DrawLine(0, BTN_Y - 12, W, BTN_Y - 12, COL_BORDER);
            if (Button(btnAgain, "Enter Again", (Color){25, 65, 130, 255}, COL_ACCENT, COL_WHITE, 17)) {
                inputBuf[0] = '\0';
                inputLen = 0;
                gResultCount = 0;
                scrollY = 0;
                state = STATE_INPUT;
            }
            if (Button(btnExit, "Exit", (Color){80, 25, 25, 255}, COL_RED, COL_WHITE, 17))
                break;
            int LIST_TOP = 116;
            int LIST_BOT = BTN_Y - 20;
            int LIST_H = LIST_BOT - LIST_TOP;
            int CARD_BASE = 66;
            int totalContentH = 0;
            for (int i = 0; i < gResultCount; i++) {
                int extra = 0;
                if (!gResults[i].correct) {
                    if (gResults[i].pred[0]) extra += 22;
                    if (gResults[i].succ[0]) extra += 22;
                }
                totalContentH += CARD_BASE + extra + 10;
            }
            maxScroll = (float)(totalContentH - LIST_H);
            if (maxScroll < 0) maxScroll = 0;
            if (scrollY > maxScroll) scrollY = maxScroll;
            BeginScissorMode(0, LIST_TOP, W, LIST_H);
            int cardY = LIST_TOP - (int)scrollY;
            for (int i = 0; i < gResultCount; i++) {
                WordResult *r = &gResults[i];
                int extra = 0;
                if (!r->correct) {
                    if (r->pred[0]) extra += 22;
                    if (r->succ[0]) extra += 22;
                }
                int thisH = CARD_BASE + extra;
                if (cardY + thisH < LIST_TOP || cardY > LIST_BOT) {
                    cardY += thisH + 10;
                    continue;
                }
                Rectangle card = {(float)MARGIN, (float)cardY, (float)CW, (float)thisH};
                Color cardBg = r->correct ? (Color){14, 36, 26, 255} : (Color){38, 16, 16, 255};
                FillRR(card, 0.09f, cardBg);
                Color stripe = r->correct ? COL_GREEN : COL_RED;
                DrawRectangle(MARGIN, cardY + 6, 4, thisH - 12, stripe);
                DrawText(r->word, MARGIN + 18, cardY + 12, 21, COL_WHITE);
                const char *tag = r->correct ? "CORRECT" : "INCORRECT";
                Color tagCol = r->correct ? COL_GREEN : COL_RED;
                int tw = MeasureText(tag, 13);
                Rectangle badge = {(float)(MARGIN + CW - tw - 28), (float)(cardY + 11), (float)(tw + 20), 24};
                FillRR(badge, 0.45f, (Color){tagCol.r, tagCol.g, tagCol.b, 40});
                DrawText(tag, (int)badge.x + 10, cardY + 14, 13, tagCol);
                if (!r->correct) {
                    int sy = cardY + 40;
                    if (r->pred[0]) {
                        char buf[80];
                        snprintf(buf, sizeof(buf), "Closest before: %s", r->pred);
                        DrawText(buf, MARGIN + 18, sy, 13, COL_YELLOW);
                        sy += 22;
                    }
                    if (r->succ[0]) {
                        char buf[80];
                        snprintf(buf, sizeof(buf), "Closest after: %s", r->succ);
                        DrawText(buf, MARGIN + 18, sy, 13, COL_YELLOW);
                    }
                }
                cardY += thisH + 10;
            }
            EndScissorMode();
            if (totalContentH > LIST_H) {
                float ratio = (float)LIST_H / totalContentH;
                float barH = ratio * LIST_H;
                float barT = (scrollY / (float)totalContentH) * LIST_H + LIST_TOP;
                DrawRectangle(W - 6, (int)barT, 4, (int)barH, COL_BORDER);
                DrawRectangle(W - 6, (int)barT, 4, (int)barH, (Color){COL_ACCENT.r, COL_ACCENT.g, COL_ACCENT.b, 160});
            }
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
