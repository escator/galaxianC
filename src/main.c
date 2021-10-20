#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// GAME SETTINGS
#define X_SIZE 60
#define Y_SIZE 45
// Максимальное количество одновременно живущих снарядо
#define BULLETS_BUF_SIZE 10
// Максимальное количество одновременно живущих целей
#define ENEMY_MAX 3
// Сколько секунд задержка до генерации нового врага
#define WAIT_GEN_ENEMY 3
// Задержка в секундах между выстрелами врагов
#define WAIT_NEXT_FIRE 2
// Количество врагов в игровом раунде
unsigned int enemy_count = 5;
// Игровые очки
unsigned int score = 0;
// Скорость падения blackbox
#define BLACKBOX_SPEED 1;

typedef enum direction {up, down, left, right}direct;
// Матрица игрового поля
char matrix[Y_SIZE][X_SIZE];
typedef enum owner_enum {PLAYER, ENEMY}t_owner;
// Структура описывающая активный снаряд
typedef struct bullet {
    bool status;
    int x, y, speed;
    direct bDirection;
    t_owner owner;
}t_bullet;
// Структура описывающая корабль игрока
struct ship {
    int x, y, healt;
}ship;
// Структура описывающая вражеский корабль
typedef struct enemyShip {
    int x, y, healt;
    bool status;
    direct direction_h;
    direct direction_v;
    unsigned int lastFireTime;
}t_eShip;
// Структура описывающая чёрный ящик
struct blackbox {
    int x, y;
    bool status;
    char typeBox;
    unsigned int lastFireTime;
}blackBox;
// Количество моделей blackbox
#define BLACKBOX_NUM 4
// Модель blackbox
char blackBoxModel[BLACKBOX_NUM][10] = {"-=HEALT=-", "-=BULLET=-", "-=WEAPON=-", "-=VOLUME=-"};
// Буффер активных снарядов
t_bullet bullets_buf[BULLETS_BUF_SIZE];
// Буффер активных врагов
t_eShip enemy_buf[ENEMY_MAX];
// таймер генерации вражеских кораблей
unsigned int lastTime;
// флаг для эффекта движения
int fl = 0;
// флаги переключатели
bool AI_switch = true;

// ------------TEMP--------------
// global for debug
bool d_hits = 0;
int d_en_index = 0;
char d_sym;
int d_direct = 0;

void print_info() {
    printw("Healt: %d\n", ship.healt);
    printw("enemy yet:%d\n", enemy_count);
    printw("SCORE: %d\n", score);
    // printw("direct: %d\n", d_direct);
    // printw("enm.x: %d\n", enemy_buf[0].x);
    // printw("enm.y: %d\n", enemy_buf[0].y);
    // printw("direction_h: %d\n", enemy_buf[0].direction_h);
    // printw("direction_v: %d\n", enemy_buf[0].direction_v);
    // printw("status: %d\n", enemy_buf[0].status);
}
// ------------------------------

void init();
void generationMatrix();
void printMatrix();
void moveShip(int x);
void updateScreen();
void fire();
void moveBullets();
void putBullet(t_bullet *p);
void moveEnemyShip(int enemyIndex);
void moveShip(int x);
void enemyAI();
int isHits(t_bullet *p);
int createEmemyShip();
void logicEnemyModule(t_eShip *ship);
void fireEnemy(t_eShip *enemy);
void putEnemyShip(int enemyIndex);
bool checkImpact(int xIn, int yIn, direct direction);
void destroyEnemyShip(t_eShip *ship);
void hitAnimation(int x, int y);
bool inGameField_X(int x);
bool inGameField_Y(int y);
void putBlackBox();
void moveBlackBox();
void createBlackBox(int x, int y);
void enableBlackBox();


int main(int argc, char const *argv[]) {
    // printf("START\n");
    int err = 0;
    bool isRun = true;
    initscr();
    init ();
    halfdelay(1);
    do {


        printMatrix();
        char key = getch();
        if (key == 'e') isRun = false;
        if (key == 'a') ship.x--;
        if (key == 'd') ship.x++;
        if (key == 's') fire();
        if (key == 'i') AI_switch = !AI_switch;

        updateScreen();    
    } while(isRun);

    endwin();
    return 0;
}

void init() {
    lastTime = time(NULL);
    ship.healt = 1000;
    for (int i = 0; i < BULLETS_BUF_SIZE; i++) {
        bullets_buf[i].status = false;
        bullets_buf[i].bDirection = up;
        bullets_buf[i].x = 0;
        bullets_buf[i].y = 0;
        bullets_buf[i].speed = 0;
    }
    for (int i = 0; i < ENEMY_MAX; i++) {
        enemy_buf[i].x = 0;
        enemy_buf[i].y = 0;
        enemy_buf[i].healt = 100;
        enemy_buf[i].status = false;
        enemy_buf[i].direction_h = right;
        enemy_buf[i].direction_v = down;
    }
    generationMatrix();
    moveShip(X_SIZE / 2 - 3);
}

void updateScreen() {
    generationMatrix();
    moveShip(ship.x);
    
    enemyAI();
    moveBullets();
    moveBlackBox();
}

void enemyAI() {
    // Считаем количество активных врагов и отрисовываем их в матрице
    int count = 0;
    for (int i = 0; i < ENEMY_MAX; i++) {
        if (enemy_buf[i].healt < 1) destroyEnemyShip(&enemy_buf[i]);
        if (enemy_buf[i].status) {
            moveEnemyShip(i);
            count++;
        }
    }
    // Добавляем вражеские корабли если надо
    if (count < ENEMY_MAX && enemy_count) {
        createEmemyShip();
    }

    for (int i = 0; i < ENEMY_MAX; i++) {
        // основная логика для кораблей врага
        if (enemy_buf[i].status) {
        fireEnemy(&enemy_buf[i]);
            if (AI_switch) logicEnemyModule(&enemy_buf[i]);
        }
    }
}

void logicEnemyModule(t_eShip *e_ship) {
    if (e_ship->status) {
        if (e_ship->y < 5) {
            e_ship->direction_v = down;
        } else if (e_ship->y > Y_SIZE - 10) {
            e_ship->direction_v = up;
        }
        if (e_ship->x < 3) {
            e_ship->direction_h = right;
        } else if (e_ship->x > X_SIZE - 8) {
            e_ship->direction_h = left;
        }

        int xShift = 0, yShift = 0;
        if (e_ship->direction_v == up && !checkImpact(e_ship->x, e_ship->y - 1, up)) {
            yShift--;
        } else {
            e_ship->direction_v = down;
        }
        if (e_ship->direction_v == down && !checkImpact(e_ship->x, e_ship->y + 1, down)) {
            yShift++;
        } else {
            e_ship->direction_v = up;
        }
        if (e_ship->direction_h == left && !checkImpact(e_ship->x - 1, e_ship->y, left)) {
            xShift--;
        } else {
            e_ship->direction_h = right;
        }
        if (e_ship->direction_h == right && !checkImpact(e_ship->x + 1, e_ship->y, right)) {
            xShift++;
        } else {
            e_ship->direction_h = left;
        }
        e_ship->x = e_ship->x + xShift;
        e_ship->y = e_ship->y + yShift;
    }
}

// Проверяет столкновение вражеского корабря с объектом по указанной координате
bool checkImpact(int xIn, int yIn, direct direction) {
    int model[3] = {5, 3, 1};
    bool flag = false;
    switch (direction) {
    case up:
        for (int i = 0; i < model[0] - 1; i++) {
            if (matrix[yIn][xIn + i] != ' ') flag = true;
        }
        break;
    case down:
        if (matrix[yIn + 3][xIn + 2] != ' ') flag = true;
        break;
    case left:
        for (int i = 0; i < 3; i++) {
            if (matrix[yIn + i][xIn] != ' ') flag = true;
        }
        break;
    case right:
        for (int i = 0; i < 3; i++) {
            if (matrix[yIn + i][xIn + 5] != ' ') flag = true;
        }
        break;
    }
    return flag;
}

void fireEnemy(t_eShip *enemy) {
    if (enemy->status && (time(NULL) - enemy->lastFireTime > WAIT_NEXT_FIRE)) {
        int count = 0;
        while (bullets_buf[count].status != false && count < BULLETS_BUF_SIZE) {
            count++;
        }
        if (count < BULLETS_BUF_SIZE) {
            bullets_buf[count].status = true;
            bullets_buf[count].bDirection = down;
            bullets_buf[count].speed = 1;
            bullets_buf[count].x = enemy->x + 2;
            bullets_buf[count].y = enemy->y + 3;
            bullets_buf[count].owner = ENEMY;
        }
        enemy->lastFireTime = time(NULL);
    }
}

void moveBullets() {
    for (int i = 0; i < BULLETS_BUF_SIZE; i++) {
        if (bullets_buf[i].status) {
            if (bullets_buf[i].bDirection == up) {
                // обрабатываем вылет за экран
                if ((bullets_buf[i].y - bullets_buf[i].speed) <= 0) {
                    bullets_buf[i].status = false;
                } else {
                    bullets_buf[i].y = bullets_buf[i].y - bullets_buf[i].speed;
                }
                
            } else {
                if ((bullets_buf[i].y - bullets_buf[i].speed) > Y_SIZE - 1){
                    bullets_buf[i].status = false;
                } else {
                    bullets_buf[i].y = bullets_buf[i].y + bullets_buf[i].speed;
                }
            }
        }
        // отрисовка в матрице
        if (bullets_buf[i].status) {
            putBullet(&bullets_buf[i]);            
        }
    }
}

int isHits(t_bullet *p) {
    bool hit = false;
    if (p->bDirection == up) {
        if (matrix[p->x][(p->y)] == 'X') hit = true;
        
    }
    if (p->bDirection == down) {
        if (matrix[p->x][p->y] == 'X') hit = true;
    }
    return hit;
}

/* Отрисовывает в матрице информацию о новом положении снаряда
*  '*' снаряд продолжает полет
*  'X' произошло попадание в любое препятствие
*/
void putBullet(t_bullet *p) {
    if (((p->x) > 0 && (p->x) < X_SIZE-2) && ((p->y) >= 0 && (p->y) < Y_SIZE)) {
        if (matrix[p->y][p->x] == ' ') {
            // нет попадания
            matrix[p->y][p->x] = '*';
        } else if (matrix[p->y][p->x] == '*') {
            // попадание в снаряд
            hitAnimation(p->x, p->y);
            for (int i = 0; i < BULLETS_BUF_SIZE; i++) {
                if (bullets_buf[i].status){
                    if (bullets_buf[i].x == p->x && bullets_buf[i].y == p->y) {
                        bullets_buf[i].status = false;
                        p->status = false;
                    }
                }
            }
        } else if (matrix[p->y][p->x] >= 48 && matrix[p->y][p->x] <= 57) {
            // попадание по врагу
            if (p->owner == PLAYER) {
                int index = matrix[p->y][p->x] - 48;
                hitAnimation(p->x, p->y);
                score += 100;
                enemy_buf[index].healt -= 25;
                p->status = false;
            }
        } else if (matrix[p->y][p->x] = '^') {
            // попадание по игроку
            hitAnimation(p->x, p->y);
            ship.healt -= 50;
            p->status = false;
        }
    }
}

// Анимация попадания. 
void hitAnimation(int x, int y) {
    char s = 'X';
    if ((x > 2 && x < X_SIZE - 3) && (y > 5 && y < Y_SIZE - 1)) {
        matrix[y][x] = s;
        matrix[y - 1][x - 1] = s;
        matrix[y - 1][x + 1] = s;
        matrix[y + 1][x - 1] = s;
        matrix[y + 1][x + 1] = s;
    }
}

// Выстрел с корабля игрока
void fire() {
    int count = 0;
    while (bullets_buf[count].status != false && count < BULLETS_BUF_SIZE) {
        count++;
    }
    if (count < BULLETS_BUF_SIZE) {
        bullets_buf[count].status = true;
        bullets_buf[count].bDirection = up;
        bullets_buf[count].speed = 1;
        bullets_buf[count].x = ship.x + 3;
        bullets_buf[count].y = Y_SIZE - 1 - 3;
        bullets_buf[count].owner = PLAYER;
    }

}

void moveShip(int x) {
    char sym = '^';
    int model[4] = {7, 5, 3, 1};
    if (x < 1) {
        x = 1;
    } else if (x > X_SIZE - 1 - model[0]) {
        x = X_SIZE - 1 - model[0];
    } 
    ship.x = x;
    ship.y = 0;
    for (int y = 0; y < 4; y++) {
        for (int i = 0; i < model[y]; i++) {
            matrix[Y_SIZE - 1 - y][x + i + y] = sym;
        }
    }
}

/* Функция создает вражеский корабль в случайных 
* Х координатах на 0 Y координате
* если координаты заняты, то корабль не создается
* возвращается -1 если успешно возвращает индекс в enemy_buf
*/
int createEmemyShip() {
    int index = -1, hit = 0, err = 0;
    if (time(NULL) - lastTime < WAIT_GEN_ENEMY) {
        return -99;
    }
    lastTime = time(NULL);
    for (int i = 0; i < ENEMY_MAX; i++) {
        if (!enemy_buf[i].status) {
            index = i;
            break;
        }
    }

    if (index > -1 && !enemy_buf[index].status) {
        srand((unsigned int)time(NULL) * 3 % 1000);
        int tX = rand() % (X_SIZE - 5);
        bool hitFl = false;
        for (int y = 5; y < 8; y++) {
            for (int x = 0; x < 5; x++) {
                if (matrix[y][x + tX] != ' ') {
                    hit = true;
                    break;
                }
            }
        }
        if (!hit) {
            enemy_buf[index].x = tX;
            enemy_buf[index].y = 0;
            enemy_buf[index].status = true;
            enemy_buf[index].healt = 100;
            if (ship.x < tX + 2) {
                enemy_buf[index].direction_h = left;
            } else {
                enemy_buf[index].direction_h = right;
            }
        }
        putEnemyShip(index);
    }
    return index;
}

void destroyEnemyShip(t_eShip *ship) {
        if (!blackBox.status) createBlackBox(ship->x, ship->y);
        ship->status = false;
        ship->x = 0;
        ship->y = 0;
        ship->healt = 100;
        ship->direction_h = right;
        ship->direction_v = down;
        if (enemy_count) enemy_count--;
}

/* Функция перемещает вражеский корабль с индексом в 
* enemy_buf по указанным координатам
*/
void moveEnemyShip(int enemyIndex) {
    int xIn, yIn;
    int model[3] = {5, 3, 1};
    xIn = enemy_buf[enemyIndex].x;
    yIn = enemy_buf[enemyIndex].y;

    if (xIn < 1) {
        xIn = 1;
    } else if (xIn > X_SIZE - 1 - model[0]) {
        xIn = X_SIZE - 1 - model[0];
    }
    enemy_buf[enemyIndex].x = xIn;
    enemy_buf[enemyIndex].y = yIn;
    putEnemyShip(enemyIndex);
    
}
void putEnemyShip(int enemyIndex){
    int model[3] = {5, 3, 1};
    for (int y = 0; y < 3; y++) {
        for (int i = 0; i < model[y]; i++) {
            matrix[enemy_buf[enemyIndex].y + y][enemy_buf[enemyIndex].x + i + y] = 48 + enemyIndex;
        }
    }
}

void createBlackBox(int x, int y) {
    srand(time(NULL));
    int mBox = rand() % BLACKBOX_NUM;
    if (x < 1) x = 1;
    if (x > X_SIZE - 12) x = X_SIZE - 12;
    if (y < 0) y = 0;
    if (y > Y_SIZE - 8) y = 8;
    blackBox.x = x;
    blackBox.y = y;
    blackBox.typeBox = mBox;
    blackBox.status = true;
    blackBox.lastFireTime = time(NULL) - 1;
}

void moveBlackBox() {
    if (blackBox.status) {
        if (inGameField_X(blackBox.x) && inGameField_Y(blackBox.y + 1)) {
            if (time(NULL) - blackBox.lastFireTime > 0) {
                blackBox.y++;
                blackBox.lastFireTime = time(NULL);
            }
            putBlackBox();
        } else {
            blackBox.status = false;
        }
    }
}

void putBlackBox() {
    for (int i = 0; i < 10; i++) {
        matrix[blackBox.y][blackBox.x + i] = blackBoxModel[blackBox.typeBox][i];
    }
}

void enableBlackBox() {

}

bool inGameField_X(int x) {
    return (x > 0 && x < Y_SIZE - 2);
}

bool inGameField_Y(int y) {
    return (y >= 0 && y < Y_SIZE - 1);
}

/* Генерирует матрицу заполнив ее пробелами
*  и создает по краям границу с эффектом движения
*/
void generationMatrix() {
    for (int y = 0; y < Y_SIZE; y++) {
        for (int x = 0; x < X_SIZE; x++) {
            if (x == 0 || x == X_SIZE - 1)
                if (fl){
                    if (y % 2)
                        matrix[y][x] = '$';
                    else
                        matrix[y][x] = ' ';
                } else {
                    if (y % 2)
                        matrix[y][x] = ' ';
                    else
                        matrix[y][x] = '$';
                }
            else
                matrix[y][x] = ' ';
        }
    }
    if (fl)
        fl = 0;
    else
        fl = 1; 
}

void printMatrix() {
    clear();
    move(0, 0);
    for (int y = 5; y < Y_SIZE; y++) {
        for (int x = 0; x < X_SIZE; x++) {
           printw("%c", matrix[y][x]);
        }
        printw("\n");
    }
    print_info();
    refresh();
}
