
#include "parse.h"

typedef struct arg{
	uint8_t args[4];
}arg_t;

typedef struct cmd{
	arg_t field[5];
}cmd_t;

cmd_t mCmd;

QueueHandle_t xCmdIN;

void ExecCmd(cmd_t* lastCMD);
void LedExe(void);
void MemWExe(uint8_t *mem);
void MemRExe(uint8_t mem);

void mTaskGetCmd(void *parm){
	uint8_t cmd [32];
	uint8_t index_l = 0;
	uint8_t index_cmd = 0;
	xCmdIN = xQueueCreate(8, sizeof(cmd));
	while(1){
		if(index_l != outRx_index){
			if(buffer[index_l] == '\n'){
				index_cmd = 0;
				while (xQueueSendToBack(xCmdIN, cmd, 0) != pdPASS){
					vTaskDelay(5);
				}
			}
			else if(buffer[index_l] == '\r'){
				cmd[index_cmd] = 0x00;
			}
			else{
				cmd[index_cmd] = buffer[index_l];
				index_cmd++;
			}
			index_l++;
		}
		vTaskDelay(10);
	}
}

void mTaskParseCmd(void *parm){
	TaskHandle_t xHandleExecCmd = (TaskHandle_t) parm;
	uint8_t cmd [32];
	uint8_t fields, chars, error;
	while(1){
		while(uxQueueMessagesWaiting(xCmdIN) !=0){
			if(xQueueReceive(xCmdIN, cmd, 0) == pdPASS ){
				fields = 0;
				chars = 0;
				error = 0;
				for(int i = 0; i < 30; i++){
					if (cmd[i] == 0)
						break;
					else if(cmd[i] == ' '){
						mCmd.field[fields].args[chars] = 0x00;
						fields++;
						chars = 0;
					}
					else {
						if(chars < 4){
							mCmd.field[fields].args[chars] = cmd[i];
							chars++;
						}
						else{
							error = 1;
							break;
						}
					}
				}
				if(error){
					printDMA("CMD not found\r\n", 15);
				}
				else{
					xTaskNotify(xHandleExecCmd, (1<<0), eSetBits );
				}
			}
		}
		vTaskDelay(10);
	}
}

void mTaskExecCmd(void *parm){
	static cmd_t lastCMD;
	uint32_t noti = 0;
	while(1){
		if(xTaskNotifyWait((1<<0), 0x00, &noti, portMAX_DELAY) == pdPASS){
			ExecCmd(&lastCMD);
		}
	}
}

void ExecCmd(cmd_t* lastCMD){
	static uint8_t memory;
	if(mCmd.field[0].args[0] == 'L' && mCmd.field[0].args[1] == 'D'){
		LedExe();
		*lastCMD = mCmd;
	}
	else if(mCmd.field[0].args[0] == 'M' && mCmd.field[0].args[1] == 'W'){
		MemWExe(&memory);
		*lastCMD = mCmd;
	}
	else if(mCmd.field[0].args[0] == 'M' && mCmd.field[0].args[1] == 'R'){
		MemRExe(memory);
		*lastCMD = mCmd;
	}
	else if(mCmd.field[0].args[0] == 'A' && mCmd.field[0].args[1] == 'D' && mCmd.field[0].args[2] == 'C'){
		printDMA("Not implemented, yet!!\r\n", 24);
		*lastCMD = mCmd;
	}
	else if(mCmd.field[0].args[0] == 'V' && mCmd.field[0].args[1] == 'E' && mCmd.field[0].args[2] == 'R'){
		printDMA("vession: 0.1.1 - ACL\r\n", 22);
		*lastCMD = mCmd;
	}
	else if(mCmd.field[0].args[0] == '!' && mCmd.field[0].args[1] == '!'){
		mCmd = *lastCMD;
		ExecCmd(lastCMD);
	}
	else
		printDMA("cmd not found\r\n", 15);
}

void LedExe(void){
	int state = 0;
	if(mCmd.field[1].args[0] == '0'){
		if(mCmd.field[2].args[0] == 'O' && mCmd.field[2].args[1] == 'N')
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
		else if(mCmd.field[2].args[0] == 'O' && mCmd.field[2].args[1] == 'F' && mCmd.field[2].args[2] == 'F')
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
		else 
			state = 1;
	}
	else if(mCmd.field[1].args[0] == '7'){
		if(mCmd.field[2].args[0] == 'O' && mCmd.field[2].args[1] == 'N')
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
		else if(mCmd.field[2].args[0] == 'O' && mCmd.field[2].args[1] == 'F' && mCmd.field[2].args[2] == 'F')
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
		else 
			state = 1;
	}
	else if(mCmd.field[1].args[0] == '1' && mCmd.field[1].args[1] == '4'){
		if(mCmd.field[2].args[0] == 'O' && mCmd.field[2].args[1] == 'N')
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
		else if(mCmd.field[2].args[0] == 'O' && mCmd.field[2].args[1] == 'F' && mCmd.field[2].args[2] == 'F')
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
		else 
			state = 1;
	}
	else 
		state = 1;
	if (state)
		printDMA("cmd not found\r\n", 15);
}

void MemWExe(uint8_t *mem){
	int i = 2;
	int factor = 1;
	uint16_t mem_aux = 0;
	if ((mCmd.field[1].args[3] != 0x00) ||  (mCmd.field[1].args[0] == 0x00))
		printDMA("cmd not found\r\n", 15);
	else{
		for(; i >= 0; i--){
			if (mCmd.field[1].args[i] == 0x00)
				mem_aux = 0;
			else if (mCmd.field[1].args[i] < '0' || mCmd.field[1].args[i] > '9'){
				printDMA("cmd not found\r\n", 15);
				break;
			}
			else{
				mem_aux += (mCmd.field[1].args[i] - '0') * factor;
				factor *= 10;
			}
		}
		if(mem_aux < 256)
			*mem = mem_aux;
		else
			printDMA("bigger then 255\r\n", 17);
	}
}

void MemRExe(uint8_t mem){
	uint8_t str[6];
	str[0] = mem / 100;
	mem %= 100;
	str[1] = mem / 10;
	mem %= 10;
	str[2] = mem;
	for (int i = 0; i < 3; i++){
		str[i] += '0';
	}
	while(printDMA((char *)str, 3));
	while(printDMA("\r\n",2));
}



