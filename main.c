/*
TODO 
스탭모터로 문 열고 닫기 구현 (동작이 이상한 포트가 많아서 일단 건너뜀)
파워 서플라이 등으로 전원 안정적으로 공급 (모터 드라이버 2개에 다 줘야됨, 병렬 될듯 짜피 두 모터 동시에 사용되지 않음)

1024 
오늘 포토인터럽트 적용시켰는데 잘 되는지 확인을 못함. 다음시간에 확인하기

1029 
확인해봤는데 B1에 계속 High가 들어가는 것을 확인함. 
일단 급하게 B0~B3를 G0~G3으로 옮김
이 경우 문을 열고 닫는 동작이 불가능하게 되는데 일단 추후에 도전하기로...
가능은 함. B1을 안쓴다고 햇을때 남는 포트가 4개가 넘긴 한데 포트가 여러군데 나뉘어서 불편할듯
추가로 비상정지(1층복귀) 기능을 구현함.
memset으로 배열 상태를 전부 초기화.
및, 1층에 도달할때까지 하강.

1107
문 열고 닫는 기능을 구현함

*/

#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void ready(){
	DDRA = 0xFF;
	DDRB = 0xF0;
	DDRC = 0xFF;
	DDRD = 0x00;
	DDRE = 0xC0;
	DDRF = 0xFF;
	DDRG = 0x10;
	
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
	
volatile int sw_floorLED[5] = {0,0,0,0,0};
volatile int sw_upLED[5] = {0,0,0,0,0};
volatile int sw_downLED[5] = {0,0,0,0,0};

volatile int photo[5] = {0,0,0,0,0};
volatile const double MotorDel[3] = {5, 15, 10};
volatile int MotorDel_idx = 0;
volatile int curFloor = 1;
volatile int destFloor = -1;
volatile int speedState = 0;
volatile int VeryPowerfulVAR = 0; //1층 복귀 flag
#pragma endregion

#pragma region INTERRUPT
ISR(INT0_vect){
	sw_up[1] = 1;
	sw_upLED[1] = 1;
}
ISR(INT1_vect){
	sw_down[2] = 1;
	sw_downLED[2] = 1;
}
ISR(INT2_vect){
	sw_up[2] = 1;
	sw_upLED[2] = 1;
}
ISR(INT3_vect){
	sw_down[3] = 1;
	sw_downLED[3] = 1;
}
ISR(INT4_vect){
	sw_up[3] = 1;
	sw_upLED[3] = 1;
}
ISR(INT5_vect){
	sw_down[4] = 1;
	sw_downLED[4] = 1;
}
#pragma endregion

//TODO : IMPL open(), close() OKEY? need to check
#pragma region UTILITY
int my_abs(int a){
	if(a<0) return -1*a;
	return a;
}

void my_delay(double x){
	while(x--){_delay_ms(1);}
}

void move_up(){
	PORTB = (PORTB&~0xF0)|0x10; my_delay(MotorDel[MotorDel_idx]);
	PORTB = (PORTB&~0xF0)|0x20; my_delay(MotorDel[MotorDel_idx]);
	PORTB = (PORTB&~0xF0)|0x40; my_delay(MotorDel[MotorDel_idx]);
	PORTB = (PORTB&~0xF0)|0x80; my_delay(MotorDel[MotorDel_idx]);
}

void move_down(){
	PORTB = (PORTB&~0xF0)|0x80; my_delay(MotorDel[MotorDel_idx]);
	PORTB = (PORTB&~0xF0)|0x40; my_delay(MotorDel[MotorDel_idx]);
	PORTB = (PORTB&~0xF0)|0x20; my_delay(MotorDel[MotorDel_idx]);
	PORTB = (PORTB&~0xF0)|0x10; my_delay(MotorDel[MotorDel_idx]);
}

void open(){ //TODO
	int T = 60;
	while(T--){
		PORTE = (PORTE&~0xC0)|0x40; my_delay(4);
		PORTE = (PORTE&~0xC0)|0x80; my_delay(4);
		PORTE = PORTE&~0xC0;
		PORTF = (PORTF&~0xC0)|0x40; my_delay(4);
		PORTF = (PORTF&~0xC0)|0x80; my_delay(4);
		PORTF = PORTF&~0xC0;	
	}
}

void close(){ //TODO
	int T = 60;
	while(T--){
		PORTF = (PORTF&~0xC0)|0x80; my_delay(4);
		PORTF = (PORTF&~0xC0)|0x40; my_delay(4);
		PORTF = PORTF&~0xC0;
		PORTE = (PORTE&~0xC0)|0x80; my_delay(4);
		PORTE = (PORTE&~0xC0)|0x40; my_delay(4);
		PORTE = PORTE&~0xC0;
	}
}

void updateSw(){
	int D1 = (~PIND) & 0xF0;
	if(D1 & 0x10){ //이동속도 설정
		speedState = (speedState+1)%3;
		if(speedState==0) MotorDel_idx = 2; //일반
		if(speedState==1) MotorDel_idx = 0; //고속
		if(speedState==2) MotorDel_idx = 1; //저속
	}
	
	if(D1 & 0x80){ //비상정지
		VeryPowerfulVAR = 1;
	}
	
	//만약에 현재 층이 특정 층이고, 
	int D2 = (~PINE) & 0x0F;
	if((D2 & 0x01) && (curFloor!=1 || isTimerFin)) {sw_floor[1] = 1; sw_floorLED[1] = 1;}
	if((D2 & 0x02) && (curFloor!=2 || isTimerFin)) {sw_floor[2] = 1; sw_floorLED[2] = 1;}
	if((D2 & 0x04) && (curFloor!=3 || isTimerFin)) {sw_floor[3] = 1; sw_floorLED[3] = 1;}
	if((D2 & 0x08) && (curFloor!=4 || isTimerFin)) {sw_floor[4] = 1; sw_floorLED[4] = 1;}
	
	//엘리베이터 내부에서 문이 열려있는 상태에 현재 층 버튼을 누르면 적용되면 안됨.
	//이를 한 줄짜리 조건식으로 줄임
	int D3 = PING&0x0F;
	if(PINB & 0x08) photo[1] = 1;
	else photo[1] = 0;
	if(D3 & 0x02) photo[2] = 1;
	else photo[2] = 0;
	if(D3 & 0x04) photo[3] = 1;
	else photo[3] = 0;
	if(D3 & 0x08) photo[4] = 1;
	else photo[4] = 0;
}

void updateLed(){
	if(sw_floorLED[1]) PORTC |= 0x10; 
	else PORTC &= ~0x10;
	
	if(sw_floorLED[2]) PORTC |= 0x20;
	else PORTC &= ~0x20;
	
	if(sw_floorLED[3]) PORTC |= 0x40;
	else PORTC &= ~0x40;
	
	if(sw_floorLED[4]) PORTC |= 0x80;
	else PORTC &= ~0x80;
	
	if(sw_upLED[1]) PORTF |= 0x01;
	else PORTF &= ~0x01;
	
	if(sw_downLED[2]) PORTF |= 0x02;
	else PORTF &= ~0x02;
	
	if(sw_upLED[2]) PORTF |= 0x04;
	else PORTF &= ~0x04;
	
	if(sw_downLED[3]) PORTF |= 0x08;
	else PORTF &= ~0x08;
	
	if(sw_upLED[3]) PORTF |= 0x10;
	else PORTF &= ~0x10;
	
	if(sw_downLED[4]) PORTF |= 0x20;
	else PORTF &= ~0x20;
}

void updateFndAnd3LEDs(){
	PORTA = FND[curFloor];
	PORTC &= ~0x07;
	if(destFloor==-1) return;
	if(curFloor<destFloor) PORTC |= 0x03;
	if(curFloor>destFloor) PORTC |= 0x06;
}
#pragma endregion

ISR(TIMER0_OVF_vect){
	TCNT0 = 256 - 250;
	cnt++;
	int D1 = (~PIND) & 0x60;
	//3초 지났고, 문 열기 버튼이 꺼져있고, 해당 층의 상/하 버튼도 꺼져있는 경우
	//2초 지났고, 문 닫힘 버튼이 눌려있는 경우
	if((cnt>=24000 && !(D1&0x20)) || (cnt>=16000 && (D1&0x40))){
		close();
		PORTG &= ~0x10;
		isTimerFin = 1;
		cnt = 0;
		TIMSK = 0x00;
	}
}

int main(void){
    ready();
    while(1){
		//전부 초기화하고, 1층으로 복귀
		if(VeryPowerfulVAR && isTimerFin){
			memset(sw_down, 0, sizeof(sw_down));
			memset(sw_floor, 0, sizeof(sw_floor));
			memset(sw_up, 0, sizeof(sw_up));
			memset(sw_downLED, 0, sizeof(sw_downLED));
			memset(sw_floorLED, 0, sizeof(sw_floorLED));
			memset(sw_upLED, 0, sizeof(sw_upLED));
			VeryPowerfulVAR = 0;
			MotorDel_idx = 0;
			curFloor = 1;
			destFloor = -1;
			speedState = 0;
			updateSw();
			updateLed();
			updateFndAnd3LEDs();
			while(photo[1]==0){
				updateSw();
				move_down();
				updateSw();
			}
		}
		
		updateSw();
		updateLed();
		updateFndAnd3LEDs();
		
		//목적지가 정해지지 않았다면 현재 층에서 거리가 가장 먼 버튼이 눌린 층을 찾고, destFloor에 저장한다.
		if(destFloor==-1 && isTimerFin==1){
			int gap = -1;
			for(int i=curFloor; i<=4; i++){
				if(sw_up[i] || sw_down[i] || sw_floor[i]){
					if(my_abs(i-curFloor) > gap){
						gap = my_abs(i-curFloor);
						destFloor = i;
					}
				}
			}
			for(int i=curFloor; i>=1; i--){
				if(sw_up[i] || sw_down[i] || sw_floor[i]){
					if(my_abs(i-curFloor) > gap){
						gap = my_abs(i-curFloor);
						destFloor = i;
					}
				}
			}
			if(destFloor!=-1) sw_up[destFloor] = sw_down[destFloor] = sw_floor[destFloor] = 0;
		}
		
		updateSw();
		updateLed();
		updateFndAnd3LEDs();
		
		//목적지가 정해졌다면
		if(destFloor!=-1 && isTimerFin==1){
			int dir = (curFloor<destFloor)?1:-1;
			while(curFloor!=destFloor){
				if(VeryPowerfulVAR && isTimerFin) break;
				
				if(isTimerFin==0){ //타이머가 끝날때까지 대기
					updateSw();
					updateLed();
					updateFndAnd3LEDs();
					continue;
				}
				
				if(((dir==-1&&sw_down[curFloor])||(dir==1&&sw_up[curFloor])||sw_floor[curFloor])&&photo[curFloor]==1){
					sw_floor[curFloor] = 0;
					sw_floorLED[curFloor] = 0;
					if(dir == -1) {sw_down[curFloor] = 0; sw_downLED[curFloor] = 0;}
					if(dir == 1) {sw_up[curFloor] = 0; sw_upLED[curFloor] = 0;}
					updateLed();
					open();
					isTimerFin=0;
					TIMSK = 0x01;
					PORTG |= 0x10;
					continue;
				}
				
				if(dir==1) move_up(); //dir이 1이면 위로 이동
				else move_down(); //dir이 -1이면 아래로 이동
				updateSw();
				updateLed();
				
				//다음 층 이동이 물리적으로 완료되었다면 현재 층 정보도 업데이트해줌
				if(photo[curFloor+dir]==1) curFloor+=dir; //0으로 일단 했는데 나중에 포토인터럽트 오면 고쳐야됨
				updateFndAnd3LEDs();
			}
			if(!(VeryPowerfulVAR && isTimerFin)){
				sw_floorLED[destFloor] = sw_downLED[destFloor] = sw_upLED[destFloor] = 0;
				updateSw();
				updateLed();
				updateFndAnd3LEDs();
				
				open();
				isTimerFin=0;
				TIMSK = 0x01;
				PORTG |= 0x10;
				
				sw_up[destFloor] = sw_down[destFloor] = sw_floor[destFloor] = 0;
				sw_floorLED[destFloor] = sw_downLED[destFloor] = sw_upLED[destFloor] = 0;
				destFloor = -1; //목적지를 지워준다.	
			}
		}
    }
}
