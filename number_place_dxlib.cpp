#include "DxLib.h"
#include <stdio.h>
#include <cstdlib>
#include <ctime>

int scene = 0;
/*
現在のシーン番号
0:メニュー画面
1:問題画面
2:クリア画面
3:ポーズ画面
*/

int start_time = 0; //開始時間
int elapsed_time = 0;   //経過時間
bool is_paused = false; //一時停止フラグ
int pause_start_time = 0;   //一時停止の開始時間

const int GRID_SIZE = 9;     // グリッドのサイズ
const int CELL_SIZE = 40;    // 各マスのサイズ
int answer[GRID_SIZE][GRID_SIZE];     // 完成したナンプレ
int puzzle[GRID_SIZE][GRID_SIZE];     // 回答用グリッド
int memoGrid[GRID_SIZE][GRID_SIZE][GRID_SIZE] = { {{0}} };  // 各グリッドに最大9つのメモを保持
int currentX = 0, currentY = 0;       // 現在選択中のマスの位置
bool isFixed[GRID_SIZE][GRID_SIZE];   // 初期値として固定されているマス
bool memoMode = false;  // メモモードのON/OFFを管理するフラグ

// キーの状態を取得する配列
char keyState[256];  // 現在のキーの状態
char prevKeyState[256];  // 前フレームのキーの状態

// 前フレームのマウス入力状態を保存する変数
int prevMouseInput = 0;

// ボタンの位置とサイズ
struct Button {
    int x, y, width, height;
    const char* label;
};

Button buttons[4] = {
    {CELL_SIZE, CELL_SIZE * 14, (CELL_SIZE * 1.5), CELL_SIZE, "memo"},
    {(CELL_SIZE * 3.5), CELL_SIZE * 14, (CELL_SIZE * 1.5), CELL_SIZE, "erase"},
    {CELL_SIZE * 6, CELL_SIZE * 14, (CELL_SIZE * 1.5), CELL_SIZE, "automemo"},
    {(CELL_SIZE * 8.5), CELL_SIZE * 14, (CELL_SIZE * 1.5), CELL_SIZE, "fill"}
};

//表記順の都合上必要なプロトタイプ宣言
bool IsNumberPossible(int y, int x, int num);

//------------------------------------ここから初期状態の設定------------------------------------
// ナンプレ完成図の作成
void GenerateCompletedGrid() {
    srand((unsigned int)time(NULL));
    int tmp_answer[GRID_SIZE][GRID_SIZE] = {
        {5, 1, 3, 4, 8, 9, 7, 2, 6},
        {6, 9, 4, 7, 5, 2, 3, 1, 8},
        {8, 7, 2, 3, 1, 6, 4, 9, 5},
        {4, 6, 9, 2, 3, 8, 5, 7, 1},
        {2, 8, 1, 5, 4, 7, 9, 6, 3},
        {7, 3, 5, 6, 9, 1, 8, 4, 2},
        {9, 2, 6, 8, 7, 3, 1, 5, 4},
        {1, 5, 8, 9, 2, 4, 6, 3, 7},
        {3, 4, 7, 1, 6, 5, 2, 8, 9}
    };

    //行のランダム入れ替え
    for (int block = 0; block < 3; block++) {
        int row1 = rand() % 3 + block * 3;
        int row2 = rand() % 3 + block * 3;
        if (row1 != row2) {
            for (int col = 0; col < GRID_SIZE; col++) {
                int w = tmp_answer[row1][col];
                tmp_answer[row1][col] = tmp_answer[row2][col];
                tmp_answer[row2][col] = w;
            }
        }
    }

    //列のランダム入れ替え
    for (int block = 0; block < 3; block++) {
        int col1 = rand() % 3 + block * 3;
        int col2 = rand() % 3 + block * 3;
        if (col1 != col2) {
            for (int row = 0; row < GRID_SIZE; row++) {
                int w = tmp_answer[row][col1];
                tmp_answer[row][col1] = tmp_answer[row][col2];
                tmp_answer[row][col2] = w;
            }
        }
    }

    //行ブロック全体の入れ替え
    int block1 = rand() % 3;
    int block2 = rand() % 3;
    if (block1 != block2) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < GRID_SIZE; col++) {
                int w = tmp_answer[block1 * 3 + row][col];
                tmp_answer[block1 * 3 + row][col] = tmp_answer[block2 * 3 + row][col];
                tmp_answer[block2 * 3 + row][col] = w;
            }
        }
    }

    //列ブロック全体の入れ替え
    block1 = rand() % 3;
    block2 = rand() % 3;
    if (block1 != block2) {
        for (int col = 0; col < 3; col++) {
            for (int row = 0; row < GRID_SIZE; row++) {
                int w = tmp_answer[row][block1 * 3 + col];
                tmp_answer[row][block1 * 3 + col] = tmp_answer[row][block2 * 3 + col];
                tmp_answer[row][block2 * 3 + col] = w;
            }
        }
    }

    // answerに完成図をコピー
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            answer[y][x] = tmp_answer[y][x];
        }
    }
}

// 回答用のグリッドを作成
void GeneratePuzzleGrid() {
    srand((unsigned)time(NULL));

    // まずはすべてのグリッドをコピー
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            puzzle[y][x] = answer[y][x];
            isFixed[y][x] = false;  // すべてのマスを未固定に初期化
        }
    }

    // 26個のランダムなマスを残し、それ以外を0にして非表示にする
    int cellsToRemove = 81 - 26;  // 削除するマスの数
    while (cellsToRemove > 0) {
        int randX = rand() % GRID_SIZE;
        int randY = rand() % GRID_SIZE;

        if (puzzle[randY][randX] != 0) {
            puzzle[randY][randX] = 0;  // このマスを空にする
            cellsToRemove--;
        }
    }
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (puzzle[y][x] != 0) {
                isFixed[y][x] = true;   //  このマスを固定する
            }
        }
    }
}

//------------------------------------ここから描画関係------------------------------------
// メニュー画面の描画
void DrawMenuScreen() {
    DrawString(CELL_SIZE * 2, CELL_SIZE * 5, "Number Place", GetColor(0, 0, 0));

    // ボタンの描画
    DrawBox(CELL_SIZE * 1, CELL_SIZE * 10, CELL_SIZE * 5, CELL_SIZE * 12, GetColor(0, 0, 0), TRUE);
    DrawString(CELL_SIZE * 2, CELL_SIZE * 10 + 30, "NewGame", GetColor(255, 255, 255));

    DrawBox(CELL_SIZE * 6, CELL_SIZE * 10, CELL_SIZE * 10, CELL_SIZE * 12, GetColor(0, 0, 0), TRUE);
    DrawString(CELL_SIZE * 7, CELL_SIZE * 10 + 30, "Quit", GetColor(255, 255, 255));
}

//タイマーの描画
void DrawTimer() {
    if (!is_paused) {
        elapsed_time = GetNowCount() - start_time;  //経過時間の更新
    }
    int seconds = (elapsed_time / 1000) % 60;
    int minutes = (elapsed_time / 1000) / 60;

    char timerText[64];
    sprintf_s(timerText, "%02d:%02d", minutes, seconds);
    DrawString(CELL_SIZE * 8, CELL_SIZE * 4 - 20, timerText, GetColor(0, 0, 0));  // 時間を描画
}

//ポーズボタンの描画
void DrawPauseButton() {
    DrawCircle(CELL_SIZE * 9.5, CELL_SIZE * 4 - 15, CELL_SIZE / 4, GetColor(0, 0, 0), 0);
    DrawBox(CELL_SIZE * 9 + 15, CELL_SIZE * 4 - 21, CELL_SIZE * 9 + 19, CELL_SIZE * 4 - 8, GetColor(0, 0, 0), TRUE);
    DrawBox(CELL_SIZE * 9 + 22, CELL_SIZE * 4 - 21, CELL_SIZE * 9 + 26, CELL_SIZE * 4 - 8, GetColor(0, 0, 0), TRUE);
}

//メモの描画
void DrawMemo(int y, int x) {
    int cellX = (x + 1) * CELL_SIZE;
    int cellY = (y + 4) * CELL_SIZE;

    for (int i = 0; i < 9; i++) {
        if (memoGrid[y][x][i] != 0) {
            // 各数字の位置を計算して、メモを描画
            int memoX = cellX + (i % 3) * (CELL_SIZE / 3) + 5;
            int memoY = cellY + (i / 3) * (CELL_SIZE / 3);
            DrawFormatString(memoX, memoY, GetColor(128, 128, 128), "%d", memoGrid[y][x][i]);
        }
    }
}

// グリッドの描画
void DrawGrid() {
    for (int y = 0; y <= GRID_SIZE; y++) {
        for (int x = 0; x <= GRID_SIZE; x++) {
            // 線を描く
            DrawLine((x + 1) * CELL_SIZE, 4 * CELL_SIZE, (x + 1) * CELL_SIZE, (GRID_SIZE + 4) * CELL_SIZE, GetColor(0, 0, 0));
            DrawLine(CELL_SIZE, (y + 4) * CELL_SIZE, (GRID_SIZE + 1) * CELL_SIZE, (y + 4) * CELL_SIZE, GetColor(0, 0, 0));
            if (x % 3 == 0 && y % 3 == 0) {
                DrawLine((x + 1) * CELL_SIZE - 1, 4 * CELL_SIZE, (x + 1) * CELL_SIZE - 1, (GRID_SIZE + 4) * CELL_SIZE, GetColor(0, 0, 0));
                DrawLine((x + 1) * CELL_SIZE + 1, 4 * CELL_SIZE, (x + 1) * CELL_SIZE + 1, (GRID_SIZE + 4) * CELL_SIZE, GetColor(0, 0, 0));
                DrawLine(CELL_SIZE, (y + 4) * CELL_SIZE - 1, (GRID_SIZE + 1) * CELL_SIZE, (y + 4) * CELL_SIZE - 1, GetColor(0, 0, 0));
                DrawLine(CELL_SIZE, (y + 4) * CELL_SIZE + 1, (GRID_SIZE + 1) * CELL_SIZE, (y + 4) * CELL_SIZE + 1, GetColor(0, 0, 0));
            }
        }
    }

    // マス内の数字またはメモを描画
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (isFixed[y][x]) {
                DrawFormatString((x + 1) * CELL_SIZE + 15, (y + 4) * CELL_SIZE + 10, GetColor(0, 0, 0), "%d", puzzle[y][x]);
            }
            else if (puzzle[y][x] != 0) {
                DrawFormatString((x + 1) * CELL_SIZE + 15, (y + 4) * CELL_SIZE + 10, GetColor(255, 0, 0), "%d", puzzle[y][x]);
            }
            else if (memoGrid[y][x] != 0) {
                DrawMemo(y, x);  // メモを描画
            }
        }
    }

    // 選択中のマスを視覚的に強調
    DrawBox((currentX + 1) * CELL_SIZE, (currentY + 4) * CELL_SIZE, (currentX + 2) * CELL_SIZE, (currentY + 5) * CELL_SIZE, GetColor(255, 0, 0), FALSE);
}

// 4つのボタンの描画
void DrawButtons() {
    for (int i = 0; i < 4; i++) {
        int color = (i == 0 && memoMode) ? GetColor(128, 128, 128) : GetColor(0, 0, 0);  // メモモード時はグレー
        DrawBox(buttons[i].x, buttons[i].y, buttons[i].x + buttons[i].width, buttons[i].y + buttons[i].height, color, TRUE);
        DrawString(buttons[i].x + 10, buttons[i].y + 10, buttons[i].label, GetColor(255, 255, 255));
    }
}

// 1から9の数字の描画
void DrawNumbers() {
    if (!memoMode) {
        for (int i = 1; i <= 9; i++) {
            DrawFormatString((i + 0.5) * CELL_SIZE, CELL_SIZE * 16, GetColor(0, 0, 0), "%d", i);
        }
    }
    else {
        for (int i = 1; i <= 9; i++) {
            DrawFormatString((i + 0.5) * CELL_SIZE, CELL_SIZE * 16, GetColor(128, 128, 128), "%d", i);
        }
    }
}

//終了画面の描画
void DrawClearScreen() {
    DrawString(CELL_SIZE * 2, CELL_SIZE * 5, "Clear! Congratulations!", GetColor(0, 0, 0));

    char clearTimeStr[256];
    sprintf_s(clearTimeStr, "Clear Time : %2d:%2d", ((elapsed_time / 1000) / 60), ((elapsed_time / 1000) % 60));
    DrawString(CELL_SIZE * 2, CELL_SIZE * 6, clearTimeStr, GetColor(0, 0, 0));

    // ボタンの描画
    DrawBox(CELL_SIZE * 1, CELL_SIZE * 10, CELL_SIZE * 5, CELL_SIZE * 12, GetColor(0, 0, 0), TRUE);
    DrawString(CELL_SIZE * 2 - 20, CELL_SIZE * 10 + 30, "Back to menu", GetColor(255, 255, 255));

    DrawBox(CELL_SIZE * 6, CELL_SIZE * 10, CELL_SIZE * 10, CELL_SIZE * 12, GetColor(0, 0, 0), TRUE);
    DrawString(CELL_SIZE * 7, CELL_SIZE * 10 + 30, "Quit", GetColor(255, 255, 255));
}

// ポーズ画面の描画
void DrawPauseScreen() {
    DrawString(CELL_SIZE * 2, CELL_SIZE * 5, "Pause", GetColor(0, 0, 0));

    // ボタンの描画
    DrawBox(CELL_SIZE * 1, CELL_SIZE * 10, CELL_SIZE * 5, CELL_SIZE * 12, GetColor(0, 0, 0), TRUE);
    DrawString(CELL_SIZE * 2, CELL_SIZE * 10 + 30, "ReStart", GetColor(255, 255, 255));

    DrawBox(CELL_SIZE * 6, CELL_SIZE * 10, CELL_SIZE * 10, CELL_SIZE * 12, GetColor(0, 0, 0), TRUE);
    DrawString(CELL_SIZE * 7, CELL_SIZE * 10 + 30, "Quit", GetColor(255, 255, 255));
}

//------------------------------------ここから入力関係------------------------------------
// キー入力で数字を入力
void InputNumber() {
    int key = GetInputChar(TRUE);
    int num = -1;    //入力された数字を保持

    if (key >= '1' && key <= '9') {
        // 固定されていないマスに入力する
        if (!isFixed[currentY][currentX]) {
            num = key - '0';
        }
    }

    //テンキー入力
    if (keyState[KEY_INPUT_NUMPAD1] && prevKeyState[KEY_INPUT_NUMPAD1] == 0 && !isFixed[currentY][currentX]) {
        num = 1;
    }
    if (keyState[KEY_INPUT_NUMPAD2] && prevKeyState[KEY_INPUT_NUMPAD2] == 0 && !isFixed[currentY][currentX]) {
        num = 2;
    }
    if (keyState[KEY_INPUT_NUMPAD3] && prevKeyState[KEY_INPUT_NUMPAD3] == 0 && !isFixed[currentY][currentX]) {
        num = 3;
    }
    if (keyState[KEY_INPUT_NUMPAD4] && prevKeyState[KEY_INPUT_NUMPAD4] == 0 && !isFixed[currentY][currentX]) {
        num = 4;
    }
    if (keyState[KEY_INPUT_NUMPAD5] && prevKeyState[KEY_INPUT_NUMPAD5] == 0 && !isFixed[currentY][currentX]) {
        num = 5;
    }
    if (keyState[KEY_INPUT_NUMPAD6] && prevKeyState[KEY_INPUT_NUMPAD6] == 0 && !isFixed[currentY][currentX]) {
        num = 6;
    }
    if (keyState[KEY_INPUT_NUMPAD7] && prevKeyState[KEY_INPUT_NUMPAD7] == 0 && !isFixed[currentY][currentX]) {
        num = 7;
    }
    if (keyState[KEY_INPUT_NUMPAD8] && prevKeyState[KEY_INPUT_NUMPAD8] == 0 && !isFixed[currentY][currentX]) {
        num = 8;
    }
    if (keyState[KEY_INPUT_NUMPAD9] && prevKeyState[KEY_INPUT_NUMPAD9] == 0 && !isFixed[currentY][currentX]) {
        num = 9;
    }

    if(num != -1){
        if (memoMode) {
            // メモモード時、選択中のマスにメモを追加
            if (!isFixed[currentY][currentX]) {
                puzzle[currentY][currentX] = 0; //通常入力をクリア
                if (memoGrid[currentY][currentX][num - 1] == 0) {
                    memoGrid[currentY][currentX][num - 1] = num;
                }
                else {
                    memoGrid[currentY][currentX][num - 1] = 0;
                }
            }
        }
        else {
            // 通常モード時、数字を入力し、メモをクリア
            if (!isFixed[currentY][currentX]) {
                puzzle[currentY][currentX] = num;
                for (int i = 0; i < 9; i++) {
                    memoGrid[currentY][currentX][i] = 0;  // メモをクリア
                }
            }
        }
    }
}

// 矢印キーでカーソルを移動する関数
void MoveCursor() {
    // 現在のキーの状態を取得
    GetHitKeyStateAll(keyState);

    // 上キーが押された瞬間を検知
    if (keyState[KEY_INPUT_UP] == 1 && prevKeyState[KEY_INPUT_UP] == 0) {
        currentY--;
        if (currentY < 0) {
            currentY = GRID_SIZE - 1;
        }
    }
    // 下キーが押された瞬間を検知
    if (keyState[KEY_INPUT_DOWN] == 1 && prevKeyState[KEY_INPUT_DOWN] == 0) {
        currentY++;
        if (currentY > GRID_SIZE - 1) {
            currentY = 0;
        }
    }
    // 左キーが押された瞬間を検知
    if (keyState[KEY_INPUT_LEFT] == 1 && prevKeyState[KEY_INPUT_LEFT] == 0) {
        currentX--;
        if (currentX < 0) {
            currentX = GRID_SIZE - 1;
        }
    }
    // 右キーが押された瞬間を検知
    if (keyState[KEY_INPUT_RIGHT] == 1 && prevKeyState[KEY_INPUT_RIGHT] == 0) {
        currentX++;
        if (currentX > GRID_SIZE - 1) {
            currentX = 0;
        }
    }

    // 前フレームの状態を更新
    for (int i = 0; i < 256; i++) {
        prevKeyState[i] = keyState[i];
    }
}

// クリックでカーソルを変更する関数
void CheckGridClick() {
    int mouseX, mouseY;
    int mouseInput = GetMouseInput();  // マウス入力取得
    GetMousePoint(&mouseX, &mouseY);   // マウス座標取得

    // 左クリックされたとき
    if (mouseInput & MOUSE_INPUT_LEFT) {
        // クリックされた座標がグリッド内かどうかを確認
        if (mouseX >= CELL_SIZE && mouseX <= (GRID_SIZE + 1) * CELL_SIZE &&
            mouseY >= 4 * CELL_SIZE && mouseY <= (GRID_SIZE + 4) * CELL_SIZE) {

            // マウスの座標をグリッドの座標に変換
            currentX = (mouseX / CELL_SIZE) - 1;
            currentY = (mouseY / CELL_SIZE) - 4;
        }
    }
}

//全マスにメモを入力
static void FillMemo() {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (!isFixed[y][x]) {
                puzzle[y][x] = 0;   //puzzle初期化
            }
        }
    }
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (!isFixed[y][x]) {
                for (int i = 0; i < 9; i++) {
                    memoGrid[y][x][i] = 0;  //メモの初期化
                    if (IsNumberPossible(y, x, i + 1)) {
                        memoGrid[y][x][i] = i + 1;
                    }
                }
            }
        }
    }
}

//消せるメモを消去
void EraceMemo(int y, int x, int num){
    if (num > 0 && num < 10 ) {
        for (int i = 0; i < 9; i++) {
            if (memoGrid[i][x][num - 1] != 0)  memoGrid[i][x][num - 1] = 0;   //縦列の走査
            if (memoGrid[y][i][num - 1] != 0)  memoGrid[y][i][num - 1] = 0;   //横列の走査
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (memoGrid[(y / 3) * 3 + i][(x / 3) * 3 + j][num - 1] != 0) memoGrid[(y / 3) * 3 + i][(x / 3) * 3 + j][num - 1] = 0;   //ブロックの走査
            }
        }
    }
}

//------------------------------------ここから判定関係------------------------------------
//メニュー画面で押されたボタンに応じた処理
void CheckMenuButtonClick() {
    int mouseX, mouseY;
    int mouseInput = GetMouseInput();
    GetMousePoint(&mouseX, &mouseY);

    // 「Start」ボタンがクリックされた
    if ((mouseInput & MOUSE_INPUT_LEFT) == 0 && (prevMouseInput & MOUSE_INPUT_LEFT)) {
        if (mouseX >= CELL_SIZE * 1 && mouseX <= CELL_SIZE * 5 &&
            mouseY >= CELL_SIZE * 10 && mouseY <= CELL_SIZE * 12) {
            scene = 1;  //ゲーム画面に遷移
            GenerateCompletedGrid();    //完成盤面の作成
            GeneratePuzzleGrid();   //問題画面の作成
            start_time = GetNowCount(); //開始時間を記録
        }

        // 「Quit」ボタンがクリックされた
        if (mouseX >= CELL_SIZE * 6 && mouseX <= CELL_SIZE * 10 &&
            mouseY >= CELL_SIZE * 10 && mouseY <= CELL_SIZE * 12) {
            // プログラムを終了
            DxLib_End();
        }
    }
    prevMouseInput = mouseInput;  // マウスの状態を保存
}

//puzzle[y][x]に数字numが入る可能性があるかを判定する関数
bool IsNumberPossible(int y, int x, int num) {
    for (int i = 0; i < GRID_SIZE; i++) {
        if (puzzle[i][x] == num)   return false;  //縦列の走査
        if (puzzle[y][i] == num)   return false;  //横列の走査
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (puzzle[(y / 3) * 3 + i][(x / 3) * 3 + j] == num)   return false;  //3*3ブロックの走査
        }
    }
    return true;
}

// 問題画面で押されたボタンに応じた処理
void CheckButtonClick() {
    int mouseX, mouseY;
    int mouseInput = GetMouseInput();  // マウス入力取得
    GetMousePoint(&mouseX, &mouseY);   // マウス座標取得

    // マウスボタンが押されなくなった瞬間を検出
    if ((mouseInput & MOUSE_INPUT_LEFT) == 0 && (prevMouseInput & MOUSE_INPUT_LEFT)) {
        // ポーズボタンの処理
        // 円の中心の座標
        float centerX = CELL_SIZE * 9.5;
        float centerY = CELL_SIZE * 4 - 15;
        float radius = CELL_SIZE / 4;

        // マウス座標と円の中心との距離を計算
        float distance = sqrt(pow(mouseX - centerX, 2) + pow(mouseY - centerY, 2));

        // 距離が半径以内ならクリックされたとみなす
        if (distance <= radius) {
            // ポーズ
            is_paused = !is_paused; //ポーズチェックの反転
            scene = 3;  //シーンを4(ポーズ画面)に変更
            pause_start_time = GetNowCount();
        }
        //数字ボタンの処理
        for (int i = 1; i <= 9; i++) {
            int numX = i * CELL_SIZE;
            int numY = CELL_SIZE * 16;

            // クリックされた座標が数字の位置かどうかを確認
            if (mouseX >= numX && mouseX <= numX + CELL_SIZE &&
                mouseY >= numY && mouseY <= numY + CELL_SIZE) {

                if (memoMode) {
                    // メモモード時、数字をクリアし、メモを追加(メモが記入されている場合メモをクリア)
                    if (!isFixed[currentY][currentX]) {
                        puzzle[currentY][currentX] = 0;
                        if (memoGrid[currentY][currentX][i - 1] == 0) {
                            memoGrid[currentY][currentX][i - 1] = i;
                        }
                        else {
                            memoGrid[currentY][currentX][i - 1] = 0;
                        }
                    }
                }
                else {
                    // 通常モード時、数字を入力し、メモをクリア
                    if (!isFixed[currentY][currentX]) {
                        puzzle[currentY][currentX] = i;
                        for (int i = 0; i < 9; i++) {
                            memoGrid[currentY][currentX][i] = 0;
                        }
                    }
                }
            }
        }
        //4ボタンの処理
        if (mouseX >= buttons[0].x && mouseX <= buttons[0].x + buttons[0].width &&
            mouseY >= buttons[0].y && mouseY <= buttons[0].y + buttons[0].height) {
            // ボタン「memo」がクリックされたときの処理
            memoMode = !memoMode;  // メモモードをトグル
        }
        else if (mouseX >= buttons[1].x && mouseX <= buttons[1].x + buttons[1].width &&
            mouseY >= buttons[1].y && mouseY <= buttons[1].y + buttons[1].height) {
            // ボタン「erace」がクリックされたときの処理
            if (!isFixed[currentY][currentX]) {
                puzzle[currentY][currentX] = 0; //通常入力値をクリア
                for (int i = 0; i < GRID_SIZE; i++) {
                    memoGrid[currentY][currentX][i] = 0;    //メモをクリア
                }
            }
        }
        else if (mouseX >= buttons[2].x && mouseX <= buttons[2].x + buttons[2].width &&
            mouseY >= buttons[2].y && mouseY <= buttons[2].y + buttons[2].height) {
            // ボタンが「auto memo」クリックされたときの処理
            FillMemo();
        }
        else if (mouseX >= buttons[3].x && mouseX <= buttons[3].x + buttons[3].width &&
            mouseY >= buttons[3].y && mouseY <= buttons[3].y + buttons[3].height) {
            // ボタン「fill」がクリックされたときの処理
            if (!isFixed[currentY][currentX]) {
                puzzle[currentY][currentX] = answer[currentY][currentX];
            }
            memoMode = false;   //メモモードをオフ
        }
    }
    prevMouseInput = mouseInput;  // マウスの状態を保存
}

// 答えの確認
void CheckAnswer() {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (puzzle[y][x] != 0 && puzzle[y][x] != answer[y][x]) {
                DrawFormatString((x + 1) * CELL_SIZE + 15, (y + 4) * CELL_SIZE + 10, GetColor(255, 0, 0), "%d", puzzle[y][x]);  // 赤で表示
            }
            else if (puzzle[y][x] != 0 && puzzle[y][x] == answer[y][x]) {
                isFixed[y][x] = true;   //グリッドを固定
                EraceMemo(y, x, puzzle[y][x]);//メモの消去
            }
        }
    }
}

//ナンプレを解けたか確認
bool CheckClear() {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (puzzle[y][x] != answer[y][x]) {
                return false;  // 一致していないマスがあればクリアしていない
            }
        }
    }
    return true;  // 全て一致していればクリア
}

//終了画面で押されたボタンに応じた処理
void CheckClearButtonClick() {
    int mouseX, mouseY;
    int mouseInput = GetMouseInput();
    GetMousePoint(&mouseX, &mouseY);

    // マウスボタンが押されていて、かつ前フレームでは押されていなかった場合
    if (!(mouseInput & MOUSE_INPUT_LEFT) && (prevMouseInput & MOUSE_INPUT_LEFT)) {
        // 「Back to menu」ボタンがクリックされた
        if (mouseX >= CELL_SIZE * 1 && mouseX <= CELL_SIZE * 5 &&
            mouseY >= CELL_SIZE * 10 && mouseY <= CELL_SIZE * 12) {
            // シーン番号を0にする
            scene = 0;
        }

        // 「Quit」ボタンがクリックされた
        if (mouseX >= CELL_SIZE * 6 && mouseX <= CELL_SIZE * 10 &&
            mouseY >= CELL_SIZE * 10 && mouseY <= CELL_SIZE * 12) {
            // プログラムを終了
            DxLib_End();
        }
    }

    // 現在のマウス状態を次のフレーム用に保存
    prevMouseInput = mouseInput;
}

//ポーズ画面で押されたボタンに応じた処理
void CheckPauseButtonClick() {
    int mouseX, mouseY;
    int mouseInput = GetMouseInput();
    GetMousePoint(&mouseX, &mouseY);

    // マウスボタンが押されていて、かつ前フレームでは押されていなかった場合
    if (!(mouseInput & MOUSE_INPUT_LEFT) && (prevMouseInput & MOUSE_INPUT_LEFT)) {
        // 「ReStart」ボタンがクリックされた
        if (mouseX >= CELL_SIZE * 1 && mouseX <= CELL_SIZE * 5 &&
            mouseY >= CELL_SIZE * 10 && mouseY <= CELL_SIZE * 12) {
            // シーン番号を1にする
            start_time += GetNowCount() - pause_start_time; //開始時間の更新
            is_paused = !is_paused; //ポーズチェックの反転
            scene = 1;
        }

        // 「Quit」ボタンがクリックされた
        if (mouseX >= CELL_SIZE * 6 && mouseX <= CELL_SIZE * 10 &&
            mouseY >= CELL_SIZE * 10 && mouseY <= CELL_SIZE * 12) {
            // プログラムを終了
            DxLib_End();
        }
    }

    // 現在のマウス状態を次のフレーム用に保存
    prevMouseInput = mouseInput;
}

//------------------------------------メイン関数------------------------------------
// ゲームのメインループ
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE);  // ウィンドウモードに変更
    // ウィンドウサイズを縦長に設定（例: 720x1280）
    SetGraphMode(720 * 0.6, 1280 * 0.6, 32);  // 幅720、高さ1280、32ビットカラー
    // メインウインドウのバックグラウンドカラーを設定する(各色0～255)
    SetBackgroundColor(255, 255, 255);  //背景色を白に
    if (DxLib_Init() == -1) return -1;  // DXライブラリの初期化に失敗した場合は終了

    SetDrawScreen(DX_SCREEN_BACK);  // バックバッファを使用

    GetHitKeyStateAll(prevKeyState);    //前フレームのキーの状態を初期化

    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0) {
        ClearDrawScreen();

        // シーン番号に応じてシーン切り替え
        if (scene == 0) {
            DrawMenuScreen();   //メニュー画面の描画
            CheckMenuButtonClick(); //メニュー画面でのクリック処理
        }
        else if (scene == 1) {
            MoveCursor();  // 矢印キーでカーソル移動
            InputNumber();  // キーボード入力で数字を入力

            DrawTimer();    //タイマーの描画
            DrawPauseButton();  //ポーズボタンの描画
            DrawGrid();     // グリッドを描画
            DrawButtons();  // ボタンを描画
            DrawNumbers();  // 1から9までの数字を描画

            CheckButtonClick();  // ボタンのクリック処理
            CheckGridClick();   // マスのクリック処理
            CheckAnswer();  // 答えの確認
            if (CheckClear()) {
                scene = 2;  //シーン番号を2(クリア画面)に変更
            }
        }
        else if (scene == 2) {
            DrawClearScreen();  // クリア画面を描画
            CheckClearButtonClick();    //クリア画面でのクリック処理
        }
        else if (scene == 3) {
            DrawPauseScreen();  //ポーズ画面を描画
            CheckPauseButtonClick();    //ポーズ画面でのクリック処理
        }

        ScreenFlip();  // 画面更新
    }

    DxLib_End();  // DXライブラリの終了処理
    return 0;
}