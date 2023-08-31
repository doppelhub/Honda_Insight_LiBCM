//Copyright 2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles buzzer

#ifndef buzzer_h
	#define buzzer_h

	//Each subsystem stores its buzzer tone request in 2 bits (OFF/LOW/HIGH)
	#define BUZZER_REQUESTOR_MASK 0b11

	//Requestor value correpond to their bit positions in the mask
	#define BUZZER_REQUESTOR_GRIDCHARGER 0
	#define BUZZER_REQUESTOR_USER        2
	#define BUZZER_REQUESTOR_UNUSED_A    4
	#define BUZZER_REQUESTOR_UNUSED_B    6

	//buzzer request status bits (for each subsystem)
	#define BUZZER_OFF  0b00
	#define BUZZER_LOW  0b01
	#define BUZZER_HIGH 0b10

	#define BUZZER_HI_MASK   0b10101010 //OR this mask with buzzerTone_allRequestors to see if any subsystem is requesting high tone buzzer
	#define BUZZER_LO_MASK   0b01010101 //OR this mask with buzzerTone_allRequestors to see if any subsystem is requesting  low tone buzzer
	#define BUZZER_FORCE_OFF 0b00000000

	#define BUZZER_CHANGE_MIN_DURATION_ms (100) //period required before buzzer tone can change

	void buzzer_handler(void);

	uint8_t buzzer_getTone_now(void);
	uint8_t buzzer_getAllRequestors_mask(void);

	void buzzer_requestTone(uint8_t requestor, uint8_t newBuzzerTone); //request buzzer tone //Example: buzzer_requestTone(BUZZER_REQUESTOR_GRIDCHARGER, BUZZER_HIGH)
#endif