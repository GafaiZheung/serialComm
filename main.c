#include     <stdio.h>      /*标准输入输出定义*/
#include     <stdlib.h>     /*标准函数库定义*/
#include     <unistd.h>     /*Unix 标准函数定义*/
#include     <sys/types.h>
#include     <sys/stat.h>
#include     "string.h"
#include     <fcntl.h>      /*文件控制定义*/
#include     <termios.h>    /*PPSIX 终端控制定义*/
#include     <errno.h>      /*错误号定义*/
#include     "pthread.h"    /*多线程*/
#include     "time.h"       /*时间戳*/

#define MAX 5
#define FALSE -1
#define TRUE 0

pthread_t thread[2];
pthread_mutex_t mut;

//波特率数组
int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
                    B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,
                  19200,  9600, 4800, 2400, 1200,  300, };

int fd;
int end_flag = 1;

int OpenDev(char *Dev);//打开串口
void set_speed(int speed);//设置串口速率
int set_Parity(int databits,int stopbits,int parity);
void *thread1();
void *thread2();
void thread_create();
void thread_wait();
void print_time();


int main(int argc, char **argv)
{
     char *dev = "/dev/cu.usbserial-FTB6SPL3";//真实端口
//    char *dev = "/dev/ttys002";//调试虚拟端口
    fd = OpenDev(dev);
    set_speed(9600);

    if (set_Parity(8, 1, 'N') == FALSE)
    {
        printf("Set Parity Error/n");
        exit(0);
    }
    thread_create();
    thread_wait();
    close(fd);
    return 0;
}



int OpenDev(char *Dev)
{
    fd = open(Dev, O_RDWR);
    if (fd == -1)
    {
        perror("Can't Open Serial Port");
        return -1;
    }
    else
    {
        printf("Serial Port Connected\n");
        sleep(2);
        return fd;
    }
}


void set_speed(int speed)
{
    int   i;
    int   status;
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
        if  (speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if  (status != 0) {
                perror("tcsetattr fd1");
                return;
            }
            tcflush(fd,TCIOFLUSH);
        }
    }
}


int set_Parity(int databits,int stopbits,int parity)
{
    struct termios options;
    options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    options.c_oflag  &= ~OPOST;   /*Output*/
    if  ( tcgetattr( fd,&options)  !=  0) {
        perror("SetupSerial 1");
        return(FALSE);
    }
    options.c_cflag &= ~CSIZE;
    switch (databits) /*设置数据位数*/
    {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr,"Unsupported data size/n"); return (FALSE);
    }
    switch (parity)
    {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;   /* Clear parity enable */
            options.c_iflag &= ~INPCK;     /* Enable parity checking */
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
            options.c_iflag |= INPCK;             /* Disnable parity checking */
            break;
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;     /* Enable parity */
            options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
            options.c_iflag |= INPCK;       /* Disnable parity checking */
            break;
        case 'S':
        case 's':  /*as no parity*/
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;break;
        default:
            fprintf(stderr,"Unsupported parity/n");
            return (FALSE);
    }
    /* 设置停止位*/
    switch (stopbits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr,"Unsupported stop bits/n");
            return (FALSE);
    }
    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;
    tcflush(fd,TCIFLUSH);
    options.c_cc[VTIME] = 150; /* 设置超时15 seconds*/
    options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        perror("SetupSerial 3");
        return (FALSE);
    }
    return (TRUE);
}

void *thread1()
{
    int i;
    pthread_mutex_lock(&mut);
//  printf("write %d\n",i+1);
//    for (i = 0; i < 1; i++)
    while(end_flag)
    {
        char ch;
        char buf1[256];
        i = 0;
        while ((ch = getchar()) != '\n')
        {
            buf1[i] = ch;
            i++;
        }
//        scanf("%s", buf1);
        if(buf1[0] == 'q' && buf1[1] == 'u' && buf1[2] == 'i' && buf1[3] == 't')
            end_flag = 0;

        int length = sizeof(buf1);
        int j = write(fd,buf1,length);

        print_time();
        printf("send:");
        puts(buf1);

        if(j < 0)
            printf("send failure\n");
//      printf("%d n",j);
        memset(buf1, 0, sizeof(buf1));
        sleep(1);
        pthread_mutex_unlock(&mut);
    }
    printf("thread1 has killed\n");
    pthread_exit(NULL);
}

void *thread2()

{
    int j;
    sleep(1);
//    printf("thread2\n");
//    for (j = 0; j< MAX; j++)
    while(end_flag)
    {
        pthread_mutex_lock(&mut);
        sleep(1);
        char buf[256];
//        printf("read %d\n",j+1);
        read(fd, buf, 256);
//        printf("k+%d\n",k);
        if(strcmp(buf, "") != 0)
        {
            print_time();
            printf("recv:");
            puts(buf);
            memset(buf, 0 , sizeof(buf));
        }
        pthread_mutex_unlock(&mut);
        sleep(1);
    }
    printf("thread2 has killed\n");

    pthread_exit(NULL);
}

void thread_create()
{
    int temp;
    memset(&thread, 0, sizeof(thread));          //comment1
    /*创建线程*/
    if((temp = pthread_create(&thread[0], NULL, thread1, NULL)) != 0)
    {
        printf("thread1 create failure\n");
        exit(-1);
    }//comment2
    else
        printf("thread1 create success\n");
    if((temp = pthread_create(&thread[1], NULL, thread2, NULL)) != 0)  //comment3
    {
        printf("thread2 create failure\n");
        exit(-1);
    }
    else
        printf("thread2 create success\n");
}

void thread_wait()
{
    /*等待线程结束*/
    if(thread[0] !=0) {                   //comment4
        pthread_join(thread[0],NULL);
//        printf("1 stop \n");

    }
    if(thread[1] !=0) {                //comment5
        pthread_join(thread[1],NULL);
//        printf("2 stop \n");
    }
}

void print_time()
{
    time_t t;
    struct tm *lt;
    time(&t);
    lt = localtime(&t);
    printf("[%02d:%02d:%02d]",lt->tm_hour,lt->tm_min,lt->tm_sec);
}
