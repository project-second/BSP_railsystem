#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define DEVICE_NAME "vms_rail_hal"
#define CLASS_NAME "vms_class"

// IOCTL 매직 넘버 및 커맨드 정의
#define VMS_IOC_MAGIC 'V'
#define VMS_IOC_SET_ZERO        _IO(VMS_IOC_MAGIC, 1)
#define VMS_IOC_SET_SAFEDIST    _IOW(VMS_IOC_MAGIC, 2, int)
#define VMS_IOC_GET_ESTOP_STATE _IOR(VMS_IOC_MAGIC, 3, int)

// [TODO] 실제 라즈베리파이 결선에 맞게 GPIO 핀 번호 수정 필요
#define GPIO_MOTOR_IN1 17
#define GPIO_MOTOR_IN2 27
#define GPIO_MOTOR_PWM 22
#define GPIO_ENCODER_A 23
#define GPIO_ENCODER_B 24
#define GPIO_IR_SENSOR 25 // 긴급 정지용 인터럽트 핀

static int major_number;
static struct class* vms_char_class = NULL;
static struct device* vms_char_device = NULL;

// 상태 변수
static long pulse_count = 0;
static int is_estop_active = 0;
static int ir_irq_number;

// 모터 제어 명령 구조체 (User Space와 동일해야 함)
struct motor_cmd {
    int direction; // 1: Forward, -1: Reverse, 0: Stop
    int speed;     // PWM 값
};

// 위치 데이터 구조체 (User Space와 동일해야 함)
struct position_data {
    long pulse;
    float distance_cm;
};

// --- 인터럽트 서비스 루틴 (긴급 정지) ---
static irqreturn_t ir_sensor_isr(int irq, void *dev_id) {
    // IR 센서 감지 시 즉각 모터 정지
    gpio_set_value(GPIO_MOTOR_IN1, 0);
    gpio_set_value(GPIO_MOTOR_IN2, 0);
    is_estop_active = 1;
    
    printk(KERN_ALERT "VMS_HAL: [EMERGENCY STOP] Obstacle detected!\n");
    return IRQ_HANDLED;
}

// --- File Operations ---

// Read: 현재 위치 정보 반환
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    struct position_data pos;
    int error_count = 0;

    pos.pulse = pulse_count;
    pos.distance_cm = (float)pulse_count * 0.05; // [TODO] 실제 바퀴 둘레에 맞게 팩터 수정

    error_count = copy_to_user(buffer, &pos, sizeof(struct position_data));
    if (error_count == 0) {
        return sizeof(struct position_data);
    } else {
        return -EFAULT;
    }
}

// Write: 모터 구동 명령 수신
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    struct motor_cmd cmd;
    
    if (is_estop_active) {
        printk(KERN_WARNING "VMS_HAL: Motor locked due to E-STOP.\n");
        return -EPERM; // 긴급 정지 상태에서는 명령 무시
    }

    if (copy_from_user(&cmd, buffer, sizeof(struct motor_cmd)) != 0) {
        return -EFAULT;
    }

    // 모터 방향 제어
    if (cmd.direction == 1) {
        gpio_set_value(GPIO_MOTOR_IN1, 1);
        gpio_set_value(GPIO_MOTOR_IN2, 0);
    } else if (cmd.direction == -1) {
        gpio_set_value(GPIO_MOTOR_IN1, 0);
        gpio_set_value(GPIO_MOTOR_IN2, 1);
    } else {
        gpio_set_value(GPIO_MOTOR_IN1, 0);
        gpio_set_value(GPIO_MOTOR_IN2, 0);
    }
    
    // [TODO] PWM 제어 로직 (소프트웨어 PWM 또는 하드웨어 PWM 레지스터 제어) 추가 필요

    return len;
}

// IOCTL: 영점 조절 및 상태 확인
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case VMS_IOC_SET_ZERO:
            pulse_count = 0;
            printk(KERN_INFO "VMS_HAL: Position zeroed.\n");
            break;
        case VMS_IOC_GET_ESTOP_STATE:
            if (copy_to_user((int *)arg, &is_estop_active, sizeof(int))) {
                return -EFAULT;
            }
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static int dev_open(struct inode *inodep, struct file *filep) { return 0; }
static int dev_release(struct inode *inodep, struct file *filep) { return 0; }

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
    .release = dev_release,
};

// --- 모듈 초기화 및 종료 ---
static int __init vms_hal_init(void) {
    printk(KERN_INFO "VMS_HAL: Initializing Edge Device HAL...\n");

    // 1. 디바이스 넘버 할당 및 클래스 생성
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    vms_char_class = class_create(THIS_MODULE, CLASS_NAME);
    vms_char_device = device_create(vms_char_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);

    // 2. GPIO 초기화 및 설정 (예시: IR 센서)
    gpio_request(GPIO_IR_SENSOR, "sysfs");
    gpio_direction_input(GPIO_IR_SENSOR);
    
    // 3. IR 센서 인터럽트 등록 (Falling edge 감지 예시)
    ir_irq_number = gpio_to_irq(GPIO_IR_SENSOR);
    request_irq(ir_irq_number, ir_sensor_isr, IRQF_TRIGGER_FALLING, "vms_ir_sensor", NULL);

    printk(KERN_INFO "VMS_HAL: Initialization complete. Ready for commands.\n");
    return 0;
}

static void __exit vms_hal_exit(void) {
    free_irq(ir_irq_number, NULL);
    gpio_free(GPIO_IR_SENSOR);
    device_destroy(vms_char_class, MKDEV(major_number, 0));
    class_unregister(vms_char_class);
    class_destroy(vms_char_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "VMS_HAL: Module successfully unloaded.\n");
}

module_init(vms_hal_init);
module_exit(vms_hal_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeonghan Lee");
MODULE_DESCRIPTION("Hardware Abstraction Layer for Rail-integrated CCTV");