#include <graphics.h>
#include<iostream>
#include <conio.h>
#include<random>
#include<time.h>
#include <windows.h> 
using namespace std;

#define B 20          // 方块边长
#define X_origin 0    // X轴原点
#define Y_origin 0    // Y轴原点
mt19937 g(time(0));   // 随机数生成器

// 游戏区域边界常量
const int GAME_AREA_MIN_X = 1;
const int GAME_AREA_MAX_X = 20;
const int GAME_AREA_MIN_Y = 0;
const int GAME_AREA_MAX_Y = 37;

// 预览区独立坐标（不参与游戏区域校验）
const int PREVIEW_X = 29;
const int PREVIEW_Y = 10;

int fell[21][38] = { 0 };// 记录已下落方块的数组（1表示有方块，0表示无）
int block_color[21][38][3] = { 0 };// 记录每个已落地方块的颜色
int x = 0, y = 0, type = 0, dir = 0;// x/y：方块坐标，type：方块类型，dir：旋转方向
int sa = 0;// 分数
int line_count = 0;// 消行总数
int sleep_time = 300;// 当前下落速度（毫秒）
int base_speed = 300;// 原始基础速度（用于恢复）
int over = 0;// 游戏结束标记（1结束，0继续）
DWORD game_start_time = 0;// 游戏开始时间（毫秒）
DWORD game_duration = 0;// 游戏时长（秒）
int depth = 37; // 核心：深度变量，初始37，≤0时游戏结束
DWORD landing_start = 0; // 着陆计时开始时间（毫秒）
bool grounded = false; // 是否处于着陆待定状态
const DWORD LANDING_GRACE = 300; // 着陆缓冲时间（毫秒），在此期间允许左右移动

// 7种俄罗斯方块的形状数据（每种4个旋转方向，每个方向4个小方块坐标）
int blocks[7][4][4][2] = {
    // "T"型方块
    {{{0, 0}, {-1, 0}, {0, -1}, {1, 0}},
     {{0, 0}, {0, -1}, {1, 0}, {0, 1}},
     {{0, 0}, {1, 0}, {0, 1}, {-1, 0}},
     {{0, 0}, {0, 1}, {-1, 0}, {0, -1}}},
     // 反"Z"型方块
     {{{0, 0}, {-1, 0}, {0, -1}, {1, -1}},
      {{0, 0}, {0, -1}, {1, 0}, {1, 1}},
      {{0, 0}, {1, 0}, {0, 1}, {-1, 1}},
      {{0, 0}, {0, 1}, {-1, 0}, {-1, -1}}},
      // "Z"型方块
      {{{0, 0}, {-1, -1}, {0, -1}, {1, 0}},
       {{0, 0}, {1, -1}, {1, 0}, {0, 1}},
       {{0, 0}, {1, 1}, {0, 1}, {-1, 0}},
       {{0, 0}, {-1, 1}, {-1, 0}, {0, -1}}},
       // "一"型方块
       {{{0, 0}, {-2, 0}, {-1, 0}, {1, 0}},
        {{0, 0}, {0, -2}, {0, -1}, {0, 1}},
        {{0, 0}, {2, 0}, {1, 0}, {-1, 0}},
        {{0, 0}, {0, 2}, {0, 1}, {0, -1}}},
        // 方块型（田字）
        {{{0, 0}, {-1, 0}, {-1, -1}, {0, -1}},
         {{0, 0}, {-1, 0}, {-1, -1}, {0, -1}},
         {{0, 0}, {-1, 0}, {-1, -1}, {0, -1}},
         {{0, 0}, {-1, 0}, {-1, -1}, {0, -1}}},
         // L型方块
         {{{0, 0}, {-1, 0}, {1, 0}, {1, -1}},
          {{0, 0}, {0, -1}, {0, 1}, {1, 1}},
          {{0, 0},{1, 0}, {-1, 0}, {-1, 1}},
          {{0, 0}, {0, 1}, {0, -1}, {-1, -1}}},
          // 反L型方块
          {{{0, 0}, {-1, 0}, {1, 0}, {1, 1}},
           {{0, 0}, {0, -1}, {0, 1}, {-1, 1}},
           {{0, 0}, {1, 0}, {-1, 0}, {-1, -1}},
           {{0, 0}, {0, 1}, {0, -1}, {1, -1}}}
};

// 绘制单个方块单元（带指定颜色）
void drawUnitBlock(int x, int y, int color[3])
{
    int left, top, right, bottom;
    left = X_origin + x * B;
    top = Y_origin + y * B;
    right = left + B;
    bottom = top + B;
    setfillcolor(RGB(color[0], color[1], color[2]));
    fillrectangle(left, top, right, bottom);
    // 绘制方块边框
    setlinecolor(RGB(0, 0, 0));
    rectangle(left, top, right, bottom);
}

// 生成当前方块的随机颜色（每个方块固定一个颜色）
void generateBlockColor(int color[3])
{
    uniform_int_distribution<int>c(50, 225);
    color[0] = c(g);
    color[1] = c(g);
    color[2] = c(g);
}

// 绘制俄罗斯方块（区分游戏区/预览区，预览区不做边界校验）
void drawTetrisBlock(int type, int dir, int x, int y, int color[3], bool is_preview = false)
{
    for (int i = 0; i < 4; i++)
    {
        int draw_x = blocks[type][dir][i][0] + x;
        int draw_y = blocks[type][dir][i][1] + y;

        // 预览区直接绘制，不做边界校验
        if (is_preview) {
            drawUnitBlock(draw_x, draw_y, color);
        }
        // 游戏区只绘制有效坐标
        else if (draw_x >= GAME_AREA_MIN_X && draw_x <= GAME_AREA_MAX_X &&
            draw_y >= GAME_AREA_MIN_Y && draw_y <= GAME_AREA_MAX_Y) {
            drawUnitBlock(draw_x, draw_y, color);
        }
    }
}

// 绘制围墙单元
void bkuni(int i, int j) {
    IMAGE img3;	// 定义 IMAGE 对象
    // 先尝试加载图片（不判断返回值，兼容原有逻辑）
    loadimage(&img3, _T("..\\resources\\围墙.jpg"));

    // 检查图片是否有效（宽度为0表示加载失败）
    if (img3.getwidth() == 0) {
        int wall_color[3] = { 120, 120, 120 };
        drawUnitBlock(i, j, wall_color);
        return;
    }
    // 图片有效，绘制
    putimage(i * B, j * B, &img3);
}

// 绘制提示文字（新增深度显示）
void text() {
    // 下一个方块提示
    RECT r = { 400, 0, 780, 200 };
    settextcolor(RGB(225, 100, 100));
    settextstyle(50, 0, _T("Consolas"));
    drawtext(_T("The next:"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 当前分数、消行数、深度
    RECT r3 = { 400, 400, 780, 500 };
    settextstyle(30, 0, _T("Consolas"));
    wchar_t score_text[100];
    swprintf_s(score_text, L"Score: %d |  Dep: %d", sa, depth);
    drawtext(score_text, &r3, DT_CENTER | DT_VCENTER);

    // 操作提示（移回分数正下方）
    RECT r2 = { 400, 450, 780, 580 };
    settextstyle(25, 0, _T("Consolas"));
    drawtext(_T("W:Rotate  \n A:Left  D:Right \n S:Hold to Speed Up \n E:Speed+  Q:Speed- \n R:Reset"),
        &r2, DT_CENTER | DT_TOP);

    // 当前速度
    RECT r4 = { 400, 580, 780, 650 };
    settextstyle(25, 0, _T("Consolas"));
    swprintf_s(score_text, L"Speed: %d ms/step", base_speed);
    drawtext(score_text, &r4, DT_CENTER | DT_VCENTER);
}

// 绘制背景、围墙、网格、提示文字
void drawbk() {
    void display();
    // 绘制左右边界和分隔墙
    for (int i = 0; i < 54; i++) {
        bkuni(0, i);
        bkuni(38, i);
        bkuni(21, i);
    }
    // 绘制下方边界
    for (int i = 21; i < 54; i++) {
        bkuni(i, 18);
    }
    // 绘制上下边界
    for (int i = 0; i < 54; i++) {
        bkuni(i, 0);
        bkuni(i, 38);
    }
    // 绘制网格线（横线）
    int n = 0;
    setbkcolor(RGB(225, 225, 225));
    setlinecolor(RGB(200, 200, 200)); // 网格线为浅灰色
    while (n < 780) {
        int pts[] = { 0,n , 780, n };
        polyline((POINT*)pts, 2);
        n += 20;
    }
    // 绘制网格线（竖线）
    n = 0;
    while (n < 780) {
        int pts[] = { n ,0 , n , 780 };
        polyline((POINT*)pts, 2);
        n += 20;
    }
    text();// 绘制提示文字
    display();// 显示已下落的方块
}

// 判断是否能向左移动
bool cangoleft() {
    int willX, willY;
    for (int i = 0; i < 4; i++) {
        willX = blocks[type][dir][i][0] + x - 1;
        willY = blocks[type][dir][i][1] + y;
        // 如果将要移出左边界则不可移动
        if (willX < GAME_AREA_MIN_X) return false;
        // 如果将要移到可视区域上方（y 小于最小y），视为无阻挡
        if (willY < GAME_AREA_MIN_Y) continue;
        // 超出下边界或右边界视为阻挡
        if (willY > GAME_AREA_MAX_Y) return false;
        if (willX > GAME_AREA_MAX_X) return false;
        // 只有在坐标合法时才访问 fell，避免越界访问
        if (fell[willX][willY] == 1) return false;
    }
    return true;
}

// 判断是否能向右移动
bool cangoright() {
    int willX, willY;
    for (int i = 0; i < 4; i++) {
        willX = blocks[type][dir][i][0] + x + 1;
        willY = blocks[type][dir][i][1] + y;
        // 如果将要移出右边界则不可移动
        if (willX > GAME_AREA_MAX_X) return false;
        // 如果将要移到可视区域上方（y < min），视为无阻挡
        if (willY < GAME_AREA_MIN_Y) continue;
        // 超出下边界视为阻挡
        if (willY > GAME_AREA_MAX_Y) return false;
        if (willX < GAME_AREA_MIN_X) return false;
        // 只有在坐标合法时才访问 fell
        if (fell[willX][willY] == 1) return false;
    }
    return true;
}

// 判断是否能向下下落
bool canfall() {
    int willX, willY;
    for (int i = 0; i < 4; i++) {
        willX = blocks[type][dir][i][0] + x;
        willY = blocks[type][dir][i][1] + y + 1;
        // 如果将要移出下边界则视为已落地
        if (willY > GAME_AREA_MAX_Y) return false;
        // 横向越界视为不能下落
        if (willX < GAME_AREA_MIN_X || willX > GAME_AREA_MAX_X) return false;
        // 在可视区域上方（尚未进入场地）不认为被阻挡
        if (willY < GAME_AREA_MIN_Y) continue;
        // 只有在坐标合法时才访问 fell
        if (fell[willX][willY] == 1) return false;
    }
    return true;
}

// 判断是否能旋转
bool canrotate() {
    int willX, willY;
    int willdir = (dir + 1) % 4; // 简化旋转方向计算
    for (int i = 0; i < 4; i++) {
        willX = blocks[type][willdir][i][0] + x;
        willY = blocks[type][willdir][i][1] + y;
        // 如果旋转后横向越界或越过底部，禁止旋转
        if (willX < GAME_AREA_MIN_X || willX > GAME_AREA_MAX_X) return false;
        if (willY > GAME_AREA_MAX_Y) return false;
        // 旋转到场地上方是允许的（无需访问 fell）
        if (willY < GAME_AREA_MIN_Y) continue;
        // 只有在坐标合法时才访问 fell
        if (fell[willX][willY] == 1) return false;
    }
    return true;
}

// 核心：计算当前方块的最小y坐标（仅游戏区域内的有效坐标）
int get_block_min_y() {
    int min_y = GAME_AREA_MAX_Y + 1; // 初始值设为游戏区域最大y+1
    for (int i = 0; i < 4; i++) {
        int current_x = blocks[type][dir][i][0] + x;
        int current_y = blocks[type][dir][i][1] + y;
        // 只计算游戏区域内的坐标
        if (current_x >= GAME_AREA_MIN_X && current_x <= GAME_AREA_MAX_X &&
            current_y >= GAME_AREA_MIN_Y && current_y <= GAME_AREA_MAX_Y) {
            if (current_y < min_y) {
                min_y = current_y;
            }
        }
    }
    // 如果没有有效坐标，返回当前depth（不更新）
    return (min_y == GAME_AREA_MAX_Y + 1) ? depth : min_y;
}

// 核心：检测深度是否≤0 → 触发GameOver
bool check_game_over() {
    if (depth <= 0) { // 深度为0时游戏结束
        game_duration = (GetTickCount() - game_start_time) / 1000;
        return true;
    }
    return false;
}

// 移动/旋转方块（a左 d右 w旋转）
void moveblock(char ch, int current_color[3]) {
    if (over) return; // 游戏结束后禁止操作

    bool moved = false;
    if (ch == 'a' || ch == 'A') {
        if (cangoleft()) {
            x--;
            moved = true;
        }
    }
    else if (ch == 'd' || ch == 'D') {
        if (cangoright()) {
            x++;
            moved = true;
        }
    }
    else if (ch == 'w' || ch == 'W') {
        if (canrotate()) {
            dir = (dir + 1) % 4;
            moved = true;
        }
    }

    // 只有移动/旋转成功才刷新画面
    if (moved) {
        cleardevice();
        drawbk();
        drawTetrisBlock(type, dir, x, y, current_color, false);
    }
}

// 方块自动下落
void automove(int current_color[3]) {
    if (over || !canfall()) return; // 游戏结束/无法下落时停止

    y++;
    cleardevice();
    drawbk();
    drawTetrisBlock(type, dir, x, y, current_color, false);
}

int w_type, w_dir, w_x, w_y;// 下一个方块的参数
int w_color[3] = { 0 };// 下一个方块的颜色

// 生成下一个方块的参数（包含颜色，确保生成在游戏区域内）
void willgen() {
    w_type = rand() % 7;
    w_dir = rand() % 4;
    // 限制生成位置在游戏区域内，避免初始越界
    w_x = rand() % (GAME_AREA_MAX_X - 4) + 3; // 3~17，避免边缘
    w_y = 0;
    generateBlockColor(w_color);
}

// 显示下一个方块（预览区，强制绘制）
void w_dis() {
    if (over) return;
    // 预览区调用drawTetrisBlock，传入is_preview=true
    drawTetrisBlock(w_type, w_dir, PREVIEW_X, PREVIEW_Y, w_color, true);
}

// 生成新的下落方块
void gen() {
    if (over) return;

    type = w_type;
    dir = w_dir;
    x = w_x;
    y = w_y;

    willgen();// 重新生成下一个方块
    // 重置着陆状态
    grounded = false;
    landing_start = 0;
}

// 保存已下落方块的位置和颜色到数组（安全版）
void savefell(int current_color[3]) {
    // 初始化最小y为当前depth（默认不更新）
    int block_min_y = depth;
    int placed_count = 0;

    for (int i = 0; i < 4; i++) {
        int pos_x = blocks[type][dir][i][0] + x;
        int pos_y = blocks[type][dir][i][1] + y;

        // 仅处理游戏区域内的坐标
        if (pos_x >= GAME_AREA_MIN_X && pos_x <= GAME_AREA_MAX_X &&
            pos_y >= GAME_AREA_MIN_Y && pos_y <= GAME_AREA_MAX_Y) {

            fell[pos_x][pos_y] = 1;
            // 保存当前方块的颜色
            block_color[pos_x][pos_y][0] = current_color[0];
            block_color[pos_x][pos_y][1] = current_color[1];
            block_color[pos_x][pos_y][2] = current_color[2];

            // 仅更新有效坐标的最小y
            if (pos_y < block_min_y) {
                block_min_y = pos_y;
            }
            placed_count++;
        }
    }

    // 核心修复：仅当有方块成功放置时，才更新depth
    if (placed_count > 0) {
        // 确保depth不小于0（防止异常值）
        depth = max(0, block_min_y);
    }
    // 若没有方块成功放置（全部越界），直接触发GameOver
    else {
        over = 1;
        game_duration = (GetTickCount() - game_start_time) / 1000;
    }
}

// 判断方块是否落地，落地则保存位置和颜色 + 检测GameOver
int judge(int current_color[3]) {
    if (over) return 0;

    // 如果仍可下落，则取消任何着陆计时
    if (canfall()) {
        grounded = false;
        landing_start = 0;
        return 0;
    }

    // 已达到接触地面的状态，启动或检查着陆缓冲计时
    if (!grounded) {
        grounded = true;
        landing_start = GetTickCount();
        return 0; // 在缓冲期内不立即固定方块，允许左右移动
    }

    // 如果缓冲时间未到，继续允许玩家移动
    if (GetTickCount() - landing_start < LANDING_GRACE) {
        return 0;
    }

    // 缓冲时间到，真正固定方块并处理后续逻辑
    savefell(current_color);
    grounded = false;
    landing_start = 0;

    // 检查是否因全部越界直接结束
    if (over) return 1;

    // 检查depth是否触发GameOver
    if (check_game_over()) {
        over = 1;
    }

    return 1;
}

// 显示已下落的所有方块
void display() {
    for (int i = GAME_AREA_MIN_X; i <= GAME_AREA_MAX_X; i++) {
        for (int j = GAME_AREA_MIN_Y; j <= GAME_AREA_MAX_Y; j++) {
            if (fell[i][j] == 1) {
                drawUnitBlock(i, j, block_color[i][j]);
            }
        }
    }
}

// 判断某一行是否填满
bool iscom(int row) {
    if (row < GAME_AREA_MIN_Y || row > GAME_AREA_MAX_Y) return false;

    for (int i = GAME_AREA_MIN_X; i <= GAME_AREA_MAX_X; i++) {
        if (fell[i][row] != 1) return false;
    }
    return true;
}

// 移除指定行，并让上方方块下落（同步移动颜色）
void removerow(int row) {
    // 消除增加深度
    depth += 1;
    // 限制深度不超过初始值37
    depth = min(depth, GAME_AREA_MAX_Y);

    for (int rowx = GAME_AREA_MIN_X; rowx <= GAME_AREA_MAX_X; rowx++) {
        for (int rowy = row; rowy > GAME_AREA_MIN_Y; rowy--) {
            fell[rowx][rowy] = fell[rowx][rowy - 1];
            // 同步移动颜色
            block_color[rowx][rowy][0] = block_color[rowx][rowy - 1][0];
            block_color[rowx][rowy][1] = block_color[rowx][rowy - 1][1];
            block_color[rowx][rowy][2] = block_color[rowx][rowy - 1][2];
        }
        // 清空第一行颜色
        block_color[rowx][GAME_AREA_MIN_Y][0] = 0;
        block_color[rowx][GAME_AREA_MIN_Y][1] = 0;
        block_color[rowx][GAME_AREA_MIN_Y][2] = 0;
        fell[rowx][GAME_AREA_MIN_Y] = 0;
    }
}

// 检测并消除满行
void remove() {
    int lines_removed = 0;
    // 从下往上检测，避免漏消
    for (int i = GAME_AREA_MAX_Y; i >= GAME_AREA_MIN_Y; i--) {
        if (iscom(i)) {
            removerow(i);
            lines_removed++;
            line_count++;
            // 每行固定加分 100
            sa += 100;
            // 消行后基础速度小幅提升
            base_speed = max(50, base_speed - 5);
            sleep_time = base_speed;
            // 消行后行上移，需要重新检查当前行
            i++;
        }
    }
}

// 检测按键长按状态（S键长按加速）
void check_key_press(int current_color[3]) {
    if (over) return; // 游戏结束后禁止按键

    // 检测S键长按（加速下落）
    if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState('s') & 0x8000) {
        sleep_time = 50; // 长按S键时加速到50ms
    }
    else {
        sleep_time = base_speed; // 松开S键恢复基础速度
    }

    // 检测单次按键（A/D/W/E/Q/R）
    if (_kbhit()) {
        char input = _getch();
        moveblock(input, current_color);

        // 永久调速逻辑（E加速/Q减速/R重置）
        if (input == 'e' || input == 'E') {
            base_speed = max(50, base_speed - 50);
            sleep_time = base_speed;
        }
        else if (input == 'q' || input == 'Q') {
            base_speed = min(500, base_speed + 50);
            sleep_time = base_speed;
        }
        else if (input == 'r' || input == 'R') {
            base_speed = 300;
            sleep_time = base_speed;
        }
    }
}

// 绘制半透明遮罩层
void draw_mask() {
    // 创建半透明黑色遮罩
    DWORD mask_color = RGB(0, 0, 0);
    setfillcolor(COLORREF(mask_color | 0x80000000)); // 0x80000000 表示半透明
    solidrectangle(0, 0, 780, 780);
}

// 完善的gameover展示界面（新增深度信息）
void gameover() {
    // 绘制半透明遮罩，突出结束提示
    draw_mask();

    // 标题：Game Over
    RECT r1 = { 0, 100, 780, 250 };
    settextcolor(RGB(255, 50, 50));
    settextstyle(120, 0, _T("Consolas"));
    drawtext(_T("Game Over"), &r1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 游戏统计信息
    RECT r2 = { 0, 280, 780, 450 };
    settextcolor(RGB(255, 255, 255));
    settextstyle(40, 0, _T("Consolas"));
    wchar_t stats_text[200];
    swprintf_s(stats_text,
        L"Final Score: %d\nTotal Lines: %d\nFinal Depth: %d\nGame Time: %d seconds\nFinal Speed: %d ms/step",
        sa, line_count, depth, game_duration, base_speed);
    drawtext(stats_text, &r2, DT_CENTER | DT_VCENTER);

    // 退出提示
    RECT r3 = { 0, 500, 780, 600 };
    settextstyle(30, 0, _T("Consolas"));
    settextcolor(RGB(200, 200, 200));
    drawtext(_T("Press Any Key to Exit"), &r3, DT_CENTER | DT_VCENTER);

    FlushBatchDraw();
    _getch(); // 等待按键退出
}

int main() {
    initgraph(780, 780);
    BeginBatchDraw();
    srand((unsigned int)time(NULL)); // 初始化随机数
    game_start_time = GetTickCount(); // 记录游戏开始时间

    drawbk();
    willgen();// 生成第一个下一个方块（含颜色）
    gen();// 生成第一个下落方块

    // 当前方块的颜色（每个方块固定一个）
    int current_color[3] = { 0 };
    generateBlockColor(current_color);

    // 游戏主循环
    while (!over) {
        // 1. 检测按键（包含S键长按加速）
        check_key_press(current_color);

        // 2. 自动下落（速度由sleep_time控制）
        static DWORD last_fall_time = GetTickCount();
        if (GetTickCount() - last_fall_time >= sleep_time) {
            automove(current_color);
            last_fall_time = GetTickCount();
        }

        // 3. 显示下一个方块
        w_dis();

        // 4. 检测方块落地，落地则消除满行并生成新方块
        if (judge(current_color) == 1) {
            remove();
            gen();
            generateBlockColor(current_color); // 新方块生成新颜色
        }

        // 5. 刷新画面
        FlushBatchDraw();
        Sleep(10); // 小幅延时，降低CPU占用
    }

    // 游戏结束展示
    gameover();

    EndBatchDraw();
    closegraph();
    return 0;
}
