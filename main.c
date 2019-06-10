/*
 * vamat001_FinalProject.c
 *
 * Created: 5/29/2019 3:47:54 PM
 * Author : vamat
 */  
#define F_CPU   1000000UL

#include <time.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include "io.c"
#include <string.h> 
#include <stdio.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <stdlib.h> 
#include <util/delay.h> 
#include "nokia5110.c" 
#include <stdlib.h>

volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;



void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1=0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B = 0x00;
}

void TimerISR() {
	TimerFlag = 1;
}


ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void startGame();

unsigned char Back[8] = { 0x01, 0x03, 0x07, 0x0F, 0x07, 0x03, 0x01, 0x00 }; 

#define B1 (~PINA & 0x04) 
#define B2 (~PINA & 0x08) 
#define B3 (~PINA & 0x10) 

#define ON 0x01
#define OFF 0x00

unsigned char Game = OFF; 
unsigned short score = 0; 
unsigned short highScore = 0; 
unsigned short level = 1;

void LCD_Custom_Char (unsigned char loc, unsigned char *msg)
{
	unsigned char i;
	if(loc<8)
	{
		LCD_WriteCommand (0x40 + (loc*8));	/* Command 0x40 and onwards forces the device to point CGRAM address */
		for(i=0;i<8;i++)	/* Write 8 byte for generation of 1 character */
		LCD_WriteData(msg[i]);
	}
}  

void updateScore(unsigned short currScore) {  
	LCD_Cursor(23); 
	if(currScore > 9) {
		LCD_WriteData('0' + (currScore / 10)); 
		LCD_Cursor(24); 
		LCD_WriteData('0' + (currScore % 10)); 
	} else {
		LCD_WriteData('0' + currScore); 
	}
	LCD_Cursor(33);
} 

void updateHighScore(unsigned char currScore) {  
	LCD_Cursor(12);
	if(currScore > 9) {
		LCD_WriteData('0' + (currScore / 10));
		LCD_Cursor(13);
		LCD_WriteData('0' + (currScore % 10));
		} else {
		LCD_WriteData('0' + currScore);
	}
	LCD_Cursor(33);
} 

void updateLevel() {
	LCD_Cursor(7);
	LCD_WriteData('0' + level);
	LCD_Cursor(33);
}

enum Menu_States { Menu_Init, Menu_Display, Menu_Select, Menu_HighScore, Menu_Play, Menu_ClearLCD, Menu_PlayScreen, Menu_DisplayHighScore, Menu_GameOverScreen, Menu_GameOver } Menu_State; 

int Menu(int Menu_State) {
	
	switch( Menu_State ) { //transitions
		case Menu_Init: 
			Menu_State = Menu_Display;
			break;  
		case Menu_Display: 
			Menu_State = Menu_Select; 
			break;
		case Menu_Select: 
			if(B1) {
				Menu_State = Menu_PlayScreen; 
			} else if(B3) {
				Menu_State = Menu_DisplayHighScore; 

			} else {
				Menu_State = Menu_Select; 
			} 
			break; 
		case Menu_DisplayHighScore: 
			Menu_State = Menu_HighScore; 
			break; 
		case Menu_HighScore: 
			if(B2) {
				Menu_State = Menu_Display; 
			} else {
				Menu_State = Menu_HighScore; 
			} 
			break;  
		case Menu_PlayScreen: 
			Menu_State = Menu_Play; 
			break;
		case Menu_Play: 
			Menu_State = Menu_GameOverScreen;
			break;  
		case Menu_GameOverScreen: 
			Menu_State = Menu_GameOver;
			break;
		case Menu_GameOver: 
			if(B2) {
				Menu_State = Menu_Display;  
				nokia_lcd_clear(); 
				nokia_lcd_render();
			} else {
				Menu_State = Menu_GameOver;
			} 
			break;
		default: 
			Menu_State = Menu_Init; 
			break; 
	} 
	
	switch( Menu_State ) { //actions
		case Menu_Init:   
			score = 0; 
			highScore = 0;
			break;
		case Menu_Display: 
			LCD_DisplayString(1, "B1:Play B2:Back B3:High Score");
			LCD_Cursor(16);
			LCD_WriteData(0);   
			//nokia_lcd_write_image(Bitmap); 
			startScreen(); 
			nokia_lcd_render();
			break;  
		case Menu_Select: 
			break; 
		case Menu_DisplayHighScore: 
			LCD_ClearScreen();
			LCD_DisplayString(1, "High Score:"); 
			LCD_Cursor(16); 
			LCD_WriteData(0); 
			LCD_Cursor(33); 
			updateHighScore(highScore);
			break;
		case Menu_HighScore:
			break;
		case Menu_PlayScreen:
			LCD_ClearScreen(); 
			LCD_Cursor(1);
			LCD_DisplayString(1, "Level:1         Score:0");  
			LCD_Cursor(16); 
			LCD_WriteData(0);
			LCD_Cursor(33);
			break;
		case Menu_Play: 
			startGame(); 
			break; 
		case Menu_GameOverScreen: 
			nokia_lcd_clear(); 
			nokia_lcd_write_image(GameOver);   
			nokia_lcd_render(); 
			if(score >= highScore) {
				highScore = score;
				eeprom_update_byte((uint8_t*)1, (uint8_t)highScore);
			}  
// 			highScore = 0; 
// 			eeprom_update_byte((uint8_t*)1, (uint8_t)highScore);
			score = 0; 
			level = 1;
			break; 
		case Menu_GameOver: 
			break;
		default:
			break; 
	} 
	return Menu_State;
}

enum Game_States { Game_Init, Game_Wait, Game_Display, Game_Start, Game_Run, Game_Win, Game_Lose, Game_End, Game_Restart } Game_State; 

int Game(int Game_State) {
	switch ( Game_State ) { //transitions
		case Game_Init: 
			Game_State = Game_Wait;
			break; 
		case Game_Wait: 
			if(Game) {
				Game_State = Game_Display;
			} else {
				Game_State = Game_Wait;
			} 
			break; 
		case Game_Display: 
			Game_State = Game_Start; 
			break;
		case Game_Start: 
			Game_State = Game_Run;
			break;
		case Game_Run: 
			if(win) {
				Game_State = Game_Win;
			} else if(lose) {
				Game_State = Game_Lose;
			} else {
				Game_State = Game_Run;
			} 
			break; 
		case Game_Win: 
			Game_State = Game_End; 
			break; 
		case Game_Lose: 
			Game_State = Game_End; 
			break;
		case Game_End: 
			Game_State = Game_Wait; 
			break;
		default: 
			Game_State = Game_Init; 
			break;
	} 
	
	switch ( Game_State ) { //actions
		case Game_Init: 
			score = 0; 
			playerPos = 1; 
			enemy1Pos = 1; 
			enemy2Pos = (rand() % 4) + 1;
			break;
		case Game_Display: 
			startGame(); 
			break;
	} 
	return Game_State;
} 

// enum Button_States { Button_Init, Button_Press, Button1_Release, Button2_Release, Button3_Release } Button_State; 
// 
// unsigned char button1 = 0; 
// unsigned char button2 = 0; 
// unsigned char button3 = 0; 
// 
// void Button() {
// 	switch ( Button_State ) {
// 		case Button_Init: 
// 			Button_State = Button_Press;  
// 			break;
// 		case Button_Press: 
// 			if(B1){ 
// 				button1 = 0;
// 				Button_State = Button1_Release; 
// 			} else if(B2){ 
// 				button2 = 0;
// 				Button_State = Button2_Release;
// 			} else if(B3){ 
// 				button3 = 0;
// 				Button_State = Button3_Release;
// 			} else {
// 				Button_State = Button_Press;
// 			}
// 			break; 
// 		case Button1_Release: 
// 			if(!B1){ 
// 				button1 = 1;
// 				Button_State = Button_Press;
// 			} else {
// 				Button_State = Button1_Release;
// 			}
// 			break; 
// 		case Button2_Release: 
// 			if(!B2){ 
// 				button2 = 1;
// 				Button_State = Button_Press;
// 				} else {
// 				Button_State = Button2_Release;
// 			}
// 			break; 
// 		case Button3_Release: 
// 			if(!B3){ 
// 				button3 = 1;
// 				Button_State = Button_Press;
// 				} else {
// 				Button_State = Button3_Release;
// 			}
// 			break; 
// 		default: 
// 			Button_State = Button_Init;
// 	} 
// }

void Level_Controller(unsigned short game_score) //Increase the speed of game based on the score.
{
	if (game_score>=0 && game_score<=2) //If score 0-10
	{
		delay_ms(200); //slow the game by 80ms 
		level = 1;
	}
	if (game_score>2 && game_score<=5) //If score 10-40
	{
		delay_ms(100); //slow the game by 70ms 
		level = 2;
	}
	if (game_score>5 && game_score<=8) //If score 20-40
	{
		delay_ms(50); //slow the game by 60ms 
		level = 3;
	}
	if (game_score>8 && game_score<=20) //If score 30-40
	{
		delay_ms(25); //slow the game by 50ms 
		level = 4;
	}  
	if (game_score>20) //If score 30-40
	{
		delay_ms(10); //slow the game by 50ms
		level = 5;
	}
}

void startGame(){   
	unsigned short enemy1Pos, enemy2Pos, enemy3Pos; 
	unsigned short enemy_phase;
	unsigned char enemy_dead = 1;
	unsigned int pos = (rand() % 4) + 1; 
	nokia_lcd_clear();
	nokia_lcd_drawBorders();
	nokia_lcd_drawCharacter(pos);
	while(1) {
		if(B1) { 
		//if(button1) {
			if(pos > 1) {
				eraseCharacter(pos); 
				pos--; 
				nokia_lcd_drawCharacter(pos);
			}
		} else if(B3) { 
			//else if(button3) {
			if(pos < 4) {
				eraseCharacter(pos); 
				pos++; 
				nokia_lcd_drawCharacter(pos);
			}
		} else if(B2) {
			break;
		}
		//_delay_ms(800); 
		if(enemy_dead) {  
			enemy1Pos = pos;
			enemy2Pos = (rand() % 4) + 1;  
			enemy3Pos = (rand() % 4) + 1;
			enemy_phase = 2; 
			enemy_dead = 0; 
		} 
		nokia_lcd_drawEnemy(enemy1Pos, enemy_phase); 
		nokia_lcd_drawEnemy(enemy2Pos, enemy_phase); 
		nokia_lcd_drawEnemy(enemy3Pos, enemy_phase); 
		nokia_lcd_render();
		enemy_phase++;  
		if (enemy_phase>20 && ((enemy1Pos == pos) || (enemy2Pos == pos) || (enemy3Pos == pos))) { 
			break;
		} 
		if(enemy_phase > 32) {
			enemy_dead = 1;  
			score++;  
			updateScore(score);
			eraseEnemy(enemy1Pos); 
			eraseEnemy(enemy2Pos); 
			eraseEnemy(enemy3Pos);
			nokia_lcd_render();
		}  
		Level_Controller(score); 
		updateLevel();
	}
} 

typedef struct _task {
	//--------Task scheduler data structure---------------------------------------
	// Struct for Tasks represent a running process in our simple real-time operating system.
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
	//--------End Task scheduler data structure-----------------------------------
} task; 

unsigned long int findGCD(unsigned long int a, unsigned long int b) {
	//--------Find GCD function --------------------------------------------------
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
	//--------End find GCD function ----------------------------------------------
	
}

int main(void)
{ 
	DDRA = 0x03; PORTA = 0xFC;
	DDRD = 0xFF; PORTD = 0x00;  
	DDRB = 0xFF; PORTB = 0x00; 
	
	//eeprom 
	if(eeprom_read_byte((uint8_t*)1) == 255) {
		eeprom_update_byte((uint8_t*)1, (uint8_t) 0);
	} 
	highScore = eeprom_read_byte((uint8_t*)1);
	
	srand(time(0));
	nokia_lcd_init();
	nokia_lcd_clear;    
	// Period for the tasks
	unsigned long int SMTick1_calc = 200;
	unsigned long int SMTick2_calc = 50;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;


	//Declare an array of tasks
	static task task1, task2;
	task *tasks[] = { &task1, &task2 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &Menu;//Function pointer for the tick.

	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = SMTick2_period;//Task Period.
	task2.elapsedTime = SMTick2_period;//Task current elapsed time.
	task2.TickFct = &Game;//Function pointer for the tick

	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	
	unsigned short i; // Scheduler for-loop iterator
	while(1) {
		numGet = GetKeypadKey();
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}

	// Error: Program should not exit!
	return 0;
}
// 	unsigned short enemy1, enemy2, enemy3, player;
// 	while(1) {
// 		enemy1 = (rand() % 4) + 1; 
// 		enemy2 = (rand() % 4) + 1; 
// 		enemy3 = (rand() % 4) + 1;   
// 		for(unsigned short i = 0; i < 31; i++) {
// 			nokia_lcd_drawEnemy(enemy1, i + 2); 
//  			nokia_lcd_drawEnemy(enemy2, i + 2);  
//  			nokia_lcd_drawEnemy(enemy3, i + 2); 
// 			nokia_lcd_render();
// 			_delay_ms(1);
// 		} 
// 		eraseEnemy(enemy1); 
// 		eraseEnemy(enemy2); 
// 		eraseEnemy(enemy3); 
// 		nokia_lcd_render();
// 	}
// 	TimerOn(); 
// 	TimerSet(200); 
// 	LCD_init();    
// 	LCD_Custom_Char(0, Back);  /* Build Back Character at position 0 */   
// 	Menu_State = Menu_Init; 
//     while (1) 
//     { 
// 		//startGame(); 
// 		//Button();		
// 		Menu();
// 		//Game(); 
// 		while(!TimerFlag); 
// 		TimerFlag = 0;
//     }
}

