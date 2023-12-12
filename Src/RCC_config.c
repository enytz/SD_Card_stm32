
#include "../Inc/RCC_config.h"


void RCC_config()
{
    uint32_t CPUfreq = 72000000;
    LL_RCC_HSE_Enable();                // HSE - 16 Мгц
    while(!LL_RCC_HSE_IsReady())
        ;
    LL_SetFlashLatency(CPUfreq);       // 2

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE,LL_RCC_PLLM_DIV_16,72*2,LL_RCC_PLLP_DIV_2);     // 16/8 * 72 /2 = 72
    LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE,LL_RCC_PLLM_DIV_16,72*2,LL_RCC_PLLQ_DIV_3);     // 16/8 * 72 /3 = 48
    LL_RCC_PLL_Enable();
    while(!LL_RCC_PLL_IsReady())
        ;
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
        ;
    LL_SetSystemCoreClock(CPUfreq);

    for (int i=0;i<1000000;++i)                   // любой тип задержки (DWT счетчик лучше, но сейчас и это подойдет), для стаб. питания для SD
    {
        __NOP();
    }

}
