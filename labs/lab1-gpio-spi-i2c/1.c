
#include "main.h"

#include "stm32l476xx.h"

extern uint32_t systemTimeCounter;
uint32_t *spi2_dr = 0x40003c00 + 0x0C;

void delayMs(uint32_t delay)
{
	uint32_t curr = systemTimeCounter;
	while(systemTimeCounter - curr < delay)
	{	}
}

void i2cTransmit(uint8_t adress, uint8_t *data, uint8_t n)
{
	I2C1->CR2 |= I2C_CR2_NBYTES | I2C_CR2_RELOAD; //NBYTES-количество байт до 255, RELOAD - автоперезагрузка
	I2C1->CR2 |= adress << 1; // адрес ведомого
//	I2C1->CR1 |= I2C_CR1_PE; //
	I2C1->CR2 |= I2C_CR2_START;
	for(int i = 0; i<n;i++)
	{
		while( !(I2C1->ISR & I2C_ISR_TXIS ))
		{
			if(I2C1->ISR & I2C_ISR_NACKF)
				return;
		}
		I2C1->TXDR = data[i];
	}
}
void spiTransmitRecieve(uint8_t *txBuff, uint8_t *rxBuff,uint8_t tn, uint8_t rn)
{
	GPIOD->ODR &= ~GPIO_ODR_ODR_7;
	uint8_t dummy;
	for(int i = 0; i<tn; i++)
	{
		while(!(SPI2->SR & SPI_SR_TXE));
		SPI2->DR = txBuff[i];
		while(!(SPI2->SR & SPI_SR_RXNE));
		dummy = SPI2->DR;
	}
	while(SPI2->SR & SPI_SR_BSY);
		for(int i = 0; i<rn; i++)
		{
			while(!(SPI2->SR & SPI_SR_TXE));
			SPI2->DR = dummy;
			while(!(SPI2->SR & SPI_SR_RXNE));
			rxBuff[i] = SPI2->DR;
		}
		while(SPI2->SR & SPI_SR_BSY);
	GPIOD->ODR |= GPIO_ODR_ODR_7;

}


int main(void)
{
	// PWR AND SYS ENABLE
	//APB2(1)-линия тактирования (см.даташит 17/270)
	//RM0351 (референс мануал) стр.223/1906
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;  //RM0351 (референс мануал) стр.251/1906
	RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
	RCC->CR |= RCC_CR_MSIRGSEL;
	SysTick_Config(4000000/1000);
	NVIC_SetPriority(SysTick_IRQn,0);
	NVIC_EnableIRQ(SysTick_IRQn);

	//настраиваем тактирование на пинах (указываем литеру А/В/ и тд. в зависимости от необходимости)
	//RM0351 стр.251/1906
	// как пример: RCC_AHB2ENR_GPIOНEN- включили тактирование на пинах "H"
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEN | RCC_AHB2ENR_GPIOEN;

    // настройка пина "чип-селект" (выбираем тот что входирт в SPI или выбираем другой)
	// настройка пинов gpioХ, а именно пина №0-15
	//RM0351 стр.304/1906
	GPIO->MODER &= ~(3 << GPIO_MODER_MODE_Pos); //сброс режима
	GPIO->MODER |= (1 << GPIO_MODER_MODE_Pos); //выбор режима вывода общего назначения
	GPIO->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR; // настройка скорости работы
	GPIO->ODR |= GPIO_ODR_ODR_; //настройка пина input/output

	// Инициализация SPI
	//RM0351 стр.1405/1906
	// выбираем линию SPI и настраиваем пины (3/4 шт)
	RCC->APB1ENR1 |= RCC_APB1ENR1_SPIEN; // подключение тактирования
	// настройка режима работы пинов - стереть что было, записать что надо
	//RM0351 стр.1405/1906 регистры; режимы как с настройкой пина ранее
	GPIO->MODER &= ~((3 << GPIO_MODER_MODE_Pos)|(3 << GPIO_MODER_MODE_Pos)|(3 << GPIO_MODER_MODE_Pos));
	GPIO->MODER = ((2 << GPIO_MODER_MODE_Pos)|(2 << GPIO_MODER_MODE_Pos)|(2 << GPIO_MODER_MODE_Pos)); // 2 ALTERNATIVE

	GPIO->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR | GPIO_OSPEEDER_OSPEEDR | GPIO_OSPEEDER_OSPEEDR;
	// AFR-alternate function low register
	GPIO->AFR[0] |=  (5 << GPIO_AFRL_AFSEL_Pos) | (5 << GPIO_AFRL_AFSEL_Pos) | ( 5<< GPIO_AFRL_AFSEL_Pos);
	//RM0351 стр.1476/1906
	SPI->CR1 &= ~SPI_CR1_BR; // сброс скорости передачи данных
	SPI->CR1 |= (1 << SPI_CR1_BR_Pos); // установк скорости передачи данных
	SPI->CR1 |=  SPI_CR1_SSI| SPI_CR1_SSM;
	SPI->CR1 |= SPI_CR1_MSTR;
	SPI->CR2 &= ~(SPI_CR2_DS | SPI_CR2_SSOE);
	SPI->CR2 |= (7<< SPI_CR2_DS_Pos) | SPI_CR2_SSOE;
	SPI->CR2 |= SPI_CR2_NSSP | SPI_CR2_SSOE; //( DATA SIZE BITS DONT NEDD SET? WHEN 8 BIT DATA)
//	SPI2->CR2 |= SPI_CR2_FRXTH;
	  SPI->CR1 |= SPI_CR1_SPE;

	//I2C INIT
	RCC->CCIPR &= ~(3<< RCC_CCIPR_I2CSEL);
	RCC->APB1ENR1 |= RCC_APB1ENR1_I2CEN;


	GPIO->MODER &= ~((3 << GPIO_MODER_MODE_Pos)|(3 << GPIO_MODER_MODE_Pos));
	GPIO->MODER |= ((2 << GPIO_MODER_MODE_Pos)|(2 << GPIO_MODER_MODE_Pos));
	GPIO->OTYPER |= GPIO_OTYPER_IDR_ |GPIO_OTYPER_IDR_;
	GPIO->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR | GPIO_OSPEEDER_OSPEEDR;
	GPIO->AFR[] |=  (4 << GPIO_AFRL_AFSEL_Pos) | (4 << GPIO_AFRL_AFSEL_Pos);

	I2C->TIMINGR |= (0x14 << I2C_TIMINGR_SCLL_Pos) | (0xE << I2C_TIMINGR_SCLH_Pos) ;
	I2C->OAR1 |= I2C_OAR1_OA1EN ;
	I2C->CR2 |= I2C_CR2_AUTOEND | I2C_CR2_NACK;
	I2C->CR1 |= I2C_CR1_PE;

   // Led
	GPIOB->MODER &=  ~(3<< GPIO_MODER_MODE2_Pos);
	GPIOB->MODER |= 1 << GPIO_MODER_MODE2_Pos;

	GPIOB->ODR |= GPIO_ODR_OD2;



  while (1)
  {
	  delayMs(1000);
	  GPIOB->ODR ^= GPIO_ODR_OD2;

  }

}

