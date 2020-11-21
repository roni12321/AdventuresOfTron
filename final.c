#include <stdio.h>
#include <dos.h>
#include <conf.h>
#include <kernel.h>
#include <io.h>
#include <bios.h>
#include <stdlib.h>
#include <limits.h>
#define SCORE	0
#define LIVES	3
#define INTERRUPT70H	0x70
#define TRUE	1
#define FALSE	0
#define COLS	80
#define ROWS	25
#define FLOORS_LENGTH	5
#define ELEVATORS_LENGTH	10
#define FLOORS_DISTANCE	6
#define FBITS	7
#define RTC_RATE	1024
#define SAILERS	4
#define ENEMIES	6
#define JUMP	' '
#define Freq 1193180L

extern SYSCALL sleept(int);
extern struct intmap far *sys_imp; //Pointer to the interupt vector of xinu
INTPROC Int0x70Handler(int mdevno);//defenition of our new int 70
int intmap(int vec, int(*Handler)(), int mdevno);
//Speaker functions and controls
void SpeakerOFF();
void ChangeSpeakerLatch();
void SpeakerOn();
void ChangeSpeed();

//structs for varios information, also explained in the 2nd part of the assignment
typedef struct
{
	int x;
	int y;
}Position;
typedef struct
{
	char character;
	unsigned char attribute;
}Block;
typedef struct
{
	Position position;
	Block blocks[4];
	int height;
	int width;
	int type;
	int current_floor;
	int effective_position;
	int active;
	int direction;
	unsigned speed;
}Enemy;
typedef struct
{
	int height;
	int y;
	Block blocks[COLS];
}Floor;
typedef struct
{
	Position position;
	Block blocks[ROWS];
	int isPassed;
}Ladder;
typedef struct
{
	Position position;
	Block blocks[16];
	int isActivated;
	int width;
	int pId;
	int distance;
}Elevator;
typedef struct
{
	Position position;
	Block blocks[3];
	int height;
	int width;
	int type;
	int current_floor;
	int init;
	int effective_position;
}Tron;
typedef struct
{
	Position position;
	Block blocks;
	char direction;
	unsigned score;
	int active;
}Fbits;
typedef struct
{
	Position position;
	char direction;
	Block block;
	int active;
	int spawn;
	int caught;
}Sailers;
typedef struct
{
	Position position;
	char attribute;
	char direction;
	char upper_left;
	char upper_right;
	char lower_left;
	char lower_irght;
	int active;
}Tank;
typedef struct
{
	Position position;
	char bullet;
	char active;
	char attribute;
	char direction;
}Fire;

Fbits fbits[FBITS];//Array to hold the floating bits
Sailers sailers[SAILERS];//Array to hold the sailers
Block display_draft[25][80];
Ladder ladder;
Floor floors[FLOORS_LENGTH];//Array for holding the floors
Elevator elevators[ELEVATORS_LENGTH];//array for the elevators
Enemy enemy;
Enemy allEnemy[ENEMIES];//Array for the enemies
Tron tron;
Block display[2000];
Block blank;
Tank tank;
Fire fire;

volatile int first_lvl3_init = 1;
volatile int no_down = 0;
volatile int drop_tron = 0;
volatile char direction = 0;
volatile lock_s = 0;
volatile lock_keyboard;
volatile unsigned long blue_ui = 0;
int uppid, dispid, recvpid, chekpid;
volatile int immune_flag = FALSE;
volatile char ch;
volatile unsigned long immune_cnt = 0;
volatile unsigned long count0x70 = 0;
int lvl_flag = 0;
unsigned long speeds = 0;
int enemy_index = 1;
int enemy_index2 = 1;
int enemy_index3 = 1;
volatile int second_group = 0;
volatile int second_group2 = 0;
volatile int second_group3 = 0;
int move_tron = 0;
int move_ele_flag = 0;
int up_times = 6;
int pilar = 0;
int receiver_pid;
int sched_arr_pid[5];
int sched_arr_int[5];
unsigned char far *b800h; // screen pointer
char ch_arr[2048];
int front = -1;
int rear = -1;
int point_in_cycle;
int gcycle_length;
int gno_of_pids;
unsigned long score = SCORE;
int lives = LIVES;
volatile int ENEMY_RATE = 2;
int downSteps = 6;
int sPressed = FALSE;
int speed_up = 1;
volatile int jump = FALSE;
volatile int down = FALSE;
volatile int jumpUP = 2;
volatile int jumpDown = 2;
int secondStart = FALSE;
int sumEatenBits = 0;
int blueNow = FALSE;
int ladderColor = 108;
int floorColor = 6; // floor color
int level = 1;
int secondStartNow = FALSE;
int firstTime = TRUE;
int secondTime = TRUE;
int startFlag = TRUE;
int secondStartFlag = TRUE;
volatile int not_lvlup = TRUE;
//music
int latch = 1;
int speed = 0;
int levelUp = TRUE;

void change_enemy_speed()
{
	int i;
	for (i = 0; i < ENEMIES; i++)
		if (!i)
			allEnemy[i].speed = (rand() % 4) + 4;
		else
			allEnemy[i].speed = (unsigned) (allEnemy[i - 1].speed * 1.25);
}
void init_enemy3()
{
	/*	(x,y)	(x+1,y)
	(x,y+1)	(x+1,y+1)
	*/
	int i, j;
	for (j = 0; j < ENEMIES; j++)
	{
		enemy.current_floor = 1;
		enemy.height = enemy.width = 2;
		enemy.position.y = 4;
		if (j < 3)
		{
			enemy.blocks[0].character = 201;//ASCII CODE OF ENEMY
			enemy.blocks[1].character = 187;
		}
		else
		{
			enemy.blocks[0].character = 206;//ASCII CODE OF ENEMY
			enemy.blocks[1].character = 206;
		}
		enemy.active = FALSE;
		if (!j)
			enemy.speed = (rand() % 4) + 4;
		else
			enemy.speed = (unsigned)(allEnemy[j - 1].speed * 1.25);
		if (rand() % 2 == 0)
		{
			enemy.direction = '<';
			enemy.position.x = 78;
		}
		else
		{
			enemy.position.x = 1;
			enemy.direction = '>';
		}
		for (i = 0; i < 2; i++)
			enemy.blocks[i].attribute = 66;
		allEnemy[j] = enemy;
	}
	allEnemy[0].active = TRUE;
}
void init_fire()
{
	fire.active = TRUE;
	fire.bullet = 0xd;
	fire.attribute = 0xb;
	fire.position.x = tank.position.x;
	fire.position.y = tank.position.y;
	fire.direction = tank.direction;
}//Initiate the attributes of tank's projectile
void display_fire()
{
	display_draft[fire.position.y][fire.position.x].character = fire.bullet;
	display_draft[fire.position.y][fire.position.x].attribute = fire.attribute;
}//A matrix to help move the coordinates of the screen (x,y) to the b800h adress
void init_tank()
{
	/*	(x,y)	(x+1,y)
	(x,y+1)	(x+1,y+1)
	*/
	tank.active = FALSE;
	tank.attribute = 0xb;
	tank.direction = rand() % 2 == 0 ? '<' : '>';
	if (tank.direction == '<')
	{
		tank.position.x = 78;
		tank.upper_left = 205;
		tank.upper_right = 219;
	}
	else if (tank.direction == '>')
	{
		tank.position.x = 0;
		tank.upper_left = 219;
		tank.upper_right = 205;
	}
	tank.position.y = floors[1].y - 2;
	tank.lower_left = tank.lower_irght = 236;
}//Initiate tank info
void display_tank()
{
	if (tank.active)
	{
		display_draft[tank.position.y][tank.position.x].character = tank.upper_left;
		display_draft[tank.position.y][tank.position.x + 1].character = tank.upper_right;
		display_draft[tank.position.y + 1][tank.position.x].character = tank.lower_left;
		display_draft[tank.position.y + 1][tank.position.x + 1].character = tank.lower_irght;
		display_draft[tank.position.y][tank.position.x].attribute = display_draft[tank.position.y][tank.position.x + 1].attribute = display_draft[tank.position.y + 1][tank.position.x].attribute = display_draft[tank.position.y + 1][tank.position.x + 1].attribute = tank.attribute;
		if (fire.active)
			display_fire();
	}
}//Move changes of the tank in order to be able to display it
void SpeakerOFF() {
	asm{
		IN AL,61h
		AND AL,11111100b
		OUT 61h,al
	}
}//Close the speaker using the ports
void ChangeSpeakerLatch() {
	asm{
		MOV BX, WORD PTR latch
		MOV AL,10110110b
		OUT 43h,AL
		MOV AX,BX
		OUT 42h,AL
		MOV AL,AH
		OUT 42h,AL
	}
}//Chage the latch and accordingly change the sound
void SpeakerOn() {
	asm{
		IN AL,61h
		OR AL,00000011b
		OUT 61h,AL
	}
}//Speaker off
void ChangeSpeed() {
	unsigned x = Freq / (18.2065 * speed);
	asm{
		PUSH AX
		PUSH BX
		MOV AL,36h
		OUT 43h,AL
		MOV BX,x
		MOV AL,BL
		OUT 40h,AL
		MOV AL,BH
		OUT 40h,AL
		POP AX
		POP BX
	}
}//Change the speed of the sound
void game_over()
{
	int i;
	int ps;
	unsigned long time_played;
	disable(ps);
	kill(receiver_pid);
	kill(dispid);
	kill(uppid);
	kill(chekpid);
	restore(ps);
	latch = 0;
	speed = 0;
	SpeakerOFF();
	for (i = 0; i < ROWS * COLS * 2; i += 2)
	{
		b800h[i] = ' ';
		b800h[i + 1] = 1;
	}
	time_played = count0x70 / RTC_RATE; // time played in sec
	b800h[COLS - 14] = ')';
	b800h[COLS - 12] = ':';
	b800h[COLS - 10] = ' ';
	b800h[COLS - 8] = 'G';
	b800h[COLS - 6] = 'A';
	b800h[COLS - 4] = 'M';
	b800h[COLS - 2] = 'E';
	b800h[COLS] = ' ';
	b800h[COLS + 2] = ' ';
	b800h[COLS + 4] = 'O';
	b800h[COLS + 6] = 'V';
	b800h[COLS + 8] = 'E';
	b800h[COLS + 10] = 'R';
	b800h[COLS + 12] = ' ';
	b800h[COLS + 14] = ':';
	b800h[COLS + 16] = '(';
	b800h[COLS * 2 * 2 + 26] = 'S';
	b800h[COLS * 2 * 2 + 28] = 'C';
	b800h[COLS * 2 * 2 + 30] = 'O';
	b800h[COLS * 2 * 2 + 32] = 'R';
	b800h[COLS * 2 * 2 + 34] = 'E';
	b800h[COLS * 2 * 2 + 36] = ':';
	b800h[COLS * 2 * 2 + 38] = '0' + (score % 10);
	b800h[COLS * 2 * 2 + 40] = '0' + ((score / 10) % 10);
	b800h[COLS * 2 * 2 + 42] = '0' + ((score / 100) % 10);
	b800h[COLS * 2 * 2 + 44] = '0' + ((score / 1000) % 10);
	b800h[COLS * 2 * 2 + 46] = '0' + ((score / 10000) % 10);
	b800h[COLS * 2 * 2 + 56 * 2] = 'P';
	b800h[COLS * 2 * 2 + 56 * 2 + 2] = 'L';
	b800h[COLS * 2 * 2 + 56 * 2 + 4] = 'A';
	b800h[COLS * 2 * 2 + 56 * 2 + 6] = 'Y';
	b800h[COLS * 2 * 2 + 56 * 2 + 8] = 'E';
	b800h[COLS * 2 * 2 + 56 * 2 + 10] = 'D';
	b800h[COLS * 2 * 2 + 56 * 2 + 12] = ':';
	b800h[COLS * 2 * 2 + 56 * 2 + 14] = '0' + ((time_played / 10000) % 10);
	b800h[COLS * 2 * 2 + 56 * 2 + 16] = '0' + ((time_played / 1000) % 10);
	b800h[COLS * 2 * 2 + 56 * 2 + 18] = '0' + ((time_played / 100) % 10);
	b800h[COLS * 2 * 2 + 56 * 2 + 20] = '0' + ((time_played / 10) % 10);
	b800h[COLS * 2 * 2 + 56 * 2 + 22] = '0' + (time_played % 10);
	b800h[COLS * 2 * 2 + 56 * 2 + 24] = '(';
	b800h[COLS * 2 * 2 + 56 * 2 + 26] = 'S';
	b800h[COLS * 2 * 2 + 56 * 2 + 28] = 'E';
	b800h[COLS * 2 * 2 + 56 * 2 + 30] = 'C';
	b800h[COLS * 2 * 2 + 56 * 2 + 32] = ')';
}//A process to end all previous processes and present the last UI
void set_ui()
{
	static int initiali = 0;
	int i, place;
	if (!initiali)
	{
		initiali = -1;
		b800h[0] = 'L';			b800h[1] = 0xa;
		b800h[2] = 'I';			b800h[3] = 0xa;
		b800h[4] = 'V';			b800h[5] = 0xa;
		b800h[6] = 'E';			b800h[7] = 0xa;
		b800h[8] = 'S';			b800h[9] = 0xa;
		b800h[10] = ':';		b800h[11] = 0xa;
		b800h[12] = 3;		b800h[13] = 0xa;
		b800h[14] = 3;		b800h[15] = 0xa;
		b800h[16] = 3;		b800h[17] = 0xa;
		b800h[79 * 2 + 1] = 0xa;
		b800h[78 * 2 + 1] = 0xa;
		b800h[77 * 2 + 1] = 0xa;
		b800h[76 * 2 + 1] = 0xa;
		b800h[74 * 2] = ':';	b800h[75 * 2 + 1] = 0xa;
		b800h[73 * 2] = 'E';	b800h[74 * 2 + 1] = 0xa;
		b800h[72 * 2] = 'R';	b800h[73 * 2 + 1] = 0xa;
		b800h[71 * 2] = 'O';	b800h[72 * 2 + 1] = 0xa;
		b800h[70 * 2] = 'C';	b800h[71 * 2 + 1] = 0xa;
		b800h[69 * 2] = 'S';	b800h[70 * 2 + 1] = 0xa;
		b800h[69 * 2 + 1] = 0xa;
	}
	switch (lives)
	{
	case 0:
	{	b800h[12] = ' ';	break;	}
	case 1:
	{	b800h[14] = ' ';	break;	}
	case 2:
	{	b800h[16] = ' ';	break;	}
	}
	b800h[79 * 2] = '0' + (score % 10);
	b800h[78 * 2] = '0' + ((score / 10) % 10);
	b800h[77 * 2] = '0' + ((score / 100) % 10);
	b800h[76 * 2] = '0' + ((score / 1000) % 10);
	b800h[75 * 2] = '0' + ((score / 10000) % 10);
}///Initiates the UI  - lives and score for the player to see
void initEnemy(Enemy allEnemy[])
{
	/*	(x,y)	(x+1,y)
	(x,y+1)	(x+1,y+1)
	*/
	int i, j;
	for (j = 0; j<3; j++)
	{
		enemy.current_floor = 1;
		enemy.height = enemy.width = 2;
		enemy.position.y = 4;
		enemy.position.x = 0;
		enemy.blocks[0].character = 201;//ASCII CODE OF ENEMY
		enemy.blocks[1].character = 187;
		enemy.active = FALSE;
		for (i = 0; i < 2; i++)

			enemy.blocks[i].attribute = 2;
		allEnemy[j] = enemy;
	}
	for (j = 3; j < 6; j++)
	{
		enemy.current_floor = 1;
		enemy.height = enemy.width = 2;
		enemy.position.y = 4;
		enemy.position.x = 0;
		enemy.blocks[0].character = 206;//ASCII CODE OF ENEMY
		enemy.blocks[1].character = 206;

		enemy.active = FALSE;
		for (i = 0; i < 2; i++)

			enemy.blocks[i].attribute = 7;
		allEnemy[j] = enemy;
	}
	allEnemy[0].active = TRUE;
}//Initiate two sets of enemies, by splitting them to 2 trios
void initEnemy2()
{
	/*	(x,y)	(x+1,y)
	(x,y+1)	(x+1,y+1)
	*/
	int i, j;
	int temp;
	for (j = 0; j < ENEMIES; j++)
	{
		enemy.current_floor = 1;
		enemy.height = enemy.width = 2;
		enemy.position.y = 4;
		if (j < 3)
		{
			enemy.blocks[0].character = 201;//ASCII CODE OF ENEMY
			enemy.blocks[1].character = 187;
		}
		else
		{
			enemy.blocks[0].character = 206;//ASCII CODE OF ENEMY
			enemy.blocks[1].character = 206;
		}
		enemy.active = FALSE;
		if (rand() % 2 == 0)
		{
			enemy.direction = '<';
			enemy.position.x = 78;
		}
		else
		{
			enemy.position.x = 1;
			enemy.direction = '>';
		}
		for (i = 0; i < 2; i++)
			enemy.blocks[i].attribute = 66;
		allEnemy[j] = enemy;
	}
	allEnemy[0].active = TRUE;
}//Initiates the enemies for evel 2, with the random spawn side
void displayEnemy(Enemy* enemy)
{
	int i;
	for (i = 0; i < enemy->width; i++)
		if (enemy->active)
		{
			display_draft[enemy->position.y][i + enemy->position.x] = enemy->blocks[i];
			display_draft[enemy->position.y + 1][i + enemy->position.x] = enemy->blocks[i];
		}
}//Move the enemies one by one to change their changes
void initLadder(Ladder* ladder)
{
	int i;
	ladder->position.y = ROWS;
	ladder->position.x = COLS / 2;
	ladder->isPassed = FALSE;
	for (i = 0; i < ROWS; i++)
	{
		ladder->blocks[i].character = ' ';
		ladder->blocks[i].attribute = ladderColor;
	}

}//Initiate the ladder/pole position and attributes
void initElevators()
{
	int i;
	for (i = 0; i < ELEVATORS_LENGTH; i++)
	{
		char ch, attr;
		int j, distance = 3;
		elevators[i].width = 8;
		elevators[i].isActivated = TRUE; // changed
		for (j = 0; j < elevators[i].width; j++)
		{
			elevators[i].blocks[j].character = 178; //ELEVATOR FLOOR
			elevators[i].blocks[j].attribute = 0x64; //backround
		}
		elevators[i].distance = FLOORS_DISTANCE;
		elevators[i].position.y = floors[i / 2].y;
		if (i % 2 == 0)
			elevators[i].position.x = COLS * 3 / 4 - elevators[i].width / 2; // left
		else
			elevators[i].position.x = COLS / 4 - elevators[i].width / 2 + 1; // right
	}
}//Initiate the values for the elevator attributes and locations
void initFloors(Floor floors[])
{
	int i, y = 0;
	for (i = 0; i < FLOORS_LENGTH; i++, y += FLOORS_DISTANCE)
	{
		int j;
		floors[i].height = FLOORS_DISTANCE - 1;
		floors[i].y = y;
		for (j = 0; j < COLS; j++)
		{
			int ladderWidth = 1;
			if ((j <= COLS / 2 - ladderWidth) || (j >= COLS / 2 + ladderWidth))
			{
				floors[i].blocks[j].character = 176; //REGULAR FLOOR
				floors[i].blocks[j].attribute = floorColor;
			}
		}
	}
}//Initiate the floor values
void initTron(Tron* tron)
{
	int i;
	tron->init = TRUE;
	tron->current_floor = 0;
	tron->height = 3;
	tron->width = 1;
	tron->position.y = 1;
	tron->position.x = 40;
	tron->blocks[0].character = 1;//ascii of head
	if (level == 1)
		tron->blocks[1].character = 49; //ascii of hands and body
	if (level == 2)
		tron->blocks[1].character = 50; //ascii of hands and body
	if (level == 3)
		tron->blocks[1].character = 51; //ascii of hands and body
	tron->blocks[2].character = 239; //ascii of legs
	tron->effective_position = tron->position.y * 80 * 2 + tron->position.x * 2;
	for (i = 0; i<3; i++)
		tron->blocks[i].attribute = 7;
}//Initiate the values of the main protagonist
void displayTron(Tron* tron)
{
	int i;
	for (i = 0; i< 3; i++)
		display_draft[i + tron->position.y][tron->position.x] = tron->blocks[i];
}//Move the changes of the player into the draft of the display
INTPROC new_int9(int mdevno)
{
	char result = 0;
	int scan = 0;
	int ascii = 0;
	asm{
		MOV AH,1
		INT 16h
		JZ Skip1
		MOV AH,0
		INT 16h

		MOV BYTE PTR scan,AH
		MOV BYTE PTR ascii,AL
	}//asm
		if (scan == 0x1e)
			result = 'a';
		else if (scan == 0x11)
			result = 'w';
		else if (scan == 0x20)
			result = 'd';
		else if (scan == 31)
			result = 's';
		else if (scan == 57)
			result = JUMP;
	if ((scan == 46) && (ascii == 3)) // Ctrl-C?
		asm INT 27; // terminate xinu
	send(receiver_pid, result);
Skip1:
}//Read a key from the port, send the key to the receiver
void set_new_int9_newisr()
{
	int i;
	for (i = 0; i < 32; i++)
		if (sys_imp[i].ivec == 9)
			sys_imp[i].newisr = new_int9;
}//Set the interupt to the xinu int map
void displayFloor(Floor floor)
{
	int i;
	for (i = 0; i < COLS; i++)
		display_draft[floor.y][i] = floor.blocks[i];
}//Changes and presentation of the floor is moved via here
void displayLadder(Ladder* ladder)
{
	int screenIndex;
	for (screenIndex = 0; screenIndex < ROWS; screenIndex++)
	{
		display_draft[screenIndex][COLS / 2] = ladder->blocks[screenIndex];
		display_draft[screenIndex][COLS / 2 + 1] = ladder->blocks[screenIndex];
	}
}//Changes and presentation of the ladder/pole is moved via here
void displayElevator(Elevator* elevator)
{
	int i;
	for (i = 0; i < elevator->width; i++)
		display_draft[elevator->position.y][i + elevator->position.x] = elevator->blocks[i];
}//Changes and presentation for one elevator is moved via here
void displayElevators(Elevator elevators[])
{
	int i;
	for (i = 0; i < ELEVATORS_LENGTH; i++)
		displayElevator(&elevators[i]);
}//Send the elevators one by one to the function displayElevator()
void displayFloors(Floor floors[])
{
	int i;
	for (i = 0; i < FLOORS_LENGTH; i++)
		displayFloor(floors[i]);
}//Send the floors one by one to the function displayFloor
void displayer(void)
{
	int i;
	while (TRUE)
	{
		receive();
		for (i = 0; i < ROWS*COLS * 2; i += 2)
		{
			if (i > 80 * 2 || (i > 16 && i < 69 * 2)) // as long as it's the first row - exclude ui set up
			{
				b800h[i] = display[i / 2].character;
				b800h[i + 1] = display[i / 2].attribute;
			}
		}
		set_ui();//Sets the UI
	}//Show the main game screen
}
void receiver()
{
	char temp;
	while (TRUE)
	{
		temp = receive();
		rear++;
		ch_arr[rear] = temp;
		if (front == -1)
			front = 0;
	}
}//Manage the char array - creating a buffer of sorts
void move_ele()
{
	int i;
	for (i = 0; i < 2; i++)
		elevators[i].width = 0;
	for (i = 2; i < 10; i++)
	{
		elevators[i].position.y--;
	}
}//Moves the elevators up
void move_ele_tron()
{
	move_ele();
	tron.position.y--;
}//Move tron if he is on the elevator
void updater()
{
	int i, j;
	while (TRUE)
	{
		receive();
		while (front != -1)
		{
			ch = ch_arr[front];
			if (front != rear)
				front++;
			else
				front = rear = -1;
			if (ch_arr[rear - 1] != ch_arr[rear])
				if (!lock_keyboard)
					if ((ch == 'a') || (ch == 'A')) // left arrow
					{
						if (tron.position.x > 0)
						{
							tron.position.x--;
							b800h[tron.position.y*COLS + tron.position.x * 2] = '*';
							direction = '<';
						}
					}
					else if ((ch == 'd') || (ch == 'D')) // right arrow
					{
						if (tron.position.x < 79)
						{
							tron.position.x++;
							direction = '>';
						}
					}
					else if ((ch == 'w') || (ch == 'W')) // up arrow
					{
						int i;
						for (i = 4; i < 10; i++) // i = 4 -> ignoring first 4 elevators.
							if (pilar == 1 && tron.position.x >= elevators[i].position.x && tron.position.x <= elevators[i].position.x + elevators[i].width - 1 && !lock_keyboard)
							{
								latch = 100;
								SpeakerOn();
								if (tron.current_floor < 1)
									move_tron = 1;
								break;
							}
						//tron.position.y--; // easy for checking
					}
					else if ((ch == 's') || (ch == 'S'))
					{
						if (tron.position.y < 21 && lock_s == FALSE && !lock_keyboard) {
							latch = 100;
							SpeakerOn();
							sPressed = TRUE;
						}
						else
							sPressed = FALSE;
						//tron.position.y++; // easy for checking
					}
					else if (ch == JUMP && jump == FALSE && down == FALSE && !lock_keyboard)
					{
						latch = 100;
						SpeakerOn();
						jump = TRUE;
						lock_s = 1;

					}
		} // while(front != -1), move the tron according to the keyboard input, also handle the movenemt if he uses the elevator
		ch = 0;
		for (i = 0; i < 25; i++)
			for (j = 0; j < 80; j++)
				display_draft[i][j] = blank;  // blank
		displayFloors(floors);
		displayLadder(&ladder);
		for (i = 0; i < 6; i++)
			displayEnemy(&allEnemy[i]);
		displayElevators(elevators);
		displayTron(&tron);
		display_sailers();
		display_fbits();
		display_tank();
		//All changes above are moved to the correct display arrays of the info changed
		//moves the elevator
		if (move_ele_flag)
			if (up_times--)
				move_ele();
			else
			{
				up_times = 6;
				move_ele_flag = 0;
				initElevators();
				pilar = 1;
			}
		//moves tron up
		if (move_tron)
		{
			if (up_times--)
				move_ele_tron();
			else
			{
				SpeakerOFF();
				up_times = 6;
				move_tron = 0;
				pilar = 0;
				initElevators();
				tron.current_floor++;
			}
		}
		//updating display array
		for (i = 0; i < ROWS; i++)
			for (j = 0; j < COLS; j++)
				display[i * COLS + j] = display_draft[i][j];
	}//while(TRUE)
}//updater
void set_Int0x70Handler()
{/* setting interupt 0x70 to act periodically*/
	asm{
		CLI
		PUSH AX
		IN AL,0A1h
		AND AL,0FEh
		OUT 0A1h,AL
		IN AL,70h
		MOV AL,0Ah
		OUT 70h,AL
		MOV AL,8Ah
		OUT 70h,AL
		IN AL,71h
		AND AL,10000000b
		OR AL,00100110b // base = 010, rate = 0110, therefor : 1024 interrupts per second.
		OUT 71h,AL
		IN AL,71h
		IN AL,70h
		MOV AL,0Bh
		OUT 70h,AL
		MOV AL,8Bh
		OUT 70h,AL
		IN AL,71h
		OR AL,40h
		OUT 71h,AL
		IN AL,71h
		IN AL, 021h
		AND AL, 0FBh
		OUT 021h, AL
		IN AL, 70h
		MOV AL, 0Ch
		OUT 70h, AL
		IN AL, 70h
		MOV AL, 8Ch
		OUT 70h, AL
		IN AL, 71h
		IN AL, 70h
		MOV AL, 0Dh
		OUT 70h, AL
		IN AL, 70h
		MOV AL, 8Dh
		OUT 70h, AL
		IN AL, 71h
		POP AX
		STI
	}
	mapinit(INTERRUPT70H, Int0x70Handler, INTERRUPT70H); // sets a new entry at interrupts map, for interrupt 0x70.
}
display_sailers()
{
	int i;
	for (i = 0; i < FLOORS_LENGTH - 1; i++)
	{
		if (sailers[i].active)
		{
			display_draft[sailers[i].position.y][sailers[i].position.x].character = sailers[i].block.character;
			display_draft[sailers[i].position.y][sailers[i].position.x].attribute = sailers[i].block.attribute;
		}
	}
}//Moves the changes of movement for the sailors
void init_sailers()
{
	int i, temp, h = 1;
	for (i = 0; i < SAILERS; i++)
	{
		temp = rand() % 2;
		sailers[i].direction = (temp == 0 ? '<' : '>');
		sailers[i].block.character = sailers[i].direction;
		sailers[i].block.attribute = 0xc;
		sailers[i].position.x = (temp == 0 ? 79 : 0);
		sailers[i].position.y = h;
		sailers[i].active = FALSE;
		sailers[i].spawn = rand() % 14 + 4;
		sailers[i].caught = FALSE;
		h += FLOORS_LENGTH + 1;
	}
}//Initiate the info for the sailors
display_fbits()
{
	int i;
	for (i = 0; i < FBITS; i++)
		if (fbits[i].active)
		{
			display_draft[fbits[i].position.y][fbits[i].position.x].character = fbits[i].blocks.character;
			display_draft[fbits[i].position.y][fbits[i].position.x].attribute = fbits[i].blocks.attribute;
		}
}//Move changes for the floating bits to the display draft
void init_fbits()
{
	int i, fbit_score = 100, h = 25 - FLOORS_LENGTH;
	for (i = 0; i < FBITS; i++)
	{
		fbits[i].blocks.character = 167;
		fbits[i].blocks.attribute = 0xd;
		fbits[i].position.y = h;
		fbits[i].score = fbit_score;
		fbits[i].active = TRUE;
		if (i % 2 == 0)
		{
			fbits[i].position.x = rand() % 78;
			fbits[i].direction = '>';
		}
		else
		{
			fbits[i].position.x = rand() % 78;
			fbits[i].direction = '<';
			h -= FLOORS_LENGTH + 1;
			fbit_score *= 2;
		}
	}
}//Initiate the bits info  
SYSCALL schedule(int no_of_pids, int cycle_length, int pid1, ...)
{
	int i;
	int ps;
	int *iptr;
	disable(ps);
	gcycle_length = cycle_length;
	point_in_cycle = 0;
	gno_of_pids = no_of_pids;
	iptr = &pid1;
	for (i = 0; i < no_of_pids; i++)
	{
		sched_arr_pid[i] = *iptr;
		iptr++;
		sched_arr_int[i] = *iptr;
		iptr++;
	}
	restore(ps);
}//schedule the given PID'S and cycle them
void checker()
{
	int i;
	while (TRUE)
	{
		if (tron.position.x == 40 || tron.position.x == 41)
			move_ele_flag = 1;
	}
}//Chekcs for the passing of the ladder/pole by the player, allowing for the pit fall or the elevator movement, also handle immunity for the player
void caught_sailer()
{
	int i;
	for (i = 0; i < SAILERS; i++)
		if (tron.position.x == sailers[i].position.x && tron.position.y == sailers[i].position.y && jump)
			if (sailers[i].active)
			{
				sailers[i].caught = 1;
				no_down = 1;
				lock_keyboard = 1;
				jump = 0;
				jumpUP = 0;
				jumpDown = 0;
			}
}//Check for the player grabbing the sailor, and change the rules accordingly to move the player with it in 70H
int Int0x70Handler(int mdevno)
{
	int i;
	asm{
		/* basic settings & protocols of the original 0x70 inerrupt. */
		PUSH AX
		PUSH BX
		IN AL,70h
		MOV BX,AX
		MOV AL,0Ch
		OUT 70h,AL
		MOV AL,8Ch
		OUT 70h,AL
		IN AL,71h
		MOV AX,BX
		OUT 70h,AL
		MOV AL,20h
		OUT 0A0h,AL
		OUT 020h,AL
		POP BX
		POP AX
	}
		/* additions for the game. */
		if (count0x70 < ULONG_MAX) // makes sure that the counter is within range.
			count0x70++;
		else
			count0x70 = 0; // before the counter is about to reach unsigned long limits (max unsinged long value), resets it (to 0).
	if ((tron.position.x == 40 || tron.position.x == 41) && tron.position.y < 16 && jump == FALSE && down == FALSE  && no_down == FALSE)
	{//falling from center implementation
		sPressed = TRUE;
	}
	for (i = 0; i < ENEMIES; i++)
		if (!immune_flag)
			if (tron.position.x >= allEnemy[i].position.x && tron.position.x <= allEnemy[i].position.x + 1
				&&
				(tron.position.y == allEnemy[i].position.y || tron.position.y == allEnemy[i].position.y + 1
					||
					tron.position.y + 2 == allEnemy[i].position.y || tron.position.y + 2 == allEnemy[i].position.y + 1)
				)//Check for enemy location and tron location, reduce life accordingly, also turn on sound, and reset player position
				if (!(--lives))
					resume(create(game_over, INITSTK, INITPRIO, "GAME OVER", 0));//IF need be, finish the game
				else
				{
					latch = 200;
					ChangeSpeed();
					ChangeSpeakerLatch();
					SpeakerOn();
					tron.position.x = 40;
					tron.position.y = 1;//
					sPressed = TRUE;//
					tron.init = TRUE;
					immune_flag = TRUE;

				}
	//move the tron from top to lower floor
	if (count0x70 % (RTC_RATE * 60) == 0)
		if (speed_up)
		{
			ENEMY_RATE *= 2; // speeds up by 2 the enemies after 60s.
			speed_up = 0;
		}
	if (tron.init == TRUE)
	{
		lock_keyboard = 1;
		if (count0x70 % (RTC_RATE / 8) == 0)
		{
			if (tron.position.y < 21) {
				tron.position.y++;
				SpeakerOFF();
			}
			else
			{
				tron.init = FALSE; // the init movement is finished
				lock_keyboard = 0;
				SpeakerOFF();
			}
		}
	}//Drop the player if we starta new level/lose a life
	if (immune_flag)
		if ((++immune_cnt) % (RTC_RATE * 2) == 0)
		{
			immune_cnt = 0;
			immune_flag = 0;
		}
	if (count0x70 % (RTC_RATE / 16) == 0)
	{
		/* char movement may be placed here*/

	}
	if (count0x70 % (RTC_RATE / 8) == 0)
		for (i = 0; i < SAILERS; i++)
		{
			if (!sailers[i].caught)
				if (count0x70 % (RTC_RATE * sailers[i].spawn) == 0)
					if (!sailers[i].active)
						sailers[i].active = 1;
					else
						sailers[i].active = 0;
			if (sailers[i].active)
			{
				caught_sailer();
				if (sailers[i].direction == '>')
					if (sailers[i].position.x < 79)
						sailers[i].position.x++;
					else
					{
						sailers[i].direction = '<';
						sailers[i].block.character = '<';
						sailers[i].position.x--;
					}
				else if (sailers[i].direction == '<')
					if (sailers[i].position.x > 0)
						sailers[i].position.x--;
					else
					{
						sailers[i].direction = '>';
						sailers[i].block.character = '>';
						sailers[i].position.x++;
					}
			}//Movement of the sailors
			if (sailers[i].caught)//move the player if we caught a sailer
			{
				tron.position.y = sailers[i].position.y;
				if (sailers[i].direction == '>')
				{
					if (sailers[i].position.x == 79)
					{
						sailers[i].caught = 0;
						drop_tron = 1;
						tron.position.x = 79;
					}
					else
						tron.position.x++;
				}
				else if (sailers[i].direction == '<')
				{
					if (sailers[i].position.x == 0)
					{
						sailers[i].caught = 0;
						drop_tron = 1;
						tron.position.x = 0;
					}
					else
						tron.position.x--;
				}
			}
			if (drop_tron)
			{
				drop_tron = 0;
				sailers[i].caught = FALSE;
				tron.position.y = sailers[i].position.y + 2;
				no_down = 0;
				jump = 0;
				jumpUP = 2;
				jumpDown = 0;
				lock_keyboard = 0;
			}
		}//Drops the player after the movement was finished
	if (count0x70 % (RTC_RATE / 8) == 0)
	{
		for (i = 0; i < FBITS; i++)
			if (fbits[i].active)
				if (fbits[i].direction == '>')
					if (fbits[i].position.x < 79)
						fbits[i].position.x++;
					else
					{
						fbits[i].direction = '<';
						fbits[i].position.x--;
					}
				else if (fbits[i].direction == '<')
					if (fbits[i].position.x > 0)
						fbits[i].position.x--;
					else
					{
						fbits[i].direction = '>';
						fbits[i].position.x++;
					}
	}
	 //Enemy activation according to the level, and also spawn delay
	if (level == 1)
	{
		if (enemy_index < ENEMIES)
		{
			if (count0x70 % (RTC_RATE * 8) == 0)
			{
				if (enemy_index > 2)
				{
					if (second_group)
						allEnemy[enemy_index++].active = TRUE;
					if (allEnemy[0].current_floor == 2)
						if (allEnemy[0].position.x == 0)
						{
							second_group = 1;
							allEnemy[enemy_index++].active = TRUE;
						}
				}
				else
					allEnemy[enemy_index++].active = TRUE;
			}/*if enemy delay*/
		}/*if enemy index*/
		else
			second_group = 0;
	}
	else if (level == 2)
	{
		if (enemy_index2 < ENEMIES)
		{
			if (count0x70 % (RTC_RATE * 8) == 0)
			{
				if (enemy_index2 > 2)
				{
					if (second_group2)
						allEnemy[enemy_index2++].active = TRUE;
					if (allEnemy[0].current_floor == 2 && !second_group2)
					{
						second_group2 = 1;
						allEnemy[enemy_index2++].active = TRUE;
					}
				}
				else
					allEnemy[enemy_index2++].active = TRUE;
			}
		}
		else
			second_group2 = 0;
	}
	else if (level == 3)
	{
		if (first_lvl3_init)
		{
			first_lvl3_init = FALSE;
			tank.active = TRUE;
			init_enemy3();
		}
		if (count0x70 % (RTC_RATE * 5) == 0)
		{
			change_enemy_speed();
			speeds = 0;
		}
		if (enemy_index3 < ENEMIES)
			if(count0x70 % (RTC_RATE * 2) == 0)
			allEnemy[enemy_index3++].active = TRUE;
	}
	//Determine the movement pacing/rate here, also move the enemies in this code part
	if (count0x70 % (RTC_RATE / ENEMY_RATE) == 0)
	{
		for (i = 0; i < ENEMIES; i++)
		{
			if (allEnemy[i].active)
			{
				if (level == 1)
				{
					if (allEnemy[i].position.x < COLS - allEnemy[i].width)
						allEnemy[i].position.x++;
					else
					{
						if (allEnemy[i].current_floor == FLOORS_LENGTH - 1)
						{
							allEnemy[i].current_floor = 1;
							allEnemy[i].position.y = floors[1].y - allEnemy[i].height;
						}
						else
						{
							allEnemy[i].position.y = floors[allEnemy[i].current_floor + 1].y - allEnemy[i].height;
							allEnemy[i].current_floor++;
						}
						allEnemy[i].position.x = 0;
					}
				}
				else if (level == 2)
				{
					if (!lvl_flag)
					{
						initEnemy2();
						lvl_flag = 1;
					}
					if (allEnemy[i].direction == '<')
					{
						if (allEnemy[i].position.x > 0)
							allEnemy[i].position.x--;

					}
					else if (allEnemy[i].direction == '>')
					{
						if (allEnemy[i].position.x < 78)
							allEnemy[i].position.x++;
					}
					if (allEnemy[i].position.x == 0 || allEnemy[i].position.x == 78)
					{
						if (allEnemy[i].current_floor == FLOORS_LENGTH - 1)
						{
							allEnemy[i].current_floor = 1;
							allEnemy[i].position.y = floors[1].y - allEnemy[i].height;
						}
						else
						{
							allEnemy[i].position.y = floors[allEnemy[i].current_floor + 1].y - allEnemy[i].height;
							allEnemy[i].current_floor++;
						}
						if (rand() % 2 == 0)
						{
							allEnemy[i].direction = '<';
							allEnemy[i].position.x = 78;
						}
						else
						{
							allEnemy[i].direction = '>';
							allEnemy[i].position.x = 0;
						}
					}
				}	
			}
		}/*for moving enemies*/
	}/*if clock enemy rate*/
	if (level == 3)
	{
		for (i = 0; i < ENEMIES; i++)
			if (allEnemy[i].active)
			{
				if (count0x70 % (RTC_RATE / allEnemy[i].speed) == 0)
				{
					if (allEnemy[i].direction == '<')
					{
						if (allEnemy[i].position.x > 1)
							allEnemy[i].position.x--;
					}
					else if (allEnemy[i].direction == '>')
					{
						if (allEnemy[i].position.x < 78)
							allEnemy[i].position.x++;
					}
					if (allEnemy[i].position.x == 1 || allEnemy[i].position.x == 78)
					{
						if (allEnemy[i].current_floor == FLOORS_LENGTH - 1)
						{
							allEnemy[i].current_floor = 1;
							allEnemy[i].position.y = floors[1].y - allEnemy[i].height;
						}
						else
						{
							allEnemy[i].position.y = floors[allEnemy[i].current_floor + 1].y - allEnemy[i].height;
							allEnemy[i].current_floor++;
						}
						if (rand() % 2 == 0)
						{
							allEnemy[i].direction = '<';
							allEnemy[i].position.x = 78;
						}
						else
						{
							allEnemy[i].direction = '>';
							allEnemy[i].position.x = 1;
						}
					}
				}
			}
	}


	 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	if (count0x70 % (RTC_RATE / 8) == 0)
	{ // move down the tron if 's' pressed
		if (sPressed == TRUE)
		{
			lock_keyboard = 1;
			if (downSteps-- && tron.position.y < 21)
				tron.position.y++;
			else
			{
				SpeakerOFF();
				tron.current_floor--;
				downSteps = 6;
				sPressed = FALSE;
				lock_keyboard = 0;
			}
		}
	}
	if (jump == TRUE)
	{
		if (count0x70 % (RTC_RATE / 8) == 0)
		{//We check the ruling for the sailer , manage his jump.
			if (jumpUP--)//The up part of the jump
			{
				if (lock_s == 1)
					SpeakerOFF();
				tron.position.y--;
				if (direction == '>')
				{
					if (tron.position.x < 78)
						tron.position.x += 2;
				}
				else if (direction == '<')
				{
					if (tron.position.x > 1)
						tron.position.x -= 2;
				}
			}
			else
			{
				caught_sailer();
				jump = FALSE;
				if (!no_down)
				{
					down = TRUE;
					jumpDown = 2;
				}
			}
		}
	}
	if (down == TRUE)//The down part of the jump, works only after the up movement is finished
	{
		if (count0x70 % (RTC_RATE / 4) == 0)
		{//jump ->down
			if (jumpDown--)
			{

				tron.position.y++;
				if (direction == '>')
				{
					if (tron.position.x < 78)
						tron.position.x += 2;
				}
				else if (direction == '<')
				{
					if (tron.position.x > 1)
						tron.position.x -= 2;
				}
			}
			else
			{
				down = FALSE;
				jumpUP = 2;
				lock_s = 0;
				jump = 0;
				direction = 0;
				no_down = 0;
			}
		}
	}
	//Check location for the tron catching the floating bits
	for (i = 0; i < FBITS; i++)
		if (fbits[i].active)
			if (tron.position.x == fbits[i].position.x
				&&
				(fbits[i].position.y == tron.position.y
					||
					fbits[i].position.y == tron.position.y + 1
					||
					fbits[i].position.y == tron.position.y + 2))
			{
				fbits[i].active = FALSE;
				latch = 100;
				SpeakerOn();
				score += fbits[i].score;
				blueNow = TRUE;
				sumEatenBits++;
			}
	if (count0x70 % (RTC_RATE / 4) == 0)
		if (!jump)
			direction = 0;
	//Tank implementation
	if (count0x70 % (RTC_RATE / 16) == 0)
	{
		if (tank.active)
		{
			if (tank.direction == '<')
				if (tank.position.x > 0)
					tank.position.x--;
				else
				{
					char temp;
					tank.direction = '>';
					temp = tank.upper_left;
					tank.upper_left = tank.upper_right;
					tank.upper_right = temp;
					tank.position.x = 0;
					if (tank.position.y == floors[FLOORS_LENGTH - 1].y - 2)
						tank.position.y = floors[1].y - 2;
					else
					{
						tank.position.y += FLOORS_DISTANCE;
						init_fire();
					}
				}
			if (tank.direction == '>')
				if (tank.position.x < 78)
					tank.position.x++;
				else
				{
					char temp;
					temp = tank.upper_left;
					tank.upper_left = tank.upper_right;
					tank.upper_right = temp;
					tank.direction = '<';
					tank.position.x = 78;
					if (tank.position.y == floors[FLOORS_LENGTH - 1].y - 2)
						tank.position.y = floors[1].y - 2;
					else
					{
						tank.position.y += FLOORS_DISTANCE;
						init_fire();
					}
				}

		}
	}
	//Manage the firing rate and condition for the tank enemy type
	if (count0x70 % (RTC_RATE / 32) == 0)
	{
		if (fire.active)
		{
			if (fire.direction == '<')
			{
				if (fire.position.x > 0)
					fire.position.x--;
				else
					fire.active = FALSE;
			}
			else if (fire.direction == '>')
			{
				if (fire.position.x < 79)
					fire.position.x++;
				else
					fire.active = FALSE;
			}
		}
		//If the player isnt immune, this counts as a hit
		if (!immune_flag)
			if (tron.position.x == fire.position.x
				&& (fire.position.y == tron.position.y
					|| fire.position.y == tron.position.y + 1
					|| fire.position.y == tron.position.y + 2))
				if (!(--lives))
					resume(create(game_over, INITSTK, INITPRIO, "GAME OVER", 0));
				else
				{
					latch = 512;
					ChangeSpeed();
					ChangeSpeakerLatch();
					SpeakerOn();
					tron.position.x = 40;
					tron.position.y = 1;//
					sPressed = TRUE;//
					tron.init = TRUE;
					immune_flag = TRUE;
				}
	}
	if (!immune_flag)
		if (tron.position.x >= tank.position.x && tron.position.x <= tank.position.x + 1
			&&
			(tron.position.y == tank.position.y || tron.position.y == tank.position.y + 1
				||
				tron.position.y + 2 == tank.position.y || tron.position.y + 2 == tank.position.y + 1)
			)
			if (!(--lives))
				resume(create(game_over, INITSTK, INITPRIO, "GAME OVER", 0));
			else
			{
				latch = 512;
				ChangeSpeed();
				ChangeSpeakerLatch();
				SpeakerOn();
				tron.position.x = 40;
				tron.position.y = 1;//
				sPressed = TRUE;//
				tron.init = TRUE;
				immune_flag = TRUE;
			}
	if (blueNow == TRUE && sumEatenBits < 7)
	{ // case of eating floating bits but not all of them
		ladderColor = 17;
		initLadder(&ladder);
		floorColor = 17;
		initFloors(floors);
		blueNow = FALSE;
	}
	else if (blueNow == FALSE && sumEatenBits < 7 && (count0x70 % (RTC_RATE) == 0))
	{ // still not all floating bits collected
		SpeakerOFF();
		floorColor = 6;
		initFloors(floors); // turn off the blue colors of floors
		ladderColor = 108;
		initLadder(&ladder); // turn off the blue colors of center line
	}
	if (sumEatenBits == 7)
	{ // eat all floating bits -> only the center line will be blue until the 'Tron' come to center
		{
			ladderColor = 17;
			initLadder(&ladder);
			blueNow = FALSE;
			if (tron.position.x == 40 || tron.position.x == 41)
			{
				if (level < 3 && levelUp == TRUE) {
					level++;
					score += 2000;
					immune_flag = TRUE;
					levelUp = FALSE;
					initEnemy(allEnemy);
					initTron(&tron);

				}//Handle the passing to the next stage

			}
			if ((++blue_ui) % (RTC_RATE * 6) == 0)
			{
				levelUp = TRUE;
				sumEatenBits = 0;
				SpeakerOFF();
				ladderColor = 108;
				initLadder(&ladder); // turn off the blue colors of center line
				blueNow = FALSE;
				blue_ui = 0;
				for (i = 0; i < FBITS; i++)
					fbits[i].active = TRUE;
			}//make sure the screen is turned blue only for the wanted duration
		}
	}
}
xmain()
{
	int i;
	b800h = (unsigned char far *)0xB8000000;
	blank.character = ' ';
	blank.attribute = 0;

	srand(time(NULL));
	initFloors(floors);
	initElevators(elevators);
	initLadder(&ladder);
	initEnemy(allEnemy);
	initTron(&tron);
	init_sailers();
	init_fbits();
	init_tank();
	set_new_int9_newisr();
	set_Int0x70Handler();

	speed = 1;
	latch = 100;
	ChangeSpeakerLatch();



	resume(dispid = create(displayer, INITSTK, INITPRIO, "DISPLAYER", 0));
	resume(receiver_pid = recvpid = create(receiver, INITSTK, INITPRIO + 3, "RECEIVER", 0));
	resume(uppid = create(updater, INITSTK, INITPRIO, "UPDATER", 0));
	resume(chekpid = create(checker, INITSTK, INITPRIO, "CHECKER", 0));

	schedule(3, 2, dispid, 0, uppid, 1, chekpid, 2);//(Number of pids, number of cycles, the pid and it's insertion...), manage the rotation between them
}//xmain