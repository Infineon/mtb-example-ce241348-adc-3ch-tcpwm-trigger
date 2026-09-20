/*******************************************************************************
* File Name:   main.c
*
* Description: This is the main file for the main CPU non safe application of
* the code example. It initializes the peripherals, PPCA CPU cores and starts
* it. Then it reads the data shared by the PPCA CPUs and prints.
*
* Related Document: See README.md
*
*
********************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "cycfg.h"
#include "cybsp.h"
#include <stdio.h>
#include "cy_retarget_io.h"
    
/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Debug UART variables */
static cy_stc_scb_uart_context_t    UART_context; /* UART context */
static mtb_hal_uart_t               UART_hal_obj; /* Debug UART HAL object */

/******************************************************************************
* Macros
*******************************************************************************/
/* These are the addresses where the core0 and core1 images are located and its size*/
#define CORE0_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca0_nvm_C_S_START    //   0x12030000
#define CORE1_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca1_nvm_C_S_START    //   0x12038000
#define PPCA0_IMAGE_SIZE       CYMEM_CM33_0_S_ppca0_code_SIZE
#define PPCA1_IMAGE_SIZE       CYMEM_CM33_0_S_ppca1_code_SIZE

/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 0 is using for sharing data with main CPU. */
#define PPCA_M33_0_SHARED_ADDRESS 0x53020400

/* This is the address in the memory shared between main CPU and PPCA CPUs
 * at which PPCA CPU 1 is using for sharing data with main CPU. */
#define PPCA_M33_1_SHARED_ADDRESS 0x53020800

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
*********************************************************************************
* Summary:
* This is the main function for the non safe project for the main core. It
* performs the initialization of the peripherals, initialization and starting
* of the PPCA CPU cores, and send the data received from PPCA CPU Core 0 through
* UART.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_en_tcpwm_status_t status;
    cy_rslt_t            result;

    volatile int32_t *adc0_read_data    = (int32_t *)PPCA_M33_0_SHARED_ADDRESS;
    volatile int32_t *adc1_read_data    = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 1;
    volatile int32_t *adc2_read_data    = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 2;
    volatile int32_t *adc0_conv_time_ns = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 3;
    volatile int32_t *adc1_conv_time_ns = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 4;
    volatile int32_t *adc2_conv_time_ns = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 5;
    volatile int32_t *adc_conv_status   = (int32_t *)PPCA_M33_0_SHARED_ADDRESS + 6;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Init(UART_HW, &UART_config, &UART_context);
    Cy_SCB_UART_Enable(UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&UART_hal_obj, &UART_hal_config,
                                &UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize redirecting of low level IO */
    result = cy_retarget_io_init(&UART_hal_obj);

    /* retarget IO init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Transmit header to the terminal */
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("************************************************************\r\n");
    printf("PSOC Control C3M/P8: ADC conversion using PWM trigger\r\n");
    printf("************************************************************\r\n\n");

    /* enable interrupts */
    __enable_irq();

     /* Initializing and enabling the PPCA Configuration. */
    Cy_PPCA_CNFG_Init(PPCA_CNFG_HW, &PPCA_CNFG_config);
    Cy_PPCA_Enable(PPCA_CNFG_HW);

    /* Initializing TCPWM as PWM */
    status = Cy_TCPWM_PWM_Init(START_TRIG_PWM_HW, START_TRIG_PWM_NUM, &START_TRIG_PWM_config);

    /* Initialization failed */
    if(CY_TCPWM_SUCCESS != status)
    {
        CY_ASSERT(0);
    }

    /* Initializing TCPWM as Counter for measuring the conversion time of ADC 0*/
    status = Cy_TCPWM_Counter_Init(ADC0_CAPTURE_COUNTER_HW, ADC0_CAPTURE_COUNTER_NUM, &ADC0_CAPTURE_COUNTER_config);

    /* Initialization failed */
    if(CY_TCPWM_SUCCESS != status)
    {
        CY_ASSERT(0);
    }

    /* Initializing TCPWM as Counter for measuring the conversion time of ADC 1*/
    status = Cy_TCPWM_Counter_Init(ADC1_CAPTURE_COUNTER_HW, ADC1_CAPTURE_COUNTER_NUM, &ADC1_CAPTURE_COUNTER_config);

    /* Initialization failed */
    if(CY_TCPWM_SUCCESS != status)
    {
        CY_ASSERT(0);
    }

    /* Initializing TCPWM as Counter for measuring the conversion time of ADC 2*/
    status = Cy_TCPWM_Counter_Init(ADC2_CAPTURE_COUNTER_HW, ADC2_CAPTURE_COUNTER_NUM, &ADC2_CAPTURE_COUNTER_config);

    /* Initialization failed */
    if(CY_TCPWM_SUCCESS != status)
    {
        CY_ASSERT(0);
    }

    /* Enabling PWM */
    Cy_TCPWM_PWM_Enable(START_TRIG_PWM_HW, START_TRIG_PWM_NUM);

    /* Enabling Counters */
    Cy_TCPWM_Counter_Enable(ADC0_CAPTURE_COUNTER_HW, ADC0_CAPTURE_COUNTER_NUM);
    Cy_TCPWM_Counter_Enable(ADC1_CAPTURE_COUNTER_HW, ADC1_CAPTURE_COUNTER_NUM);
    Cy_TCPWM_Counter_Enable(ADC2_CAPTURE_COUNTER_HW, ADC2_CAPTURE_COUNTER_NUM);

    /* Enable exclusive access to the EPU resources based
     * on the provided resources allocation configuration. */
    Cy_PPCA_EPU_EnableExclusiveAccess(PPCA_EPU, true);

    /* Enable EPU */
    Cy_PPCA_EPU_Enable(PPCA_EPU);

    /* Configuring and Enable EPU processing unit to receive capture 0 signal from PWM. */
    Cy_PPCA_EPU_PU_T1_Configure(START_TRIG_HW, START_TRIG_INDEX, &START_TRIG_put1_config);
    Cy_PPCA_EPU_PU_T1_Enable(START_TRIG_HW, START_TRIG_INDEX, START_TRIG_ENABLE_MODE);

    /* Configuring and Enable EPU processing unit to receive end of conversion signal from ADC 0. */
    Cy_PPCA_EPU_PU_T1_Configure(ADC0_EOC_TRIG_HW, ADC0_EOC_TRIG_INDEX, &ADC0_EOC_TRIG_put1_config);
    Cy_PPCA_EPU_PU_T1_Enable(ADC0_EOC_TRIG_HW, ADC0_EOC_TRIG_INDEX, ADC0_EOC_TRIG_ENABLE_MODE);

    /* Configuring and Enable EPU processing unit to receive end of conversion signal from ADC 1. */
    Cy_PPCA_EPU_PU_T1_Configure(ADC1_EOC_TRIG_HW, ADC1_EOC_TRIG_INDEX, &ADC1_EOC_TRIG_put1_config);
    Cy_PPCA_EPU_PU_T1_Enable(ADC1_EOC_TRIG_HW, ADC1_EOC_TRIG_INDEX, ADC1_EOC_TRIG_ENABLE_MODE);

    /* Configuring and Enable EPU processing unit to receive end of conversion signal from ADC 2. */
    Cy_PPCA_EPU_PU_T1_Configure(ADC2_EOC_TRIG_HW, ADC2_EOC_TRIG_INDEX, &ADC2_EOC_TRIG_put1_config);
    Cy_PPCA_EPU_PU_T1_Enable(ADC2_EOC_TRIG_HW, ADC2_EOC_TRIG_INDEX, ADC2_EOC_TRIG_ENABLE_MODE);

    /* Configuring EPU combiner to route the signal from PWM to Counter 0 to start counting */
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC0_CAPTURE_COUNTER_START_INDEX, &ADC0_CAPTURE_COUNTER_START_combo_config);

    /* Configuring EPU combiner to route the signal from PWM to Counter 1 to start counting */
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC1_CAPTURE_COUNTER_START_INDEX, &ADC1_CAPTURE_COUNTER_START_combo_config);

    /* Configuring EPU combiner to route the signal from PWM to Counter 2 to start counting */
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC2_CAPTURE_COUNTER_START_INDEX, &ADC2_CAPTURE_COUNTER_START_combo_config);

    /* Configuring EPU combiner to route the signal from ADC 0 to Counter 0 to capture the count*/
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC0_CAPTURE_COUNTER_CAPTURE_INDEX, &ADC0_CAPTURE_COUNTER_CAPTURE_combo_config);

    /* Configuring EPU combiner to route the signal from ADC 1 to Counter 1 to capture the count*/
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC1_CAPTURE_COUNTER_CAPTURE_INDEX, &ADC1_CAPTURE_COUNTER_CAPTURE_combo_config);

    /* Configuring EPU combiner to route the signal from ADC 2 to Counter 2 to capture the count*/
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC2_CAPTURE_COUNTER_CAPTURE_INDEX, &ADC2_CAPTURE_COUNTER_CAPTURE_combo_config);

    /* Configuring EPU combiner to route the signal from PWM to ADC 0 to start conversion */
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC0_START_TRIG_INDEX, &ADC0_START_TRIG_combo_config);

    /* Configuring EPU combiner to route the signal from PWM to ADC 1 to start conversion */
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC1_START_TRIG_INDEX, &ADC1_START_TRIG_combo_config);

    /* Configuring EPU combiner to route the signal from PWM to ADC 2 to start conversion */
    Cy_PPCA_EPU_Combo_Configure(PPCA_EPU_EPU, ADC2_START_TRIG_INDEX, &ADC2_START_TRIG_combo_config);

    /* Initializing ATOP Analog reference */
    Cy_PPCA_AREF_Init(AREF_HW, &AREF_config);

    /* Enabling AREF */
    Cy_PPCA_AREF_Enable(AREF_HW);

    /* Initializing ATOP ADC 0*/
    Cy_PPCA_ADC_Init(ADC0_HW, &ADC0_config);

    /* Initializing ATOP ADC 1*/
    Cy_PPCA_ADC_Init(ADC1_HW, &ADC1_config);

    /* Initializing ATOP ADC 2*/
    Cy_PPCA_ADC_Init(ADC2_HW, &ADC2_config);

    /* Enabling ADC 0*/
    Cy_PPCA_ADC_Enable(ADC0_HW);

    /* Enabling ADC 1*/
    Cy_PPCA_ADC_Enable(ADC1_HW);

    /* Enabling ADC 2*/
    Cy_PPCA_ADC_Enable(ADC2_HW);

    /* Initializing and starting PPCA CPU Core 0. */
    Cy_System_Init_CPU0((void*)CORE0_IMAGE_ADDRESS, PPCA0_IMAGE_SIZE);

    /* Initializing and starting PPCA CPU Core 1. Use the below line to start PPCA Core 1 */
    /*Cy_System_Init_CPU1((void*)CORE1_IMAGE_ADDRESS, PPCA1_IMAGE_SIZE);*/

    /* Starting PWM */
    Cy_TCPWM_TriggerStart_Single(START_TRIG_PWM_HW, START_TRIG_PWM_NUM);

    for (;;)
    {
        /* Checking for data update */
        if(1 == *adc_conv_status)
        {
            printf("\r\n ADC 0 Data: %d Time: %d nano seconds", (int)*adc0_read_data, (int)*adc0_conv_time_ns);
            printf("\r\n ADC 1 Data: %d Time: %d nano seconds", (int)*adc1_read_data, (int)*adc1_conv_time_ns);
            printf("\r\n ADC 2 Data: %d Time: %d nano seconds\n\r", (int)*adc2_read_data, (int)*adc2_conv_time_ns);
            *adc_conv_status = 0;
        }
    }
}
