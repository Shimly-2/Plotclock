/**
 * @file   plotclock.ino
 * @author HHH（https://gitee.com/jin-yiyang）
 * @brief  plotclock 工程文件
 * 包含舵机控制，DS3231时钟模块读取
 * 该版本只能在画板上绘制当前时间，其他功能待开发
 * @version 1.0
 * @date 2022-1-25
 *
 * @copyright Copyright (c) 2022
 *
 */
 
#include <Time.h>   //需手动安装time库
#include <TimeLib.h>
#include <Servo.h>  //servo controller
#include <Wire.h>   
#include <DS3231.h> //需手动安装DS3231库

#define normal_mode     // 开启正常模式
//#define ADJUST        // 开启调试模式
//#define GPIO          // 开启调试模式
#define extra_mode      // 开启额外模式

/* Adjust these values for servo arms in position for state 1 _| */
const double SERVO_LEFT_ZERO =  1600; //初始状态
const double SERVO_RIGHT_SCALE = 690; // + makes rotate further left

/* Adjust these values for servo arms in position for state 2 |_ */
const double SERVO_RIGHT_ZERO = 650;
const double SERVO_LEFT_SCALE = 650;

/*    必要的绘图延时     */
const double DRAW_DELAY = 5;  

/*    DS3231初始化    */
DS3231  rtc(SDA, SCL);

/*    设定舵机引脚以及LED引脚    */
const int SERVO_LEFT_PIN = 6;
const int SERVO_RIGHT_PIN = 5;
const int LED_PIN = 12;

/*    设定舵机力臂的长度    */
const double LOWER_ARM = 35;      //servo to lower arm joint
const double UPPER_ARM_LEFT = 56; //lower arm joint to led
const double LED_ARM = 13.5;      //upper arm joint to led
const double UPPER_ARM = 45;      //lower arm joint to upper arm joint
double cosineRule(double a, double b, double c);
const double LED_ANGLE = cosineRule(UPPER_ARM_LEFT,UPPER_ARM,LED_ARM);

/*    设定初始的左右舵机坐标    */
const double SERVO_LEFT_X = 22;
const double SERVO_LEFT_Y = -32;
const double SERVO_RIGHT_X = SERVO_LEFT_X + 25.5;
const double SERVO_RIGHT_Y = SERVO_LEFT_Y;

/*    计算舵机角    */
#define radian(angle) (M_PI*2* angle)
#define dist(x,y) sqrt(sq(x)+sq(y))
#define angle(x,y) atan2(y,x)

/*    设定绘制的字体大小    */
const double TIME_BOTTOM = 12;
const double TIME_WIDTH = 11;
const double TIME_HEIGHT = 18; 

/*    设定绘制的字体大小    */
const double DAY_WIDTH = 7;
const double DAY_HEIGHT = 12;
const double DAY_BOTTOM = 5;
const double DATE_BOTTOM = 24;

/*    设定初始location    */
const double HOME_X = 55, HOME_Y = -5;
Servo servoLeft, servoRight;

/*    设定星期序列    */
const char weekDays[] = {8,10,12, 5,6,12, 9,10,2, 11,2,13, 9,4,10, 3,7,14, 8,1,9}; //character set: AEFHMORSTUWNDI

double lastX = HOME_X, lastY = HOME_Y;

/*    设定画笔状态    */
bool lightOn = false;

/*    设定gpio状态    */
void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    rtc.begin();
}

/*    设定画笔状态    */
void light(bool state){
    lightOn = state == HIGH; 
    delay(100);
    digitalWrite(LED_PIN, state);
}

const int LONG_PRESS_DURATION = 750;

void loop(){
    if (!servoLeft.attached()) servoLeft.attach(SERVO_LEFT_PIN);
    if (!servoRight.attached()) servoRight.attach(SERVO_RIGHT_PIN);

    /*    开启调试模式，使起始终止点正确    */
    #ifdef ADJUST
        /** 
         *  Pressing the button alternates the servo arms between 2 states.
         *  State one if left arm pointing to 9 o'clock and right arm pointing to 12 o'clock _|
         *  State two if left arm pointing to 12 o'clock and right arm pointing to 3 o'clock |_
         *  At the very top of the code you adjust the 4 constants to get the arms into these exact positions.
         *  Adjust SERVO_LEFT_ZERO so that the left servo points to 9 o'clock when in state one
         *  Adjust SERVO_RIGHT_SCALE so that the right servo points to 12 o'clock when in state one
         *  Adjust SERVO_RIGHT_ZERO so that the right servo points to 3 o'clock when in state two
         *  Adjust SERVO_LEFT_SCALE so that the left servo points to 12 o'clock when in state two
        */
        static bool half;
        servoLeft.writeMicroseconds(floor(SERVO_LEFT_ZERO + (half ? - M_PI/2  : 0) * SERVO_LEFT_SCALE ));
        servoRight.writeMicroseconds(floor(SERVO_RIGHT_ZERO + (half ? 0 : M_PI/2 ) * SERVO_RIGHT_SCALE ));
        light(half ? LOW : HIGH);
        half = !half;
        delay(2000);
    #else //ADJUST
        /*    开始调试模式，测试LED的使用    */
        #ifdef GPIO
            for(int i = 0; i <= 70; i += 10)
                for(int j = 0; j <= 40; j += 10){
                    drawTo(i, j);
                    light(HIGH);
                    light(LOW);
                }                
        #else //GPIO
            delay(10); 
            uint32_t longpress = millis() + LONG_PRESS_DURATION;
            while ((!digitalRead(BUTTON_PIN)) && (millis() < longpress))
            { }; // wait
            bool date = false;
            if (millis() >= longpress)
                date = true;               
            drawTo(HOME_X, 0);
        #endif // NOT ADJUST OR GRID
        
        /*    主程序运行从这里开始,运行正常时钟模式    */
        #ifdef normal_mode
            /*    读取时钟的时间    */
            uint8_t Hour=rtc.gethour();
            uint8_t Min=rtc.getmin();
            uint8_t Sec=rtc.getsec();
            
            /*    由小时和分钟绘制到画板    */
            if(Hour / 10)
                drawDigit(3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, Hour / 10);
            drawDigit(3+TIME_WIDTH+3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, Hour % 10);
            // Draw colon
            drawDigit((69-TIME_WIDTH)/2, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, 11);
            // minute
            drawDigit(69-(TIME_WIDTH+3)*2, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, Min / 10);
            drawDigit(72-(TIME_WIDTH+3), TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, Min % 10);
            drawTo(HOME_X, HOME_Y);
        #endif

        /*    主程序运行从这里开始,运行额外模式    */
        #ifdef extra_mode
            drawDigit(3+TIME_WIDTH, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, 12);  // N
            drawDigit(3+TIME_WIDTH+TIME_WIDTH+3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, 2);  // E
            drawDigit(3+TIME_WIDTH+TIME_WIDTH+3+TIME_WIDTH+3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, 11);  // W

            drawDigit(3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, 12);  // N
            drawDigit(3+TIME_WIDTH+3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, 2);  // E
            drawDigit(3+TIME_WIDTH+3+TIME_WIDTH+3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, 1);  // A
            drawDigit(3+TIME_WIDTH+3+TIME_WIDTH+3+TIME_WIDTH+3, TIME_BOTTOM, TIME_WIDTH, TIME_HEIGHT, R);  // W
        #endif
    #endif // GRID or Normal Plot Time
    servoLeft.detach();
    servoRight.detach();
}

/*    定义相关的运行函数    */
#define digitMove(dx, dy) drawTo(x + width*dx, y + height*dy)
#define digitStart(dx, dy) digitMove(dx, dy); light(HIGH)
#define digitArc(dx, dy, rx, ry, start, last) drawArc(x + width*dx, y + height*dy, width*rx, height*ry, radian(start), radian(last))

/**
 * @FunctionName:  drawDigit
 * @Description：  用于绘制数字
 * @Calls:         none
 * Called By:      loop
 * Input:          x           起始点x坐标
 *                 y           起始点y坐标
 *                 width       字体宽度
 *                 height      字体长度
 *                 digit       绘制的数字
 * Output:         none
 * Return:         none
 */
void drawDigit(double x, double y, double width, double height, char digit) {
    //see macros for reference
    switch (digit) {
        case 0: //
            digitStart(1/2,1);
            digitArc(1/2,1/2, 1/2,1/2, 1/4, -3/4);
            //digitStart(1,1/2);
            //digitArc(1/2,1/2, 1/2,1/2, 0, 1.02);
            break;
        case 1: //
            digitStart(1/4,7/8);
            digitMove(1/2,1);
            digitMove(1/2,0);
            break;
        case 2: //
            digitStart(0,3/4);
            digitArc(1/2,3/4, 1/2,1/4, 1/2, -1/8);
            digitArc(1,0, 1,1/2, 3/8, 1/2);
            digitMove(1,0);
            break;
        case 3:
            digitStart(0,3/4);
            digitArc(1/2,3/4, 1/2,1/4, 3/8, -1/4);
            digitArc(1/2,1/4, 1/2,1/4, 1/4, -3/8);
            break;
        case 4:
            digitStart(1,3/8);
            digitMove(0,3/8);
            digitMove(3/4,1);
            digitMove(3/4,0);
            break;
        case 5: //wayy too many damn lines
            digitStart(1,1);
            digitMove(0,1);
            digitMove(0,1/2);
            digitMove(1/2,1/2);
            digitArc(1/2,1/4, 1/2,1/4, 1/4, -1/4);
            digitMove(0,0);
            break;
        case 6:
            digitStart(0,1/4);
            digitArc(1/2,1/4, 1/2,1/4, 1/2, -1/2);
            digitArc(1,1/2, 1,1/2, 1/2, 1/4);
            break;
        case 7:
            digitStart(0,1);
            digitMove(1,1);
            digitMove(1/4,0);   
            break;
        case 8:
            digitStart(1/2,1/2);
            digitArc(1/2,3/4, 1/2,1/4, -1/4, 3/4);
            digitArc(1/2,1/4, 1/2,1/4, 1/4, -3/4);
            break;
        case 9:
            digitStart(1,3/4);
            digitArc(1/2,3/4, 1/2,1/4, 0, 1);
            digitMove(3/4,0);
            break;
        case 10: //dot
            digitStart(0,0);
            //digitMove(0,1);
            //digitMove(1,1);
            //digitMove(1,0);
            break;
        case 11: //colon
            digitStart(1/2,3/4);
            light(LOW);
            digitStart(1/2,1/4);
            break;
        case 12: //slash
            digitStart(3/4,5/4);
            digitMove(1/4,-1/4);
            break;
    }
    light(LOW);
}

/**
 * @FunctionName:  drawChar
 * @Description：  用于绘制字符
 * @Calls:         none
 * Called By:      none        本项目暂未用到
 * Input:          x           起始点x坐标
 *                 y           起始点y坐标
 *                 width       字体宽度
 *                 height      字体长度
 *                 digit       绘制的字符
 * Output:         none
 * Return:         none
 */
void drawChar(double x, double y, double width, double height, char digit) {
    //see macros for reference
    switch (digit) {
        //letters for the day of the week
        case 1: //A
            digitStart(0,0);
            digitMove(1/2,1);
            digitMove(1,0);
            light(LOW);
            digitStart(1/4,1/2);
            digitMove(3/4,1/2);
          break;
        case 2: //E
            digitStart(1,0);
            digitMove(0,0);
            digitMove(0,1);
            digitMove(1,1);
            light(LOW);
            digitStart(0,1/2);
            digitMove(1,1/2);
            break;
        case 3: //F
            digitStart(0,0);
            digitMove(0,1);
            digitMove(1,1);
            light(LOW);
            digitStart(0,1/2);
            digitMove(1,1/2);
            break;
        case 4: //H
            digitStart(0,1);
            digitMove(0,0);
            light(LOW);
            digitStart(0,1/2);
            digitMove(1,1/2);
            light(LOW);
            digitStart(1,1);
            digitMove(1,0);
            break;
        case 5: //M
            digitStart(0,0);
            digitMove(0,1);
            digitMove(1/2,1/2);
            digitMove(1,1);
            digitMove(1,0);
            break;
        case 6: //O (0)
            digitStart(1,1/2);
            digitArc(1/2,1/2, 1/2,1/2, 0, 1.02);
            break;
        case 7: //R
            digitStart(0,0);
            digitMove(0,1);
            digitMove(1/2,1);
            digitArc(1/2,3/4, 1/2,1/4, 1/4, -1/4);
            digitMove(0,1/2);
            digitMove(1,0);
            break;
        case 8: //S
            digitStart(0,0);
            digitMove(1/2,0);
            digitArc(1/2,1/4, 1/2,1/4, -1/4, 1/4);
            digitArc(1/2,3/4, 1/2,1/4, 3/4, 1/4);
            digitMove(1,1);
            break;
        case 9: //T
            digitStart(1,1);
            digitMove(-1/2,1); //bad
            light(LOW);
            digitStart(1/2,1);
            digitMove(1/2,0);
            break;
        case 10: //U
            digitStart(0,1);
            digitMove(0,1/4);
            digitArc(1/2,1/4, 1/2,1/4, -1/2, 0);
            digitMove(1,1);
            break;
        case 11: //W
            digitStart(0,1);
            digitMove(0,0);
            digitMove(1/2,1/2);
            digitMove(1,0);
            digitMove(1,1);
            break;
        case 12: //N
            digitStart(0,0);
            digitMove(0,1);
            digitMove(1,0);
            digitMove(1,1);
            break;
        case 13: //D
            digitStart(0,0);
            digitMove(0,1);
            digitMove(1/2,1);
            digitArc(1/2,1/2, 1/2,1/2, 1/4,-1/4);
            digitMove(0,0);
            break;
        case 14: //I
            digitStart(1/2,1);
            digitMove(1/2,0);
            light(LOW);
            digitStart(0,0);
            digitMove(1,0);
            light(LOW);
            digitStart(1,1);
            digitMove(0,1);
            break;
        case 15: //P
            digitStart(0,0);
            digitMove(0,1);
            digitMove(1/2,1);
            digitArc(1/2,3/4, 1/2,1/4, 1/4, -1/4);
    }
    light(LOW);
}

/**
 * @FunctionName:  drawArc
 * @Description：  用于绘制曲线
 * @Calls:         none
 * Called By:      none        本项目暂未用到
 * Input:          x           起始点x坐标
 *                 y           起始点y坐标
 *                 rx          x方向增量
 *                 ry          y方向增量
 *                 pos         相位角
 *                 last        终点坐标
 * Output:         none
 * Return:         none
 */
#define ARCSTEP 0.05 //should change depending on radius
void drawArc(double x, double y, double rx, double ry, double pos, double last) {
    if(pos < last)
        for(; pos <= last; pos += ARCSTEP)
            drawTo(x + cos(pos)*rx, y + sin(pos)*ry);
    else
        for(; pos >= last; pos -= ARCSTEP)
            drawTo(x + cos(pos)*rx, y + sin(pos)*ry);
}

/**
 * @FunctionName:  drawTo
 * @Description：  用于使舵机移动到固定位置
 * @Calls:         none
 * Called By:      loop        
 * Input:          pX          固定点x坐标
 *                 pY          固定点y坐标
 * Output:         none
 * Return:         none
 */
void drawTo(double pX, double pY) {
    double dx, dy, c;
    int i;
    
    // dx dy of new point
    dx = pX - lastX;
    dy = pY - lastY;
    //path length in mm, times 4 equals 4 steps per mm
    c = floor(4 * dist(dx,dy));
    
    if (c < 1)
        c = 1;
    
    // draw line point by point
    for (i = 1; i <= c; i++){
        set_XY(lastX + (i * dx / c), lastY + (i * dy / c));
        if (lightOn)
            delay(DRAW_DELAY);
    }
    
    lastX = pX;
    lastY = pY;
}

/**
 * @FunctionName:  cosineRule
 * @Description：  余弦公式
 * @Calls:         none
 * Called By:      loop        
 * Input:          a           三角形a边
 *                 b           三角形b边
 *                 c           三角形c边
 * Output:         三角形b边的长度
 * Return:         三角形b边的长度
 */
double cosineRule(double a, double b, double c) {
    return acos((sq(a)+sq(c)-sq(b))/(2*a*c));
}

/**
 * @FunctionName:  set_XY
 * @Description：  用于使舵机移动到固定位置
 * @Calls:         none
 * Called By:      loop        
 * Input:          x           固定点x坐标
 *                 y           固定点y坐标
 * Output:         none
 * Return:         none
 */
void set_XY(double x, double y) {
    //Calculate triangle between left servo, left arm joint, and light
    //Position of pen relative to left servo
    //rectangular
    double penX = x - SERVO_LEFT_X;
    double penY = y - SERVO_LEFT_Y;
    //polar
    double penAngle = angle(penX,penY);
    double penDist = dist(penX,penY);
    //get angle between lower arm and a line connecting the left servo and the pen
    double bottomAngle = cosineRule(LOWER_ARM, UPPER_ARM_LEFT, penDist);
    
    servoLeft.writeMicroseconds(floor(SERVO_LEFT_ZERO + (bottomAngle + penAngle - M_PI) * SERVO_LEFT_SCALE));
    
    //calculate middle arm joint location
    double topAngle = cosineRule(UPPER_ARM_LEFT, LOWER_ARM, penDist);
    double lightAngle = penAngle - topAngle + LED_ANGLE + M_PI;
    double jointX = x - SERVO_RIGHT_X + cos(lightAngle) * LED_ARM;
    double jointY = y - SERVO_RIGHT_Y + sin(lightAngle) * LED_ARM;
    
    bottomAngle = cosineRule(LOWER_ARM, UPPER_ARM, dist(jointX, jointY));
    double jointAngle = angle(jointX, jointY);
    
    servoRight.writeMicroseconds(floor(SERVO_RIGHT_ZERO + (jointAngle - bottomAngle) * SERVO_RIGHT_SCALE));
}
