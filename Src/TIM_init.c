#include "TIM_init.h"

uint8_t volatile flagWriteLogDataInSD =0;

void TIM_init()
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

    LL_TIM_SetPrescaler(TIM2,SystemCoreClock/2000);            // APB1 = 72 MHz, 72000000/36000 = 0.0005 s
    LL_TIM_SetAutoReload(TIM2,60000);                          // 0.00005*60000 = 30 s;
    LL_TIM_EnableIT_UPDATE(TIM2);
    NVIC_EnableIRQ(TIM2_IRQn);
    LL_TIM_EnableCounter(TIM2);
}

void TIM2_IRQHandler()
{
    LL_TIM_ClearFlag_UPDATE(TIM2);
    flagWriteLogDataInSD =1;
}
