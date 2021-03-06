﻿/********************************** (C) COPYRIGHT *******************************
* File Name          : CDC.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/03/01
* Description        : CH554 as CDC device to serial port, select serial port 1
*******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <ch554.h>
#include <ch554_usb.h>
#include <debug.h>

#include "vscode.h"
#include "gpio.h"
#include "nrf24.h"

__xdata __at (0x0000) uint8_t  Ep0Buffer[DEFAULT_ENDP0_SIZE];      // Endpoint 0 OUT & IN buffer, must be an even address
__xdata __at (0x0040) uint8_t  Ep1Buffer[DEFAULT_ENDP1_SIZE];       //Endpoint 1 upload buffer
__xdata __at (0x0080) uint8_t  Ep2Buffer[2*MAX_PACKET_SIZE];        //Endpoint 2 IN & OUT buffer, must be an even address

uint16_t SetupLen;
uint8_t   SetupReq,Count,UsbConfig;
const uint8_t *  pDescr;                                                       //USB configuration flag
USB_SETUP_REQ   SetupReqBuf;                                                   //Temporary Setup package
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)

#define  SET_LINE_CODING                0X20            // Configures DTE rate, stop-bits, parity, and number-of-character
#define  GET_LINE_CODING                0X21            // This request allows the host to find out the currently configured line coding.
#define  SET_CONTROL_LINE_STATE         0X22            // This request generates RS-232/V.24 style control signals.



/*设备描述符*/
__code uint8_t DevDesc[] = {0x12,0x01,0x10,0x01,0x02,0x00,0x00,DEFAULT_ENDP0_SIZE,
                            0x86,0x1a,0x22,0x57,0x00,0x01,0x01,0x02,
                            0x03,0x01
                           };
__code uint8_t CfgDesc[] ={
    0x09,0x02,0x43,0x00,0x02,0x01,0x00,0xa0,0x32,             //Configuration descriptor (two interfaces)
// The following is the interface 0 (CDC interface) descriptor
    0x09,0x04,0x00,0x00,0x01,0x02,0x02,0x01,0x00, // CDC interface descriptor (one endpoint)
    //The following is the function descriptor
    0x05,0x24,0x00,0x10,0x01,                                 //Function descriptor (header)
    0x05,0x24,0x01,0x00,0x00,                                 //Management descriptor (no data interface) 03 01
    0x04,0x24,0x02,0x02,                                      //stand by,Set_Line_Coding、Set_Control_Line_State、Get_Line_Coding、Serial_State
    0x05,0x24,0x06,0x00,0x01,                                 //CDC interface numbered 0; data interface numbered 1
    0x07,0x05,0x81,0x03,0x08,0x00,0xFF,                       //Interrupt upload endpoint descriptor
    //The following is the interface 1 (data interface) descriptor
    0x09,0x04,0x01,0x00,0x02,0x0a,0x00,0x00,0x00,             //Data interface descriptor
    0x07,0x05,0x02,0x02,0x40,0x00,0x00,                       //Endpoint descriptor
    0x07,0x05,0x82,0x02,0x40,0x00,0x00,                       //Endpoint descriptor
};
/*字符串描述符*/
unsigned char  __code LangDes[]={0x04,0x03,0x09,0x04};           //Language descriptor
unsigned char  __code SerDes[]={                                 //Serial number string descriptor
                                                                 0x14,0x03,
                                                                 0x32,0x00,0x30,0x00,0x31,0x00,0x37,0x00,0x2D,0x00,
                                                                 0x32,0x00,0x2D,0x00,
                                                                 0x32,0x00,0x35,0x00
                               };
unsigned char  __code Prod_Des[]={                                //Product string descriptor
                                                                  0x14,0x03,
                                                                  0x43,0x00,0x48,0x00,0x35,0x00,0x35,0x00,0x34,0x00,0x5F,0x00,
                                                                  0x43,0x00,0x44,0x00,0x43,0x00,
                                 };
unsigned char  __code Manuf_Des[]={
    0x0A,0x03,
    0x5F,0x6c,0xCF,0x82,0x81,0x6c,0x52,0x60,
};

//cdc参数
__xdata uint8_t LineCoding[7]={0x00,0xe1,0x00,0x00,0x00,0x00,0x08};   //The initial baud rate is 57600, 1 stop bit, no parity, 8 data bits.

volatile __idata uint8_t USBByteCount = 0;      //Represents the data received by the USB endpoint
volatile __idata uint8_t USBBufOutPoint = 0;    //Fetch data pointer

volatile __idata uint8_t UpPoint2_Busy  = 0;   //Whether the upload endpoint is busy


/*******************************************************************************
* Function Name  : USBDeviceCfg()
* Description    : USB device mode configuration
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceCfg( void )
{
    USB_CTRL = 0x00;                                                           //Clear USB control register
    USB_CTRL &= ~bUC_HOST_MODE;                                                //This bit selects the device mode
    USB_CTRL |=  bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                    //USB device and internal pull-up enable, automatically return to NAK before interrupt flag is cleared
    USB_DEV_AD = 0x00;                                                         //Device address initialization
    //     USB_CTRL |= bUC_LOW_SPEED;
    //     UDEV_CTRL |= bUD_LOW_SPEED;                                                //Select low speed 1.5M mode
    USB_CTRL &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                             //Select full speed 12M mode, the default mode
    UDEV_CTRL = bUD_PD_DIS;  // Disable DP / DM pull-down resistor
    UDEV_CTRL |= bUD_PORT_EN;                                                  //Enable physical port
}
/*******************************************************************************
* Function Name  : USBDeviceIntCfg()
* Description    : USB device mode interrupt initialization
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceIntCfg( void )
{
    USB_INT_EN |= bUIE_SUSPEND;                                               //Enable device suspend interrupt
    USB_INT_EN |= bUIE_TRANSFER;                                              //Enable USB transfer completion interrupt
    USB_INT_EN |= bUIE_BUS_RST;                                               //Enable device mode USB bus reset interrupt
    USB_INT_FG |= 0x1F;                                                       //Clear interrupt flag
    IE_USB = 1;                                                               //Enable USB interrupt
    EA = 1;                                                                   //Allow microcontroller interrupt
}
/*******************************************************************************
* Function Name  : USBDeviceEndPointCfg()
* Description    : USB device mode endpoint configuration, simulation compatible HID device, in addition to endpoint 0 control transmission, also includes endpoint 2 batch upload
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceEndPointCfg( void )
{
    // TODO: Is casting the right thing here? What about endianness?
    UEP1_DMA = (uint16_t) Ep1Buffer;                                                      //Endpoint 1 sends data transfer address
    UEP2_DMA = (uint16_t) Ep2Buffer;                                                      //Endpoint 2 IN data transfer address
    UEP2_3_MOD = 0xCC;                                                         //Endpoint 2/3 single buffer transceiver enable
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;                 //Endpoint 2 automatically flips the synchronization flag, IN transaction returns NAK, OUT returns ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                                 //Endpoint 1 automatically flips the synchronization flag, IN transaction returns NAK
    UEP0_DMA = (uint16_t) Ep0Buffer;                                                      //Endpoint 0 data transfer address
    UEP4_1_MOD = 0X40;                                                         //Endpoint 1 upload buffer; endpoint 0 single 64-byte send and receive buffer
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //Manual flip, OUT transaction returns ACK, IN transaction returns NAK
}

/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB interrupt processing function
*******************************************************************************/
void DeviceInterrupt(void) __interrupt (INT_NO_USB)                       //USB interrupt service routine, using register set 1
{
    uint16_t len;
    if(UIF_TRANSFER)                                                            //USB transfer complete flag
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 1:                                                  //endpoint 1# 端点中断上传
            UEP1_T_LEN = 0;
            UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //默认应答NAK
            break;
        case UIS_TOKEN_IN | 2:                                                  //endpoint 2# 端点批量上传
        {
            UEP2_T_LEN = 0;                                                    //预使用发送长度一定要清空
            UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //默认应答NAK
            UpPoint2_Busy = 0;                                                  //清除忙标志
        }
            break;
        case UIS_TOKEN_OUT | 2:                                                 //endpoint 3# 端点批量下传
            if ( U_TOG_OK )                                                     // 不同步的数据包将丢弃
            {
                USBByteCount = USB_RX_LEN;                          // Grads length of recieved data
                USBBufOutPoint = 0;                                             //Get data pointer reset
                UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;       //NAK after receiving a packet of data, the main function finishes processing, and the main function modifies the response mode
            }
            break;
        case UIS_TOKEN_SETUP | 0:                                                //SETUP事务
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = ((uint16_t)UsbSetupBuf->wLengthH<<8) | (UsbSetupBuf->wLengthL);
                len = 0;                                                      // 默认为成功并且上传0长度
                SetupReq = UsbSetupBuf->bRequest;
                if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )//非标准请求
                {
                    switch( SetupReq )
                    {
                    case GET_LINE_CODING:   //0x21  currently configured
                        pDescr = LineCoding;
                        len = sizeof(LineCoding);
                        len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;  // 本次传输长度
                        memcpy(Ep0Buffer,pDescr,len);
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case SET_CONTROL_LINE_STATE:  //0x22  generates RS-232/V.24 style control signals
                        break;
                    case SET_LINE_CODING:      //0x20  Configure
                        break;
                    default:
                        len = 0xFF;  								 					                 /*Command not supported*/
                        break;
                    }
                }
                else                                                             //Standard request
                {
                    switch(SetupReq)                                             //Request code
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH)
                        {
                        case 1:                                                       //设备描述符
                            pDescr = DevDesc;                                         //把设备描述符送到要发送的缓冲区
                            len = sizeof(DevDesc);
                            break;
                        case 2:                                                        //配置描述符
                            pDescr = CfgDesc;                                          //把设备描述符送到要发送的缓冲区
                            len = sizeof(CfgDesc);
                            break;
                        case 3:
                            if(UsbSetupBuf->wValueL == 0)
                            {
                                pDescr = LangDes;
                                len = sizeof(LangDes);
                            }
                            else if(UsbSetupBuf->wValueL == 1)
                            {
                                pDescr = Manuf_Des;
                                len = sizeof(Manuf_Des);
                            }
                            else if(UsbSetupBuf->wValueL == 2)
                            {
                                pDescr = Prod_Des;
                                len = sizeof(Prod_Des);
                            }
                            else
                            {
                                pDescr = SerDes;
                                len = sizeof(SerDes);
                            }
                            break;
                        default:
                            len = 0xff;                                                //Unsupported command or error
                            break;
                        }
                        if ( SetupLen > len )
                        {
                            SetupLen = len;    //Limit total length
                        }
                        len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;                            //This transmission length
                        memcpy(Ep0Buffer,pDescr,len);                                  //Load upload data
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;                              //Temporary storage of USB device address
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 )
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        break;
                    case USB_GET_INTERFACE:
                        break;
                    case USB_CLEAR_FEATURE:                                            //Clear Feature
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == USB_REQ_RECIP_DEVICE )                  /* Remove device */
                        {
                            if( ( ( ( uint16_t )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* 唤醒 */
                                }
                                else
                                {
                                    len = 0xFF;                                        /* 操作失败 */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                            /* 操作失败 */
                            }
                        }
                        else if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// 端点
                        {
                            switch( UsbSetupBuf->wIndexL )
                            {
                            case 0x83:
                                UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x03:
                                UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x01:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF;                                         // 不支持的端点
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF;                                                // 不是端点不支持
                        }
                        break;
                    case USB_SET_FEATURE:                                          /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == USB_REQ_RECIP_DEVICE )                  /* 设置设备 */
                        {
                            if( ( ( ( uint16_t )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* 休眠 */
#ifdef DE_PRINTF
                                    printf( "suspend\n" );                                                             //睡眠状态
#endif
                                    while ( XBUS_AUX & bUART0_TX )
                                    {
                                        ;    //等待发送完成
                                    }
                                    SAFE_MOD = 0x55;
                                    SAFE_MOD = 0xAA;
                                    WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;                      //USB或者RXD0/1有信号时可被唤醒
                                    PCON |= PD;                                                                 //睡眠
                                    SAFE_MOD = 0x55;
                                    SAFE_MOD = 0xAA;
                                    WAKE_CTRL = 0x00;
                                }
                                else
                                {
                                    len = 0xFF;                                        /* 操作失败 */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                            /* 操作失败 */
                            }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == USB_REQ_RECIP_ENDP )             /* 设置端点 */
                        {
                            if( ( ( ( uint16_t )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                            {
                                switch( ( ( uint16_t )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                                {
                                case 0x83:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点3 IN STALL */
                                    break;
                                case 0x03:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点3 OUT Stall */
                                    break;
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点1 IN STALL */
                                    break;
                                case 0x01:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点1 OUT Stall */
                                default:
                                    len = 0xFF;                                    /* 操作失败 */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF;                                      /* 操作失败 */
                            }
                        }
                        else
                        {
                            len = 0xFF;                                          /* 操作失败 */
                        }
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 )
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;                                                    //操作失败
                        break;
                    }
                }
            }
            else
            {
                len = 0xff;                                                         //包长度错误
            }
            if(len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
            }
            else if(len <= DEFAULT_ENDP0_SIZE)                                                       //上传数据或者状态阶段返回0长度包
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1，返回应答ACK
            }
            else
            {
                UEP0_T_LEN = 0;  //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1,返回应答ACK
            }
            break;
        case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
            switch(SetupReq)
            {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;                                 //本次传输长度
                memcpy( Ep0Buffer, pDescr, len );                                   //加载上传数据
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;                                             //同步标志位翻转
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;                                                      //状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint0 OUT
            if(SetupReq ==SET_LINE_CODING)  //设置串口属性
            {
                if( U_TOG_OK )
                {
                    memcpy(LineCoding,UsbSetupBuf,USB_RX_LEN);
                    //Config_Uart1(LineCoding);
                    UEP0_T_LEN = 0;
                    UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;  // 准备上传0包
                }
            }
            else
            {
                UEP0_T_LEN = 0;
                UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_NAK;  //状态阶段，对IN响应NAK
            }
            break;



        default:
            break;
        }
        UIF_TRANSFER = 0;                                                           //写0清空中断
    }
    if(UIF_BUS_RST)                                                                 //设备模式USB总线复位中断
    {
#ifdef DE_PRINTF
        printf( "reset\n" );                                                             //睡眠状态
#endif
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;                                                             //清中断标志
        //Uart_Input_Point = 0;   //循环缓冲区输入指针
        //Uart_Output_Point = 0;  //循环缓冲区读出指针
        //UartByteCount = 0;      //当前缓冲区剩余待取字节数
        USBByteCount = 0;       //USB端点收到的长度
        UsbConfig = 0;          //清除配置值
        UpPoint2_Busy = 0;
    }
    if (UIF_SUSPEND)                                                                 //USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                             //挂起
        {
#ifdef DE_PRINTF
            printf( "suspend\n" );                                                             //睡眠状态
#endif
            while ( XBUS_AUX & bUART0_TX )
            {
                ;    //等待发送完成
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;                      //USB或者RXD0/1有信号时可被唤醒
            PCON |= PD;                                                                 //睡眠
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
        }
    }
    else {                                                                             //意外的中断,不可能发生的情况
        USB_INT_FG = 0xFF;                                                             //清中断标志

    }
}

void GPIOSetup( void ) {
    // Pins:
    // LED:     P3.4 Output
    // IRQ:     P3.2 Input & Interrupt
    // MOSI:    P1.5 Output
    // MISO:    P1.6 Input
    // SCK:     P1.7 Output
    // CE:      P3.1 Output
    // CSN:     P3.0 Output

    P3_DIR_PU = 0b11111011;
    P3_MOD_OC = 0b00000000;
    // Port3: 8 7 6 5 4 3 2 1 0
    // In:                0
    // Out:   1 1 1 1 1 1   1 1

    P1_DIR_PU = 0b110111111;
    P1_MOD_OC = 0b000000000;
    // Port1: 8 7 6 5 4 3 2 1 0
    // In:        0
    // Out:   1 1   1 1 1 1 1 1

    LED = 0; // LED On
    CE = 0;  // Disable TX/RX
    CSN = 1;

    mDelaymS(20);
}

void USBCDC_SendBytes(const char *buf, uint8_t len) {
    // write upload endpoint
    while (UpPoint2_Busy);
    memcpy(Ep2Buffer + MAX_PACKET_SIZE, buf, len);
    UEP2_T_LEN = len; // Pre-use send length must be cleared
    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK; // Answer ACK
    UpPoint2_Busy = 1;
}

void nRF24_Config( void ){
    nRF24_CE_L();

    nRF24_Init();
    // 关闭所有通道自动确认
    nRF24_WriteReg(nRF24_REG_EN_AA, 0x00);
    // 5 bytes 地址
    nRF24_WriteReg(nRF24_REG_SETUP_AW, 0b11);
    // 关闭自动重发
    nRF24_WriteReg(nRF24_REG_SETUP_RETR, 0x00);
    // 2.440Ghz
    nRF24_WriteReg(nRF24_REG_RF_CH, 40);
    // 250kbps, 0dBm
    nRF24_WriteReg(nRF24_REG_RF_SETUP, 0b00100011);
    // 设置发送地址
    nRF24_CSN_L();
    nRF24_LL_RW(nRF24_CMD_W_REGISTER | nRF24_REG_TX_ADDR);
    nRF24_LL_RW(0x34);
    nRF24_LL_RW(0x43);
    nRF24_LL_RW(0x10);
    nRF24_LL_RW(0x10);
    nRF24_LL_RW(0x01);
    nRF24_CSN_H();

    // 数据长度 22 bytes
    nRF24_WriteReg(nRF24_REG_RX_PW_P0, 22);

    nRF24_ClearIRQFlags();

    // 16bit CRC, 发送模式, 上电
    nRF24_WriteReg(nRF24_REG_CONFIG,
        nRF24_CONFIG_EN_CRC | nRF24_CONFIG_CRCO);

/*
    nRF24_Init();
    nRF24_DisableAA(0xFF);
    nRF24_SetRFChannel(40);
    nRF24_SetDataRate(nRF24_DR_250kbps);
    nRF24_SetCRCScheme(nRF24_CRC_2byte);
    nRF24_SetAddrWidth(5);
    __code uint8_t nRF24_ADDR0[] = {0x34, 0x43, 0x10, 0x10, 0x01};
    nRF24_SetAddr(nRF24_PIPE0, nRF24_ADDR0);
    nRF24_SetRXPipe(nRF24_PIPE0, nRF24_AA_OFF, 22);
    nRF24_SetOperationalMode(nRF24_MODE_RX);
    nRF24_SetPowerMode(nRF24_PWR_UP);
*/

    // 稍等
    mDelaymS(100);
}

//主函数
void main( void )
{
    __xdata uint32_t Uart_Timeout = 0;
    __xdata uint8_t recievedData[MAX_PACKET_SIZE] ="";
    uint8_t dataRecievedCopFlag = 0 ; 
    uint8_t buffer[32] = "Hello\r\n";
    CfgFsys( );                                                           // CH559 clock selection configuration
    mDelaymS(5);                                                          // Modify the main frequency and wait for the internal crystal to stabilize, which must be added
    mInitSTDIO( );                                                        // Serial port 0, can be used for debugging
    GPIOSetup();
    nRF24_Config();

#ifdef DE_PRINTF
    printf("start ...\n");
#endif
    //USBDeviceCfg();
    //USBDeviceEndPointCfg();                                               // Endpoint configuration
    //USBDeviceIntCfg();                                                    //Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;                                                       //Pre-use send length must be cleared
    UEP2_T_LEN = 0;                                                       //Pre-use send length must be cleared

    uint8_t status;

    while(1)
    {
        nRF24_CE_L();
        nRF24_WritePayload(buffer, 22);
        nRF24_WriteReg(nRF24_REG_STATUS, 0x7e);
        nRF24_CE_H();

        mDelaymS(20);

        nRF24_CE_L();
        nRF24_ClearIRQFlags();
        nRF24_FlushTX();

        mDelaymS(500);

        LED = ! LED;
    }
}
