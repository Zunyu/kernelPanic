//  Project
//  Engineer: Jiao Yang
//
//    IntelligentVehicle.c [kernel module]
//    Version 1.0

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <asm/uaccess.h>
#include <linux/slab.h> /* kmalloc() */

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system.h> /* cli(), *_flags */

#include <asm/arch/pxa-regs.h>
#include <asm/arch/gpio.h>
#include <asm/hardware.h>
#include <linux/interrupt.h>		/*** interrupt ***/
#include <asm-arm/arch/hardware.h>	/*** hardware ***/

#include <linux/string.h>
#include <linux/vmalloc.h>

#include <linux/timer.h>
#include <linux/jiffies.h>

/* move status of vehicle */
#define MOVING 1
#define STOPED 0

/* obstacle interrupt port: GPIO_28 */
#define OBSTACLE_DETECT 28

/* Angle(D0~D4) and Sign(S) */
#define D0 29
#define D1 30
#define D2 31
#define D3 101
#define D4 113
#define S 17

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of IntelligentVehicle.c functions */
static int __init IntelligentVehicle_init(void);
static void __exit IntelligentVehicle_exit(void);
static int IntelligentVehicle_open(struct inode *inode, struct file *filp);
static int IntelligentVehicle_release(struct inode *inode, struct file *filp);
static ssize_t IntelligentVehicle_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t IntelligentVehicle_write(struct file *filp,const char *buf, size_t count, loff_t *f_pos);

static irqreturn_t obstacleDetect_irq(int irq, void *dev_id, struct pt_regs *regs);

static void calculateAngle(void);
static void timeoutHandler(unsigned long);

/* Structure that declares the usual file access functions */
struct file_operations IntelligentVehicle_fops = {
	read: IntelligentVehicle_read,
	write: IntelligentVehicle_write,
	open: IntelligentVehicle_open,
	release: IntelligentVehicle_release,
};
/* Buffer to store data */
static char *buffer;
/* length of the current message */
static int buffer_len;

/* Declaration of the init and exit functions */
module_init(IntelligentVehicle_init);
module_exit(IntelligentVehicle_exit);

/* Global variables*/
static int IntelligentVehicle_major = 61;

static unsigned capacity = 512;
static long int xPosition[256];					//x coordinate from route info
static long int yPosition[256];					//y coordinate from route info
static int numberOfPoints;
static int sendTime = 0;
static int angle[256];
static int direction[256];

static struct timer_list *timeout;				//Timer used for timeout

static int obstacleDetectIRQ;					//obstacle interrupt number

static int routeGet = 0;
//When route info is received by Bluetooth, Bluetooth-ul will overwrite /dev/IntelligentVehicle, causing IntelligentVehicle_write to be called, and thus, routeGet become 2.
//routeGet becomes 1 after parallax board is notified by S to receive route info
//routeGet becomes 0 after all route info has been sent
static int obstacleExist = 0;
//Set obstacleExist to 1 when vehicle encounters an obstacle.

/* Function Definitions */
static int __init IntelligentVehicle_init(void) {

	int result = 0;

	/* Registering device */
	result = register_chrdev(IntelligentVehicle_major, "IntelligentVehicle", &IntelligentVehicle_fops);
	if (result < 0)
	{
		printk(KERN_ALERT "IntelligentVehicle: cannot obtain major number %d\n", IntelligentVehicle_major);
		return result;
	}

	/* Allocating buffer */
	buffer = kmalloc(capacity, GFP_KERNEL);

	if (!buffer)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	}

	memset(buffer, 0, capacity);
	buffer_len = 0;

	/* Allocating timeout */
	timeout = (struct timer_list *) kmalloc(sizeof(struct timer_list), GFP_KERNEL);

	if (!timeout)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail;
	}

	/* initialize variables */
	routeGet = 0;
	sendTime = 0;
	obstacleExist = 0;

	/* set up D0,D1,D2,D3,D4 and S */
	//GPIO_29,30,31,101,113,17: GPIO mode, output = 0
	gpio_direction_output(D0, 0);
	gpio_direction_output(D1, 0);
	gpio_direction_output(D2, 0);
	gpio_direction_output(D3, 0);
	gpio_direction_output(D4, 0);
	gpio_direction_output(S, 0);

	//initialize 0
	pxa_gpio_set_value(D0,0);
	pxa_gpio_set_value(D1,0);
	pxa_gpio_set_value(D2,0);
	pxa_gpio_set_value(D3,0);
	pxa_gpio_set_value(D4,0);
	pxa_gpio_set_value(S,0);

	//OBSTACLE_DETECT, GPIO_28: GPIO interrupt mode (as input)
	pxa_gpio_mode(OBSTACLE_DETECT | GPIO_IN);
	obstacleDetectIRQ = IRQ_GPIO(OBSTACLE_DETECT);
	if (request_irq(obstacleDetectIRQ, &obstacleDetect_irq, SA_INTERRUPT | SA_TRIGGER_RISING, "OBSTACLE_DETECT", NULL) != 0 ) {
		printk ( "obstacleDetectIRQ not acquired \n" );
		return -1;
	}

	setup_timer(timeout, timeoutHandler, 0);
	(*timeout).expires = jiffies + (1 * HZ / 2);		//500ms
	add_timer(timeout);

	printk(KERN_ALERT "Inserting IntelligentVehicle module\n");
	return 0;

fail:
	IntelligentVehicle_exit(); 
	return result;
}

static void __exit IntelligentVehicle_exit(void) {

	/* Freeing the major number */
	unregister_chrdev(IntelligentVehicle_major, "IntelligentVehicle");

	/* delete timer */
	del_timer(timeout);

	/* Freeing buffer memory */
	if (buffer)
	{
		kfree(buffer);
	}
	buffer = NULL;

	/* Freeing timeout memory */
	if (timeout) {
		kfree(timeout);
	}
	timeout = NULL;

	/* Freeing interrupt */
	free_irq(obstacleDetectIRQ, NULL);

	printk(KERN_ALERT "Removing IntelligentVehicle module\n");
}

static int IntelligentVehicle_open(struct inode *inode, struct file *filp) {
	return 0;
}

static int IntelligentVehicle_release(struct inode *inode, struct file *filp) {
	return 0;
}

static ssize_t IntelligentVehicle_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {

	char stateSign;
	int bufferCount = 5;				// [R][0][,][0][,][?]
	int xPositionCount = 0;
	int yPositionCount = 0;
	int temp = 0;
	int sign = 1;					// 1 for positive; -1 for negative
							//   up: negative
							//   down: positive
							//   left: negative
							//   right: postive

	if (copy_from_user(buffer + *f_pos, buf, count))
	{
		return -EFAULT;
	}

	(*(buffer + count)) = '\0';

	stateSign = buffer[0];				// get 'R'

	if (stateSign == 'R') {
		while (buffer[bufferCount] != 'F') {
			while (buffer[bufferCount] != ',') {
				if (buffer[bufferCount] == '-') sign = -1;
				else {
					temp = 10 * temp + buffer[bufferCount];
				}
				bufferCount++;
			}
			if(sign == 1) xPosition[xPositionCount] = temp;
			else if (sign == -1) xPosition[xPositionCount] = 0 - temp;

			temp = 0;
			sign = 1;
			xPositionCount++;
			bufferCount++;

			while (buffer[bufferCount] != ',') {
				if (buffer[bufferCount] == '-') sign = -1;
				else {
					temp = 10 * temp + buffer[bufferCount];
				}
				bufferCount++;
			}
			if(sign == 1) yPosition[yPositionCount] = temp;
			else if (sign == -1) yPosition[yPositionCount] = 0 - temp;

			temp = 0;
			sign = 1;
			yPositionCount++;
			bufferCount++;
		}
		numberOfPoints = xPositionCount;
		calculateAngle();
	}
	else printk(KERN_ALERT "Wrong Route Info!\n");

	*f_pos += count;
	buffer_len = *f_pos;

	routeGet = 2;

	return count;
}

static ssize_t IntelligentVehicle_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {

	char outputInfo[3];
	char *output = outputInfo;

	if (obstacleExist == 1) {
		output += sprintf(output, "O");
		obstacleExist = 0;
	}
	else if (obstacleExist == 0) {
		output += sprintf(output, "N");
	}
	output += sprintf(output, "\0");

	count = strlen(outputInfo);

	/* end of buffer reached */
	if (*f_pos >= count)
	{
		return 0;
	}

	if (copy_to_user(buf, outputInfo, count))
	{
		return -EFAULT;
	}

	*f_pos += count;

	return count;
}

static irqreturn_t obstacleDetect_irq(int irq, void *dev_id, struct pt_regs *regs) {

	obstacleExist = 1;

	return IRQ_HANDLED;
}

static void calculateAngle(void) {

	long int tan = 0;
	int degree;				// degree = actual degree / 10
	int offset = 0;
	int sign;				// sign = 0: clockwise; sign = 1: counterclockwise

	int degree0 = 0;
	int sign0 = 0;

	int loop;
	for (loop = 0; loop < numberOfPoints; loop++) {
		if (yPosition[loop] > 0 && xPosition[loop] > 0) {
			tan = (yPosition[loop] * 1000 / xPosition[loop]);
			offset = 9;
			sign = 0;
		}
		else if (yPosition[loop] > 0 && xPosition[loop] < 0) {
			tan = (0 - (yPosition[loop] * 1000 / xPosition[loop]));
			offset = 9;
			sign = 1;
		}
		else if (yPosition[loop] < 0 && xPosition[loop] > 0) {
			tan = (0 - (xPosition[loop] * 1000 / yPosition[loop]));
			sign = 0;
		}
		else if (yPosition[loop] < 0 && xPosition[loop] < 0) {
			tan = (xPosition[loop] * 1000 / yPosition[loop]);
			sign = 1;
		}
		else if (yPosition[loop] == 0 && xPosition[loop] < 0) {
			degree = 9;
			sign = 1;
		}
		else if (yPosition[loop] == 0 && xPosition[loop] > 0) {
			degree = 9;
			sign = 0;
		}
		else if (yPosition[loop] > 0 && xPosition[loop] == 0) {
			degree = 18;
			sign = 0;
		}
		else if (yPosition[loop] < 0 && xPosition[loop] == 0) {
			degree = 0;
			sign = 0;
		}
		else if (yPosition[loop] == 0 && xPosition[loop] == 0) {
			degree = 0;
			sign = 0;
		}

		if (tan > 5670) degree = offset + 9;				// 90
		else if (tan > 2750) degree = offset + 8;			// 80
		else if (tan > 1730) degree = offset + 7;			// 70
		else if (tan > 1190) degree = offset + 6;			// 60
		else if (tan > 839) degree = offset + 5;			// 50
		else if (tan > 577) degree = offset + 4;			// 40
		else if (tan > 364) degree = offset + 3;			// 30
		else if (tan > 176) degree = offset + 2;			// 20
		else if (tan > 0) degree = offset + 1;				// 10

		if (sign0 == 0 && sign == 0) {
			if (degree >= degree0) {
				angle[loop] = degree - degree0;
				direction[loop] = 0;
			}
			else {
				angle[loop] = degree0 - degree;
				direction[loop] = 1;
			}
		}
		else if (sign0 == 0 && sign == 1) {
			angle[loop] = degree + degree0;
			direction[loop] = 1;
			if(angle[loop] >= 18) {
				angle[loop] = 36 - angle[loop];
				direction[loop] = 0;
			}
		}
		else if (sign0 == 1 && sign == 0) {
			angle[loop] = degree + degree0;
			direction[loop] = 0;
			if(angle[loop] > 18) {
				angle[loop] = 36 - angle[loop];
				direction[loop] = 1;
			}
		}
		else if (sign0 == 1 && sign == 1) {
			if (degree >= degree0) {
				angle[loop] = degree - degree0;
				direction[loop] = 1;
			}
			else {
				angle[loop] = degree0 - degree;
				direction[loop] = 0;
			}
		}

		degree0 = degree;
		sign0 = sign;

		tan = 0;
		offset = 0;
	}
}

static void timeoutHandler(unsigned long data) {

	if (routeGet == 2) {			//notify vehicle
		pxa_gpio_set_value(S,1);

		routeGet = 1;

		setup_timer(timeout, timeoutHandler, 0);
		(*timeout).expires = jiffies + (HZ / 100);		//10ms
		add_timer(timeout);
	}

	else if (routeGet == 1) {			//driving vehicle
		if (sendTime < numberOfPoints) {
			switch(angle[sendTime]) {
			case 0:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 1:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 2:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 3:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 4:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 5:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 6:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 7:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
				break;
			case 8:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 9:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 10:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 11:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 12:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 13:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 14:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 15:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,1);
				pxa_gpio_set_value(D3,1);
				pxa_gpio_set_value(D4,0);
				break;
			case 16:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,1);
				break;
			case 17:
				pxa_gpio_set_value(D0,1);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,1);
				break;
			case 18:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,1);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,1);
				break;
			default:
				pxa_gpio_set_value(D0,0);
				pxa_gpio_set_value(D1,0);
				pxa_gpio_set_value(D2,0);
				pxa_gpio_set_value(D3,0);
				pxa_gpio_set_value(D4,0);
			}

			switch(direction[sendTime]) {
			case 0:
				pxa_gpio_set_value(S,0);
				break;
			case 1:
				pxa_gpio_set_value(S,1);
				break;
			default:
				pxa_gpio_set_value(S,0);
			}

			sendTime++;

			setup_timer(timeout, timeoutHandler, 0);
			(*timeout).expires = jiffies + (HZ / 100);		//10ms
			add_timer(timeout);
		}

		else {
			pxa_gpio_set_value(D0,1);
			pxa_gpio_set_value(D1,1);
			pxa_gpio_set_value(D2,1);
			pxa_gpio_set_value(D3,1);
			pxa_gpio_set_value(D4,1);
			pxa_gpio_set_value(S,0);

			routeGet = 0;
			sendTime = 0;

			setup_timer(timeout, timeoutHandler, 0);
			(*timeout).expires = jiffies + (1 * HZ / 2);		//500ms
			add_timer(timeout);
		}
	}

	else if (routeGet == 0) {		//waiting for route info
		setup_timer(timeout, timeoutHandler, 0);
		(*timeout).expires = jiffies + (1 * HZ / 2);			//500ms
		add_timer(timeout);
	}

}

