/*
문 열림버튼, 닫힘버튼 OK
버튼에 취소 기능을 없앰. 한 번 누르면 취소 불가
타이머 실행중일 때 엘리베이터 내부에서 현재 층 버튼 못누르게 수정

아쉬운 점
- 실제 엘리베이터라면 엘리베이터 외부에서 상/하 버튼 누르고 있으면 엘베 문 계속 열려있어야되는데 일단 구현하지 않을 예정임. (로직이 꼬임)
- 엘리베이터 외부의 상/하 버튼도 뭐가 눌렸는지에 따라 다른 처리가 필요함. 하지만 로직이 복잡해지므로 일단 배제. 추후에도 수정하고 싶진 않음...

그리고 버튼마다 달린 불은 해당 층에 도달한 후에 꺼져야됨.
업데이트 시점을 조절하거나 로직을 좀 많이 바꿔야될듯

다 해결된 코드
보완점이라고 한다면 버튼 처리 우선순위 로직을 컨테이너 자료형을 구현해서 만드는 것 정도?
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
	DDRE = 0x00;
	DDRF = 0xFF;
	DDRG = 0x01;
	
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
volatile const double MotorDel[3] = {5, 10, 15};
volatile int MotorDel_idx = 0;
volatile int curFloor = 1;
volatile int destFloor = -1;
volatile int speedState = 0;
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

//TODO : IMPL open(), close(), updateSw()
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
	
}

void close(){ //TODO
	
}

void updateSw(){
	int D1 = (~PIND) & 0xF0;
	if(D1 & 0x10){ //이동속도 설정
		speedState = (speedState+1)%3;
		if(speedState==0) MotorDel_idx = 2; //자동 (쓰레기값을 넣어둠)
		if(speedState==1) MotorDel_idx = 0; //고속
		if(speedState==2) MotorDel_idx = 1; //저속
	}
	
	if(D1 & 0x80){ //비상정지
		
	}
	
	//만약에 현재 층이 특정 층이고, 
	int D2 = (~PINE) & 0x0F;
	if((D2 & 0x01) && (curFloor!=1 || isTimerFin)) {sw_floor[1] = 1; sw_floorLED[1] = 1;}
	if((D2 & 0x02) && (curFloor!=2 || isTimerFin)) {sw_floor[2] = 1; sw_floorLED[2] = 1;}
	if((D2 & 0x04) && (curFloor!=3 || isTimerFin)) {sw_floor[3] = 1; sw_floorLED[3] = 1;}
	if((D2 & 0x08) && (curFloor!=4 || isTimerFin)) {sw_floor[4] = 1; sw_floorLED[4] = 1;}
	
	//엘리베이터 내부에서 문이 열려있는 상태에 현재 층 버튼을 누르면 적용되면 안됨.
	//이를 한 줄짜리 조건식으로 줄임
	int D3 = (~PINB) & 0x0F;
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
		/*
		//TEST
		_delay_ms(500);
		PORTA = FND[5];
		_delay_ms(500);
		PORTA = FND[curFloor];
		*/
		close();
		PORTG = 0x00;
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
				if(isTimerFin==0){ //타이머가 끝날때까지 대기
					updateSw();
					updateLed();
					updateFndAnd3LEDs();
					continue;
				}
				
				//if(curFloor==destFloor) break;
				//현재 층에 타거나 내릴 사람이 있다면
				//10.17 수정 : 물리적으로 해당 층에 있을때만 열려야됨.
				if(((dir==-1&&sw_down[curFloor])||(dir==1&&sw_up[curFloor])||sw_floor[curFloor])&&photo[curFloor]==0){
					sw_floor[curFloor] = 0;
					sw_floorLED[curFloor] = 0;
					if(dir == -1) {sw_down[curFloor] = 0; sw_downLED[curFloor] = 0;}
					if(dir == 1) {sw_up[curFloor] = 0; sw_upLED[curFloor] = 0;}
					updateLed();
					open();
					isTimerFin=0;
					TIMSK = 0x01;
					PORTG = 0x01;
					/*
					//TEST
					_delay_ms(500);
					PORTA = FND[0];
					_delay_ms(500);
					PORTA = FND[curFloor];
					*/
					continue;
				}
				//sw_up[curFloor] = sw_down[curFloor] = sw_floor[curFloor] = 0; //일단 채터링때문에 추가했는데 추후에 문제 생기면 삭제.
				
				if(dir==1) move_up(); //dir이 1이면 위로 이동
				else move_down(); //dir이 -1이면 아래로 이동
				updateSw();
				updateLed();
				
				//다음 층 이동이 물리적으로 완료되었다면 현재 층 정보도 업데이트해줌
				if(photo[curFloor+dir]==0) curFloor+=dir; //0으로 일단 했는데 나중에 포토인터럽트 오면 고쳐야됨
				updateFndAnd3LEDs();
			}
			sw_floorLED[destFloor] = sw_downLED[destFloor] = sw_upLED[destFloor] = 0;
			updateSw();
			updateLed();
			updateFndAnd3LEDs();
			
			open();
			isTimerFin=0;
			TIMSK = 0x01;
			PORTG = 0x01;
			
			/*
			//TEST
			_delay_ms(500);
			PORTA = FND[0];
			_delay_ms(500);
			PORTA = FND[curFloor];
			*/
			
			sw_up[destFloor] = sw_down[destFloor] = sw_floor[destFloor] = 0;
			sw_floorLED[destFloor] = sw_downLED[destFloor] = sw_upLED[destFloor] = 0;
			destFloor = -1; //목적지를 지워준다.
		}
    }
}

//10.15
//왜 타이머가 예상 시간보다 길게 지속되는지 모르겠음
//채터링 때문인 것을 발견.
//채터링은 물리적으로 제거가 불가능하기에 문이 열려있을 때에만 문을 닫게 하는 식으로 로직을 변경을 해야 할 것 같음
//방향 버튼은 잘 됐는데, 일반 층 버튼이 되지 않던 것은 아마 방향 버튼은 외부 인터럽트를 사용했지만 일반 층 버튼은 사용하지 않았기 때문인 것으로 추정함

/*
초기 상태에서 4층 버튼을 누른 후, 2층 상승, 3층 상승을 누른 경우
2층에서 문 한번 열렸다 닫히고,
3층에서 문 한번 열렸다 닫히고
4층에서 문 한 번 열렸다 닫히고.
인데 현재 4층에서만 문이 두 번 열렸다 닫힘

해결: while문 종류 이후에 강제로 목적지 층의 버튼 정보를 초기화시킴
*/
