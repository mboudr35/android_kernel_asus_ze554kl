/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: focaltech_test_ft8716.c
*
* Author: Software Development
*
* Created: 2016-08-01
*
* Abstract: test item for FT8716
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "../../focaltech_core.h"

#include "../include/focaltech_test_detail_threshold.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../include/focaltech_test_main.h"
#include "../focaltech_test_config.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#if (FT8716_TEST)




#define MAX_NOISE_FRAMES                32
/////////////////////////////////////////////////Reg FT8716
#define DEVIDE_MODE_ADDR                0x00
#define REG_LINE_NUM                    0x01
#define REG_TX_NUM                      0x02
#define REG_RX_NUM                      0x03
#define FT8716_LEFT_KEY_REG             0X1E
#define FT8716_RIGHT_KEY_REG            0X1F

#define REG_CbAddrH                     0x18
#define REG_CbAddrL                     0x19
#define REG_OrderAddrH                  0x1A
#define REG_OrderAddrL                  0x1B

#define REG_RawBuf0                     0x6A
#define REG_RawBuf1                     0x6B
#define REG_OrderBuf0                   0x6C
#define REG_CbBuf0                      0x6E

#define REG_K1Delay                     0x31
#define REG_K2Delay                     0x32
#define REG_SCChannelCf                 0x34
#define REG_LCD_NOISE_FRAME                   0X12
#define REG_LCD_NOISE_START                   	0X11
#define REG_LCD_NOISE_NUMBER                 0X13
#define REG_LCD_NOISE_DATA_READY         0X00
#define REG_FWVERSION       				0xA6
#define REG_FACTORYID       				0xA8
#define REG_FWCNT           				0x17
#define pre                                 1
#define REG_CLB                          0x04


/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/

enum NOISE_TYPE
{
    NT_AvgData = 0,
    NT_MaxData = 1,
    NT_MaxDevication = 2,
    NT_DifferData = 3,
};

unsigned char localbitWise;
void SetKeyBitVal( unsigned char val )
{

    localbitWise = val;

}

bool IsKeyAutoFit(void)
{
    return ( ( localbitWise & 0x0f ) == 1);
}


/*****************************************************************************
* Static variables
*****************************************************************************/

static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_CBData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static BYTE m_ucTempData[TX_NUM_MAX * RX_NUM_MAX*2] = {0};//One-dimensional
static int m_iTempRawData[TX_NUM_MAX * RX_NUM_MAX] = {0};
static unsigned char pReadBuffer[80 * 80 * 2] = {0};
static int iAdcData[TX_NUM_MAX * RX_NUM_MAX] =  {0};
static int shortRes[TX_NUM_MAX][RX_NUM_MAX] = { {0} };
static int CHX_Linearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int CHY_Linearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int LCD_Noise[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static unsigned char cbValue[TX_NUM_MAX * RX_NUM_MAX] = {0};
static int minHole[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};;
static int maxHole[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};;
static int TxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int RxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static  int tmpiUniform[TX_NUM_MAX][RX_NUM_MAX] = {{0}};





/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern struct stCfg_FT8716_BasicThreshold g_stCfg_FT8716_BasicThreshold;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/

/////////////////////////////////////////////////////////////
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelRows(unsigned char *pPanelRows);
static unsigned char GetPanelCols(unsigned char *pPanelCols);
static unsigned char GetTxRxCB(unsigned short StartNodeNo, unsigned short ReadNum, unsigned char *pReadBuffer);
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
static unsigned char WeakShort_GetAdcData( int AllAdcDataLen, int *pRevBuffer  );
//static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static unsigned char ChipClb(unsigned char *pClbResult);
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int RowArrayIndex, int ColArrayIndex,unsigned char Row, unsigned char Col, unsigned char ItemCount);
int FT8716_TestItem_visual_key(bool *bTestResult);
int FT8716_TestItem_rf_visual_key(bool *bTestResult);
static unsigned char SetTxRxCB( unsigned char* pWriteBuffer, int writeNum );



boolean FT8716_StartTest(void);
int FT8716_get_test_data(char *pTestData);//pTestData, External application for memory, buff size >= 1024*80

unsigned char FT8716_TestItem_EnterFactoryMode(void);
unsigned char FT8716_TestItem_RawDataTest(bool * bTestResult);
unsigned char FT8716_TestItem_CbTest(bool* bTestResult);
unsigned char FT8716_TestItem_ChannelsTest(bool * bTestResult);
unsigned char FT8716_TestItem_ShortCircuitTest(bool* bTestResult);      //add by LiuWeiGuang
unsigned char FT8716_TestItem_OpenTest(bool* bTestResult);                   //add by LiuWeiGuang
unsigned char FT8716_TestItem_CbUniformityTest(bool* bTestResult);    //add by LiuWeiGuang
unsigned char FT8716_TestItem_LCDNoiseTest(bool* bTestResult);             //add by LiuWeiGuang
unsigned char FT8716_CheckItem_FwVersionTest(bool* bTestResult);         //add by LiuWeiGuang
unsigned char FT8716_CheckItem_FactoryIdTest(bool* bTestResult);             //add by LiuWeiGuang
unsigned char FT8716_CheckItem_ProjectCodeTest(bool* bTestResult);          //add by LiuWeiGuang
unsigned char FT8716_TestItem_RawDataUniformityTest(bool* bTestResult);   //add by LiuWeiGuang


/************************************************************************
* Name: FT8716_StartTest
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/

boolean FT8716_StartTest()
{
    bool bTestResult = true, bTempResult = 1;
    unsigned char ReCode;
    unsigned char ucDevice = 0;
    int iItemCount=0;


    //--------------1. Init part
    if (InitTest() < 0)
    {
        FTS_TEST_DBG("[focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == g_TestItemNum)
        bTestResult = false;

    ////////Testing process, the order of the g_stTestItem structure of the test items
    for (iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
    {
        m_ucTestItemCode = g_stTestItem[ucDevice][iItemCount].ItemCode;
        if(Code_FT8716_VIRTUAL_KEY_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_TestItem_visual_key(&bTempResult); 
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }
		if(Code_FT8716_RF_VK_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_TestItem_rf_visual_key(&bTempResult); 
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
            }
            else{
				bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
        }

        ///////////////////////////////////////////////////////FT8716_ENTER_FACTORY_MODE
        if (Code_FT8716_ENTER_FACTORY_MODE == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT8716_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_RAWDATA_TEST
        if (Code_FT8716_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT8716_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_CB_TEST
        if (Code_FT8716_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT8716_TestItem_CbTest(&bTempResult); //
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_CHANNEL_NUM_TEST
        if (Code_FT8716_CHANNEL_NUM_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT8716_TestItem_ChannelsTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;

        }

        ///////////////////////////////////////////////////////FT8716_ShortCircuit_TEST
        if (Code_FT8716_SHORT_CIRCUIT_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_TestItem_ShortCircuitTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_Open_TEST
        if (Code_FT8716_OPEN_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_TestItem_OpenTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_CB_UNIFORMITY_TEST
        if (Code_FT8716_CB_UNIFORMITY_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_TestItem_CbUniformityTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT8716_LCDNoise_TEST
        if (Code_FT8716_LCD_NOISE_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_TestItem_LCDNoiseTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_FW_VERSION_TEST
        if (Code_FT8716_FW_VERSION_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_CheckItem_FwVersionTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_FACTORY_ID_TEST
        if (Code_FT8716_FACTORY_ID_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_CheckItem_FactoryIdTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8716_PROJECT_CODE_TEST
        if (Code_FT8716_PROJECT_CODE_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_CheckItem_ProjectCodeTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

		 ///////////////////////////////////////////////////////FT8716_RAWDATA_UNIFORMITY_TEST
        if (Code_FT8716_RAWDATA_UNIFORMITY_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT8716_TestItem_RawDataUniformityTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }


    }

    //--------------3. End Part
    FinishTest();

    //--------------4. return result
    return bTestResult;

}

/************************************************************************
* Name: FT8716_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_EnterFactoryMode(void)
{

    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;  //If failed, repeat 5 times.
    int i ;
    unsigned char keyFit = 0;
    SysDelay(150);
    FTS_TEST_DBG("Enter factory mode...");
    for (i = 1; i <= iRedo; i++)
    {
        ReCode = EnterFactory();
        if (ERROR_CODE_OK != ReCode)
        {
            FTS_TEST_DBG("Failed to Enter factory mode...");
            if (i < iRedo)
            {
                SysDelay(50);
                continue;
            }
        }
        else
        {
            FTS_TEST_DBG(" success to Enter factory mode...");
            break;
        }

    }
    SysDelay(300);

    if (ReCode == ERROR_CODE_OK) //After the success of the factory model, read the number of channels
    {
        ReCode = GetChannelNum();

        ReCode = ReadReg( 0xFC, &keyFit );
        SetKeyBitVal( keyFit );
    }
    return ReCode;
}

/************************************************************************
* Name: FT8716_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: bTestResult
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_RawDataTest(bool * bTestResult)
{
    unsigned char ReCode;
    bool btmpresult = true;
    int RawDataMin;
    int RawDataMax;
    int iValue = 0;
    int i=0;
    int iRow, iCol;
    bool bIncludeKey = false;



    FTS_TEST_DBG("\n\n==============================Test Item: -------- Raw Data Test\n");

    bIncludeKey = g_stCfg_FT8716_BasicThreshold.bRawDataTest_VKey_Check;

    //----------------------------------------------------------Read RawData
    for (i = 0 ; i < 3; i++) //Lost 3 Frames, In order to obtain stable data
    {
        ReCode = WriteReg( 0x06, 0x00 );
        SysDelay( 10 );
        ReCode = GetRawData();
    }

    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_DBG("Failed to get Raw Data!! Error Code: %d",  ReCode);
        return ReCode;
    }
    //----------------------------------------------------------Show RawData


    FTS_TEST_PRINT("\nVA Channels: ");
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {
        FTS_TEST_PRINT("\nCh_%02d:  ", iRow+1);
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            FTS_TEST_PRINT("%5d, ", m_RawData[iRow][iCol]);
        }
    }

    FTS_TEST_PRINT("\nKeys:  ");
    if (IsKeyAutoFit() )
    {
        for ( iCol = 0; iCol <g_stSCapConfEx.KeyNum; iCol++ )
            FTS_TEST_PRINT("%5d, ",  m_RawData[g_stSCapConfEx.ChannelXNum][iCol]);
    }
    else
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
        {
            FTS_TEST_PRINT("%5d, ",  m_RawData[g_stSCapConfEx.ChannelXNum][iCol]);
        }
    }



    //----------------------------------------------------------To Determine RawData if in Range or not
    //   VA
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {

        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
            RawDataMin = g_stCfg_Incell_DetailThreshold.RawDataTest_Min[iRow][iCol];
            RawDataMax = g_stCfg_Incell_DetailThreshold.RawDataTest_Max[iRow][iCol];
            iValue = m_RawData[iRow][iCol];
            //FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
            if (iValue < RawDataMin || iValue > RawDataMax)
            {
                btmpresult = false;
                FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                             iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
            }
        }
    }

    // Key
    if (bIncludeKey)
    {

        iRow = g_stSCapConfEx.ChannelXNum;
        if ( IsKeyAutoFit() )
        {
            for ( iCol = 0; iCol <g_stSCapConfEx.KeyNum; iCol++)
            {
                if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                RawDataMin = g_stCfg_Incell_DetailThreshold.RawDataTest_Min[iRow][iCol];
                RawDataMax = g_stCfg_Incell_DetailThreshold.RawDataTest_Max[iRow][iCol];
                iValue = m_RawData[iRow][iCol];
                FTS_TEST_DBG("[touch]vk Node1=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                if (iValue < RawDataMin || iValue > RawDataMax)
                {
                    btmpresult = false;
                    FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                 iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                }
            }
        }
        else
        {
            for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
            {
                if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                RawDataMin = g_stCfg_Incell_DetailThreshold.RawDataTest_Min[iRow][iCol];
                RawDataMax = g_stCfg_Incell_DetailThreshold.RawDataTest_Max[iRow][iCol];
                iValue = m_RawData[iRow][iCol];
                //FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                if (iValue < RawDataMin || iValue > RawDataMax)
                {
                    btmpresult = false;
                    FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                 iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                }
            }
        }
    }

    //////////////////////////////Save Test Data
    Save_Test_Data(m_RawData, 0, 0, g_stSCapConfEx.ChannelXNum+1, g_stSCapConfEx.ChannelYNum, 1);
    //----------------------------------------------------------Return Result

    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));


    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//RawData Test is NG!");
    }
    return ReCode;
}


/************************************************************************
* Name: FT8716_TestItem_CbTest
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_CbTest(bool* bTestResult)
{
    bool btmpresult = true;
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int iCol = 0;
    int iMaxValue = 0;
    int iMinValue = 0;
    int iValue = 0;
    bool bIncludeKey = false;

    unsigned char bClbResult = 0;
    unsigned char ucBits = 0;
    int ReadKeyLen = g_stSCapConfEx.KeyNumTotal;

    bIncludeKey = g_stCfg_FT8716_BasicThreshold.bCBTest_VKey_Check;

    FTS_TEST_DBG("\n\n==============================Test Item: --------  CB Test\n");


    ReCode = ChipClb( &bClbResult );
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n//========= auto clb Failed!");
    }

    ReCode = ReadReg(0x0B, &ucBits);
    if (ERROR_CODE_OK != ReCode)
    {
		btmpresult = false;
		FTS_TEST_DBG("\r\n//=========  Read Reg Failed!");
    }

    ReadKeyLen = g_stSCapConfEx.KeyNumTotal;
    if (ucBits != 0)
    {
        ReadKeyLen = g_stSCapConfEx.KeyNumTotal * 2;
    }



    ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum  + ReadKeyLen), m_ucTempData );
    if ( ERROR_CODE_OK != ReCode )
    {
        btmpresult = false;
        FTS_TEST_DBG("Failed to get CB value...");
        goto TEST_ERR;
    }

    memset(m_CBData, 0, sizeof(m_CBData));
    ///VA area
    for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
        }
    }

    // Key
    for ( iCol = 0; iCol < ReadKeyLen/*g_stSCapConfEx.KeyNumTotal*/; ++iCol )
    {
        if (ucBits != 0)
        {
            m_CBData[g_stSCapConfEx.ChannelXNum][iCol/2] = (short)((m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ] & 0x01 )<<8) + m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol + 1 ];
            iCol++;
        }
        else
        {
            m_CBData[g_stSCapConfEx.ChannelXNum][iCol] = m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ];
        }

    }



    //------------------------------------------------Show CbData


    FTS_TEST_PRINT("\nVA Channels: ");
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {
        FTS_TEST_PRINT("\nCh_%02d:  ", iRow+1);
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            FTS_TEST_PRINT("%3d, ", m_CBData[iRow][iCol]);
        }
    }
    FTS_TEST_PRINT("\nKeys:  ");
    if ( IsKeyAutoFit() )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; iCol++ )
        {
            FTS_TEST_PRINT("%3d, ",  m_CBData[g_stSCapConfEx.ChannelXNum][iCol]);

        }

    }
    else
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
        {
            FTS_TEST_PRINT("%3d, ",  m_CBData[g_stSCapConfEx.ChannelXNum][iCol]);
        }
    }


    // VA
    for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; iRow++)
    {
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            if ( (0 == g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol]) )
            {
                continue;
            }
            iMinValue = g_stCfg_Incell_DetailThreshold.CBTest_Min[iRow][iCol];
            iMaxValue = g_stCfg_Incell_DetailThreshold.CBTest_Max[iRow][iCol];
            iValue = focal_abs(m_CBData[iRow][iCol]);
            //FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
            if ( iValue < iMinValue || iValue > iMaxValue)
            {
                btmpresult = false;
                FTS_TEST_DBG("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                             iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
            }
        }
    }

    // Key
    if (bIncludeKey)
    {

        iRow = g_stSCapConfEx.ChannelXNum;
        if ( IsKeyAutoFit() )
        {
            for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; iCol++)
            {
                if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)
                {
                    continue; //Invalid Node
                }
                iMinValue = g_stCfg_Incell_DetailThreshold.CBTest_Min[iRow][iCol];
                iMaxValue = g_stCfg_Incell_DetailThreshold.CBTest_Max[iRow][iCol];
                iValue = focal_abs(m_CBData[iRow][iCol]);
                // FTS_TEST_DBG("Node1=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
                if (iValue < iMinValue || iValue > iMaxValue)
                {
                    btmpresult = false;
                            FTS_TEST_DBG("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                 iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
                }
            }
        }
        else
        {
            for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
            {
                if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                iMinValue = g_stCfg_Incell_DetailThreshold.CBTest_Min[iRow][iCol];
                iMaxValue = g_stCfg_Incell_DetailThreshold.CBTest_Max[iRow][iCol];
                iValue = focal_abs(m_CBData[iRow][iCol]);
                // FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
                if (iValue < iMinValue || iValue > iMaxValue)
                {
                    btmpresult = false;
                    FTS_TEST_DBG("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                 iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
                }
            }
        }
    }

    //////////////////////////////Save Test Data
    Save_Test_Data(m_CBData, 0, 0, g_stSCapConfEx.ChannelXNum+1, g_stSCapConfEx.ChannelYNum, 1);

    TestResultLen += sprintf(TestResult+TestResultLen,"CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));


    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//CB Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//CB Test is NG!");
    }

    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_DBG("\n\n//CB Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen,"CB Test is NG. \n\n");

    return ReCode;
}


/************************************************************************
* Name: FT8716_TestItem_ChannelsTest
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_ChannelsTest(bool * bTestResult)
{
    unsigned char ReCode;

    FTS_TEST_DBG("\n\n==============================Test Item: -------- Channel Test ");

    ReCode = GetChannelNum();
    if (ReCode == ERROR_CODE_OK)
    {
        if ((g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelXNum == g_stSCapConfEx.ChannelXNum)
            && (g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelYNum == g_stSCapConfEx.ChannelYNum)
            && (g_stCfg_FT8716_BasicThreshold.ChannelNumTest_KeyNum == g_stSCapConfEx.KeyNum))
        {
            * bTestResult = true;
            FTS_TEST_DBG("\n\nGet channels: (CHx: %d, CHy: %d, Key: %d), Set channels: (CHx: %d, CHy: %d, Key: %d)",
                         g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum,
                         g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelXNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelYNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_KeyNum);

            pr_err("\n//Channel Test is OK!");
        }
        else
        {
            * bTestResult = false;
            pr_err("\n\nGet channels: (CHx: %d, CHy: %d, Key: %d), Set channels: (CHx: %d, CHy: %d, Key: %d)",
                         g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum,
                         g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelXNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelYNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_KeyNum);

            pr_err("\n//Channel Test is NG!");
        }
    }


    TestResultLen += sprintf(TestResult+TestResultLen,"Channel Test is %s. \n\n", (* bTestResult ? "OK" : "NG"));

    return ReCode;
}


/************************************************************************
* Name: FT8716_TestItem_ShortCircuitTest
* Brief:  Get short circuit test mode data, judge whether there is a short circuit
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_ShortCircuitTest(bool* bTestResult)
{

    unsigned char ReCode = ERROR_CODE_OK;
    bool bTempResult=true;
    int VAResMin = 0;
    int KeyResMin = 0;
    bool iIncludeKey = false;
    unsigned char iTxNum = 0, iRxNum = 0, iChannelNum = 0;
    int iAllAdcDataNum = 0;
    int iRow = 0;
    int iCol = 0;
    int tmpAdc = 0;
    int iValueMax = 0;
    int iValue = 0;
	//int i=0;
    FTS_TEST_DBG("\r\n\r\n==============================Test Item: -------- Short Circuit Test \r\n");

    ReCode = EnterFactory();          
    if (ERROR_CODE_OK != ReCode)
    {
        bTempResult = false;
        FTS_TEST_DBG("\r\n\r\n// Failed to Enter factory mode.");
        goto TEST_ERR;
    }

    VAResMin = g_stCfg_FT8716_BasicThreshold.ShortCircuit_VA_ResMin;
    KeyResMin = g_stCfg_FT8716_BasicThreshold.ShortCircuit_Key_ResMin;
    iIncludeKey =  g_stCfg_FT8716_BasicThreshold.bShortCircuit_VKey_Check;

    ReCode = ReadReg(0x02, &iTxNum);
    ReCode = ReadReg(0x03, &iRxNum);
    if (ERROR_CODE_OK != ReCode)
    {
        bTempResult = false;
        FTS_TEST_DBG("\r\n\r\n// Failed to read reg.");
        goto TEST_ERR;
    }

    iChannelNum = iTxNum + iRxNum;
    iAllAdcDataNum = iTxNum * iRxNum + g_stSCapConfEx.KeyNumTotal;
    memset(iAdcData, 0, sizeof(iAdcData));

    ReCode = WeakShort_GetAdcData(iAllAdcDataNum*2, iAdcData);
    SysDelay(50);            
    if (ERROR_CODE_OK != ReCode)
    {
        bTempResult = false;
        FTS_TEST_DBG(" // Failed to get AdcData. Error Code: %d", ReCode);
        goto TEST_ERR;
    }


    //show ADCData
#if 0
    FTS_TEST_DBG("ADCData:\n");
    for (i=0; i<iAllAdcDataNum; i++)
    {
        FTS_TEST_DBG("%-4d  ",iAdcData[i]);
        if (0 == (i+1)%iRxNum)
        {
            FTS_TEST_DBG("\n");
        }
    }
    FTS_TEST_DBG("\n");
#endif


  //  FTS_TEST_DBG("shortRes data:\n");
    for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum + 1; ++iRow )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            tmpAdc = iAdcData[iRow *iRxNum + iCol];
            if (2047 == tmpAdc)  tmpAdc = 2046;
            shortRes[iRow][iCol] = (tmpAdc * 100) / (2047 - tmpAdc);

        //    FTS_TEST_DBG("%-4d  ", shortRes[iRow][iCol]);
        }
     //   FTS_TEST_DBG(" \n");
    }
 //   FTS_TEST_DBG(" \n");

    //////////////////////// analyze
    iValueMax = 100000000;      
     FTS_TEST_DBG(" Short Circuit test , VA_Set_Range=(%d, %d), Key_Set_Range=(%d, %d) \n", \
                 VAResMin, iValueMax, KeyResMin, iValueMax );
     // VA
   	    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
	    {
	        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
	        {
	            if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)
		     {
		     		continue; //Invalid Node	
		     }		    
	            iValue = shortRes[iRow][iCol];
	            if (iValue < VAResMin || iValue > iValueMax)
	            {
	                bTempResult = false;
	                FTS_TEST_DBG(" Short Circuit test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d). \n", \
	                               iRow+1, iCol+1, iValue, VAResMin, iValueMax);
	            }
	        }
	    } 
	    // Key
	    if (iIncludeKey)
	    {

	        iRow = g_stSCapConfEx.ChannelXNum;
	            for ( iCol = 0; iCol <(IsKeyAutoFit() ? g_stSCapConfEx.KeyNum : g_stSCapConfEx.KeyNumTotal); iCol++)
	            {
	                if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
	                iValue = shortRes[iRow][iCol];
					/*pr_err("[touch]key raw data test  Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d). \n", \
	                               iRow+1, iCol+1, iValue, VAResMin, iValueMax);*/
	                if (iValue < KeyResMin || iValue > iValueMax)
	                {
	                    bTempResult = false;
	                    FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
	                                   iRow+1, iCol+1, iValue, KeyResMin, iValueMax);
	                }
	            }
	      }

	

    TestResultLen += sprintf(TestResult+TestResultLen," ShortCircuit Test is %s. \n\n", (bTempResult  ? "OK" : "NG"));

    if (bTempResult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//ShortCircuit Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//ShortCircuit Test is NG!");
    }

    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_DBG("\n\n//ShortCircuit Test is NG!");


    TestResultLen += sprintf(TestResult+TestResultLen," ShortCircuit Test is NG. \n\n");

    return ReCode;

}


/************************************************************************
* Name: FT8716_TestItem_OpenTest
* Brief:  Check if channel is open
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_OpenTest(bool* bTestResult)
{
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    unsigned char chValue=0xff;
    unsigned char chK1Value=0xff,chK2Value=0xff;
    unsigned char bClbResult = 0;
    int iMin = 0;
    int iMax = 0;
    int iRow = 0;
    int iCol = 0;
    int iValue = 0;

    FTS_TEST_DBG("\r\n\r\n==============================Test Item: --------  Open Test");

    ReCode = EnterFactory();      
    SysDelay(50);                      

    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n//=========  Enter Factory Failed!");
        goto TEST_ERR;
    }

    // set GIP to VGHO2/VGLO2 in factory mode (0x20 register write 2)
    ReCode = ReadReg(0x20, &chValue);
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n//=========  Read Reg Failed!");
        goto TEST_ERR;
    }

    ReCode = WriteReg(0x20, 0x02);
    SysDelay(50);                     
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n//=========  Write Reg Failed!");
        goto TEST_ERR;
    }

    //K1 cycle(0x31)
    if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K1)
    {
        ReCode = ReadReg(0x31, &chK1Value);
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            FTS_TEST_DBG("\r\n//=========  Read Reg Failed!");
            goto TEST_ERR;
        }
        ReCode = WriteReg(0x31, g_stCfg_FT8716_BasicThreshold.OpenTest_K1Threshold);
        SysDelay(50);                 
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            FTS_TEST_DBG("\r\n//=========  Write Reg Failed!");
            goto TEST_ERR;
        }
    }

    //K2 cycle(0x32)
    if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K2)
    {
        ReCode = ReadReg(0x32, &chK2Value);
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            FTS_TEST_DBG("\r\n//=========  Read Reg Failed!");
            goto TEST_ERR;
        }
        ReCode = WriteReg(0x32, g_stCfg_FT8716_BasicThreshold.OpenTest_K2Threshold);
        SysDelay(50);                 
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            FTS_TEST_DBG("\r\n//=========  Write Reg Failed!");
            goto TEST_ERR;
        }
    }


    ReCode = ChipClb(&bClbResult);
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n//========= auto clb Failed!");
        goto TEST_ERR;
    }

    ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNumTotal), m_ucTempData );
    if ( ERROR_CODE_OK != ReCode )
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n\r\n//=========get CB Failed!");
        goto TEST_ERR;
    }

    memset(m_CBData,0,sizeof(m_CBData));
    for (  iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
        }
    }


    FTS_TEST_DBG( "\r\n=========Check Min/Max \r\n" );
    iMin = g_stCfg_FT8716_BasicThreshold.OpenTest_CBMin;
    iMax = 200;

    for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
    {
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            if ( 0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] )
            {
                continue;
            }
            if ( 2 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] )
            {
                continue;
            }

            iValue =  m_CBData[iRow][iCol];
            if ( iValue < iMin || iValue > iMax)
            {
                btmpresult = false;
                FTS_TEST_DBG(" Open test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d). \n", \
                               iRow+1, iCol+1, iValue, iMin, iMax);
            }
        }
    }

    Save_Test_Data(m_CBData, 0, 0,  g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum, 1);




    // restore the register value of 0x20
    ReCode = WriteReg(0x20, chValue);
    SysDelay( 10);                      
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n//=========  Write Reg Failed!");
        goto TEST_ERR;
    }

    if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K1)
    {
        ReCode = WriteReg(0x31, chK1Value);
        SysDelay(10);                     
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            FTS_TEST_DBG("\r\n//=========  Write Reg Failed!");
        }
    }
    if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K2)
    {
        ReCode = WriteReg(0x32, chK2Value);
        SysDelay(10);                          
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            FTS_TEST_DBG("\r\n//=========  Write Reg Failed!");
        }
    }

    ReCode = ChipClb(&bClbResult);
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_DBG("\r\n//========= auto clb Failed!");
        goto TEST_ERR;
    }

    TestResultLen += sprintf(TestResult+TestResultLen," Open Test is %s. \n\n", (btmpresult  ? "OK" : "NG"));


    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//Open Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//Open Test is NG!");
    }

    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_DBG("\n\n//Open Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen," Open Test is NG. \n\n");

    return ReCode;
}


/************************************************************************
* Name: FT8716_TestItem_CbUniformityTest
* Brief:  CB uniformity test
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_CbUniformityTest(bool* bTestResult)
{

    bool btmpresult = true;
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int  iCol = 0;
    int iMin = 65535;
    int iMax = -65535;
    int iUniform = 0;
    int iDeviation = 0;
  
   
    


    FTS_TEST_DBG("\r\n\r\n==============================Test Item: --------  CB Uniformity Test");



    ReCode = EnterFactory();                //liu
    if (ReCode != ERROR_CODE_OK)
    {
        btmpresult = false;
        FTS_TEST_DBG("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    SysDelay(50);                                       //liu

    ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNumTotal), m_ucTempData );

    if ( ERROR_CODE_OK != ReCode )
    {
        btmpresult = false;
        FTS_TEST_DBG("\n\n// Failed to get Cb value. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

//  VA
    memset(m_CBData,0,sizeof(m_CBData));

    for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
        }
    }

//  Key
    memset( m_CBData[g_stSCapConfEx.ChannelXNum], 0, sizeof(int)*RX_NUM_MAX );

    for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; ++iCol )
    {
        m_CBData[g_stSCapConfEx.ChannelXNum][iCol] = m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ];
    }

    if ( g_stCfg_FT8716_BasicThreshold.CBUniformityTest_Check_MinMax )
    {

        FTS_TEST_DBG( "\r\n=========Check Min/Max \r\n" );

        for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                if ( 0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] )
                {
                    continue;
                }
                if ( 2 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] )
                {
                    continue;
                }
                if (iMin > m_CBData[iRow][iCol] ) iMin = m_CBData[iRow][iCol];
                if (iMax < m_CBData[iRow][iCol]) iMax =  m_CBData[iRow][iCol];
            }
        }

        FTS_TEST_DBG("\nVA Channels: ");
        for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
        {
            FTS_TEST_DBG("\nCh_%02d:  ", iRow+1);
            for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
            {
                FTS_TEST_DBG("%3d, ", m_CBData[iRow][iCol]);
            }
        }

        FTS_TEST_DBG("\nKeys:  ");
        for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
        {
            FTS_TEST_DBG("%3d, ",  m_CBData[g_stSCapConfEx.ChannelXNum][iCol]);
        }

        iMax = !iMax ? 1 : iMax;
        iUniform = 100 * focal_abs(iMin) / focal_abs(iMax);

        FTS_TEST_DBG("\r\n Min: %d, Max: %d, , Get Value of Min/Max: %d", iMin, iMax, iUniform );


        if ( iUniform < g_stCfg_FT8716_BasicThreshold.CBUniformityTest_MinMax_Hole )
        {
            btmpresult = false;
            FTS_TEST_DBG("\r\n MinMax Out Of Range, Set Value: %d", g_stCfg_FT8716_BasicThreshold.CBUniformityTest_MinMax_Hole  );
        }
        FTS_TEST_DBG("iUniform:%d,",iUniform);
	
	 tmpiUniform[0][0] = iUniform;
	
	 Save_Test_Data(tmpiUniform, 0, 0, 1, 1, 3);
	

    }

    if (g_stCfg_FT8716_BasicThreshold.CBUniformityTest_Check_CHX)
    {
        FTS_TEST_DBG( "\r\n=========Check CHX Linearity \r\n" );


        for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            for ( iCol = 1; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0 )   continue;
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol-1] == 0 ) continue;
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 2 )   continue;
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol-1] == 2 ) continue;

                iDeviation = focal_abs( m_CBData[iRow][iCol] - m_CBData[iRow][iCol-1] );
                iMax = ( (focal_abs(m_CBData[iRow][iCol])) > (focal_abs(m_CBData[iRow][iCol-1])) ? (focal_abs(m_CBData[iRow][iCol])) : (focal_abs(m_CBData[iRow][iCol-1])));
                iMax = iMax ? iMax : 1;
                CHX_Linearity[iRow][iCol] = 100 * iDeviation / iMax;
            }
        }

        for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            FTS_TEST_DBG("\nCh_%02d:  ", iRow+1);
            FTS_TEST_DBG("   /   ");
            for (iCol = 1; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                FTS_TEST_DBG("%3d, ", CHX_Linearity[iRow][iCol]);
            }
        }

        for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            for (iCol = 1; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0 )   continue;
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol-1] == 0 ) continue;
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 2 )   continue;
                if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol-1] == 2 ) continue;

                if (CHX_Linearity[iRow][iCol]<MIN_HOLE_LEVEL ||
                    CHX_Linearity[iRow][iCol]>g_stCfg_FT8716_BasicThreshold.CBUniformityTest_CHX_Hole)
                {
                    FTS_TEST_DBG("CHX Linearity Out Of Range, TX=%d, RX=%d, CHX_Linearity=%d, CHX_Hole=%d.\n",  iCol+1, iRow+1, CHX_Linearity[iRow][iCol], g_stCfg_FT8716_BasicThreshold.CBUniformityTest_CHX_Hole);

                    btmpresult = false;
                }
            }
        }

        Save_Test_Data(CHX_Linearity,  0, 1, g_stSCapConfEx.ChannelXNum,  g_stSCapConfEx.ChannelYNum-1, 1);

    }

    if (g_stCfg_FT8716_BasicThreshold.CBUniformityTest_Check_CHY)
    {
        FTS_TEST_DBG( "\r\n=========Check CHY Linearity \r\n" );


        for ( iRow = 1; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue;
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow-1][iCol] == 0)continue;
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 2)continue; //Invalid node does not test
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow-1][iCol] == 2)continue;

                iDeviation = focal_abs( m_CBData[iRow][iCol] - m_CBData[iRow-1][iCol] );
                iMax = ( (focal_abs(m_CBData[iRow][iCol])) > (focal_abs(m_CBData[iRow-1][iCol])) ? (focal_abs(m_CBData[iRow][iCol])) : (focal_abs(m_CBData[iRow-1][iCol])));
                iMax = iMax ? iMax : 1;
                CHY_Linearity[iRow][iCol] = 100 * iDeviation / iMax;
            }
        }

        FTS_TEST_DBG("\nCh_1:   ");
        for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow)
        {
            FTS_TEST_DBG("    /    ");
        }

        for (iRow = 1; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            FTS_TEST_DBG("\nCh_%02d:  ", iRow+1);

            for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {

                FTS_TEST_DBG("%3d, ", CHX_Linearity[iRow][iCol]);
            }
        }

        for (iRow = 1; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue;
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow-1][iCol] == 0)continue;
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 2)continue; //Invalid node does not test
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow-1][iCol] == 2)continue;

                if (CHY_Linearity[iRow][iCol]<MIN_HOLE_LEVEL ||
                    CHY_Linearity[iRow][iCol]>g_stCfg_FT8716_BasicThreshold.CBUniformityTest_CHY_Hole)
                {
                    FTS_TEST_DBG("CHY Linearity Out Of Range, TX=%d, RX=%d, CHY_Linearity=%d, CHY_Hole=%d.\n",  iCol+1, iRow+1, CHY_Linearity[iRow][iCol], g_stCfg_FT8716_BasicThreshold.CBUniformityTest_CHX_Hole);

                    btmpresult = false;
                }
            }
        }

        Save_Test_Data(CHY_Linearity,  1, 0, g_stSCapConfEx.ChannelXNum-1,  g_stSCapConfEx.ChannelYNum, 2);

    }

    TestResultLen += sprintf(TestResult+TestResultLen," CbUniformity Test is %s. \n\n", (btmpresult  ? "OK" : "NG"));

    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//CbUniformity Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//CbUniformity Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_DBG("\n\n//CbUniformity Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen," CbUniformity Test is NG. \n\n");

    return ReCode;

}


/************************************************************************
* Name: FT8716_TestItem_LCDNoiseTest
* Brief:   obtain is differ mode  the data and calculate the corresponding type of noise value.
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_LCDNoiseTest(bool* bTestResult)
{


    unsigned char ReCode = ERROR_CODE_OK;
    bool bResultFlag = true;
    int FrameNum = 0;
    int i = 0;
    int iRow = 0;
    int iCol = 0;
    int iValueMin = 0;
    int iValueMax = 0;
    int iValue = 0;
    int ikey = 0;
    unsigned char regData = 0, oldMode = 0,chNewMod = 0,DataReady = 0;
    unsigned char chNoiseValueVa = 0xff, chNoiseValueKey = 0xff;

    FTS_TEST_DBG("\r\n\r\n==============================Test Item: -------- LCD Noise Test \r\n");

    ReCode =  ReadReg( 0x06, &oldMode);
    ReCode =  WriteReg( 0x06, 0x01 );

    //switch is differ mode and wait 100ms
    SysDelay( 50);     

    ReCode = ReadReg( 0x06, &chNewMod );

    if ( ReCode != ERROR_CODE_OK || chNewMod != 1 )
    {
        bResultFlag = false;
        FTS_TEST_DBG( "\r\nSwitch Mode Failed!\r\n" );
        goto TEST_ERR;
    }

    FrameNum = g_stCfg_FT8716_BasicThreshold.LCDNoiseTest_FrameNum/4;
    ReCode = WriteReg( REG_LCD_NOISE_FRAME, FrameNum );

    // ReCode = WriteReg( 0x9B, 0 );

    //Set the parameters after the delay for a period of time waiting for FW to prepare
    SysDelay(50);
    if (ReCode != ERROR_CODE_OK)
    {
        bResultFlag = false;
        FTS_TEST_DBG( "\r\nWrite Reg Failed!\r\n" );
        goto TEST_ERR;
    }


    ReCode = WriteReg( REG_LCD_NOISE_START, 0x01 );

    for ( i=0; i<100; i++)
    {
        SysDelay( 50);     

        ReCode = ReadReg( REG_LCD_NOISE_DATA_READY, &DataReady );   
        if ( 0x00 == (DataReady>>7) )
        {
            SysDelay( 5 );
            ReCode = ReadReg( REG_LCD_NOISE_START, &DataReady );   
            if ( DataReady == 0x00 )
            {
                break;
            }
            else
            {
                continue;
            }
        }
        else
        {
             continue;
        }


        if ( 99 == i )
        {
            ReCode = WriteReg( REG_LCD_NOISE_START, 0x00 );
            if (ReCode != ERROR_CODE_OK)
            {
                bResultFlag = false;
                FTS_TEST_DBG( "\r\nRestore Failed!\r\n" );
                goto TEST_ERR;
            }

            bResultFlag = false;
            FTS_TEST_DBG( "\r\nTime Over!\r\n" );
            goto TEST_ERR;
        }
    }


    memset(m_RawData, 0, sizeof(m_RawData));

    //--------------------------------------------Read RawData
    ReCode = ReadRawData( 0, 0xAD, g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum * 2, m_iTempRawData );
    for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow * g_stSCapConfEx.ChannelYNum + iCol];
        }
    }


    ReCode = ReadRawData( 0, 0xAE, g_stSCapConfEx.KeyNumTotal * 2, m_iTempRawData );
    for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; ++iCol )
    {
        m_RawData[g_stSCapConfEx.ChannelXNum][iCol] = m_iTempRawData[iCol];
    }

    //  ReCode = WriteReg( 0x9B, 4 );           //zi   //song 9b 8
    ReCode = WriteReg( REG_LCD_NOISE_START, 0x00 );
    if (ReCode != ERROR_CODE_OK)
    {
        bResultFlag = false;
        FTS_TEST_DBG( "\r\nRestore Failed!\r\n" );
        goto TEST_ERR;
    }
    ReCode = ReadReg( REG_LCD_NOISE_NUMBER, &regData );
    if ( regData <= 0 )
    {
        regData = 1;
    }

    ReCode = WriteReg( 0x06, oldMode );
    if (ReCode != ERROR_CODE_OK)
    {
        bResultFlag = false;
        FTS_TEST_DBG( "\r\nWrite Reg Failed!\r\n" );
        goto TEST_ERR;
    }

    if (0 == g_stCfg_FT8716_BasicThreshold.LCDNoiseTest_NoiseMode)
    {
        for (  iRow = 0; iRow <= g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                LCD_Noise[iRow][iCol] = m_RawData[iRow][iCol];
            }
        }
    }

    if (1 == g_stCfg_FT8716_BasicThreshold.LCDNoiseTest_NoiseMode)
    {
        for ( iRow = 0; iRow <= g_stSCapConfEx.ChannelXNum; ++iRow )
        {
            for (  iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
            {
                LCD_Noise[iRow][iCol] = SqrtNew( m_RawData[iRow][iCol] / regData);
            }
        }
    }

    ReCode = EnterWork();
    SysDelay(100);        
    ReCode = ReadReg( 0x80, &chNoiseValueVa );
    ReCode = ReadReg( 0x82, &chNoiseValueKey );
    ReCode = EnterFactory();
    SysDelay( 200);          
    if (ReCode != ERROR_CODE_OK)
    {
        bResultFlag = false;
        FTS_TEST_DBG("\r\nEnter factory mode failed.\r\n");
    }

#if 1
    FTS_TEST_DBG("\nVA Channels: ");
    for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum+1; ++iRow )
    {
        FTS_TEST_DBG("\nCh_%02d:  ", iRow+1);
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            //  FTS_TEST_DBG("%d, ", m_RawData[iRow][iCol] / regData);
            FTS_TEST_DBG("%4d, ", LCD_Noise[iRow][iCol]);
        }
    }
    FTS_TEST_DBG("\n");
#endif


// VA
    iValueMin = 0;
    iValueMax = g_stCfg_FT8716_BasicThreshold.LCDNoiseTest_Coefficient*chNoiseValueVa* 32 / 100;    //

    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node

            iValue = LCD_Noise[iRow][iCol];
            if (iValue < iValueMin || iValue > iValueMax)
            {
                bResultFlag = false;
                FTS_TEST_DBG(" LCD Noise test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d). \n",
                               iRow+1, iCol+1, iValue, iValueMin, iValueMax);
            }
        }
    }

    FTS_TEST_DBG(" Va_Set_Range=(%d, %d). ",iValueMin, iValueMax);

// Key
    iValueMin = 0;
    iValueMax = g_stCfg_FT8716_BasicThreshold.LCDNoiseTest_Coefficient_Key*chNoiseValueKey* 32 / 100;
    for (ikey = 0; ikey < g_stSCapConfEx.KeyNumTotal; ikey++ )
    {
        if (g_stCfg_MCap_DetailThreshold.InvalidNode[g_stSCapConfEx.ChannelXNum][ikey] == 0)continue;
        iValue = LCD_Noise[g_stSCapConfEx.ChannelXNum][ikey];
        if (iValue < iValueMin || iValue > iValueMax)
        {
            bResultFlag = false;
            FTS_TEST_DBG(" LCD Noise test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d). \n",
                           g_stSCapConfEx.ChannelXNum+1, ikey+1, iValue, iValueMin, iValueMax);
        }
    }

    FTS_TEST_DBG("Key_Set_Range=(%d, %d). ",iValueMin, iValueMax);


    Save_Test_Data(LCD_Noise, 0, 0, g_stSCapConfEx.ChannelXNum + 1, g_stSCapConfEx.ChannelYNum, 1 );

    TestResultLen += sprintf(TestResult+TestResultLen," LCD Noise Test is %s. \n\n", (bResultFlag  ? "OK" : "NG"));

    if (bResultFlag)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//LCD Noise Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//LCD Noise Test  is NG!");
    }
	
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_DBG("\n\n//LCD Noise Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen," LCD Noise Test is NG. \n\n");

    return ReCode;

}


/************************************************************************
* Name: FT8716_CheckItem_FwVersionTest
* Brief:   Fw Version Test.
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_CheckItem_FwVersionTest(bool* bTestResult)
{
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    int i = 0;
    unsigned char regData = 0;
    unsigned char FwVersion = 0;

    FTS_TEST_DBG("\n\n==============================Test Item: -------- Fw Version Test\n");

    ReCode = EnterWork();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG(" EnterWork failed.. Error Code: %d \n", ReCode);
        btmpresult = false;
        goto TEST_ERR;
    }

    for (i = 0; i < 3; i++ )
    {
        SysDelay(60);           //liu
        ReCode = ReadReg(REG_FWVERSION, &regData);

        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_DBG("  Failed to Read Register: %x. Error Code: %d \n", REG_FWVERSION, ReCode);
            goto TEST_ERR;
        }

        FwVersion = g_stCfg_FT8716_BasicThreshold.FW_VER_VALUE;

        FTS_TEST_DBG("\r\nGet FwVersion:0x%02x, Set FwVersion: 0x%02x  \n", regData, FwVersion);

        if (FwVersion == regData)
        {
            btmpresult = true;
            break;
        }
        else
        {
            btmpresult = false;
        }
    }

    TestResultLen += sprintf(TestResult+TestResultLen," FwVersion Test is %s. \n\n", (btmpresult  ? "OK" : "NG"));

    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//FwVersion Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//FwVersion Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_DBG("\n\n//FwVersion Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen," FwVersion Test is NG. \n\n");

    return ReCode;
}


/************************************************************************
* Name: FT8716_CheckItem_FactoryIdTest
* Brief:   Factory Id Testt.
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_CheckItem_FactoryIdTest(bool* bTestResult)
{
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    unsigned char regData = 0;
    unsigned char FactoryId = 0;

    FTS_TEST_DBG("\n\n==============================Test Item: -------- Factory Id Test\n");

    ReCode = EnterWork();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG(" EnterWork failed.. Error Code: %d \n", ReCode);
        btmpresult = false;
        goto TEST_ERR;
    }


    SysDelay(60);               //liu
    ReCode = ReadReg(REG_FACTORYID, &regData);

    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG("  Failed to Read Register: %x. Error Code: %d \n", REG_FACTORYID, ReCode);
        goto TEST_ERR;
    }

    FactoryId = g_stCfg_FT8716_BasicThreshold.Factory_ID_Number;

    FTS_TEST_DBG("\r\nGet FactoryId:0x%02x, Set FactoryId: 0x%02x  \n", regData, FactoryId);

    if (FactoryId == regData)
    {
        btmpresult = true;

    }
    else
    {
        btmpresult = false;
    }

    TestResultLen += sprintf(TestResult+TestResultLen," FactoryId Test is %s. \n\n", (btmpresult  ? "OK" : "NG"));

    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//FactoryId Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//FactoryId Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_DBG("\n\n//FactoryId Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen," FactoryId Test is NG. \n\n");

    return ReCode;
}


/************************************************************************
* Name: FT8716_CheckItem_ProjectCodeTest
* Brief:   Factory Id Testt.
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_CheckItem_ProjectCodeTest(bool* bTestResult)
{
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    unsigned char regData = 0x20;
    unsigned char I2C_wBuffer[1] = {0x81};
    unsigned char buffer[100] = {0};
    FTS_TEST_DBG("\n\n==============================Test Item: -------- Project Code Test\n");

    ReCode = EnterFactory();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_DBG("Failed to Enter Factory Mode...");
        btmpresult = false;
        goto TEST_ERR;
    }

    SysDelay(60);
    ReCode = WriteReg(REG_FWCNT, regData);
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG("  Failed to Write Register: %x. Error Code: %d \n", REG_FWCNT, ReCode);
        goto TEST_ERR;
    }

    ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, buffer, 0x20);
//  FTS_TEST_DBG("\r\nProject Code from Firmware: %s    "  buffer);
//  FTS_TEST_DBG("\r\nProject Code from Setting: %s "  g_stCfg_FT8716_BasicThreshold.Project_Code);
    if (strncmp(buffer,g_stCfg_FT8716_BasicThreshold.Project_Code,32)==0)
    {
        btmpresult = true;
    }
    else
    {
        btmpresult = false;
    }

    TestResultLen += sprintf(TestResult+TestResultLen," ProjectCode Test is %s. \n\n", (btmpresult  ? "OK" : "NG"));

    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_DBG("\n\n//ProjectCode Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_DBG("\n\n//ProjectCode Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_DBG("\n\n//ProjectCode Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen," ProjectCode Test is NG. \n\n");

    return ReCode;

}

unsigned char FT8716_TestItem_RawDataUniformityTest(bool* bTestResult)
{

	 int iRow = 0, iCol = 0;
	 int index = 0;
	 unsigned char ReCode = ERROR_CODE_OK;
	 int iValue = 0, RawDataMin = 0, RawDataMax = 0;
	 bool btmpresult = true;
	 int iDeviation = 0;
	 int iMin = 0, iMax = 0;
	 unsigned char bClbResult = 0;

	FTS_TEST_DBG("\r\n\r\n==============================Test Item: --------  RawData Uniformity Test");
	
	EnterFactory();
	SysDelay(50);

	if( g_stCfg_FT8716_BasicThreshold.RawDataUniformityTest_Check_FIXCB )
	{
		for( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
		{
			for( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
			{
				cbValue[iRow * g_stSCapConfEx.ChannelYNum + iCol] = (unsigned char)g_stCfg_Incell_DetailThreshold.RawDataUniformityTest_CB_Data[iRow][iCol];	
			}
		}
							
		ReCode = SetTxRxCB( cbValue, g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum );
		for(  index = 0; index < 3; ++index )
		{
			GetRawData();	
		}
	}

	memset( minHole, MIN_HOLE_LEVEL, sizeof(minHole) );
	memset( maxHole, MAX_HOLE_LEVEL, sizeof(maxHole) );

    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {
        for (iCol = 1; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)
            {
            		continue; //Invalid Node
            }		
	     RawDataMin = minHole[iRow][iCol];
            RawDataMax = maxHole[iRow][iCol];
     	     iValue = m_RawData[iRow][iCol];			
            if (iValue < RawDataMin || iValue > RawDataMax)
            {
                btmpresult = false;
                FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                             iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
            }
        }
    }
	

	if( g_stCfg_FT8716_BasicThreshold.RawDataUniformityTest_Check_CHX )
	{
		FTS_TEST_DBG("\r\n=========Check Tx Linearity \r\n" );	

		 for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    		{
        		for (iCol = 1; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        		{
        			iDeviation = focal_abs( m_RawData[iRow][iCol] - m_RawData[iRow][iCol-1] );
				TxLinearity[iRow][iCol] = iDeviation;

				iMin = MIN_HOLE_LEVEL;
				iMax = g_stCfg_Incell_DetailThreshold.RawDataUniformityTest_CHX_Linearity[iRow][iCol];
				
				 //FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Tx_Range=(%d, %d) \n",  iRow+1, iCol+1, iDeviation, iMin, iMax);
				 
				 if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0 )   continue;
                		if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol-1] == 0 ) continue;

				if(iDeviation < iMin || iDeviation >  iMax)
				{					 
                    				FTS_TEST_DBG("TX Linearity Out Of Range, TX=%d, RX=%d, TX_Linearity=%d, TX_Hole=%d.\n", \
										iCol+1, iRow+1, iDeviation, iMax);
						btmpresult = false;                      
				}
               				
        		}
		 }

		 Save_Test_Data(TxLinearity, 0, 1,  g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum-1, 1);
	}


	if( g_stCfg_FT8716_BasicThreshold.RawDataUniformityTest_Check_CHY)
	{
		FTS_TEST_DBG("\r\n=========Check Rx Linearity \r\n");	

		 for (iRow = 1; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    		{
        		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        		{
        			iDeviation = focal_abs( m_RawData[iRow][iCol] - m_RawData[iRow-1][iCol] );
				RxLinearity[iRow][iCol] = iDeviation;

				iMin = MIN_HOLE_LEVEL;
				iMax = g_stCfg_Incell_DetailThreshold.RawDataUniformityTest_CHY_Linearity[iRow][iCol];

				//FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Rx_Range=(%d, %d) \n",  iRow+1, iCol+1, iDeviation, iMin, iMax);
				
				 if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0 )   continue;
                		if ( g_stCfg_MCap_DetailThreshold.InvalidNode[iRow-1][iCol] == 0 ) continue;

				if(iDeviation < iMin || iDeviation >  iMax)
				{					 
                    				FTS_TEST_DBG("RX Linearity Out Of Range, TX=%d, RX=%d, RX_Linearity=%d, RX_Hole=%d.\n", \
										iCol+1, iRow+1, iDeviation, iMax);
						btmpresult = false;                      
				}
               				
        		}
		 }

		  Save_Test_Data(RxLinearity, 1, 0,  g_stSCapConfEx.ChannelXNum-1, g_stSCapConfEx.ChannelYNum, 2);
	}

	//auto clb 
	ReCode = ChipClb( &bClbResult );
	if (ERROR_CODE_OK != ReCode)
	{
		btmpresult = false;
		FTS_TEST_DBG("\r\n//========= auto clb Failed!");
	}

	TestResultLen += sprintf(TestResult+TestResultLen,"RawData Uniformity Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

	if( btmpresult )
	{
		*bTestResult = true;
		FTS_TEST_DBG("\r\n\r\n//RawData Uniformity Test is OK!\r\n");
	}
	else
	{
		*bTestResult = false;
		FTS_TEST_DBG("\r\n\r\n//RawData Uniformity Test is NG!\r\n");
	}
	return ReCode;
	
}

static unsigned char SetTxRxCB( unsigned char* pWriteBuffer, int writeNum )
{
	unsigned char ReCode = ERROR_CODE_OK;
	int iOnceWriteMax = BYTES_PER_TIME - 1, iWriteAddress = 0,  iOnceWrite = 0;
	unsigned char writeBuffer[BYTES_PER_TIME] = {0};
	while ( writeNum > 0 )
	{
		iOnceWrite = min( iOnceWriteMax, writeNum );

		//Ð´Æ«ÒÆ
		ReCode = WriteReg( REG_CbAddrH, iWriteAddress >> 8 );
		if(ReCode != ERROR_CODE_OK)return ReCode;

		ReCode = WriteReg( REG_CbAddrL, iWriteAddress & 0xff );
		if(ReCode != ERROR_CODE_OK)return ReCode;

		//Ð´Êý¾Ý
		writeBuffer[0] = 0x6E;
		memcpy( writeBuffer + 1, pWriteBuffer, iOnceWrite );

		ReCode = Comm_Base_IIC_IO( writeBuffer, iOnceWrite + 1, NULL, 0 );

		pWriteBuffer += iOnceWrite;
		writeNum -= iOnceWrite;
		iWriteAddress += iOnceWrite;
	}

	return ReCode;
}
static unsigned char ChipClb(unsigned char *pClbResult)
{
    unsigned char RegData=0;
    unsigned char TimeOutTimes = 50;        //5s
    unsigned char ReCode = ERROR_CODE_OK;

    ReCode = WriteReg(REG_CLB, 4);  //start auto clb

    if (ReCode == ERROR_CODE_OK)
    {
        while (TimeOutTimes--)
        {
            SysDelay( 50);  //delay 50ms          
            ReCode = WriteReg(DEVIDE_MODE_ADDR, 0x04<<4);
            ReCode = ReadReg(0x04, &RegData);
            if (ReCode == ERROR_CODE_OK)
            {
                if (RegData == 0x02)
                {
                    *pClbResult = 1;
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if (TimeOutTimes == 0)
        {
            *pClbResult = 0;
        }
    }
    return ReCode;
}
/************************************************************************
* Name: FT8716_get_test_data
* Brief:  get data of test result
* Input: none
* Output: pTestData, the returned buff
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int FT8716_get_test_data(char *pTestData)
{
    if (NULL == pTestData)
    {
        FTS_TEST_DBG("[focal] %s pTestData == NULL ",  __func__);
        return -1;
    }
    memcpy(pTestData, g_pStoreAllData, (g_lenStoreMsgArea+g_lenStoreDataArea));
    return (g_lenStoreMsgArea+g_lenStoreDataArea);
}


/************************************************************************
* Name: Save_Test_Data
* Brief:  Storage format of test data
* Input: int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount
* Output: none
* Return: none
***********************************************************************/
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int RowArrayIndex, int ColArrayIndex,unsigned char Row, unsigned char Col, unsigned char ItemCount)
{
    int iLen = 0;
    int i = 0, j = 0;

    //Save  Msg (ItemCode is enough, ItemName is not necessary, so set it to "NA".)
    iLen= sprintf(g_pTmpBuff,"NA, %d, %d, %d, %d, %d, ", \
                  m_ucTestItemCode, Row, Col, m_iStartLine, ItemCount);
    memcpy(g_pMsgAreaLine2+g_lenMsgAreaLine2, g_pTmpBuff, iLen);
    g_lenMsgAreaLine2 += iLen;

    m_iStartLine += Row;
    m_iTestDataCount++;

    //Save Data
    for (i = 0+RowArrayIndex; (i < Row+RowArrayIndex) && (i < TX_NUM_MAX); i++)
    {
        for (j = 0+ColArrayIndex; (j < Col+ColArrayIndex) && (j < RX_NUM_MAX); j++)
        {
            if (j == (Col+ColArrayIndex -1)) //The Last Data of the Row, add "\n"
                iLen= sprintf(g_pTmpBuff,"%d, \n",  iData[i][j]);
            else
                iLen= sprintf(g_pTmpBuff,"%d, ", iData[i][j]);

            memcpy(g_pStoreDataArea+g_lenStoreDataArea, g_pTmpBuff, iLen);
            g_lenStoreDataArea += iLen;
        }
    }

}

////////////////////////////////////////////////////////////
/************************************************************************
* Name: StartScan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int StartScan(void)
{
    unsigned char RegVal = 0x00;
    unsigned char times = 0;
    const unsigned char MaxTimes = 20;  //The longest wait 160ms
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    //if(hDevice == NULL)       return ERROR_CODE_NO_DEVICE;

    ReCode = ReadReg(DEVIDE_MODE_ADDR,&RegVal);
    if (ReCode == ERROR_CODE_OK)
    {
        RegVal |= 0x80;     //Top bit position 1, start scan
        ReCode = WriteReg(DEVIDE_MODE_ADDR,RegVal);
        if (ReCode == ERROR_CODE_OK)
        {
            while (times++ < MaxTimes)      //Wait for the scan to complete
            {
                SysDelay(8);    //8ms
                ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
                if (ReCode == ERROR_CODE_OK)
                {
                    if ((RegVal>>7) == 0)    break;
                }
                else
                {
                    break;
                }
            }
            if (times < MaxTimes)    ReCode = ERROR_CODE_OK;
            else ReCode = ERROR_CODE_COMM_ERROR;
        }
    }
    return ReCode;

}
/************************************************************************
* Name: ReadRawData(Same function name as FT_MultipleTest)
* Brief:  read Raw Data
* Input: Freq(No longer used, reserved), LineNum, ByteNum
* Output: pRevBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer)
{
    unsigned char ReCode=ERROR_CODE_COMM_ERROR;
    unsigned char I2C_wBuffer[3] = {0};
    unsigned char pReadData[ByteNum];
    //unsigned char pReadDataTmp[ByteNum*2];
    int i, iReadNum;
    unsigned short BytesNumInTestMode1=0;

    iReadNum=ByteNum/BYTES_PER_TIME;

    if (0 != (ByteNum%BYTES_PER_TIME)) iReadNum++;

    if (ByteNum <= BYTES_PER_TIME)
    {
        BytesNumInTestMode1 = ByteNum;
    }
    else
    {
        BytesNumInTestMode1 = BYTES_PER_TIME;
    }

    ReCode = WriteReg(REG_LINE_NUM, LineNum);//Set row addr;


    //***********************************************************Read raw data in test mode1
    I2C_wBuffer[0] = REG_RawBuf0;   //set begin address
    if (ReCode == ERROR_CODE_OK)
    {
        focal_msleep(10);
        ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, pReadData, BytesNumInTestMode1);
    }

    for (i=1; i<iReadNum; i++)
    {
        if (ReCode != ERROR_CODE_OK) break;

        if (i==iReadNum-1) //last packet
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, ByteNum-BYTES_PER_TIME*i);
        }
        else
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, BYTES_PER_TIME);
        }

    }

    if (ReCode == ERROR_CODE_OK)
    {
        for (i=0; i<(ByteNum>>1); i++)
        {
            pRevBuffer[i] = (pReadData[i<<1]<<8)+pReadData[(i<<1)+1];
            //if(pRevBuffer[i] & 0x8000)//The sign bit
            //{
            //  pRevBuffer[i] -= 0xffff + 1;
            //}
        }
    }


    return ReCode;

}
/************************************************************************
* Name: GetTxRxCB(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx/Rx
* Input: StartNodeNo, ReadNum
* Output: pReadBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetTxRxCB(unsigned short StartNodeNo, unsigned short ReadNum, unsigned char *pReadBuffer)
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned short usReturnNum = 0;//Number to return in every time
    unsigned short usTotalReturnNum = 0;//Total return number
    unsigned char wBuffer[4];
    int i, iReadNum;

    iReadNum = ReadNum/BYTES_PER_TIME;

    if (0 != (ReadNum%BYTES_PER_TIME)) iReadNum++;

    wBuffer[0] = REG_CbBuf0;

    usTotalReturnNum = 0;

    for (i = 1; i <= iReadNum; i++)
    {
        if (i*BYTES_PER_TIME > ReadNum)
            usReturnNum = ReadNum - (i-1)*BYTES_PER_TIME;
        else
            usReturnNum = BYTES_PER_TIME;

        wBuffer[1] = (StartNodeNo+usTotalReturnNum) >>8;//Address offset high 8 bit
        wBuffer[2] = (StartNodeNo+usTotalReturnNum)&0xff;//Address offset low 8 bit

        ReCode = WriteReg(REG_CbAddrH, wBuffer[1]);
        ReCode = WriteReg(REG_CbAddrL, wBuffer[2]);
        //ReCode = fts_i2c_read(wBuffer, 1, pReadBuffer+usTotalReturnNum, usReturnNum);
        ReCode = Comm_Base_IIC_IO(wBuffer, 1, pReadBuffer+usTotalReturnNum, usReturnNum);

        usTotalReturnNum += usReturnNum;

        if (ReCode != ERROR_CODE_OK)return ReCode;

        //if(ReCode < 0) return ReCode;
    }

    return ReCode;
}

//***********************************************
//Get PanelRows
//***********************************************
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
    return ReadReg(REG_TX_NUM, pPanelRows);
}

//***********************************************
//Get PanelCols
//***********************************************
static unsigned char GetPanelCols(unsigned char *pPanelCols)
{
    return ReadReg(REG_RX_NUM, pPanelCols);
}


/************************************************************************
* Name: GetChannelNum
* Brief:  Get Num of Ch_X, Ch_Y and key
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNum(void)
{
    unsigned char ReCode;
    //int TxNum, RxNum;
    int i ;
    unsigned char rBuffer[1]; //= new unsigned char;

    //FTS_TEST_DBG("Enter GetChannelNum...");
    //--------------------------------------------"Get Channel X Num...";
    for (i = 0; i < 3; i++)
    {
        ReCode = GetPanelRows(rBuffer);
        if (ReCode == ERROR_CODE_OK)
        {
            if (0 < rBuffer[0] && rBuffer[0] < 80)
            {
                g_stSCapConfEx.ChannelXNum = rBuffer[0];
                if (g_stSCapConfEx.ChannelXNum > g_ScreenSetParam.iUsedMaxTxNum)
                {
                    FTS_TEST_DBG("Failed to get Channel X number, Get num = %d, UsedMaxNum = %d",
                                 g_stSCapConfEx.ChannelXNum, g_ScreenSetParam.iUsedMaxTxNum);
                    g_stSCapConfEx.ChannelXNum = 0;
                    return ERROR_CODE_INVALID_PARAM;
                }

                break;
            }
            else
            {
                SysDelay(150);
                continue;
            }
        }
        else
        {
            FTS_TEST_DBG("Failed to get Channel X number");
            SysDelay(150);
        }
    }

    //--------------------------------------------"Get Channel Y Num...";
    for (i = 0; i < 3; i++)
    {
        ReCode = GetPanelCols(rBuffer);
        if (ReCode == ERROR_CODE_OK)
        {
            if (0 < rBuffer[0] && rBuffer[0] < 80)
            {
                g_stSCapConfEx.ChannelYNum = rBuffer[0];
                if (g_stSCapConfEx.ChannelYNum > g_ScreenSetParam.iUsedMaxRxNum)
                {

                    FTS_TEST_DBG("Failed to get Channel Y number, Get num = %d, UsedMaxNum = %d",
                                 g_stSCapConfEx.ChannelYNum, g_ScreenSetParam.iUsedMaxRxNum);
                    g_stSCapConfEx.ChannelYNum = 0;
                    return ERROR_CODE_INVALID_PARAM;
                }
                break;
            }
            else
            {
                SysDelay(150);
                continue;
            }
        }
        else
        {
            FTS_TEST_DBG("Failed to get Channel Y number");
            SysDelay(150);
        }
    }

    //--------------------------------------------"Get Key Num...";
    for (i = 0; i < 3; i++)
    {
        unsigned char regData = 0;
        g_stSCapConfEx.KeyNum = 0;
        ReCode = ReadReg( FT8716_LEFT_KEY_REG, &regData );
        if (ReCode == ERROR_CODE_OK)
        {
            if (((regData >> 0) & 0x01))
            {
                g_stSCapConfEx.bLeftKey1 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 1) & 0x01))
            {
                g_stSCapConfEx.bLeftKey2 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 2) & 0x01))
            {
                g_stSCapConfEx.bLeftKey3 = true;
                ++g_stSCapConfEx.KeyNum;
            }
        }
        else
        {
            FTS_TEST_DBG("Failed to get Key number");
            SysDelay(150);
            continue;
        }
        ReCode = ReadReg( FT8716_RIGHT_KEY_REG, &regData );
        if (ReCode == ERROR_CODE_OK)
        {
            if (((regData >> 0) & 0x01))
            {
                g_stSCapConfEx.bRightKey1 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 1) & 0x01))
            {
                g_stSCapConfEx.bRightKey2 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 2) & 0x01))
            {
                g_stSCapConfEx.bRightKey3 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            break;
        }
        else
        {
            FTS_TEST_DBG("Failed to get Key number");
            SysDelay(150);
            continue;
        }
    }

    //g_stSCapConfEx.KeyNumTotal = g_stSCapConfEx.KeyNum;

    FTS_TEST_DBG("CH_X = %d, CH_Y = %d, Key = %d",  g_stSCapConfEx.ChannelXNum ,g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum );
    return ReCode;
}

/************************************************************************
* Name: GetRawData
* Brief:  Get Raw Data of VA area and Key area
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetRawData(void)
{
    int ReCode = ERROR_CODE_OK;
    int iRow, iCol;

    //--------------------------------------------Enter Factory Mode
    ReCode = EnterFactory();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_DBG("Failed to Enter Factory Mode...");
        return ReCode;
    }


    //--------------------------------------------Check Num of Channel
    if (0 == (g_stSCapConfEx.ChannelXNum + g_stSCapConfEx.ChannelYNum))
    {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode )
        {
            FTS_TEST_DBG("Error Channel Num...");
            return ERROR_CODE_INVALID_PARAM;
        }
    }

    //--------------------------------------------Start Scanning
    //FTS_TEST_DBG("Start Scan ...");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_DBG("Failed to Scan ...");
        return ReCode;
    }


    //--------------------------------------------Read RawData for Channel Area
    //FTS_TEST_DBG("Read RawData...");
    memset(m_RawData, 0, sizeof(m_RawData));
    memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
    ReCode = ReadRawData(0, 0xAD, g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum * 2, m_iTempRawData);
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_DBG("Failed to Get RawData");
        return ReCode;
    }

    for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow)
    {
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol)
        {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow * g_stSCapConfEx.ChannelYNum + iCol];
        }
    }

    //--------------------------------------------Read RawData for Key Area
    memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
    ReCode = ReadRawData( 0, 0xAE, g_stSCapConfEx.KeyNumTotal * 2, m_iTempRawData );
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_DBG("Failed to Get RawData");
        return ReCode;
    }

    for (iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; ++iCol)
    {
        m_RawData[g_stSCapConfEx.ChannelXNum][iCol] = m_iTempRawData[iCol];
    }

    return ReCode;

}


/************************************************************************
* Name: WeakShort_GetAdcData
* Brief:  Get Adc Data
* Input: AllAdcDataLen
* Output: pRevBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char WeakShort_GetAdcData( int AllAdcDataLen, int *pRevBuffer  )
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char RegMark = 0;
    int index = 0;
    int i = 0;
    int usReturnNum = 0;
    unsigned char wBuffer[2] = {0};

    int iReadNum = AllAdcDataLen / BYTES_PER_TIME;

    FTS_TEST_FUNC_ENTER();

    memset( wBuffer, 0, sizeof(wBuffer) );
    wBuffer[0] = 0x89;

    if ((AllAdcDataLen % BYTES_PER_TIME) > 0) ++iReadNum;

    ReCode = WriteReg( 0x0F, 1 );  //Start ADC sample

    for ( index = 0; index < 50; ++index )
    {
        SysDelay( 50 );                   
        ReCode = ReadReg( 0x10, &RegMark );  //Polling sampling end mark
        if ( ERROR_CODE_OK == ReCode && 0 == RegMark )
            break;
    }
    if ( index >= 50)
    {
        FTS_TEST_DBG("ReadReg failed, ADC data not OK.");
        return 6;
    }

    {
        usReturnNum = BYTES_PER_TIME;
        if (ReCode == ERROR_CODE_OK)
        {
            ReCode = Comm_Base_IIC_IO(wBuffer, 1, pReadBuffer, usReturnNum);
        }

        for ( i=1; i<iReadNum; i++)
        {
            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_DBG("Comm_Base_IIC_IO  error.   !!!");
                break;
            }

            if (i==iReadNum-1) //last packet
            {
                usReturnNum = AllAdcDataLen-BYTES_PER_TIME*i;
                ReCode = Comm_Base_IIC_IO(NULL, 0, pReadBuffer+BYTES_PER_TIME*i, usReturnNum);
            }
            else
            {
                usReturnNum = BYTES_PER_TIME;
                ReCode = Comm_Base_IIC_IO(NULL, 0, pReadBuffer+BYTES_PER_TIME*i, usReturnNum);
            }
        }
    }

    for ( index = 0; index < AllAdcDataLen/2; ++index )
    {
        pRevBuffer[index] = (pReadBuffer[index * 2] << 8) + pReadBuffer[index * 2 + 1];
    }

   FTS_TEST_FUNC_EXIT();
    return ReCode;
}

unsigned char FT8716_GetTestResult(void)
{
    //bool bTestResult = true;
    unsigned char ucDevice = 0;
    int iItemCount=0;
    unsigned char ucResultData = 0;
    //int iLen = 0;

    for (iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
    {
        ///////////////////////////////////////////////////////FT8716_RAWDATA_TEST
        if (Code_FT8716_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            if (RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
                ucResultData |= 0x01<<2;//bit2
        }

        ///////////////////////////////////////////////////////Code_FT8716_CB_TEST
        if (Code_FT8716_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            if (RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
                ucResultData |= 0x01<<1;//bit1
        }

        ///////////////////////////////////////////////////////Code_FT8716_WEAK_SHORT_CIRCUIT_TEST
        if (Code_FT8716_SHORT_CIRCUIT_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            if (RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
                ucResultData |= 0x01;//bit0
        }
    }

    FTS_TEST_DBG("Test_result:  0x%02x", ucResultData);
    //sprintf(tmp + count, "AUOW");
    //iLen= sprintf(g_pTmpBuff,"\nTest_result:  0x%02x\n", ucResultData);
    //memcpy(g_pPrintMsg+g_lenPrintMsg, g_pTmpBuff, iLen);
    //g_lenPrintMsg+=iLen;

    return ucResultData;
}

int vk_result[8] = {0};
#define LOW_WORD(X)  ((X) & 0XFFFF)
#define HIGH_WORD(X) (((X) >>16) & 0XFFFF)
int visual_key_active = 0;
int fts_test_vk(u16 x, u16 y)
{
    int i = 0;

    FTS_TEST_DBG("%s start", __func__);

    for(i = 0; i < g_stCfg_FT8716_BasicThreshold.Set_Key_Num; i++)
    {
        if((x >= g_stCfg_FT8716_BasicThreshold.Key_Left[i])
            && (x <= g_stCfg_FT8716_BasicThreshold.Key_Right[i])
            && (y >= g_stCfg_FT8716_BasicThreshold.Key_Top[i])
            && (y <= g_stCfg_FT8716_BasicThreshold.Key_Bottom[i]))
        {
            vk_result[i] = 1;
            FTS_TEST_DBG("vk result[%d]=%d", i, vk_result[i]);
        }
    }        
    FTS_TEST_DBG("%s end", __func__);
    return 0;
}
int FT8716_TestItem_visual_key(bool *bTestResult)
{
    int i = 0;
   // int j = 0;
    u8 reg_val = 0;
    u8 m_lastTouchThreshold = 0;
    int time_cycle = 0;
    struct i2c_client *client = fts_i2c_client;
    
    FTS_TEST_DBG("%s start", __func__);


    if (0 == g_stCfg_FT8716_BasicThreshold.Set_Key_Num)
        return ERROR_CODE_INVALID_PARAM;

    enable_irq(client->irq);
    EnterWork(); 
    visual_key_active = 1;
    memset(vk_result, 0, sizeof(int)*8);

   if(g_stCfg_FT8716_BasicThreshold.bDynamicHole)
   {
   	 WriteReg(0x84, 0xAA);
	 ReadReg(0x82, &reg_val);
	 m_lastTouchThreshold = reg_val;
	 if( HIGH_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1])> 0 && LOW_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1]) > 0 )
	 {
	 	reg_val = reg_val * (g_stCfg_FT8716_BasicThreshold.iPerservedKey[1]*10+100)/100;
		WriteReg(0x82, reg_val);
	 }
	 else if(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[0])
	 {
	 	reg_val =  reg_val * (g_stCfg_FT8716_BasicThreshold.iPerservedKey[0]*10+100)/100;
		WriteReg(0x82, reg_val);
	 }
   }
   else
   {
   	 WriteReg(0x84, 0xAA);
	 ReadReg(0x82, &reg_val);
	 m_lastTouchThreshold = reg_val;
	 if( HIGH_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1])> 0 && LOW_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1]) > 0 )
	 {
	 	reg_val = g_stCfg_FT8716_BasicThreshold.iPerservedKey[1];
		WriteReg(0x82, reg_val);
	 }
	 else if(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[0])
	 {
	 	reg_val = g_stCfg_FT8716_BasicThreshold.iPerservedKey[0];
		WriteReg(0x82, reg_val);
	 }
   }
   FTS_TEST_DBG("touchthreshold = %d", reg_val);
      
    time_cycle = g_stCfg_FT8716_BasicThreshold.LimitTime_Key * 1000/500;
    if(0 == time_cycle)   // no limit time
    {
    		time_cycle = 1000;
    }
    for(i = 0; i < time_cycle; i++)
    {
    //virtual key (back & menu all touch break)
        /*for(j = 0; j < g_stCfg_FT8716_BasicThreshold.Set_Key_Num; j++)
        {
            if(vk_result[j] == 0)
            {
                FTS_TEST_DBG("vk%d wait %d", j, vk_result[j]);
                break;
            }
	     else
	     {
	         FTS_TEST_DBG("vk%d touch %d", j, vk_result[j]); 		
	     }
        }
        if (j < g_stCfg_FT8716_BasicThreshold.Set_Key_Num)
        {
            msleep(500);
            continue;
        }
        break;*/
       //virtual key which one touch break
        if(vk_result[0] == 0)
            {
             msleep(500);
            }
		else{
			break;
			}
        
    }

    WriteReg(0x82, m_lastTouchThreshold);
    memset(vk_result, 0, sizeof(int)*8);
    visual_key_active = 0;
	   
    FTS_TEST_DBG("%s i=%d end", __func__, i);

    disable_irq(client->irq);

    if(i >= time_cycle)
    {
        *bTestResult = false;
	 pr_err("FT8716_TestItem_visual_key is NG");
	 TestResultLen += sprintf(TestResult+TestResultLen,"FT8716_TestItem_visual_key is NG. \n\n");
        return ERROR_CODE_INVALID_PARAM;
    }
    else
    {
        *bTestResult = true;
	pr_err("FT8716_TestItem_visual_key is OK");
	 TestResultLen += sprintf(TestResult+TestResultLen,"FT8716_TestItem_visual_key is OK. \n\n");
        return ERROR_CODE_OK;
    }
}


int FT8716_TestItem_rf_visual_key(bool *bTestResult)
{
    int i = 0;
    int j = 0;
    u8 reg_val = 0;
    u8 m_lastTouchThreshold = 0;
    int time_cycle = 0;
    struct i2c_client *client = fts_i2c_client;
    
    FTS_TEST_DBG("%s start", __func__);


    if (0 == g_stCfg_FT8716_BasicThreshold.Set_Key_Num)
        return ERROR_CODE_INVALID_PARAM;

    enable_irq(client->irq);
    EnterWork(); 
    visual_key_active = 1;
    memset(vk_result, 0, sizeof(int)*8);

   if(g_stCfg_FT8716_BasicThreshold.bDynamicHole)
   {
   	 WriteReg(0x84, 0xAA);
	 ReadReg(0x82, &reg_val);
	 m_lastTouchThreshold = reg_val;
	 if( HIGH_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1])> 0 && LOW_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1]) > 0 )
	 {
	 	reg_val = reg_val * (g_stCfg_FT8716_BasicThreshold.iPerservedKey[1]*10+100)/100;
		WriteReg(0x82, reg_val);
	 }
	 else if(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[0])
	 {
	 	reg_val =  reg_val * (g_stCfg_FT8716_BasicThreshold.iPerservedKey[0]*10+100)/100;
		WriteReg(0x82, reg_val);
	 }
   }
   else
   {
   	 WriteReg(0x84, 0xAA);
	 ReadReg(0x82, &reg_val);
	 m_lastTouchThreshold = reg_val;
	 if( HIGH_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1])> 0 && LOW_WORD(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[1]) > 0 )
	 {
	 	reg_val = g_stCfg_FT8716_BasicThreshold.iPerservedKey[1];
		WriteReg(0x82, reg_val);
	 }
	 else if(g_stCfg_FT8716_BasicThreshold.bSetTouchThreshold[0])
	 {
	 	reg_val = g_stCfg_FT8716_BasicThreshold.iPerservedKey[0];
		WriteReg(0x82, reg_val);
	 }
   }
   FTS_TEST_DBG("touchthreshold = %d", reg_val);
      
    time_cycle = g_stCfg_FT8716_BasicThreshold.LimitTime_Key * 1000/500;
    if(0 == time_cycle)   // no limit time
    {
    		time_cycle = 1000;
    }
    for(i = 0; i < time_cycle; i++)
    {
        for(j = 0; j < g_stCfg_FT8716_BasicThreshold.Set_Key_Num; j++)
        {
            if(vk_result[j] == 0)
            {
                FTS_TEST_DBG("vk%d wait %d", j, vk_result[j]);
            }
	     else
	     {
	         FTS_TEST_DBG("vk%d touch %d", j, vk_result[j]); 
			 break;
	     }
        }
        if (j == g_stCfg_FT8716_BasicThreshold.Set_Key_Num)
        {
            msleep(500);
            continue;
        }
        break;
    }

    WriteReg(0x82, m_lastTouchThreshold);
    memset(vk_result, 0, sizeof(int)*8);
    visual_key_active = 0;
	   
    FTS_TEST_DBG("%s i=%d end", __func__, i);

    disable_irq(client->irq);

    if(i >= time_cycle)
    {
        *bTestResult = false;
	 pr_err("FT8716_TestItem_rf_visual_key is NG");
	 TestResultLen += sprintf(TestResult+TestResultLen,"FT8716_TestItem_visual_key is NG. \n\n");
        return ERROR_CODE_INVALID_PARAM;
    }
    else
    {
        *bTestResult = true;
	pr_err("FT8716_TestItem_rf_visual_key is OK");
	 TestResultLen += sprintf(TestResult+TestResultLen,"FT8716_TestItem_visual_key is OK. \n\n");
        return ERROR_CODE_OK;
    }
}

#endif
