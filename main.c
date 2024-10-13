#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void ready(){
	DDRA = 0xFF;
	DDRB = 0xF0;
	DDRC = 0xFF;
	DDRD = 0x00;
	DDRE = 0x00;
	DDRF = 0xFF;
	
	EIMSK = 0x3F;
	EICRA = 0b10101010;
	EICRB = 0b00001010;
	
	TCCR0 = 0x02;
	TCNT0 = 256 - 250;
	TIMSK = 0x00; //타이머는 초기에 꺼둠
	sei();
}

#pragma region VARIABLES, Constants
const int FND[] = {0b01000000, 0b01111001, 0b00100100, 0b00110000, 0b00011001, 0b0010010, 0b0000010, 0b11111000, 0b00000000, 0b00011000, 0b01111111};
volatile unsigned int cnt = 0; //타이머 기록 저장용
volatile int isTimerFin = 1; //타이머가 끝났는가?
volatile int sw_floor[5] = {0,0,0,0,0};
volatile int sw_up[5] = {0,0,0,0,0};
volatile int sw_down[5] = {0,0,0,0,0};
volatile int photo[5] = {0,0,0,0,0};
volatile double MotorDel = 10;
volatile int curFloor = 1;
volatile int destFloor = -1;
#pragma endregion

#pragma region INTERRUPT
ISR(INT0_vect){
	sw_up[1] = !sw_up[1];	
}
ISR(INT1_vect){
	sw_down[2] = !sw_down[2];
}
ISR(INT2_vect){
	sw_up[2] = !sw_up[2];
}
ISR(INT3_vect){
	sw_down[3] = !sw_down[3];
}
ISR(INT4_vect){
	sw_up[3] = !sw_up[3];
}
ISR(INT5_vect){
	sw_down[4] = !sw_down[4];
}
#pragma endregion

//TODO : IMPL move_up(), move_down(), open(), close(), updateSw()
#pragma region UTILITY
void move_up(){
	_delay_ms(MotorDel);
	_delay_ms(MotorDel);
	_delay_ms(MotorDel);
	_delay_ms(MotorDel);
}

void move_down(){
	_delay_ms(MotorDel);
	_delay_ms(MotorDel);
	_delay_ms(MotorDel);
	_delay_ms(MotorDel);
}

void open(){
	
}

void close(){
	
}

void updateSw(){
	int D1 = PIND & 0xF0;
	if(D1 & 0x10) MotorDel = 5; //고속이동
	if(D1 & 0x20) MotorDel = 10; //저속이동
	if(D1 & 0x40){ //자동 속도 조절
		
	}
	if(D1 & 0x80){ //비상정지
		
	}
	
	int D2 = PINE & 0x0F;
	if(D2 & 0x01) sw_floor[1] = !sw_floor[1];
	if(D2 & 0x02) sw_floor[2] = !sw_floor[2];
	if(D2 & 0x04) sw_floor[3] = !sw_floor[3];
	if(D2 & 0x08) sw_floor[4] = !sw_floor[4];
	
	int D3 = PINB & 0x0F;
	if(D3 & 0x01) photo[1] = 1;
	else photo[1] = 0;
	if(D3 & 0x02) photo[2] = 1;
	else photo[2] = 0;
	if(D3 & 0x04) photo[3] = 1;
	else photo[3] = 0;
	if(D3 & 0x08) photo[4] = 1;
	else photo[4] = 0;
}

void updateLed(){
	if(sw_floor[1]) PORTC |= 0x10; 
	else PORTC &= ~0x10;
	
	if(sw_floor[2]) PORTC |= 0x20;
	else PORTC &= ~0x20;
	
	if(sw_floor[3]) PORTC |= 0x40;
	else PORTC &= ~0x40;
	
	if(sw_floor[4]) PORTC |= 0x80;
	else PORTC &= ~0x80;
	
	if(sw_up[1]) PORTF |= 0x01;
	else PORTF &= ~0x01;
	
	if(sw_down[2]) PORTF |= 0x02;
	else PORTF &= ~0x02;
	
	if(sw_up[2]) PORTF |= 0x04;
	else PORTF &= ~0x04;
	
	if(sw_down[3]) PORTF |= 0x08;
	else PORTF &= ~0x08;
	
	if(sw_up[3]) PORTF |= 0x10;
	else PORTF &= ~0x10;
	
	if(sw_down[4]) PORTF |= 0x20;
	else PORTF &= ~0x20;
}

void updateFndAnd3LEDs(){
	PORTA = FND[curFloor];
	PORTC &= ~0x07;
	if(curFloor<destFloor) PORTC |= 0x03;
	if(curFloor>destFloor) PORTC |= 0x06;
}
#pragma endregion

ISR(TIMER0_OVF_vect){
	TCNT0 = 256 - 250;
	cnt++;
	if(cnt==24000){ //3초 지나면
		close();
		isTimerFin = 1;
		cnt = 0;
		TIMSK = 0x00;
	}
}

int main(void){
    ready();
    while(1){
		updateSw();
		updateLed();
		updateFndAnd3LEDs();
		
		//목적지가 정해지지 않았다면 현재 층에서 거리가 가장 먼 버튼이 눌린 층을 찾고, destFloor에 저장한다.
		if(destFloor==-1 && isTimerFin==1){
			int gap = -1;
			for(int i=curFloor; i<=4; i++){
				if(sw_up[i] || sw_down[i] || sw_floor[i]){
					if(abs(i-curFloor) > gap){
						gap = abs(i-curFloor);
						destFloor = i;
					}
				}
			}
			for(int i=curFloor; i>=1; i--){
				if(sw_up[i] || sw_down[i] || sw_floor[i]){
					if(abs(i-curFloor) > gap){
						gap = abs(i-curFloor);
						destFloor = i;
					}
				}
			}
			if(destFloor!=-1) sw_up[destFloor] = sw_down[destFloor] = sw_floor[destFloor] = 0;
		}
		
		//목적지가 정해졌다면
		if(destFloor!=-1 && isTimerFin==1){
			int dir = (curFloor<destFloor):1?-1;
			while(curFloor!=destFloor){
				if(isTimerFin==0){ //타이머가 끝날때까지 대기
					updateSw();
					updateLed();
					updateFndAnd3LEDs();
					continue;
				}
				
				//현재 층에 타거나 내릴 사람이 있다면
				if((dir==-1&&sw_down[curFloor])||(dir==1&&sw_up[curFloor])||sw_floor[curFloor]){
					sw_up[curFloor] = sw_down[curFloor] = sw_floor[curFloor] = 0;
					open();
					isTimerFin=0;
					TIMSK = 0x01;
					continue;
				}
				
				if(dir==1) move_up(); //dir이 1이면 위로 이동
				else move_down(); //dir이 -1이면 아래로 이동
				updateSw();
				updateLed();
				
				//다음 층 이동이 물리적으로 완료되었다면 현재 층 정보도 업데이트해줌
				if(photo[curFloor+dir]) curFloor+=dir;
				updateFndAnd3LEDs();
			}
			open();
			TIMSK = 0x01;
			isTimerFin = 0;
			destFloor = -1; //목적지를 지워준다.
		}
    }
}
