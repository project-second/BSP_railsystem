#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

#define DEVICE_NAME "vms_hw_ctrl"
#define CLASS_NAME "vms_ctrl_class"

// [TODO] 실제 결선에 맞게 핀 번호 및 ADC 채널 수정 필요
#define GPIO_SW_CAM1 17 // CCTV 1번 선택 스위치
#define GPIO_SW_CAM2 27 // CCTV 2번 선택 스위치
// 조이스틱 X/Y축은 보통 ADC 모듈(예: MCP3008)을 통해 SPI 통신으로 읽어옵니다.
// 이 예제에서는 구조를 보여주기 위해 가상의 ADC 읽기 함수를 사용합니다.

static int major_number;
static struct class* ctrl_class = NULL;
static struct device* ctrl_device = NULL;

// 컨트롤러 상태 구조체 (User Space의 Qt 앱과 동일해야 함)
struct ctrl_state {
    int selected_cctv_id; // 현재 선택된 CCTV (1, 2, ...)
    int joystick_x;       // -100 (좌) ~ 100 (우)
    int joystick_y;       // -100 (하) ~ 100 (상) - 상하 이동이 필요 없다면 무시
    int is_button_pressed;// 조이스틱 Z버튼 (눌림 여부)
};

static struct ctrl_state current_state = {1, 0, 0, 0};

// 가상의 ADC 읽기 함수 (실제로는 SPI/I2C 통신 코드 구현 필요)
static int read_adc_channel(int channel) {
    // [TODO] MCP3008 등 ADC 모듈에서 값을 읽어오는 로직
    // 임시 반환값 (중앙값 512 기준 정규화 필요)
    return 512; 
}

// --- File Operations ---

// Read: Qt 앱이 주기적으로 컨트롤러 상태를 폴링(Polling)할 때 호출됨
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;

    // 1. 스위치 상태 업데이트 (어느 CCTV를 조작할 것인가?)
    if (gpio_get_value(GPIO_SW_CAM1) == 0) { // Active Low 가정
        current_state.selected_cctv_id = 1;
    } else if (gpio_get_value(GPIO_SW_CAM2) == 0) {
        current_state.selected_cctv_id = 2;
    }

    // 2. 조이스틱 아날로그 값 읽기 및 정규화 (-100 ~ 100)
    // ADC 값이 0~1023 이고 중앙이 512라고 가정
    int raw_x = read_adc_channel(0); 
    int raw_y = read_adc_channel(1);
    
    current_state.joystick_x = ((raw_x - 512) * 100) / 512;
    current_state.joystick_y = ((raw_y - 512) * 100) / 512;

    // 데드존(Deadzone) 처리: 스틱이 완벽히 중앙으로 돌아오지 않는 오차 무시
    if (current_state.joystick_x > -10 && current_state.joystick_x < 10) current_state.joystick_x = 0;
    if (current_state.joystick_y > -10 && current_state.joystick_y < 10) current_state.joystick_y = 0;

    // 3. User Space로 데이터 복사
    error_count = copy_to_user(buffer, &current_state, sizeof(struct ctrl_state));
    if (error_count == 0) {
        return sizeof(struct ctrl_state);
    } else {
        return -EFAULT;
    }
}

static int dev_open(struct inode *inodep, struct file *filep) { return 0; }
static int dev_release(struct inode *inodep, struct file *filep) { return 0; }

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .release = dev_release,
};

// --- 모듈 초기화 및 종료 ---
static int __init vms_ctrl_init(void) {
    printk(KERN_INFO "VMS_CTRL: Initializing Controller Driver...\n");

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    ctrl_class = class_create(THIS_MODULE, CLASS_NAME);
    ctrl_device = device_create(ctrl_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);

    // GPIO 초기화 (스위치 입력)
    gpio_request(GPIO_SW_CAM1, "sysfs");
    gpio_direction_input(GPIO_SW_CAM1);
    gpio_request(GPIO_SW_CAM2, "sysfs");
    gpio_direction_input(GPIO_SW_CAM2);
    
    // [TODO] SPI/ADC 모듈 초기화 로직 추가

    printk(KERN_INFO "VMS_CTRL: Initialization complete.\n");
    return 0;
}

static void __exit vms_ctrl_exit(void) {
    gpio_free(GPIO_SW_CAM1);
    gpio_free(GPIO_SW_CAM2);
    // [TODO] SPI/ADC 모듈 해제 로직 추가
    
    device_destroy(ctrl_class, MKDEV(major_number, 0));
    class_unregister(ctrl_class);
    class_destroy(ctrl_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "VMS_CTRL: Module unloaded.\n");
}

module_init(vms_ctrl_init);
module_exit(vms_ctrl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeonghan Lee");
MODULE_DESCRIPTION("Hardware Controller Driver for VMS Client");