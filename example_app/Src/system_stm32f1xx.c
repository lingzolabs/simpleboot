/**
 ******************************************************************************
 * @file    system_stm32f1xx.c
 * @author  MCD Application Team
 * @brief   CMSIS Cortex-M3 Device Peripheral Access Layer System Source File.
 *
 * 1.  This file provides two functions and one global variable to be called
 *from user application:
 *      - SystemInit(): Setups the system clock (System clock source, PLL
 *Multiplier factors, AHB/APBx prescalers and Flash settings). This function is
 *called at startup just after reset and before branch to main program. This
 *call is made inside the "startup_stm32f1xx_xx.s" file.
 *
 *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be
 *used by the user application to setup the SysTick timer or configure other
 *parameters.
 *
 *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
 *                                 be called whenever the core clock is changed
 *                                 during program execution.
 *
 * 2. After each device reset the HSI (8 MHz) is used as system clock source.
 *    Then SystemInit() function is called, in "startup_stm32f1xx_xx.s" file, to
 *    configure the system clock before to branch to main program.
 *
 * 4. The default value of HSE crystal is set to 8 MHz (or 25 MHz, depending on
 *    the product used), refer to "HSE_VALUE".
 *    When HSE is used as system clock source, directly or through PLL, and you
 *    are using different crystal you have to adapt the HSE value to your own
 *    configuration.
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/** @addtogroup CMSIS
 * @{
 */

/** @addtogroup stm32f1xx_system
 * @{
 */

/** @addtogroup STM32F1xx_System_Private_Includes
 * @{
 */

#include "stm32f1xx.h"

/**
 * @}
 */

/** @addtogroup STM32F1xx_System_Private_TypesDefinitions
 * @{
 */

/**
 * @}
 */

/** @addtogroup STM32F1xx_System_Private_Defines
 * @{
 */

#if !defined(HSE_VALUE)
#define HSE_VALUE                                                              \
  ((uint32_t)8000000) /*!< Default value of the External oscillator in Hz.     \
                           This value can be provided and adapted by the user  \
                         application. */
#endif                /* HSE_VALUE */

#if !defined(HSI_VALUE)
#define HSI_VALUE                                                              \
  ((uint32_t)8000000) /*!< Default value of the Internal oscillator in Hz.     \
                           This value can be provided and adapted by the user  \
                         application. */
#endif                /* HSI_VALUE */

/*!< Uncomment the following line if you need to use external SRAM  */
#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) ||    \
    defined(STM32F103xE) || defined(STM32F103xG)
/* #define DATA_IN_ExtSRAM */
#endif /* STM32F100xE || STM32F101xE || STM32F101xG || STM32F103xE ||          \
          STM32F103xG */

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET                                                        \
  0x4000 /*!< Vector Table base offset field.                                  \
           This value must be a multiple of 0x200. */

/**
 * @}
 */

/** @addtogroup STM32F1xx_System_Private_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup STM32F1xx_System_Private_Variables
 * @{
 */
/* This variable is updated in three ways:
    1) by calling CMSIS function SystemCoreClockUpdate()
    2) by calling HAL API function HAL_RCC_GetHCLKFreq()
    3) each time HAL_RCC_ClockConfig() is called to configure the system clock
   frequency Note: If you use this function to configure the system clock; then
   there is no need to call the 2 first functions listed above, since
   SystemCoreClock variable is updated automatically.
*/
uint32_t SystemCoreClock = 8000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                   1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
/**
 * @}
 */

/** @addtogroup STM32F1xx_System_Private_FunctionPrototypes
 * @{
 */

#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) ||    \
    defined(STM32F103xE) || defined(STM32F103xG)
#ifdef DATA_IN_ExtSRAM
static void SystemInit_ExtMemCtl(void);
#endif /* DATA_IN_ExtSRAM */
#endif /* STM32F100xE || STM32F101xE || STM32F101xG || STM32F103xE ||          \
          STM32F103xG */

/**
 * @}
 */

/** @addtogroup STM32F1xx_System_Private_Functions
 * @{
 */

/**
 * @brief  Setup the microcontroller system
 *         Initialize the Embedded Flash Interface, the PLL and update the
 *         SystemCoreClock variable.
 * @note   This function should be used only after reset.
 * @param  None
 * @retval None
 */
void SystemInit(void) {
  /* Reset the RCC clock configuration to the default reset state(for debug
   * purpose) */
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
#if !defined(STM32F105xC) && !defined(STM32F107xC)
  RCC->CFGR &= (uint32_t)0xF8FF0000;
#else
  RCC->CFGR &= (uint32_t)0xF0FF0000;
#endif /* STM32F105xC */

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
  RCC->CFGR &= (uint32_t)0xFF80FFFF;

#if defined(STM32F105xC) || defined(STM32F107xC)
  /* Reset PLL2ON and PLL3ON bits */
  RCC->CR &= (uint32_t)0xEBFFFFFF;

  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x00FF0000;

  /* Reset CFGR2 register */
  RCC->CFGR2 = 0x00000000;
#elif defined(STM32F100xB) || defined(STM32F100xE)
  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x009F0000;

  /* Reset CFGR2 register */
  RCC->CFGR2 = 0x00000000;
#else
  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x009F0000;
#endif /* STM32F105xC */

#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) ||    \
    defined(STM32F103xE) || defined(STM32F103xG)
#ifdef DATA_IN_ExtSRAM
  SystemInit_ExtMemCtl();
#endif /* DATA_IN_ExtSRAM */
#endif

#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE |
              VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM. */
#else
  SCB->VTOR = FLASH_BASE |
              VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH. */
#endif
}

/**
 * @brief  Update SystemCoreClock variable according to Clock Register Values.
 *         The SystemCoreClock variable contains the core clock (HCLK), it can
 *         be used by the user application to setup the SysTick timer or
 * configure other parameters.
 *
 * @note   Each time the core clock (HCLK) changes, this function must be called
 *         to update SystemCoreClock variable value. Otherwise, any
 * configuration based on this variable will be incorrect.
 *
 * @note   - The system frequency computed by this function is not the real
 *           frequency in the chip. It is calculated based on the predefined
 *           constant and the selected clock source:
 *
 *           - If SYSCLK source is HSI, SystemCoreClock will contain the
 * HSI_VALUE(*)
 *
 *           - If SYSCLK source is HSE, SystemCoreClock will contain the
 * HSE_VALUE(**)
 *
 *           - If SYSCLK source is PLL, SystemCoreClock will contain the
 * HSE_VALUE(**) or HSI_VALUE(*) multiplied by the PLL factors.
 *
 *         (*) HSI_VALUE is a constant defined in stm32f1xx.h file (default
 * value 8 MHz) but the real value may vary depending on the variations in
 * voltage and temperature.
 *
 *         (**) HSE_VALUE is a constant defined in stm32f1xx.h file (default
 * value 8 MHz or 25 MHz, depending on the product used), user has to ensure
 *              that HSE_VALUE is same as the real frequency of the crystal
 * used. Otherwise, this function may have wrong result.
 *
 *         - The result of this function could be not correct when using
 * fractional value for HSE crystal.
 * @param  None
 * @retval None
 */
void SystemCoreClockUpdate(void) {
  uint32_t tmp = 0, pllmull = 0, pllsource = 0;

#if defined(STM32F105xC) || defined(STM32F107xC)
  uint32_t prediv1source = 0, prediv1factor = 0, prediv2factor = 0,
           pll2mull = 0;
#endif /* STM32F105xC */

#if defined(STM32F100xB) || defined(STM32F100xE)
  uint32_t prediv1factor = 0;
#endif /* STM32F100xB or STM32F100xE */

  /* Get SYSCLK source -------------------------------------------------------*/
  tmp = RCC->CFGR & RCC_CFGR_SWS;

  switch (tmp) {
  case 0x00: /* HSI used as system clock */
    SystemCoreClock = HSI_VALUE;
    break;
  case 0x04: /* HSE used as system clock */
    SystemCoreClock = HSE_VALUE;
    break;
  case 0x08: /* PLL used as system clock */

    /* Get PLL clock source and multiplication factor ----------------------*/
    pllmull = RCC->CFGR & RCC_CFGR_PLLMULL;
    pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;

#if !defined(STM32F105xC) && !defined(STM32F107xC)
    pllmull = (pllmull >> 18) + 2;

    if (pllsource == 0x00) {
      /* HSI oscillator clock divided by 2 selected as PLL clock entry */
      SystemCoreClock = (HSI_VALUE >> 1) * pllmull;
    } else {
#if defined(STM32F100xB) || defined(STM32F100xE)
      prediv1factor = (RCC->CFGR2 & RCC_CFGR2_PREDIV1) + 1;
      /* HSE oscillator clock selected as PREDIV1 clock entry */
      SystemCoreClock = (HSE_VALUE / prediv1factor) * pllmull;
#else
      /* HSE selected as PLL clock entry */
      if ((RCC->CFGR & RCC_CFGR_PLLXTPRE) !=
          (uint32_t)RESET) { /* HSE oscillator clock divided by 2 */
        SystemCoreClock = (HSE_VALUE >> 1) * pllmull;
      } else {
        SystemCoreClock = HSE_VALUE * pllmull;
      }
#endif
    }
#else
    pllmull = pllmull >> 18;

    if (pllmull != 0x0D) {
      pllmull += 2;
    } else { /* PLL multiplication factor = PLL input clock * 6.5 */
      pllmull = 13 / 2;
    }

    if (pllsource == 0x00) {
      /* HSI oscillator clock divided by 2 selected as PLL clock entry */
      SystemCoreClock = (HSI_VALUE >> 1) * pllmull;
    } else { /* PREDIV1 selected as PLL clock entry */

      /* Get PREDIV1 clock source and division factor */
      prediv1source = RCC->CFGR2 & RCC_CFGR2_PREDIV1SRC;
      prediv1factor = (RCC->CFGR2 & RCC_CFGR2_PREDIV1) + 1;

      if (prediv1source == 0) {
        /* HSE oscillator clock selected as PREDIV1 clock entry */
        SystemCoreClock = (HSE_VALUE / prediv1factor) * pllmull;
      } else { /* PLL2 clock selected as PREDIV1 clock entry */

        /* Get PREDIV2 division factor and PLL2 multiplication factor */
        prediv2factor = ((RCC->CFGR2 & RCC_CFGR2_PREDIV2) >> 4) + 1;
        pll2mull = ((RCC->CFGR2 & RCC_CFGR2_PLL2MUL) >> 8) + 2;
        SystemCoreClock =
            (((HSE_VALUE / prediv2factor) * pll2mull) / prediv1factor) *
            pllmull;
      }
    }
#endif /* STM32F105xC */
    break;

  default:
    SystemCoreClock = HSI_VALUE;
    break;
  }

  /* Compute HCLK clock frequency ----------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;
}

#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) ||    \
    defined(STM32F103xE) || defined(STM32F103xG)
/**
 * @brief  Setup the external memory controller. Called in startup_stm32f1xx.s
 *          before jump to __main
 * @param  None
 * @retval None
 */
#ifdef DATA_IN_ExtSRAM
/**
 * @brief  Setup the external memory controller.
 *         Called in startup_stm32f1xx_xx.s/.c before jump to main.
 *         This function configures the external SRAM mounted on STM3210E-EVAL
 *         board (STM32 High density devices). This SRAM will be used as program
 *         data memory (including heap and stack).
 * @param  None
 * @retval None
 */
void SystemInit_ExtMemCtl(void) {
  __IO uint32_t tmpreg;
  /*!< FSMC Bank1 NOR/SRAM3 is used for the STM3210E-EVAL, if another Bank is
    required, then adjust the Register Addresses */

  /* Enable FSMC clock */
  RCC->AHBENR = 0x00000114;

  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_FSMCEN);

  /* Enable GPIOD, GPIOE, GPIOF and GPIOG clocks */
  RCC->APB2ENR = 0x000001E0;

  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPDEN);

  (void)(tmpreg);

  /* ---------------  SRAM Data lines, NOE and NWE configuration
   * ---------------*/
  /*----------------  SRAM Address lines configuration
   * -------------------------*/
  /*----------------  NOE and NWE configuration
   * --------------------------------*/
  /*----------------  NE3 configuration
   * ----------------------------------------*/
  /*----------------  NBL0, NBL1 configuration
   * ---------------------------------*/

  GPIOD->CRL = 0x44BB44BB;
  GPIOD->CRH = 0xBBBBBBBB;

  GPIOE->CRL = 0xB44444BB;
  GPIOE->CRH = 0xBBBBBBBB;

  GPIOF->CRL = 0x44BBBBBB;
  GPIOF->CRH = 0xBBBB4444;

  GPIOG->CRL = 0x44BBBBBB;
  GPIOG->CRH = 0x44444B44;

  /*----------------  FSMC Configuration
   * ---------------------------------------*/
  /*----------------  Enable FSMC Bank1_SRAM Bank
   * ------------------------------*/

  FSMC_Bank1->BTCR[4] = 0x00001091;
  FSMC_Bank1->BTCR[5] = 0x00110212;
}
#endif /* DATA_IN_ExtSRAM */
#endif /* STM32F100xE || STM32F101xE || STM32F101xG || STM32F103xE ||          \
          STM32F103xG */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
