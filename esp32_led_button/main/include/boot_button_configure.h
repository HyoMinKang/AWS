/*
 * boot_button_setting.h
 *
 *  Created on: 2018. 1. 19.
 *      Author: joondong
 */

#define GPIO_INPUT_IO_0     0 // boot button gpio

/*
 * @ brief Initialize gpio and task about boot button.
 *
 * Boot button is connected to GPIO 0 and gpio_isr_handler about this have to execute gpio_task.
 *
 * @ param 	gpio_isr_handler[in]		Invoked when button is pressed.
 * @ param 	gpio_task[in]			Invoked by gpio_isr_handler.
 * @ param 	priority	[in]				Priority of gpio_task. The more number the more high.
 * @ param 	StackDepth[in]			4-bytes stack size of gpio_task
 * @ param 	gpio_task_handle[out]	It is returned to communicate with other tasks outside this module.
 *
 * */
void boot_button_configure(gpio_isr_t gpio_isr_handler, TaskFunction_t gpio_task, UBaseType_t priority ,const uint32_t StackDepth, TaskHandle_t * gpio_task_handle);
